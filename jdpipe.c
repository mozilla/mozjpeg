/*
 * jdpipe.c
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains decompression pipeline controllers.
 * These routines are invoked via the d_pipeline_controller method.
 *
 * There are two basic pipeline controllers.  The simpler one handles a
 * single-scan JPEG file (single component or fully interleaved) with no
 * color quantization or 1-pass quantization.  In this case, the file can
 * be processed in one top-to-bottom pass.  The more complex controller is
 * used when 2-pass color quantization is requested and/or the JPEG file
 * has multiple scans (noninterleaved or partially interleaved).  In this
 * case, the entire image must be buffered up in a "big" array.
 *
 * If you need to make a minimal implementation, the more complex controller
 * can be compiled out by disabling the appropriate configuration options.
 * We don't recommend this, since then you can't handle all legal JPEG files.
 */

#include "jinclude.h"


#ifdef D_MULTISCAN_FILES_SUPPORTED /* wish we could assume ANSI's defined() */
#define NEED_COMPLEX_CONTROLLER
#else
#ifdef QUANT_2PASS_SUPPORTED
#define NEED_COMPLEX_CONTROLLER
#endif
#endif


/*
 * About the data structures:
 *
 * The processing chunk size for upsampling is referred to in this file as
 * a "row group": a row group is defined as Vk (v_samp_factor) sample rows of
 * any component while downsampled, or Vmax (max_v_samp_factor) unsubsampled
 * rows.  In an interleaved scan each MCU row contains exactly DCTSIZE row
 * groups of each component in the scan.  In a noninterleaved scan an MCU row
 * is one row of blocks, which might not be an integral number of row groups;
 * therefore, we read in Vk MCU rows to obtain the same amount of data as we'd
 * have in an interleaved scan.
 * To provide context for the upsampling step, we have to retain the last
 * two row groups of the previous MCU row while reading in the next MCU row
 * (or set of Vk MCU rows).  To do this without copying data about, we create
 * a rather strange data structure.  Exactly DCTSIZE+2 row groups of samples
 * are allocated, but we create two different sets of pointers to this array.
 * The second set swaps the last two pairs of row groups.  By working
 * alternately with the two sets of pointers, we can access the data in the
 * desired order.
 *
 * Cross-block smoothing also needs context above and below the "current" row.
 * Since this is an optional feature, I've implemented it in a way that is
 * much simpler but requires more than the minimum amount of memory.  We
 * simply allocate three extra MCU rows worth of coefficient blocks and use
 * them to "read ahead" one MCU row in the file.  For a typical 1000-pixel-wide
 * image with 2x2,1x1,1x1 sampling, each MCU row is about 50Kb; an 80x86
 * machine may be unable to apply cross-block smoothing to wider images.
 */


/*
 * These variables are logically local to the pipeline controller,
 * but we make them static so that scan_big_image can use them
 * without having to pass them through the quantization routines.
 */

static int rows_in_mem;		/* # of sample rows in full-size buffers */
/* Work buffer for data being passed to output module. */
/* This has color_out_comps components if not quantizing, */
/* but only one component when quantizing. */
static JSAMPIMAGE output_workspace;

#ifdef NEED_COMPLEX_CONTROLLER
/* Full-size image array holding upsampled, but not color-processed data. */
static big_sarray_ptr *fullsize_image;
static JSAMPIMAGE fullsize_ptrs; /* workspace for access_big_sarray() result */
#endif


/*
 * Utility routines: common code for pipeline controllers
 */

LOCAL void
interleaved_scan_setup (decompress_info_ptr cinfo)
/* Compute all derived info for an interleaved (multi-component) scan */
/* On entry, cinfo->comps_in_scan and cinfo->cur_comp_info[] are set up */
{
  short ci, mcublks;
  jpeg_component_info *compptr;

  if (cinfo->comps_in_scan > MAX_COMPS_IN_SCAN)
    ERREXIT(cinfo->emethods, "Too many components for interleaved scan");

  cinfo->MCUs_per_row = (cinfo->image_width
			 + cinfo->max_h_samp_factor*DCTSIZE - 1)
			/ (cinfo->max_h_samp_factor*DCTSIZE);

  cinfo->MCU_rows_in_scan = (cinfo->image_height
			     + cinfo->max_v_samp_factor*DCTSIZE - 1)
			    / (cinfo->max_v_samp_factor*DCTSIZE);
  
  cinfo->blocks_in_MCU = 0;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    /* for interleaved scan, sampling factors give # of blocks per component */
    compptr->MCU_width = compptr->h_samp_factor;
    compptr->MCU_height = compptr->v_samp_factor;
    compptr->MCU_blocks = compptr->MCU_width * compptr->MCU_height;
    /* compute physical dimensions of component */
    compptr->downsampled_width = jround_up(compptr->true_comp_width,
					   (long) (compptr->MCU_width*DCTSIZE));
    compptr->downsampled_height = jround_up(compptr->true_comp_height,
					    (long) (compptr->MCU_height*DCTSIZE));
    /* Sanity check */
    if (compptr->downsampled_width !=
	(cinfo->MCUs_per_row * (compptr->MCU_width*DCTSIZE)))
      ERREXIT(cinfo->emethods, "I'm confused about the image width");
    /* Prepare array describing MCU composition */
    mcublks = compptr->MCU_blocks;
    if (cinfo->blocks_in_MCU + mcublks > MAX_BLOCKS_IN_MCU)
      ERREXIT(cinfo->emethods, "Sampling factors too large for interleaved scan");
    while (mcublks-- > 0) {
      cinfo->MCU_membership[cinfo->blocks_in_MCU++] = ci;
    }
  }

  (*cinfo->methods->d_per_scan_method_selection) (cinfo);
}


LOCAL void
noninterleaved_scan_setup (decompress_info_ptr cinfo)
/* Compute all derived info for a noninterleaved (single-component) scan */
/* On entry, cinfo->comps_in_scan = 1 and cinfo->cur_comp_info[0] is set up */
{
  jpeg_component_info *compptr = cinfo->cur_comp_info[0];

  /* for noninterleaved scan, always one block per MCU */
  compptr->MCU_width = 1;
  compptr->MCU_height = 1;
  compptr->MCU_blocks = 1;
  /* compute physical dimensions of component */
  compptr->downsampled_width = jround_up(compptr->true_comp_width,
					 (long) DCTSIZE);
  compptr->downsampled_height = jround_up(compptr->true_comp_height,
					  (long) DCTSIZE);

  cinfo->MCUs_per_row = compptr->downsampled_width / DCTSIZE;
  cinfo->MCU_rows_in_scan = compptr->downsampled_height / DCTSIZE;

  /* Prepare array describing MCU composition */
  cinfo->blocks_in_MCU = 1;
  cinfo->MCU_membership[0] = 0;

  (*cinfo->methods->d_per_scan_method_selection) (cinfo);
}



LOCAL JSAMPIMAGE
alloc_sampimage (decompress_info_ptr cinfo,
		 int num_comps, long num_rows, long num_cols)
/* Allocate an in-memory sample image (all components same size) */
{
  JSAMPIMAGE image;
  int ci;

  image = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(num_comps * SIZEOF(JSAMPARRAY));
  for (ci = 0; ci < num_comps; ci++) {
    image[ci] = (*cinfo->emethods->alloc_small_sarray) (num_cols, num_rows);
  }
  return image;
}


#if 0				/* this routine not currently needed */

LOCAL void
free_sampimage (decompress_info_ptr cinfo, JSAMPIMAGE image, int num_comps)
/* Release a sample image created by alloc_sampimage */
{
  int ci;

  for (ci = 0; ci < num_comps; ci++) {
      (*cinfo->emethods->free_small_sarray) (image[ci]);
  }
  (*cinfo->emethods->free_small) ((void *) image);
}

#endif


LOCAL JBLOCKIMAGE
alloc_MCU_row (decompress_info_ptr cinfo)
/* Allocate one MCU row's worth of coefficient blocks */
{
  JBLOCKIMAGE image;
  int ci;

  image = (JBLOCKIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->comps_in_scan * SIZEOF(JBLOCKARRAY));
  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    image[ci] = (*cinfo->emethods->alloc_small_barray)
			(cinfo->cur_comp_info[ci]->downsampled_width / DCTSIZE,
			 (long) cinfo->cur_comp_info[ci]->MCU_height);
  }
  return image;
}


#ifdef NEED_COMPLEX_CONTROLLER	/* not used by simple controller */

LOCAL void
free_MCU_row (decompress_info_ptr cinfo, JBLOCKIMAGE image)
/* Release a coefficient block array created by alloc_MCU_row */
{
  int ci;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    (*cinfo->emethods->free_small_barray) (image[ci]);
  }
  (*cinfo->emethods->free_small) ((void *) image);
}

#endif


LOCAL void
alloc_sampling_buffer (decompress_info_ptr cinfo, JSAMPIMAGE sampled_data[2])
/* Create a downsampled-data buffer having the desired structure */
/* (see comments at head of file) */
{
  short ci, vs, i;

  /* Get top-level space for array pointers */
  sampled_data[0] = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->comps_in_scan * SIZEOF(JSAMPARRAY));
  sampled_data[1] = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->comps_in_scan * SIZEOF(JSAMPARRAY));

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    vs = cinfo->cur_comp_info[ci]->v_samp_factor; /* row group height */
    /* Allocate the real storage */
    sampled_data[0][ci] = (*cinfo->emethods->alloc_small_sarray)
				(cinfo->cur_comp_info[ci]->downsampled_width,
				(long) (vs * (DCTSIZE+2)));
    /* Create space for the scrambled-order pointers */
    sampled_data[1][ci] = (JSAMPARRAY) (*cinfo->emethods->alloc_small)
				(vs * (DCTSIZE+2) * SIZEOF(JSAMPROW));
    /* Duplicate the first DCTSIZE-2 row groups */
    for (i = 0; i < vs * (DCTSIZE-2); i++) {
      sampled_data[1][ci][i] = sampled_data[0][ci][i];
    }
    /* Copy the last four row groups in swapped order */
    for (i = 0; i < vs * 2; i++) {
      sampled_data[1][ci][vs*DCTSIZE + i] = sampled_data[0][ci][vs*(DCTSIZE-2) + i];
      sampled_data[1][ci][vs*(DCTSIZE-2) + i] = sampled_data[0][ci][vs*DCTSIZE + i];
    }
  }
}


#ifdef NEED_COMPLEX_CONTROLLER	/* not used by simple controller */

LOCAL void
free_sampling_buffer (decompress_info_ptr cinfo, JSAMPIMAGE sampled_data[2])
/* Release a sampling buffer created by alloc_sampling_buffer */
{
  short ci;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    /* Free the real storage */
    (*cinfo->emethods->free_small_sarray) (sampled_data[0][ci]);
    /* Free the scrambled-order pointers */
    (*cinfo->emethods->free_small) ((void *) sampled_data[1][ci]);
  }

  /* Free the top-level space */
  (*cinfo->emethods->free_small) ((void *) sampled_data[0]);
  (*cinfo->emethods->free_small) ((void *) sampled_data[1]);
}

#endif


/*
 * Several decompression processes need to range-limit values to the range
 * 0..MAXJSAMPLE; the input value may fall somewhat outside this range
 * due to noise introduced by quantization, roundoff error, etc.  These
 * processes are inner loops and need to be as fast as possible.  On most
 * machines, particularly CPUs with pipelines or instruction prefetch,
 * a (range-check-less) C table lookup
 *		x = sample_range_limit[x];
 * is faster than explicit tests
 *		if (x < 0)  x = 0;
 *		else if (x > MAXJSAMPLE)  x = MAXJSAMPLE;
 * These processes all use a common table prepared by the routine below.
 *
 * The table will work correctly for x within MAXJSAMPLE+1 of the legal
 * range.  This is a much wider range than is needed for most cases,
 * but the wide range is handy for color quantization.
 * Note that the table is allocated in near data space on PCs; it's small
 * enough and used often enough to justify this.
 */

LOCAL void
prepare_range_limit_table (decompress_info_ptr cinfo)
/* Allocate and fill in the sample_range_limit table */
{
  JSAMPLE * table;
  int i;

  table = (JSAMPLE *) (*cinfo->emethods->alloc_small)
			(3 * (MAXJSAMPLE+1) * SIZEOF(JSAMPLE));
  cinfo->sample_range_limit = table + (MAXJSAMPLE+1);
  for (i = 0; i <= MAXJSAMPLE; i++) {
    table[i] = 0;			/* sample_range_limit[x] = 0 for x<0 */
    table[i+(MAXJSAMPLE+1)] = (JSAMPLE) i;	/* sample_range_limit[x] = x */
    table[i+(MAXJSAMPLE+1)*2] = MAXJSAMPLE;	/* x beyond MAXJSAMPLE */
  }
}


LOCAL void
duplicate_row (JSAMPARRAY image_data,
	       long num_cols, int source_row, int num_rows)
/* Duplicate the source_row at source_row+1 .. source_row+num_rows */
/* This happens only at the bottom of the image, */
/* so it needn't be super-efficient */
{
  register int row;

  for (row = 1; row <= num_rows; row++) {
    jcopy_sample_rows(image_data, source_row, image_data, source_row + row,
		      1, num_cols);
  }
}


LOCAL void
expand (decompress_info_ptr cinfo,
	JSAMPIMAGE sampled_data, JSAMPIMAGE fullsize_data,
	long fullsize_width,
	short above, short current, short below, short out)
/* Do upsampling expansion of a single row group (of each component). */
/* above, current, below are indexes of row groups in sampled_data;       */
/* out is the index of the target row group in fullsize_data.             */
/* Special case: above, below can be -1 to indicate top, bottom of image. */
{
  jpeg_component_info *compptr;
  JSAMPARRAY above_ptr, below_ptr;
  JSAMPROW dummy[MAX_SAMP_FACTOR]; /* for downsample expansion at top/bottom */
  short ci, vs, i;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    /* don't bother to upsample an uninteresting component */
    if (! compptr->component_needed)
      continue;

    vs = compptr->v_samp_factor; /* row group height */

    if (above >= 0)
      above_ptr = sampled_data[ci] + above * vs;
    else {
      /* Top of image: make a dummy above-context with copies of 1st row */
      /* We assume current=0 in this case */
      for (i = 0; i < vs; i++)
	dummy[i] = sampled_data[ci][0];
      above_ptr = (JSAMPARRAY) dummy; /* possible near->far pointer conv */
    }

    if (below >= 0)
      below_ptr = sampled_data[ci] + below * vs;
    else {
      /* Bot of image: make a dummy below-context with copies of last row */
      for (i = 0; i < vs; i++)
	dummy[i] = sampled_data[ci][(current+1)*vs-1];
      below_ptr = (JSAMPARRAY) dummy; /* possible near->far pointer conv */
    }

    (*cinfo->methods->upsample[ci])
		(cinfo, (int) ci,
		 compptr->downsampled_width, (int) vs,
		 fullsize_width, (int) cinfo->max_v_samp_factor,
		 above_ptr,
		 sampled_data[ci] + current * vs,
		 below_ptr,
		 fullsize_data[ci] + out * cinfo->max_v_samp_factor);
  }
}


LOCAL void
emit_1pass (decompress_info_ptr cinfo, int num_rows, JSAMPIMAGE fullsize_data,
	    JSAMPARRAY dummy)
/* Do color processing and output of num_rows full-size rows. */
/* This is not used when doing 2-pass color quantization. */
/* The dummy argument simply lets this be called via scan_big_image. */
{
  if (cinfo->quantize_colors) {
    (*cinfo->methods->color_quantize) (cinfo, num_rows, fullsize_data,
				       output_workspace[0]);
  } else {
    (*cinfo->methods->color_convert) (cinfo, num_rows, cinfo->image_width,
				      fullsize_data, output_workspace);
  }
    
  (*cinfo->methods->put_pixel_rows) (cinfo, num_rows, output_workspace);
}


/*
 * Support routines for complex controller.
 */

#ifdef NEED_COMPLEX_CONTROLLER

METHODDEF void
scan_big_image (decompress_info_ptr cinfo, quantize_method_ptr quantize_method)
/* Apply quantize_method to entire image stored in fullsize_image[]. */
/* This is the "iterator" routine used by the 2-pass color quantizer. */
/* We also use it directly in some cases. */
{
  long pixel_rows_output;
  short ci;

  for (pixel_rows_output = 0; pixel_rows_output < cinfo->image_height;
       pixel_rows_output += rows_in_mem) {
    (*cinfo->methods->progress_monitor) (cinfo, pixel_rows_output,
					 cinfo->image_height);
    /* Realign the big buffers */
    for (ci = 0; ci < cinfo->num_components; ci++) {
      fullsize_ptrs[ci] = (*cinfo->emethods->access_big_sarray)
	(fullsize_image[ci], pixel_rows_output, FALSE);
    }
    /* Let the quantizer have its way with the data.
     * Note that output_workspace is simply workspace for the quantizer;
     * when it's ready to output, it must call put_pixel_rows itself.
     */
    (*quantize_method) (cinfo,
			(int) MIN((long) rows_in_mem,
				  cinfo->image_height - pixel_rows_output),
			fullsize_ptrs, output_workspace[0]);
  }

  cinfo->completed_passes++;
}

#endif /* NEED_COMPLEX_CONTROLLER */


/*
 * Support routines for cross-block smoothing.
 */

#ifdef BLOCK_SMOOTHING_SUPPORTED


LOCAL void
smooth_mcu_row (decompress_info_ptr cinfo,
		JBLOCKIMAGE above, JBLOCKIMAGE input, JBLOCKIMAGE below,
		JBLOCKIMAGE output)
/* Apply cross-block smoothing to one MCU row's worth of coefficient blocks. */
/* above,below are NULL if at top/bottom of image. */
{
  jpeg_component_info *compptr;
  short ci, ri, last;
  JBLOCKROW prev;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    /* don't bother to smooth an uninteresting component */
    if (! compptr->component_needed)
      continue;

    last = compptr->MCU_height - 1;

    if (above == NULL)
      prev = NULL;
    else
      prev = above[ci][last];

    for (ri = 0; ri < last; ri++) {
      (*cinfo->methods->smooth_coefficients) (cinfo, compptr,
				prev, input[ci][ri], input[ci][ri+1],
				output[ci][ri]);
      prev = input[ci][ri];
    }

    if (below == NULL)
      (*cinfo->methods->smooth_coefficients) (cinfo, compptr,
				prev, input[ci][last], (JBLOCKROW) NULL,
				output[ci][last]);
    else
      (*cinfo->methods->smooth_coefficients) (cinfo, compptr,
				prev, input[ci][last], below[ci][0],
				output[ci][last]);
  }
}


LOCAL void
get_smoothed_row (decompress_info_ptr cinfo, JBLOCKIMAGE coeff_data,
		  JBLOCKIMAGE bsmooth[3], int * whichb, long cur_mcu_row)
/* Get an MCU row of coefficients, applying cross-block smoothing. */
/* The output row is placed in coeff_data.  bsmooth and whichb hold */
/* working state, and cur_row is needed to check for image top/bottom. */
/* This routine just takes care of the buffering logic. */
{
  int prev, cur, next;
  
  /* Special case for top of image: need to pre-fetch a row & init whichb */
  if (cur_mcu_row == 0) {
    (*cinfo->methods->disassemble_MCU) (cinfo, bsmooth[0]);
    if (cinfo->MCU_rows_in_scan > 1) {
      (*cinfo->methods->disassemble_MCU) (cinfo, bsmooth[1]);
      smooth_mcu_row(cinfo, (JBLOCKIMAGE) NULL, bsmooth[0], bsmooth[1],
		     coeff_data);
    } else {
      smooth_mcu_row(cinfo, (JBLOCKIMAGE) NULL, bsmooth[0], (JBLOCKIMAGE) NULL,
		     coeff_data);
    }
    *whichb = 1;		/* points to next bsmooth[] element to use */
    return;
  }
  
  cur = *whichb;		/* set up references */
  prev = (cur == 0 ? 2 : cur - 1);
  next = (cur == 2 ? 0 : cur + 1);
  *whichb = next;		/* advance whichb for next time */
  
  /* Special case for bottom of image: don't read another row */
  if (cur_mcu_row >= cinfo->MCU_rows_in_scan - 1) {
    smooth_mcu_row(cinfo, bsmooth[prev], bsmooth[cur], (JBLOCKIMAGE) NULL,
		   coeff_data);
    return;
  }
  
  /* Normal case: read ahead a new row, smooth the one I got before */
  (*cinfo->methods->disassemble_MCU) (cinfo, bsmooth[next]);
  smooth_mcu_row(cinfo, bsmooth[prev], bsmooth[cur], bsmooth[next],
		 coeff_data);
}


#endif /* BLOCK_SMOOTHING_SUPPORTED */



/*
 * Decompression pipeline controller used for single-scan files
 * without 2-pass color quantization.
 */

METHODDEF void
simple_dcontroller (decompress_info_ptr cinfo)
{
  long fullsize_width;		/* # of samples per row in full-size buffers */
  long cur_mcu_row;		/* counts # of MCU rows processed */
  long pixel_rows_output;	/* # of pixel rows actually emitted */
  int mcu_rows_per_loop;	/* # of MCU rows processed per outer loop */
  /* Work buffer for dequantized coefficients (IDCT input) */
  JBLOCKIMAGE coeff_data;
  /* Work buffer for cross-block smoothing input */
#ifdef BLOCK_SMOOTHING_SUPPORTED
  JBLOCKIMAGE bsmooth[3];	/* this is optional */
  int whichb;
#endif
  /* Work buffer for downsampled image data (see comments at head of file) */
  JSAMPIMAGE sampled_data[2];
  /* Work buffer for upsampled data */
  JSAMPIMAGE fullsize_data;
  int whichss, ri;
  short i;

  /* Compute dimensions of full-size pixel buffers */
  /* Note these are the same whether interleaved or not. */
  rows_in_mem = cinfo->max_v_samp_factor * DCTSIZE;
  fullsize_width = jround_up(cinfo->image_width,
			     (long) (cinfo->max_h_samp_factor * DCTSIZE));

  /* Prepare for single scan containing all components */
  if (cinfo->comps_in_scan == 1) {
    noninterleaved_scan_setup(cinfo);
    /* Need to read Vk MCU rows to obtain Vk block rows */
    mcu_rows_per_loop = cinfo->cur_comp_info[0]->v_samp_factor;
  } else {
    interleaved_scan_setup(cinfo);
    /* in an interleaved scan, one MCU row provides Vk block rows */
    mcu_rows_per_loop = 1;
  }
  cinfo->total_passes++;

  /* Allocate working memory: */
  prepare_range_limit_table(cinfo);
  /* coeff_data holds a single MCU row of coefficient blocks */
  coeff_data = alloc_MCU_row(cinfo);
  /* if doing cross-block smoothing, need extra space for its input */
#ifdef BLOCK_SMOOTHING_SUPPORTED
  if (cinfo->do_block_smoothing) {
    bsmooth[0] = alloc_MCU_row(cinfo);
    bsmooth[1] = alloc_MCU_row(cinfo);
    bsmooth[2] = alloc_MCU_row(cinfo);
  }
#endif
  /* sampled_data is sample data before upsampling */
  alloc_sampling_buffer(cinfo, sampled_data);
  /* fullsize_data is sample data after upsampling */
  fullsize_data = alloc_sampimage(cinfo, (int) cinfo->num_components,
				  (long) rows_in_mem, fullsize_width);
  /* output_workspace is the color-processed data */
  output_workspace = alloc_sampimage(cinfo, (int) cinfo->final_out_comps,
				     (long) rows_in_mem, fullsize_width);

  /* Tell the memory manager to instantiate big arrays.
   * We don't need any big arrays in this controller,
   * but some other module (like the output file writer) may need one.
   */
  (*cinfo->emethods->alloc_big_arrays)
	((long) 0,				/* no more small sarrays */
	 (long) 0,				/* no more small barrays */
	 (long) 0);				/* no more "medium" objects */
  /* NB: if quantizer needs any "medium" size objects, it must get them */
  /* at color_quant_init time */

  /* Initialize to read scan data */

  (*cinfo->methods->entropy_decode_init) (cinfo);
  (*cinfo->methods->upsample_init) (cinfo);
  (*cinfo->methods->disassemble_init) (cinfo);

  /* Loop over scan's data: rows_in_mem pixel rows are processed per loop */

  pixel_rows_output = 0;
  whichss = 1;			/* arrange to start with sampled_data[0] */

  for (cur_mcu_row = 0; cur_mcu_row < cinfo->MCU_rows_in_scan;
       cur_mcu_row += mcu_rows_per_loop) {
    (*cinfo->methods->progress_monitor) (cinfo, cur_mcu_row,
					 cinfo->MCU_rows_in_scan);

    whichss ^= 1;		/* switch to other downsampled-data buffer */

    /* Obtain v_samp_factor block rows of each component in the scan. */
    /* This is a single MCU row if interleaved, multiple MCU rows if not. */
    /* In the noninterleaved case there might be fewer than v_samp_factor */
    /* block rows remaining; if so, pad with copies of the last pixel row */
    /* so that upsampling doesn't have to treat it as a special case. */

    for (ri = 0; ri < mcu_rows_per_loop; ri++) {
      if (cur_mcu_row + ri < cinfo->MCU_rows_in_scan) {
	/* OK to actually read an MCU row. */
#ifdef BLOCK_SMOOTHING_SUPPORTED
	if (cinfo->do_block_smoothing)
	  get_smoothed_row(cinfo, coeff_data,
			   bsmooth, &whichb, cur_mcu_row + ri);
	else
#endif
	  (*cinfo->methods->disassemble_MCU) (cinfo, coeff_data);
      
	(*cinfo->methods->reverse_DCT) (cinfo, coeff_data,
					sampled_data[whichss],
					ri * DCTSIZE);
      } else {
	/* Need to pad out with copies of the last downsampled row. */
	/* This can only happen if there is just one component. */
	duplicate_row(sampled_data[whichss][0],
		      cinfo->cur_comp_info[0]->downsampled_width,
		      ri * DCTSIZE - 1, DCTSIZE);
      }
    }

    /* Upsample the data */
    /* First time through is a special case */

    if (cur_mcu_row) {
      /* Expand last row group of previous set */
      expand(cinfo, sampled_data[whichss], fullsize_data, fullsize_width,
	     (short) DCTSIZE, (short) (DCTSIZE+1), (short) 0,
	     (short) (DCTSIZE-1));
      /* and dump the previous set's expanded data */
      emit_1pass (cinfo, rows_in_mem, fullsize_data, (JSAMPARRAY) NULL);
      pixel_rows_output += rows_in_mem;
      /* Expand first row group of this set */
      expand(cinfo, sampled_data[whichss], fullsize_data, fullsize_width,
	     (short) (DCTSIZE+1), (short) 0, (short) 1,
	     (short) 0);
    } else {
      /* Expand first row group with dummy above-context */
      expand(cinfo, sampled_data[whichss], fullsize_data, fullsize_width,
	     (short) (-1), (short) 0, (short) 1,
	     (short) 0);
    }
    /* Expand second through next-to-last row groups of this set */
    for (i = 1; i <= DCTSIZE-2; i++) {
      expand(cinfo, sampled_data[whichss], fullsize_data, fullsize_width,
	     (short) (i-1), (short) i, (short) (i+1),
	     (short) i);
    }
  } /* end of outer loop */

  /* Expand the last row group with dummy below-context */
  /* Note whichss points to last buffer side used */
  expand(cinfo, sampled_data[whichss], fullsize_data, fullsize_width,
	 (short) (DCTSIZE-2), (short) (DCTSIZE-1), (short) (-1),
	 (short) (DCTSIZE-1));
  /* and dump the remaining data (may be less than full height) */
  emit_1pass (cinfo, (int) (cinfo->image_height - pixel_rows_output),
	      fullsize_data, (JSAMPARRAY) NULL);

  /* Clean up after the scan */
  (*cinfo->methods->disassemble_term) (cinfo);
  (*cinfo->methods->upsample_term) (cinfo);
  (*cinfo->methods->entropy_decode_term) (cinfo);
  (*cinfo->methods->read_scan_trailer) (cinfo);
  cinfo->completed_passes++;

  /* Verify that we've seen the whole input file */
  if ((*cinfo->methods->read_scan_header) (cinfo))
    WARNMS(cinfo->emethods, "Didn't expect more than one scan");

  /* Release working memory */
  /* (no work -- we let free_all release what's needful) */
}


/*
 * Decompression pipeline controller used for multiple-scan files
 * and/or 2-pass color quantization.
 *
 * The current implementation places the "big" buffer at the stage of
 * upsampled, non-color-processed data.  This is the only place that
 * makes sense when doing 2-pass quantization.  For processing multiple-scan
 * files without 2-pass quantization, it would be possible to develop another
 * controller that buffers the downsampled data instead, thus reducing the size
 * of the temp files (by about a factor of 2 in typical cases).  However,
 * our present upsampling logic is dependent on the assumption that
 * upsampling occurs during a scan, so it's much easier to do the
 * enlargement as the JPEG file is read.  This also simplifies life for the
 * memory manager, which would otherwise have to deal with overlapping
 * access_big_sarray() requests.
 * At present it appears that most JPEG files will be single-scan,
 * so it doesn't seem worthwhile to worry about this optimization.
 */

#ifdef NEED_COMPLEX_CONTROLLER

METHODDEF void
complex_dcontroller (decompress_info_ptr cinfo)
{
  long fullsize_width;		/* # of samples per row in full-size buffers */
  long cur_mcu_row;		/* counts # of MCU rows processed */
  long pixel_rows_output;	/* # of pixel rows actually emitted */
  int mcu_rows_per_loop;	/* # of MCU rows processed per outer loop */
  /* Work buffer for dequantized coefficients (IDCT input) */
  JBLOCKIMAGE coeff_data;
  /* Work buffer for cross-block smoothing input */
#ifdef BLOCK_SMOOTHING_SUPPORTED
  JBLOCKIMAGE bsmooth[3];	/* this is optional */
  int whichb;
#endif
  /* Work buffer for downsampled image data (see comments at head of file) */
  JSAMPIMAGE sampled_data[2];
  int whichss, ri;
  short ci, i;
  boolean single_scan;

  /* Compute dimensions of full-size pixel buffers */
  /* Note these are the same whether interleaved or not. */
  rows_in_mem = cinfo->max_v_samp_factor * DCTSIZE;
  fullsize_width = jround_up(cinfo->image_width,
			     (long) (cinfo->max_h_samp_factor * DCTSIZE));

  /* Allocate all working memory that doesn't depend on scan info */
  prepare_range_limit_table(cinfo);
  /* output_workspace is the color-processed data */
  output_workspace = alloc_sampimage(cinfo, (int) cinfo->final_out_comps,
				     (long) rows_in_mem, fullsize_width);

  /* Get a big image: fullsize_image is sample data after upsampling. */
  fullsize_image = (big_sarray_ptr *) (*cinfo->emethods->alloc_small)
			(cinfo->num_components * SIZEOF(big_sarray_ptr));
  for (ci = 0; ci < cinfo->num_components; ci++) {
    fullsize_image[ci] = (*cinfo->emethods->request_big_sarray)
			(fullsize_width,
			 jround_up(cinfo->image_height, (long) rows_in_mem),
			 (long) rows_in_mem);
  }
  /* Also get an area for pointers to currently accessible chunks */
  fullsize_ptrs = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->num_components * SIZEOF(JSAMPARRAY));

  /* Tell the memory manager to instantiate big arrays */
  (*cinfo->emethods->alloc_big_arrays)
	 /* extra sarray space is for downsampled-data buffers: */
	((long) (fullsize_width			/* max width in samples */
	 * cinfo->max_v_samp_factor*(DCTSIZE+2)	/* max height */
	 * cinfo->num_components),		/* max components per scan */
	 /* extra barray space is for MCU-row buffers: */
	 (long) ((fullsize_width / DCTSIZE)	/* max width in blocks */
	 * cinfo->max_v_samp_factor		/* max height */
	 * cinfo->num_components		/* max components per scan */
	 * (cinfo->do_block_smoothing ? 4 : 1)),/* how many of these we need */
	 /* no extra "medium"-object space */
	 (long) 0);
  /* NB: if quantizer needs any "medium" size objects, it must get them */
  /* at color_quant_init time */

  /* If file is single-scan, we can do color quantization prescan on-the-fly
   * during the scan (we must be doing 2-pass quantization, else this method
   * would not have been selected).  If it is multiple scans, we have to make
   * a separate pass after we've collected all the components.  (We could save
   * some I/O by doing CQ prescan during the last scan, but the extra logic
   * doesn't seem worth the trouble.)
   */

  single_scan = (cinfo->comps_in_scan == cinfo->num_components);

  /* Account for passes needed (color quantizer adds its passes separately).
   * If multiscan file, we guess that each component has its own scan,
   * and increment completed_passes by the number of components in the scan.
   */

  if (single_scan)
    cinfo->total_passes++;	/* the single scan */
  else {
    cinfo->total_passes += cinfo->num_components; /* guessed # of scans */
    if (cinfo->two_pass_quantize)
      cinfo->total_passes++;	/* account for separate CQ prescan pass */
  }
  if (! cinfo->two_pass_quantize)
    cinfo->total_passes++;	/* count output pass unless quantizer does it */

  /* Loop over scans in file */

  do {
    
    /* Prepare for this scan */
    if (cinfo->comps_in_scan == 1) {
      noninterleaved_scan_setup(cinfo);
      /* Need to read Vk MCU rows to obtain Vk block rows */
      mcu_rows_per_loop = cinfo->cur_comp_info[0]->v_samp_factor;
    } else {
      interleaved_scan_setup(cinfo);
      /* in an interleaved scan, one MCU row provides Vk block rows */
      mcu_rows_per_loop = 1;
    }
    
    /* Allocate scan-local working memory */
    /* coeff_data holds a single MCU row of coefficient blocks */
    coeff_data = alloc_MCU_row(cinfo);
    /* if doing cross-block smoothing, need extra space for its input */
#ifdef BLOCK_SMOOTHING_SUPPORTED
    if (cinfo->do_block_smoothing) {
      bsmooth[0] = alloc_MCU_row(cinfo);
      bsmooth[1] = alloc_MCU_row(cinfo);
      bsmooth[2] = alloc_MCU_row(cinfo);
    }
#endif
    /* sampled_data is sample data before upsampling */
    alloc_sampling_buffer(cinfo, sampled_data);

    /* line up the big buffers for components in this scan */
    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      fullsize_ptrs[ci] = (*cinfo->emethods->access_big_sarray)
	(fullsize_image[cinfo->cur_comp_info[ci]->component_index],
	 (long) 0, TRUE);
    }
    
    /* Initialize to read scan data */
    
    (*cinfo->methods->entropy_decode_init) (cinfo);
    (*cinfo->methods->upsample_init) (cinfo);
    (*cinfo->methods->disassemble_init) (cinfo);
    
    /* Loop over scan's data: rows_in_mem pixel rows are processed per loop */
    
    pixel_rows_output = 0;
    whichss = 1;		/* arrange to start with sampled_data[0] */
    
    for (cur_mcu_row = 0; cur_mcu_row < cinfo->MCU_rows_in_scan;
	 cur_mcu_row += mcu_rows_per_loop) {
      (*cinfo->methods->progress_monitor) (cinfo, cur_mcu_row,
					   cinfo->MCU_rows_in_scan);

      whichss ^= 1;		/* switch to other downsampled-data buffer */

      /* Obtain v_samp_factor block rows of each component in the scan. */
      /* This is a single MCU row if interleaved, multiple MCU rows if not. */
      /* In the noninterleaved case there might be fewer than v_samp_factor */
      /* block rows remaining; if so, pad with copies of the last pixel row */
      /* so that upsampling doesn't have to treat it as a special case. */
      
      for (ri = 0; ri < mcu_rows_per_loop; ri++) {
	if (cur_mcu_row + ri < cinfo->MCU_rows_in_scan) {
	  /* OK to actually read an MCU row. */
#ifdef BLOCK_SMOOTHING_SUPPORTED
	  if (cinfo->do_block_smoothing)
	    get_smoothed_row(cinfo, coeff_data,
			     bsmooth, &whichb, cur_mcu_row + ri);
	  else
#endif
	    (*cinfo->methods->disassemble_MCU) (cinfo, coeff_data);
	  
	  (*cinfo->methods->reverse_DCT) (cinfo, coeff_data,
					  sampled_data[whichss],
					  ri * DCTSIZE);
	} else {
	  /* Need to pad out with copies of the last downsampled row. */
	  /* This can only happen if there is just one component. */
	  duplicate_row(sampled_data[whichss][0],
			cinfo->cur_comp_info[0]->downsampled_width,
			ri * DCTSIZE - 1, DCTSIZE);
	}
      }
      
      /* Upsample the data */
      /* First time through is a special case */
      
      if (cur_mcu_row) {
	/* Expand last row group of previous set */
	expand(cinfo, sampled_data[whichss], fullsize_ptrs, fullsize_width,
	       (short) DCTSIZE, (short) (DCTSIZE+1), (short) 0,
	       (short) (DCTSIZE-1));
	/* If single scan, can do color quantization prescan on-the-fly */
	if (single_scan)
	  (*cinfo->methods->color_quant_prescan) (cinfo, rows_in_mem,
						  fullsize_ptrs,
						  output_workspace[0]);
	/* Realign the big buffers */
	pixel_rows_output += rows_in_mem;
	for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
	  fullsize_ptrs[ci] = (*cinfo->emethods->access_big_sarray)
	    (fullsize_image[cinfo->cur_comp_info[ci]->component_index],
	     pixel_rows_output, TRUE);
	}
	/* Expand first row group of this set */
	expand(cinfo, sampled_data[whichss], fullsize_ptrs, fullsize_width,
	       (short) (DCTSIZE+1), (short) 0, (short) 1,
	       (short) 0);
      } else {
	/* Expand first row group with dummy above-context */
	expand(cinfo, sampled_data[whichss], fullsize_ptrs, fullsize_width,
	       (short) (-1), (short) 0, (short) 1,
	       (short) 0);
      }
      /* Expand second through next-to-last row groups of this set */
      for (i = 1; i <= DCTSIZE-2; i++) {
	expand(cinfo, sampled_data[whichss], fullsize_ptrs, fullsize_width,
	       (short) (i-1), (short) i, (short) (i+1),
	       (short) i);
      }
    } /* end of loop over scan's data */
    
    /* Expand the last row group with dummy below-context */
    /* Note whichss points to last buffer side used */
    expand(cinfo, sampled_data[whichss], fullsize_ptrs, fullsize_width,
	   (short) (DCTSIZE-2), (short) (DCTSIZE-1), (short) (-1),
	   (short) (DCTSIZE-1));
    /* If single scan, finish on-the-fly color quantization prescan */
    if (single_scan)
      (*cinfo->methods->color_quant_prescan) (cinfo,
			(int) (cinfo->image_height - pixel_rows_output),
			fullsize_ptrs, output_workspace[0]);
    
    /* Clean up after the scan */
    (*cinfo->methods->disassemble_term) (cinfo);
    (*cinfo->methods->upsample_term) (cinfo);
    (*cinfo->methods->entropy_decode_term) (cinfo);
    (*cinfo->methods->read_scan_trailer) (cinfo);
    if (single_scan)
      cinfo->completed_passes++;
    else
      cinfo->completed_passes += cinfo->comps_in_scan;

    /* Release scan-local working memory */
    free_MCU_row(cinfo, coeff_data);
#ifdef BLOCK_SMOOTHING_SUPPORTED
    if (cinfo->do_block_smoothing) {
      free_MCU_row(cinfo, bsmooth[0]);
      free_MCU_row(cinfo, bsmooth[1]);
      free_MCU_row(cinfo, bsmooth[2]);
    }
#endif
    free_sampling_buffer(cinfo, sampled_data);
    
    /* Repeat if there is another scan */
  } while ((!single_scan) && (*cinfo->methods->read_scan_header) (cinfo));

  if (single_scan) {
    /* If we expected just one scan, make SURE there's just one */
    if ((*cinfo->methods->read_scan_header) (cinfo))
      WARNMS(cinfo->emethods, "Didn't expect more than one scan");
    /* We did the CQ prescan on-the-fly, so we are all set. */
  } else {
    /* For multiple-scan file, do the CQ prescan as a separate pass. */
    /* The main reason why prescan is passed the output_workspace is */
    /* so that we can use scan_big_image to call it... */
    if (cinfo->two_pass_quantize)
      scan_big_image(cinfo, cinfo->methods->color_quant_prescan);
  }

  /* Now that we've collected the data, do color processing and output */
  if (cinfo->two_pass_quantize)
    (*cinfo->methods->color_quant_doit) (cinfo, scan_big_image);
  else
    scan_big_image(cinfo, emit_1pass);

  /* Release working memory */
  /* (no work -- we let free_all release what's needful) */
}

#endif /* NEED_COMPLEX_CONTROLLER */


/*
 * The method selection routine for decompression pipeline controllers.
 * Note that at this point we've already read the JPEG header and first SOS,
 * so we can tell whether the input is one scan or not.
 */

GLOBAL void
jseldpipeline (decompress_info_ptr cinfo)
{
  /* simplify subsequent tests on color quantization */
  if (! cinfo->quantize_colors)
    cinfo->two_pass_quantize = FALSE;
  
  if (cinfo->comps_in_scan == cinfo->num_components) {
    /* It's a single-scan file */
    if (cinfo->two_pass_quantize) {
#ifdef NEED_COMPLEX_CONTROLLER
      cinfo->methods->d_pipeline_controller = complex_dcontroller;
#else
      ERREXIT(cinfo->emethods, "2-pass quantization support was not compiled");
#endif
    } else
      cinfo->methods->d_pipeline_controller = simple_dcontroller;
  } else {
    /* It's a multiple-scan file */
#ifdef NEED_COMPLEX_CONTROLLER
    cinfo->methods->d_pipeline_controller = complex_dcontroller;
#else
    ERREXIT(cinfo->emethods, "Multiple-scan support was not compiled");
#endif
  }
}

/*
 * jdpipe.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains decompression pipeline controllers.
 * These routines are invoked via the d_pipeline_controller method.
 *
 * There are four basic pipeline controllers, one for each combination of:
 *	single-scan JPEG file (single component or fully interleaved)
 *  vs. multiple-scan JPEG file (noninterleaved or partially interleaved).
 *
 *	2-pass color quantization
 *  vs. no color quantization or 1-pass quantization.
 *
 * Note that these conditions determine the needs for "big" images:
 * multiple scans imply a big image for recombining the color components;
 * 2-pass color quantization needs a big image for saving the data for pass 2.
 *
 * All but the simplest controller (single-scan, no 2-pass quantization) can be
 * compiled out through configuration options, if you need to make a minimal
 * implementation.  You should leave in multiple-scan support if at all
 * possible, so that you can handle all legal JPEG files.
 */

#include "jinclude.h"


/*
 * About the data structures:
 *
 * The processing chunk size for unsubsampling is referred to in this file as
 * a "row group": a row group is defined as Vk (v_samp_factor) sample rows of
 * any component while subsampled, or Vmax (max_v_samp_factor) unsubsampled
 * rows.  In an interleaved scan each MCU row contains exactly DCTSIZE row
 * groups of each component in the scan.  In a noninterleaved scan an MCU row
 * is one row of blocks, which might not be an integral number of row groups;
 * therefore, we read in Vk MCU rows to obtain the same amount of data as we'd
 * have in an interleaved scan.
 * To provide context for the unsubsampling step, we have to retain the last
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
 * If you don't support 2-pass quantization, you could make them locals.
 */

static int rows_in_mem;		/* # of sample rows in full-size buffers */
/* Full-size image array holding desubsampled, color-converted data. */
static big_sarray_ptr *fullsize_cnvt_image;
static JSAMPIMAGE fullsize_cnvt_ptrs; /* workspace for access_big_sarray() results */
/* Work buffer for color quantization output (full size, only 1 component). */
static JSAMPARRAY quantize_out;


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
    compptr->subsampled_width = jround_up(compptr->true_comp_width,
					  (long) (compptr->MCU_width*DCTSIZE));
    compptr->subsampled_height = jround_up(compptr->true_comp_height,
					   (long) (compptr->MCU_height*DCTSIZE));
    /* Sanity check */
    if (compptr->subsampled_width !=
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
  compptr->subsampled_width = jround_up(compptr->true_comp_width,
					(long) DCTSIZE);
  compptr->subsampled_height = jround_up(compptr->true_comp_height,
					 (long) DCTSIZE);

  cinfo->MCUs_per_row = compptr->subsampled_width / DCTSIZE;
  cinfo->MCU_rows_in_scan = compptr->subsampled_height / DCTSIZE;

  /* Prepare array describing MCU composition */
  cinfo->blocks_in_MCU = 1;
  cinfo->MCU_membership[0] = 0;

  (*cinfo->methods->d_per_scan_method_selection) (cinfo);
}


LOCAL void
reverse_DCT (decompress_info_ptr cinfo,
	     JBLOCKIMAGE coeff_data, JSAMPIMAGE output_data,
	     int start_row)
/* Perform inverse DCT on each block in an MCU row's worth of data; */
/* output the results into a sample array starting at row start_row. */
/* NB: start_row can only be nonzero when dealing with a single-component */
/* scan; otherwise we'd have to provide for different offsets for different */
/* components, since the heights of interleaved MCU rows can vary. */
{
  DCTBLOCK block;
  JBLOCKROW browptr;
  JSAMPARRAY srowptr;
  long blocksperrow, bi;
  short numrows, ri;
  short ci;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    /* calc size of an MCU row in this component */
    blocksperrow = cinfo->cur_comp_info[ci]->subsampled_width / DCTSIZE;
    numrows = cinfo->cur_comp_info[ci]->MCU_height;
    /* iterate through all blocks in MCU row */
    for (ri = 0; ri < numrows; ri++) {
      browptr = coeff_data[ci][ri];
      srowptr = output_data[ci] + (ri * DCTSIZE + start_row);
      for (bi = 0; bi < blocksperrow; bi++) {
	/* copy the data into a local DCTBLOCK.  This allows for change of
	 * representation (if DCTELEM != JCOEF).  On 80x86 machines it also
	 * brings the data back from FAR storage to NEAR storage.
	 */
	{ register JCOEFPTR elemptr = browptr[bi];
	  register DCTELEM *localblkptr = block;
	  register short elem = DCTSIZE2;

	  while (--elem >= 0)
	    *localblkptr++ = (DCTELEM) *elemptr++;
	}

	j_rev_dct(block);	/* perform inverse DCT */

	/* output the data into the sample array.
	 * Note change from signed to unsigned representation:
	 * DCT calculation works with values +-CENTERJSAMPLE,
	 * but sample arrays always hold 0..MAXJSAMPLE.
	 * Have to do explicit range-limiting because of quantization errors
	 * and so forth in the DCT/IDCT phase.
	 */
	{ register JSAMPROW elemptr;
	  register DCTELEM *localblkptr = block;
	  register short elemr, elemc;
	  register DCTELEM temp;

	  for (elemr = 0; elemr < DCTSIZE; elemr++) {
	    elemptr = srowptr[elemr] + (bi * DCTSIZE);
	    for (elemc = 0; elemc < DCTSIZE; elemc++) {
	      temp = (*localblkptr++) + CENTERJSAMPLE;
	      if (temp < 0) temp = 0;
	      else if (temp > MAXJSAMPLE) temp = MAXJSAMPLE;
	      *elemptr++ = (JSAMPLE) temp;
	    }
	  }
	}
      }
    }
  }
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


LOCAL void
free_sampimage (decompress_info_ptr cinfo, JSAMPIMAGE image,
		int num_comps, long num_rows)
/* Release a sample image created by alloc_sampimage */
{
  int ci;

  for (ci = 0; ci < num_comps; ci++) {
      (*cinfo->emethods->free_small_sarray) (image[ci], num_rows);
  }
  (*cinfo->emethods->free_small) ((void *) image);
}


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
			(cinfo->cur_comp_info[ci]->subsampled_width / DCTSIZE,
			 (long) cinfo->cur_comp_info[ci]->MCU_height);
  }
  return image;
}


LOCAL void
free_MCU_row (decompress_info_ptr cinfo, JBLOCKIMAGE image)
/* Release a coefficient block array created by alloc_MCU_row */
{
  int ci;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    (*cinfo->emethods->free_small_barray)
		(image[ci], (long) cinfo->cur_comp_info[ci]->MCU_height);
  }
  (*cinfo->emethods->free_small) ((void *) image);
}


LOCAL void
alloc_sampling_buffer (decompress_info_ptr cinfo, JSAMPIMAGE subsampled_data[2])
/* Create a subsampled-data buffer having the desired structure */
/* (see comments at head of file) */
{
  short ci, vs, i;

  /* Get top-level space for array pointers */
  subsampled_data[0] = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->comps_in_scan * SIZEOF(JSAMPARRAY));
  subsampled_data[1] = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->comps_in_scan * SIZEOF(JSAMPARRAY));

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    vs = cinfo->cur_comp_info[ci]->v_samp_factor; /* row group height */
    /* Allocate the real storage */
    subsampled_data[0][ci] = (*cinfo->emethods->alloc_small_sarray)
				(cinfo->cur_comp_info[ci]->subsampled_width,
				(long) (vs * (DCTSIZE+2)));
    /* Create space for the scrambled-order pointers */
    subsampled_data[1][ci] = (JSAMPARRAY) (*cinfo->emethods->alloc_small)
				(vs * (DCTSIZE+2) * SIZEOF(JSAMPROW));
    /* Duplicate the first DCTSIZE-2 row groups */
    for (i = 0; i < vs * (DCTSIZE-2); i++) {
      subsampled_data[1][ci][i] = subsampled_data[0][ci][i];
    }
    /* Copy the last four row groups in swapped order */
    for (i = 0; i < vs * 2; i++) {
      subsampled_data[1][ci][vs*DCTSIZE + i] = subsampled_data[0][ci][vs*(DCTSIZE-2) + i];
      subsampled_data[1][ci][vs*(DCTSIZE-2) + i] = subsampled_data[0][ci][vs*DCTSIZE + i];
    }
  }
}


LOCAL void
free_sampling_buffer (decompress_info_ptr cinfo, JSAMPIMAGE subsampled_data[2])
/* Release a sampling buffer created by alloc_sampling_buffer */
{
  short ci, vs;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    vs = cinfo->cur_comp_info[ci]->v_samp_factor; /* row group height */
    /* Free the real storage */
    (*cinfo->emethods->free_small_sarray)
		(subsampled_data[0][ci], (long) (vs * (DCTSIZE+2)));
    /* Free the scrambled-order pointers */
    (*cinfo->emethods->free_small) ((void *) subsampled_data[1][ci]);
  }

  /* Free the top-level space */
  (*cinfo->emethods->free_small) ((void *) subsampled_data[0]);
  (*cinfo->emethods->free_small) ((void *) subsampled_data[1]);
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
	JSAMPIMAGE subsampled_data, JSAMPIMAGE fullsize_data,
	long fullsize_width,
	short above, short current, short below, short out)
/* Do unsubsampling expansion of a single row group (of each component).  */
/* above, current, below are indexes of row groups in subsampled_data;    */
/* out is the index of the target row group in fullsize_data.             */
/* Special case: above, below can be -1 to indicate top, bottom of image. */
{
  jpeg_component_info *compptr;
  JSAMPARRAY above_ptr, below_ptr;
  JSAMPROW dummy[MAX_SAMP_FACTOR]; /* for subsample expansion at top/bottom */
  short ci, vs, i;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    vs = compptr->v_samp_factor; /* row group height */

    if (above >= 0)
      above_ptr = subsampled_data[ci] + above * vs;
    else {
      /* Top of image: make a dummy above-context with copies of 1st row */
      /* We assume current=0 in this case */
      for (i = 0; i < vs; i++)
	dummy[i] = subsampled_data[ci][0];
      above_ptr = (JSAMPARRAY) dummy; /* possible near->far pointer conv */
    }

    if (below >= 0)
      below_ptr = subsampled_data[ci] + below * vs;
    else {
      /* Bot of image: make a dummy below-context with copies of last row */
      for (i = 0; i < vs; i++)
	dummy[i] = subsampled_data[ci][(current+1)*vs-1];
      below_ptr = (JSAMPARRAY) dummy; /* possible near->far pointer conv */
    }

    (*cinfo->methods->unsubsample[ci])
		(cinfo, (int) ci,
		 compptr->subsampled_width, (int) vs,
		 fullsize_width, (int) cinfo->max_v_samp_factor,
		 above_ptr,
		 subsampled_data[ci] + current * vs,
		 below_ptr,
		 fullsize_data[ci] + out * cinfo->max_v_samp_factor);
  }
}


LOCAL void
emit_1pass (decompress_info_ptr cinfo, int num_rows,
	    JSAMPIMAGE fullsize_data, JSAMPIMAGE color_data)
/* Do color conversion and output of num_rows full-size rows. */
/* This is not used for 2-pass color quantization. */
{
  (*cinfo->methods->color_convert) (cinfo, num_rows,
				    fullsize_data, color_data);

  if (cinfo->quantize_colors) {
    (*cinfo->methods->color_quantize) (cinfo, num_rows,
				       color_data, quantize_out);

    (*cinfo->methods->put_pixel_rows) (cinfo, num_rows,
				       &quantize_out);
  } else {
    (*cinfo->methods->put_pixel_rows) (cinfo, num_rows,
				       color_data);
  }
}


/*
 * Support routines for 2-pass color quantization.
 */

#ifdef QUANT_2PASS_SUPPORTED

LOCAL void
emit_2pass (decompress_info_ptr cinfo, long top_row, int num_rows,
	    JSAMPIMAGE fullsize_data)
/* Do color conversion and output data to the quantization buffer image. */
/* This is used only with 2-pass color quantization. */
{
  short ci;

  /* Realign the big buffers */
  for (ci = 0; ci < cinfo->num_components; ci++) {
    fullsize_cnvt_ptrs[ci] = (*cinfo->emethods->access_big_sarray)
      (fullsize_cnvt_image[ci], top_row, TRUE);
  }

  /* Do colorspace conversion */
  (*cinfo->methods->color_convert) (cinfo, num_rows,
				    fullsize_data, fullsize_cnvt_ptrs);
  /* Let quantizer get first-pass peek at the data. */
  /* (Quantizer could change data if it wants to.)  */
  (*cinfo->methods->color_quant_prescan) (cinfo, num_rows, fullsize_cnvt_ptrs);
}


METHODDEF void
scan_big_image (decompress_info_ptr cinfo, quantize_method_ptr quantize_method)
/* This is the "iterator" routine used by the quantizer. */
{
  long pixel_rows_output;
  short ci;

  for (pixel_rows_output = 0; pixel_rows_output < cinfo->image_height;
       pixel_rows_output += rows_in_mem) {
    /* Realign the big buffers */
    for (ci = 0; ci < cinfo->num_components; ci++) {
      fullsize_cnvt_ptrs[ci] = (*cinfo->emethods->access_big_sarray)
	(fullsize_cnvt_image[ci], pixel_rows_output, FALSE);
    }
    /* Let the quantizer have its way with the data.
     * Note that quantize_out is simply workspace for the quantizer;
     * when it's ready to output, it must call put_pixel_rows itself.
     */
    (*quantize_method) (cinfo,
			(int) MIN(rows_in_mem,
				  cinfo->image_height - pixel_rows_output),
			fullsize_cnvt_ptrs, quantize_out);
  }
}

#endif /* QUANT_2PASS_SUPPORTED */


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
single_dcontroller (decompress_info_ptr cinfo)
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
  /* Work buffer for subsampled image data (see comments at head of file) */
  JSAMPIMAGE subsampled_data[2];
  /* Work buffer for desubsampled data */
  JSAMPIMAGE fullsize_data;
  /* Work buffer for color conversion output (full size) */
  JSAMPIMAGE color_data;
  int whichss, ri;
  short i;

  /* Initialize for 1-pass color quantization, if needed */
  if (cinfo->quantize_colors)
    (*cinfo->methods->color_quant_init) (cinfo);

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

  /* Compute dimensions of full-size pixel buffers */
  /* Note these are the same whether interleaved or not. */
  rows_in_mem = cinfo->max_v_samp_factor * DCTSIZE;
  fullsize_width = jround_up(cinfo->image_width,
			     (long) (cinfo->max_h_samp_factor * DCTSIZE));

  /* Allocate working memory: */
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
  /* subsampled_data is sample data before unsubsampling */
  alloc_sampling_buffer(cinfo, subsampled_data);
  /* fullsize_data is sample data after unsubsampling */
  fullsize_data = alloc_sampimage(cinfo, (int) cinfo->num_components,
				  (long) rows_in_mem, fullsize_width);
  /* color_data is the result of the colorspace conversion step */
  color_data = alloc_sampimage(cinfo, (int) cinfo->color_out_comps,
			       (long) rows_in_mem, fullsize_width);
  /* if quantizing colors, also need a one-component output area for that. */
  if (cinfo->quantize_colors)
    quantize_out = (*cinfo->emethods->alloc_small_sarray)
				(fullsize_width, (long) rows_in_mem);

  /* Tell the memory manager to instantiate big arrays.
   * We don't need any big arrays in this controller,
   * but some other module (like the output file writer) may need one.
   */
  (*cinfo->emethods->alloc_big_arrays)
	((long) 0,				/* no more small sarrays */
	 (long) 0,				/* no more small barrays */
	 (long) 0);				/* no more "medium" objects */
	 /* NB: quantizer must get any such objects at color_quant_init time */

  /* Initialize to read scan data */

  (*cinfo->methods->entropy_decoder_init) (cinfo);
  (*cinfo->methods->unsubsample_init) (cinfo);
  (*cinfo->methods->disassemble_init) (cinfo);

  /* Loop over scan's data: rows_in_mem pixel rows are processed per loop */

  pixel_rows_output = 0;
  whichss = 1;			/* arrange to start with subsampled_data[0] */

  for (cur_mcu_row = 0; cur_mcu_row < cinfo->MCU_rows_in_scan;
       cur_mcu_row += mcu_rows_per_loop) {
    whichss ^= 1;		/* switch to other subsample buffer */

    /* Obtain v_samp_factor block rows of each component in the scan. */
    /* This is a single MCU row if interleaved, multiple MCU rows if not. */
    /* In the noninterleaved case there might be fewer than v_samp_factor */
    /* block rows remaining; if so, pad with copies of the last pixel row */
    /* so that unsubsampling doesn't have to treat it as a special case. */

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
      
	reverse_DCT(cinfo, coeff_data, subsampled_data[whichss],
		    ri * DCTSIZE);
      } else {
	/* Need to pad out with copies of the last subsampled row. */
	/* This can only happen if there is just one component. */
	duplicate_row(subsampled_data[whichss][0],
		      cinfo->cur_comp_info[0]->subsampled_width,
		      ri * DCTSIZE - 1, DCTSIZE);
      }
    }

    /* Unsubsample the data */
    /* First time through is a special case */

    if (cur_mcu_row) {
      /* Expand last row group of previous set */
      expand(cinfo, subsampled_data[whichss], fullsize_data, fullsize_width,
	     (short) DCTSIZE, (short) (DCTSIZE+1), (short) 0,
	     (short) (DCTSIZE-1));
      /* and dump the previous set's expanded data */
      emit_1pass (cinfo, rows_in_mem, fullsize_data, color_data);
      pixel_rows_output += rows_in_mem;
      /* Expand first row group of this set */
      expand(cinfo, subsampled_data[whichss], fullsize_data, fullsize_width,
	     (short) (DCTSIZE+1), (short) 0, (short) 1,
	     (short) 0);
    } else {
      /* Expand first row group with dummy above-context */
      expand(cinfo, subsampled_data[whichss], fullsize_data, fullsize_width,
	     (short) (-1), (short) 0, (short) 1,
	     (short) 0);
    }
    /* Expand second through next-to-last row groups of this set */
    for (i = 1; i <= DCTSIZE-2; i++) {
      expand(cinfo, subsampled_data[whichss], fullsize_data, fullsize_width,
	     (short) (i-1), (short) i, (short) (i+1),
	     (short) i);
    }
  } /* end of outer loop */

  /* Expand the last row group with dummy below-context */
  /* Note whichss points to last buffer side used */
  expand(cinfo, subsampled_data[whichss], fullsize_data, fullsize_width,
	 (short) (DCTSIZE-2), (short) (DCTSIZE-1), (short) (-1),
	 (short) (DCTSIZE-1));
  /* and dump the remaining data (may be less than full height) */
  emit_1pass (cinfo, (int) (cinfo->image_height - pixel_rows_output),
	      fullsize_data, color_data);

  /* Clean up after the scan */
  (*cinfo->methods->disassemble_term) (cinfo);
  (*cinfo->methods->unsubsample_term) (cinfo);
  (*cinfo->methods->entropy_decoder_term) (cinfo);
  (*cinfo->methods->read_scan_trailer) (cinfo);

  /* Verify that we've seen the whole input file */
  if ((*cinfo->methods->read_scan_header) (cinfo))
    ERREXIT(cinfo->emethods, "Didn't expect more than one scan");

  /* Release working memory */
  free_MCU_row(cinfo, coeff_data);
#ifdef BLOCK_SMOOTHING_SUPPORTED
  if (cinfo->do_block_smoothing) {
    free_MCU_row(cinfo, bsmooth[0]);
    free_MCU_row(cinfo, bsmooth[1]);
    free_MCU_row(cinfo, bsmooth[2]);
  }
#endif
  free_sampling_buffer(cinfo, subsampled_data);
  free_sampimage(cinfo, fullsize_data, (int) cinfo->num_components,
		 (long) rows_in_mem);
  free_sampimage(cinfo, color_data, (int) cinfo->color_out_comps,
		 (long) rows_in_mem);
  if (cinfo->quantize_colors)
    (*cinfo->emethods->free_small_sarray)
		(quantize_out, (long) rows_in_mem);

  /* Close up shop */
  if (cinfo->quantize_colors)
    (*cinfo->methods->color_quant_term) (cinfo);
}


/*
 * Decompression pipeline controller used for single-scan files
 * with 2-pass color quantization.
 */

#ifdef QUANT_2PASS_SUPPORTED

METHODDEF void
single_2quant_dcontroller (decompress_info_ptr cinfo)
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
  /* Work buffer for subsampled image data (see comments at head of file) */
  JSAMPIMAGE subsampled_data[2];
  /* Work buffer for desubsampled data */
  JSAMPIMAGE fullsize_data;
  int whichss, ri;
  short ci, i;

  /* Initialize for 2-pass color quantization */
  (*cinfo->methods->color_quant_init) (cinfo);

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

  /* Compute dimensions of full-size pixel buffers */
  /* Note these are the same whether interleaved or not. */
  rows_in_mem = cinfo->max_v_samp_factor * DCTSIZE;
  fullsize_width = jround_up(cinfo->image_width,
			     (long) (cinfo->max_h_samp_factor * DCTSIZE));

  /* Allocate working memory: */
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
  /* subsampled_data is sample data before unsubsampling */
  alloc_sampling_buffer(cinfo, subsampled_data);
  /* fullsize_data is sample data after unsubsampling */
  fullsize_data = alloc_sampimage(cinfo, (int) cinfo->num_components,
				  (long) rows_in_mem, fullsize_width);
  /* Also need a one-component output area for color quantizer. */
  quantize_out = (*cinfo->emethods->alloc_small_sarray)
				(fullsize_width, (long) rows_in_mem);

  /* Get a big image for quantizer input: desubsampled, color-converted data */
  fullsize_cnvt_image = (big_sarray_ptr *) (*cinfo->emethods->alloc_small)
			(cinfo->num_components * SIZEOF(big_sarray_ptr));
  for (ci = 0; ci < cinfo->num_components; ci++) {
    fullsize_cnvt_image[ci] = (*cinfo->emethods->request_big_sarray)
			(fullsize_width,
			 jround_up(cinfo->image_height, (long) rows_in_mem),
			 (long) rows_in_mem);
  }
  /* Also get an area for pointers to currently accessible chunks */
  fullsize_cnvt_ptrs = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->num_components * SIZEOF(JSAMPARRAY));

  /* Tell the memory manager to instantiate big arrays */
  (*cinfo->emethods->alloc_big_arrays)
	((long) 0,				/* no more small sarrays */
	 (long) 0,				/* no more small barrays */
	 (long) 0);				/* no more "medium" objects */
	 /* NB: quantizer must get any such objects at color_quant_init time */

  /* Initialize to read scan data */

  (*cinfo->methods->entropy_decoder_init) (cinfo);
  (*cinfo->methods->unsubsample_init) (cinfo);
  (*cinfo->methods->disassemble_init) (cinfo);

  /* Loop over scan's data: rows_in_mem pixel rows are processed per loop */

  pixel_rows_output = 0;
  whichss = 1;			/* arrange to start with subsampled_data[0] */

  for (cur_mcu_row = 0; cur_mcu_row < cinfo->MCU_rows_in_scan;
       cur_mcu_row += mcu_rows_per_loop) {
    whichss ^= 1;		/* switch to other subsample buffer */

    /* Obtain v_samp_factor block rows of each component in the scan. */
    /* This is a single MCU row if interleaved, multiple MCU rows if not. */
    /* In the noninterleaved case there might be fewer than v_samp_factor */
    /* block rows remaining; if so, pad with copies of the last pixel row */
    /* so that unsubsampling doesn't have to treat it as a special case. */

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
      
	reverse_DCT(cinfo, coeff_data, subsampled_data[whichss],
		    ri * DCTSIZE);
      } else {
	/* Need to pad out with copies of the last subsampled row. */
	/* This can only happen if there is just one component. */
	duplicate_row(subsampled_data[whichss][0],
		      cinfo->cur_comp_info[0]->subsampled_width,
		      ri * DCTSIZE - 1, DCTSIZE);
      }
    }

    /* Unsubsample the data */
    /* First time through is a special case */

    if (cur_mcu_row) {
      /* Expand last row group of previous set */
      expand(cinfo, subsampled_data[whichss], fullsize_data, fullsize_width,
	     (short) DCTSIZE, (short) (DCTSIZE+1), (short) 0,
	     (short) (DCTSIZE-1));
      /* and dump the previous set's expanded data */
      emit_2pass (cinfo, pixel_rows_output, rows_in_mem, fullsize_data);
      pixel_rows_output += rows_in_mem;
      /* Expand first row group of this set */
      expand(cinfo, subsampled_data[whichss], fullsize_data, fullsize_width,
	     (short) (DCTSIZE+1), (short) 0, (short) 1,
	     (short) 0);
    } else {
      /* Expand first row group with dummy above-context */
      expand(cinfo, subsampled_data[whichss], fullsize_data, fullsize_width,
	     (short) (-1), (short) 0, (short) 1,
	     (short) 0);
    }
    /* Expand second through next-to-last row groups of this set */
    for (i = 1; i <= DCTSIZE-2; i++) {
      expand(cinfo, subsampled_data[whichss], fullsize_data, fullsize_width,
	     (short) (i-1), (short) i, (short) (i+1),
	     (short) i);
    }
  } /* end of outer loop */

  /* Expand the last row group with dummy below-context */
  /* Note whichss points to last buffer side used */
  expand(cinfo, subsampled_data[whichss], fullsize_data, fullsize_width,
	 (short) (DCTSIZE-2), (short) (DCTSIZE-1), (short) (-1),
	 (short) (DCTSIZE-1));
  /* and dump the remaining data (may be less than full height) */
  emit_2pass (cinfo, pixel_rows_output,
	      (int) (cinfo->image_height - pixel_rows_output),
	      fullsize_data);

  /* Clean up after the scan */
  (*cinfo->methods->disassemble_term) (cinfo);
  (*cinfo->methods->unsubsample_term) (cinfo);
  (*cinfo->methods->entropy_decoder_term) (cinfo);
  (*cinfo->methods->read_scan_trailer) (cinfo);

  /* Verify that we've seen the whole input file */
  if ((*cinfo->methods->read_scan_header) (cinfo))
    ERREXIT(cinfo->emethods, "Didn't expect more than one scan");

  /* Now that we've collected the data, let the color quantizer do its thing */
  (*cinfo->methods->color_quant_doit) (cinfo, scan_big_image);

  /* Release working memory */
  free_MCU_row(cinfo, coeff_data);
#ifdef BLOCK_SMOOTHING_SUPPORTED
  if (cinfo->do_block_smoothing) {
    free_MCU_row(cinfo, bsmooth[0]);
    free_MCU_row(cinfo, bsmooth[1]);
    free_MCU_row(cinfo, bsmooth[2]);
  }
#endif
  free_sampling_buffer(cinfo, subsampled_data);
  free_sampimage(cinfo, fullsize_data, (int) cinfo->num_components,
		 (long) rows_in_mem);
  (*cinfo->emethods->free_small_sarray)
		(quantize_out, (long) rows_in_mem);
  for (ci = 0; ci < cinfo->num_components; ci++) {
    (*cinfo->emethods->free_big_sarray) (fullsize_cnvt_image[ci]);
  }
  (*cinfo->emethods->free_small) ((void *) fullsize_cnvt_image);
  (*cinfo->emethods->free_small) ((void *) fullsize_cnvt_ptrs);

  /* Close up shop */
  (*cinfo->methods->color_quant_term) (cinfo);
}

#endif /* QUANT_2PASS_SUPPORTED */


/*
 * Decompression pipeline controller used for multiple-scan files
 * without 2-pass color quantization.
 *
 * The current implementation places the "big" buffer at the stage of
 * desubsampled data.  Buffering subsampled data instead would reduce the
 * size of temp files (by about a factor of 2 in typical cases).  However,
 * the unsubsampling logic is dependent on the assumption that unsubsampling
 * occurs during a scan, so it's much easier to do the enlargement as the
 * JPEG file is read.  This also simplifies life for the memory manager,
 * which would otherwise have to deal with overlapping access_big_sarray()
 * requests.
 *
 * At present it appears that most JPEG files will be single-scan, so
 * it doesn't seem worthwhile to try to make this implementation smarter.
 */

#ifdef MULTISCAN_FILES_SUPPORTED

METHODDEF void
multi_dcontroller (decompress_info_ptr cinfo)
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
  /* Work buffer for subsampled image data (see comments at head of file) */
  JSAMPIMAGE subsampled_data[2];
  /* Full-image buffer holding desubsampled, but not color-converted, data */
  big_sarray_ptr *fullsize_image;
  JSAMPIMAGE fullsize_ptrs;	/* workspace for access_big_sarray() results */
  /* Work buffer for color conversion output (full size) */
  JSAMPIMAGE color_data;
  int whichss, ri;
  short ci, i;

  /* Initialize for 1-pass color quantization, if needed */
  if (cinfo->quantize_colors)
    (*cinfo->methods->color_quant_init) (cinfo);

  /* Compute dimensions of full-size pixel buffers */
  /* Note these are the same whether interleaved or not. */
  rows_in_mem = cinfo->max_v_samp_factor * DCTSIZE;
  fullsize_width = jround_up(cinfo->image_width,
			     (long) (cinfo->max_h_samp_factor * DCTSIZE));

  /* Allocate all working memory that doesn't depend on scan info */
  /* color_data is the result of the colorspace conversion step */
  color_data = alloc_sampimage(cinfo, (int) cinfo->color_out_comps,
			       (long) rows_in_mem, fullsize_width);
  /* if quantizing colors, also need a one-component output area for that. */
  if (cinfo->quantize_colors)
    quantize_out = (*cinfo->emethods->alloc_small_sarray)
				(fullsize_width, (long) rows_in_mem);

  /* Get a big image: fullsize_image is sample data after unsubsampling. */
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
	 /* extra sarray space is for subsampled-data buffers: */
	((long) (fullsize_width			/* max width in samples */
	 * cinfo->max_v_samp_factor*(DCTSIZE+2)	/* max height */
	 * cinfo->num_components),		/* max components per scan */
	 /* extra barray space is for MCU-row buffers: */
	 (long) ((fullsize_width / DCTSIZE)	/* max width in blocks */
	 * cinfo->max_v_samp_factor		/* max height */
	 * cinfo->num_components		/* max components per scan */
	 * (cinfo->do_block_smoothing ? 4 : 1)),/* how many of these we need */
	 /* no extra "medium"-object space */
	 /* NB: quantizer must get any such objects at color_quant_init time */
	 (long) 0);


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
    /* subsampled_data is sample data before unsubsampling */
    alloc_sampling_buffer(cinfo, subsampled_data);

    /* line up the big buffers */
    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      fullsize_ptrs[ci] = (*cinfo->emethods->access_big_sarray)
	(fullsize_image[cinfo->cur_comp_info[ci]->component_index],
	 (long) 0, TRUE);
    }
    
    /* Initialize to read scan data */
    
    (*cinfo->methods->entropy_decoder_init) (cinfo);
    (*cinfo->methods->unsubsample_init) (cinfo);
    (*cinfo->methods->disassemble_init) (cinfo);
    
    /* Loop over scan's data: rows_in_mem pixel rows are processed per loop */
    
    pixel_rows_output = 0;
    whichss = 1;		/* arrange to start with subsampled_data[0] */
    
    for (cur_mcu_row = 0; cur_mcu_row < cinfo->MCU_rows_in_scan;
	 cur_mcu_row += mcu_rows_per_loop) {
      whichss ^= 1;		/* switch to other subsample buffer */

      /* Obtain v_samp_factor block rows of each component in the scan. */
      /* This is a single MCU row if interleaved, multiple MCU rows if not. */
      /* In the noninterleaved case there might be fewer than v_samp_factor */
      /* block rows remaining; if so, pad with copies of the last pixel row */
      /* so that unsubsampling doesn't have to treat it as a special case. */
      
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
	  
	  reverse_DCT(cinfo, coeff_data, subsampled_data[whichss],
		      ri * DCTSIZE);
	} else {
	  /* Need to pad out with copies of the last subsampled row. */
	  /* This can only happen if there is just one component. */
	  duplicate_row(subsampled_data[whichss][0],
			cinfo->cur_comp_info[0]->subsampled_width,
			ri * DCTSIZE - 1, DCTSIZE);
	}
      }
      
      /* Unsubsample the data */
      /* First time through is a special case */
      
      if (cur_mcu_row) {
	/* Expand last row group of previous set */
	expand(cinfo, subsampled_data[whichss], fullsize_ptrs, fullsize_width,
	       (short) DCTSIZE, (short) (DCTSIZE+1), (short) 0,
	       (short) (DCTSIZE-1));
	/* Realign the big buffers */
	pixel_rows_output += rows_in_mem;
	for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
	  fullsize_ptrs[ci] = (*cinfo->emethods->access_big_sarray)
	    (fullsize_image[cinfo->cur_comp_info[ci]->component_index],
	     pixel_rows_output, TRUE);
	}
	/* Expand first row group of this set */
	expand(cinfo, subsampled_data[whichss], fullsize_ptrs, fullsize_width,
	       (short) (DCTSIZE+1), (short) 0, (short) 1,
	       (short) 0);
      } else {
	/* Expand first row group with dummy above-context */
	expand(cinfo, subsampled_data[whichss], fullsize_ptrs, fullsize_width,
	       (short) (-1), (short) 0, (short) 1,
	       (short) 0);
      }
      /* Expand second through next-to-last row groups of this set */
      for (i = 1; i <= DCTSIZE-2; i++) {
	expand(cinfo, subsampled_data[whichss], fullsize_ptrs, fullsize_width,
	       (short) (i-1), (short) i, (short) (i+1),
	       (short) i);
      }
    } /* end of outer loop */
    
    /* Expand the last row group with dummy below-context */
    /* Note whichss points to last buffer side used */
    expand(cinfo, subsampled_data[whichss], fullsize_ptrs, fullsize_width,
	   (short) (DCTSIZE-2), (short) (DCTSIZE-1), (short) (-1),
	   (short) (DCTSIZE-1));
    
    /* Clean up after the scan */
    (*cinfo->methods->disassemble_term) (cinfo);
    (*cinfo->methods->unsubsample_term) (cinfo);
    (*cinfo->methods->entropy_decoder_term) (cinfo);
    (*cinfo->methods->read_scan_trailer) (cinfo);

    /* Release scan-local working memory */
    free_MCU_row(cinfo, coeff_data);
#ifdef BLOCK_SMOOTHING_SUPPORTED
    if (cinfo->do_block_smoothing) {
      free_MCU_row(cinfo, bsmooth[0]);
      free_MCU_row(cinfo, bsmooth[1]);
      free_MCU_row(cinfo, bsmooth[2]);
    }
#endif
    free_sampling_buffer(cinfo, subsampled_data);
    
    /* Repeat if there is another scan */
  } while ((*cinfo->methods->read_scan_header) (cinfo));

  /* Now that we've collected all the data, color convert & output it. */

  for (pixel_rows_output = 0; pixel_rows_output < cinfo->image_height;
       pixel_rows_output += rows_in_mem) {

    /* realign the big buffers */
    for (ci = 0; ci < cinfo->num_components; ci++) {
      fullsize_ptrs[ci] = (*cinfo->emethods->access_big_sarray)
	(fullsize_image[ci], pixel_rows_output, FALSE);
    }

    emit_1pass (cinfo,
		(int) MIN(rows_in_mem, cinfo->image_height-pixel_rows_output),
		fullsize_ptrs, color_data);
  }

  /* Release working memory */
  free_sampimage(cinfo, color_data, (int) cinfo->color_out_comps,
		 (long) rows_in_mem);
  if (cinfo->quantize_colors)
    (*cinfo->emethods->free_small_sarray)
		(quantize_out, (long) rows_in_mem);
  for (ci = 0; ci < cinfo->num_components; ci++) {
    (*cinfo->emethods->free_big_sarray) (fullsize_image[ci]);
  }
  (*cinfo->emethods->free_small) ((void *) fullsize_image);
  (*cinfo->emethods->free_small) ((void *) fullsize_ptrs);

  /* Close up shop */
  if (cinfo->quantize_colors)
    (*cinfo->methods->color_quant_term) (cinfo);
}

#endif /* MULTISCAN_FILES_SUPPORTED */


/*
 * Decompression pipeline controller used for multiple-scan files
 * with 2-pass color quantization.
 */

#ifdef MULTISCAN_FILES_SUPPORTED
#ifdef QUANT_2PASS_SUPPORTED

METHODDEF void
multi_2quant_dcontroller (decompress_info_ptr cinfo)
{
  ERREXIT(cinfo->emethods, "Not implemented yet");
}

#endif /* QUANT_2PASS_SUPPORTED */
#endif /* MULTISCAN_FILES_SUPPORTED */


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
#ifdef QUANT_2PASS_SUPPORTED
    if (cinfo->two_pass_quantize)
      cinfo->methods->d_pipeline_controller = single_2quant_dcontroller;
    else
#endif
      cinfo->methods->d_pipeline_controller = single_dcontroller;
  } else {
    /* It's a multiple-scan file */
#ifdef MULTISCAN_FILES_SUPPORTED
#ifdef QUANT_2PASS_SUPPORTED
    if (cinfo->two_pass_quantize)
      cinfo->methods->d_pipeline_controller = multi_2quant_dcontroller;
    else
#endif
      cinfo->methods->d_pipeline_controller = multi_dcontroller;
#else
    ERREXIT(cinfo->emethods, "Multiple-scan support was not compiled");
#endif
  }
}

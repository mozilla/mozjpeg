/*
 * jcpipe.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains compression pipeline controllers.
 * These routines are invoked via the c_pipeline_controller method.
 *
 * There are four basic pipeline controllers, one for each combination of:
 *	single-scan JPEG file (single component or fully interleaved)
 *  vs. multiple-scan JPEG file (noninterleaved or partially interleaved).
 *
 *	optimization of entropy encoding parameters
 *  vs. usage of default encoding parameters.
 *
 * Note that these conditions determine the needs for "big" arrays:
 * multiple scans imply a big array for splitting the color components;
 * entropy encoding optimization needs a big array for the MCU data.
 *
 * All but the simplest controller (single-scan, no optimization) can be
 * compiled out through configuration options, if you need to make a minimal
 * implementation.
 */

#include "jinclude.h"


/*
 * About the data structures:
 *
 * The processing chunk size for downsampling is referred to in this file as
 * a "row group": a row group is defined as Vk (v_samp_factor) sample rows of
 * any component after downsampling, or Vmax (max_v_samp_factor) unsubsampled
 * rows.  In an interleaved scan each MCU row contains exactly DCTSIZE row
 * groups of each component in the scan.  In a noninterleaved scan an MCU row
 * is one row of blocks, which might not be an integral number of row groups;
 * for convenience we use a buffer of the same size as in interleaved scans,
 * and process Vk MCU rows in each burst of downsampling.
 * To provide context for the downsampling step, we have to retain the last
 * two row groups of the previous MCU row while reading in the next MCU row
 * (or set of Vk MCU rows).  To do this without copying data about, we create
 * a rather strange data structure.  Exactly DCTSIZE+2 row groups of samples
 * are allocated, but we create two different sets of pointers to this array.
 * The second set swaps the last two pairs of row groups.  By working
 * alternately with the two sets of pointers, we can access the data in the
 * desired order.
 */



/*
 * Utility routines: common code for pipeline controllers
 */

LOCAL void
interleaved_scan_setup (compress_info_ptr cinfo)
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

  /* Convert restart specified in rows to actual MCU count. */
  /* Note that count must fit in 16 bits, so we provide limiting. */
  if (cinfo->restart_in_rows > 0) {
    long nominal = cinfo->restart_in_rows * cinfo->MCUs_per_row;
    cinfo->restart_interval = (UINT16) MIN(nominal, 65535L);
  }

  (*cinfo->methods->c_per_scan_method_selection) (cinfo);
}


LOCAL void
noninterleaved_scan_setup (compress_info_ptr cinfo)
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

  /* Convert restart specified in rows to actual MCU count. */
  /* Note that count must fit in 16 bits, so we provide limiting. */
  if (cinfo->restart_in_rows > 0) {
    long nominal = cinfo->restart_in_rows * cinfo->MCUs_per_row;
    cinfo->restart_interval = (UINT16) MIN(nominal, 65535L);
  }

  (*cinfo->methods->c_per_scan_method_selection) (cinfo);
}



LOCAL void
alloc_sampling_buffer (compress_info_ptr cinfo, JSAMPIMAGE fullsize_data[2],
		       long fullsize_width)
/* Create a pre-downsampling data buffer having the desired structure */
/* (see comments at head of file) */
{
  short ci, vs, i;

  vs = cinfo->max_v_samp_factor; /* row group height */

  /* Get top-level space for array pointers */
  fullsize_data[0] = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->num_components * SIZEOF(JSAMPARRAY));
  fullsize_data[1] = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->num_components * SIZEOF(JSAMPARRAY));

  for (ci = 0; ci < cinfo->num_components; ci++) {
    /* Allocate the real storage */
    fullsize_data[0][ci] = (*cinfo->emethods->alloc_small_sarray)
				(fullsize_width,
				(long) (vs * (DCTSIZE+2)));
    /* Create space for the scrambled-order pointers */
    fullsize_data[1][ci] = (JSAMPARRAY) (*cinfo->emethods->alloc_small)
				(vs * (DCTSIZE+2) * SIZEOF(JSAMPROW));
    /* Duplicate the first DCTSIZE-2 row groups */
    for (i = 0; i < vs * (DCTSIZE-2); i++) {
      fullsize_data[1][ci][i] = fullsize_data[0][ci][i];
    }
    /* Copy the last four row groups in swapped order */
    for (i = 0; i < vs * 2; i++) {
      fullsize_data[1][ci][vs*DCTSIZE + i] = fullsize_data[0][ci][vs*(DCTSIZE-2) + i];
      fullsize_data[1][ci][vs*(DCTSIZE-2) + i] = fullsize_data[0][ci][vs*DCTSIZE + i];
    }
  }
}


#if 0				/* this routine not currently needed */

LOCAL void
free_sampling_buffer (compress_info_ptr cinfo, JSAMPIMAGE fullsize_data[2])
/* Release a sampling buffer created by alloc_sampling_buffer */
{
  short ci;

  for (ci = 0; ci < cinfo->num_components; ci++) {
    /* Free the real storage */
    (*cinfo->emethods->free_small_sarray) (fullsize_data[0][ci]);
    /* Free the scrambled-order pointers */
    (*cinfo->emethods->free_small) ((void *) fullsize_data[1][ci]);
  }

  /* Free the top-level space */
  (*cinfo->emethods->free_small) ((void *) fullsize_data[0]);
  (*cinfo->emethods->free_small) ((void *) fullsize_data[1]);
}

#endif


LOCAL void
downsample (compress_info_ptr cinfo,
	    JSAMPIMAGE fullsize_data, JSAMPIMAGE sampled_data,
	    long fullsize_width,
	    short above, short current, short below, short out)
/* Do downsampling of a single row group (of each component). */
/* above, current, below are indexes of row groups in fullsize_data;      */
/* out is the index of the target row group in sampled_data.              */
/* Special case: above, below can be -1 to indicate top, bottom of image. */
{
  jpeg_component_info *compptr;
  JSAMPARRAY above_ptr, below_ptr;
  JSAMPROW dummy[MAX_SAMP_FACTOR]; /* for downsample expansion at top/bottom */
  short ci, vs, i;

  vs = cinfo->max_v_samp_factor; /* row group height */

  for (ci = 0; ci < cinfo->num_components; ci++) {
    compptr = & cinfo->comp_info[ci];

    if (above >= 0)
      above_ptr = fullsize_data[ci] + above * vs;
    else {
      /* Top of image: make a dummy above-context with copies of 1st row */
      /* We assume current=0 in this case */
      for (i = 0; i < vs; i++)
	dummy[i] = fullsize_data[ci][0];
      above_ptr = (JSAMPARRAY) dummy; /* possible near->far pointer conv */
    }

    if (below >= 0)
      below_ptr = fullsize_data[ci] + below * vs;
    else {
      /* Bot of image: make a dummy below-context with copies of last row */
      for (i = 0; i < vs; i++)
	dummy[i] = fullsize_data[ci][(current+1)*vs-1];
      below_ptr = (JSAMPARRAY) dummy; /* possible near->far pointer conv */
    }

    (*cinfo->methods->downsample[ci])
		(cinfo, (int) ci,
		 fullsize_width, (int) vs,
		 compptr->downsampled_width, (int) compptr->v_samp_factor,
		 above_ptr,
		 fullsize_data[ci] + current * vs,
		 below_ptr,
		 sampled_data[ci] + out * compptr->v_samp_factor);
  }
}


/* These variables are initialized by the pipeline controller for use by
 * MCU_output_catcher.
 * To avoid a lot of row-pointer overhead, we cram as many MCUs into each
 * row of whole_scan_MCUs as we can get without exceeding 32Kbytes per row.
 * NOTE: the "arbitrary" constant here must not exceed MAX_ALLOC_CHUNK
 * defined in jmemsys.h, which is 64K-epsilon in most DOS implementations.
 */

#define MAX_WHOLE_ROW_BLOCKS	((int) (32768L / SIZEOF(JBLOCK))) /* max blocks/row */

static big_barray_ptr whole_scan_MCUs; /* Big array for saving the MCUs */
static int MCUs_in_big_row;	/* # of MCUs in each row of whole_scan_MCUs */
static long next_whole_row;	/* next row to access in whole_scan_MCUs */
static int next_MCU_index;	/* next MCU in current row */


METHODDEF void
MCU_output_catcher (compress_info_ptr cinfo, JBLOCK *MCU_data)
/* Output method for siphoning off extract_MCUs output into a big array */
{
  static JBLOCKARRAY rowptr;

  if (next_MCU_index >= MCUs_in_big_row) {
    rowptr = (*cinfo->emethods->access_big_barray) (whole_scan_MCUs,
						    next_whole_row, TRUE);
    next_whole_row++;
    next_MCU_index = 0;
  }

  /*
   * note that on 80x86, the cast applied to MCU_data implies
   * near to far pointer conversion.
   */
  jcopy_block_row((JBLOCKROW) MCU_data,
		  rowptr[0] + next_MCU_index * cinfo->blocks_in_MCU,
		  (long) cinfo->blocks_in_MCU);
  next_MCU_index++;
}


METHODDEF void
dump_scan_MCUs (compress_info_ptr cinfo, MCU_output_method_ptr output_method)
/* Dump the MCUs saved in whole_scan_MCUs to the output method. */
/* The method may be either the entropy encoder or some routine supplied */
/* by the entropy optimizer. */
{
  /* On an 80x86 machine, the entropy encoder expects the passed data block
   * to be in NEAR memory (for performance reasons), so we have to copy it
   * back from the big array to a local array.  On less brain-damaged CPUs
   * we needn't do that.
   */
#ifdef NEED_FAR_POINTERS
  JBLOCK MCU_data[MAX_BLOCKS_IN_MCU];
#endif
  long mcurow, mcuindex, next_row;
  int next_index;
  JBLOCKARRAY rowptr = NULL;	/* init only to suppress compiler complaint */

  next_row = 0;
  next_index = MCUs_in_big_row;

  for (mcurow = 0; mcurow < cinfo->MCU_rows_in_scan; mcurow++) {
    (*cinfo->methods->progress_monitor) (cinfo, mcurow,
					 cinfo->MCU_rows_in_scan);
    for (mcuindex = 0; mcuindex < cinfo->MCUs_per_row; mcuindex++) {
      if (next_index >= MCUs_in_big_row) {
	rowptr = (*cinfo->emethods->access_big_barray) (whole_scan_MCUs,
							next_row, FALSE);
	next_row++;
	next_index = 0;
      }
#ifdef NEED_FAR_POINTERS
      jcopy_block_row(rowptr[0] + next_index * cinfo->blocks_in_MCU,
		      (JBLOCKROW) MCU_data, /* casts near to far pointer! */
		      (long) cinfo->blocks_in_MCU);
      (*output_method) (cinfo, MCU_data);
#else
      (*output_method) (cinfo, rowptr[0] + next_index * cinfo->blocks_in_MCU);
#endif
      next_index++;
    }
  }

  cinfo->completed_passes++;
}



/*
 * Compression pipeline controller used for single-scan files
 * with no optimization of entropy parameters.
 */

METHODDEF void
single_ccontroller (compress_info_ptr cinfo)
{
  int rows_in_mem;		/* # of sample rows in full-size buffers */
  long fullsize_width;		/* # of samples per row in full-size buffers */
  long cur_pixel_row;		/* counts # of pixel rows processed */
  long mcu_rows_output;		/* # of MCU rows actually emitted */
  int mcu_rows_per_loop;	/* # of MCU rows processed per outer loop */
  /* Work buffer for pre-downsampling data (see comments at head of file) */
  JSAMPIMAGE fullsize_data[2];
  /* Work buffer for downsampled data */
  JSAMPIMAGE sampled_data;
  int rows_this_time;
  short ci, whichss, i;

  /* Prepare for single scan containing all components */
  if (cinfo->num_components > MAX_COMPS_IN_SCAN)
    ERREXIT(cinfo->emethods, "Too many components for interleaved scan");
  cinfo->comps_in_scan = cinfo->num_components;
  for (ci = 0; ci < cinfo->num_components; ci++) {
    cinfo->cur_comp_info[ci] = &cinfo->comp_info[ci];
  }
  if (cinfo->comps_in_scan == 1) {
    noninterleaved_scan_setup(cinfo);
    /* Vk block rows constitute the same number of MCU rows */
    mcu_rows_per_loop = cinfo->cur_comp_info[0]->v_samp_factor;
  } else {
    interleaved_scan_setup(cinfo);
    /* in an interleaved scan, one MCU row contains Vk block rows */
    mcu_rows_per_loop = 1;
  }
  cinfo->total_passes++;

  /* Compute dimensions of full-size pixel buffers */
  /* Note these are the same whether interleaved or not. */
  rows_in_mem = cinfo->max_v_samp_factor * DCTSIZE;
  fullsize_width = jround_up(cinfo->image_width,
			     (long) (cinfo->max_h_samp_factor * DCTSIZE));

  /* Allocate working memory: */
  /* fullsize_data is sample data before downsampling */
  alloc_sampling_buffer(cinfo, fullsize_data, fullsize_width);
  /* sampled_data is sample data after downsampling */
  sampled_data = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->num_components * SIZEOF(JSAMPARRAY));
  for (ci = 0; ci < cinfo->num_components; ci++) {
    sampled_data[ci] = (*cinfo->emethods->alloc_small_sarray)
			(cinfo->comp_info[ci].downsampled_width,
			 (long) (cinfo->comp_info[ci].v_samp_factor * DCTSIZE));
  }

  /* Tell the memory manager to instantiate big arrays.
   * We don't need any big arrays in this controller,
   * but some other module (like the input file reader) may need one.
   */
  (*cinfo->emethods->alloc_big_arrays)
	((long) 0,				/* no more small sarrays */
	 (long) 0,				/* no more small barrays */
	 (long) 0);				/* no more "medium" objects */

  /* Initialize output file & do per-scan object init */

  (*cinfo->methods->write_scan_header) (cinfo);
  cinfo->methods->entropy_output = cinfo->methods->write_jpeg_data;
  (*cinfo->methods->entropy_encode_init) (cinfo);
  (*cinfo->methods->downsample_init) (cinfo);
  (*cinfo->methods->extract_init) (cinfo);

  /* Loop over input image: rows_in_mem pixel rows are processed per loop */

  mcu_rows_output = 0;
  whichss = 1;			/* arrange to start with fullsize_data[0] */

  for (cur_pixel_row = 0; cur_pixel_row < cinfo->image_height;
       cur_pixel_row += rows_in_mem) {
    (*cinfo->methods->progress_monitor) (cinfo, cur_pixel_row,
					 cinfo->image_height);

    whichss ^= 1;		/* switch to other fullsize_data buffer */
    
    /* Obtain rows_this_time pixel rows and expand to rows_in_mem rows. */
    /* Then we have exactly DCTSIZE row groups for downsampling. */   
    rows_this_time = (int) MIN((long) rows_in_mem,
			       cinfo->image_height - cur_pixel_row);
 
    (*cinfo->methods->get_sample_rows) (cinfo, rows_this_time,
					fullsize_data[whichss]);
    (*cinfo->methods->edge_expand) (cinfo,
				    cinfo->image_width, rows_this_time,
				    fullsize_width, rows_in_mem,
				    fullsize_data[whichss]);
    
    /* Downsample the data (all components) */
    /* First time through is a special case */
    
    if (cur_pixel_row) {
      /* Downsample last row group of previous set */
      downsample(cinfo, fullsize_data[whichss], sampled_data, fullsize_width,
		 (short) DCTSIZE, (short) (DCTSIZE+1), (short) 0,
		 (short) (DCTSIZE-1));
      /* and dump the previous set's downsampled data */
      (*cinfo->methods->extract_MCUs) (cinfo, sampled_data, 
				       mcu_rows_per_loop,
				       cinfo->methods->entropy_encode);
      mcu_rows_output += mcu_rows_per_loop;
      /* Downsample first row group of this set */
      downsample(cinfo, fullsize_data[whichss], sampled_data, fullsize_width,
		 (short) (DCTSIZE+1), (short) 0, (short) 1,
		 (short) 0);
    } else {
      /* Downsample first row group with dummy above-context */
      downsample(cinfo, fullsize_data[whichss], sampled_data, fullsize_width,
		 (short) (-1), (short) 0, (short) 1,
		 (short) 0);
    }
    /* Downsample second through next-to-last row groups of this set */
    for (i = 1; i <= DCTSIZE-2; i++) {
      downsample(cinfo, fullsize_data[whichss], sampled_data, fullsize_width,
		 (short) (i-1), (short) i, (short) (i+1),
		 (short) i);
    }
  } /* end of outer loop */
  
  /* Downsample the last row group with dummy below-context */
  /* Note whichss points to last buffer side used */
  downsample(cinfo, fullsize_data[whichss], sampled_data, fullsize_width,
	     (short) (DCTSIZE-2), (short) (DCTSIZE-1), (short) (-1),
	     (short) (DCTSIZE-1));
  /* Dump the remaining data (may be less than full height if uninterleaved) */
  (*cinfo->methods->extract_MCUs) (cinfo, sampled_data, 
		(int) (cinfo->MCU_rows_in_scan - mcu_rows_output),
		cinfo->methods->entropy_encode);

  /* Finish output file */
  (*cinfo->methods->extract_term) (cinfo);
  (*cinfo->methods->downsample_term) (cinfo);
  (*cinfo->methods->entropy_encode_term) (cinfo);
  (*cinfo->methods->write_scan_trailer) (cinfo);
  cinfo->completed_passes++;

  /* Release working memory */
  /* (no work -- we let free_all release what's needful) */
}


/*
 * Compression pipeline controller used for single-scan files
 * with optimization of entropy parameters.
 */

#ifdef ENTROPY_OPT_SUPPORTED

METHODDEF void
single_eopt_ccontroller (compress_info_ptr cinfo)
{
  int rows_in_mem;		/* # of sample rows in full-size buffers */
  long fullsize_width;		/* # of samples per row in full-size buffers */
  long cur_pixel_row;		/* counts # of pixel rows processed */
  long mcu_rows_output;		/* # of MCU rows actually emitted */
  int mcu_rows_per_loop;	/* # of MCU rows processed per outer loop */
  /* Work buffer for pre-downsampling data (see comments at head of file) */
  JSAMPIMAGE fullsize_data[2];
  /* Work buffer for downsampled data */
  JSAMPIMAGE sampled_data;
  int rows_this_time;
  int blocks_in_big_row;
  short ci, whichss, i;

  /* Prepare for single scan containing all components */
  if (cinfo->num_components > MAX_COMPS_IN_SCAN)
    ERREXIT(cinfo->emethods, "Too many components for interleaved scan");
  cinfo->comps_in_scan = cinfo->num_components;
  for (ci = 0; ci < cinfo->num_components; ci++) {
    cinfo->cur_comp_info[ci] = &cinfo->comp_info[ci];
  }
  if (cinfo->comps_in_scan == 1) {
    noninterleaved_scan_setup(cinfo);
    /* Vk block rows constitute the same number of MCU rows */
    mcu_rows_per_loop = cinfo->cur_comp_info[0]->v_samp_factor;
  } else {
    interleaved_scan_setup(cinfo);
    /* in an interleaved scan, one MCU row contains Vk block rows */
    mcu_rows_per_loop = 1;
  }
  cinfo->total_passes += 2;	/* entropy encoder must add # passes it uses */

  /* Compute dimensions of full-size pixel buffers */
  /* Note these are the same whether interleaved or not. */
  rows_in_mem = cinfo->max_v_samp_factor * DCTSIZE;
  fullsize_width = jround_up(cinfo->image_width,
			     (long) (cinfo->max_h_samp_factor * DCTSIZE));

  /* Allocate working memory: */
  /* fullsize_data is sample data before downsampling */
  alloc_sampling_buffer(cinfo, fullsize_data, fullsize_width);
  /* sampled_data is sample data after downsampling */
  sampled_data = (JSAMPIMAGE) (*cinfo->emethods->alloc_small)
				(cinfo->num_components * SIZEOF(JSAMPARRAY));
  for (ci = 0; ci < cinfo->num_components; ci++) {
    sampled_data[ci] = (*cinfo->emethods->alloc_small_sarray)
			(cinfo->comp_info[ci].downsampled_width,
			 (long) (cinfo->comp_info[ci].v_samp_factor * DCTSIZE));
  }

  /* Figure # of MCUs to be packed in a row of whole_scan_MCUs */
  MCUs_in_big_row = MAX_WHOLE_ROW_BLOCKS / cinfo->blocks_in_MCU;
  blocks_in_big_row = MCUs_in_big_row * cinfo->blocks_in_MCU;

  /* Request a big array: whole_scan_MCUs saves the MCU data for the scan */
  whole_scan_MCUs = (*cinfo->emethods->request_big_barray)
		((long) blocks_in_big_row,
		 (long) (cinfo->MCUs_per_row * cinfo->MCU_rows_in_scan
			 + MCUs_in_big_row-1) / MCUs_in_big_row,
		 1L);		/* unit height is 1 row */

  next_whole_row = 0;		/* init output ptr for MCU_output_catcher */
  next_MCU_index = MCUs_in_big_row; /* forces access on first call! */

  /* Tell the memory manager to instantiate big arrays */
  (*cinfo->emethods->alloc_big_arrays)
	((long) 0,				/* no more small sarrays */
	 (long) 0,				/* no more small barrays */
	 (long) 0);				/* no more "medium" objects */

  /* Do per-scan object init */

  (*cinfo->methods->downsample_init) (cinfo);
  (*cinfo->methods->extract_init) (cinfo);

  /* Loop over input image: rows_in_mem pixel rows are processed per loop */
  /* MCU data goes into whole_scan_MCUs, not to the entropy encoder */

  mcu_rows_output = 0;
  whichss = 1;			/* arrange to start with fullsize_data[0] */

  for (cur_pixel_row = 0; cur_pixel_row < cinfo->image_height;
       cur_pixel_row += rows_in_mem) {
    (*cinfo->methods->progress_monitor) (cinfo, cur_pixel_row,
					 cinfo->image_height);

    whichss ^= 1;		/* switch to other fullsize_data buffer */
    
    /* Obtain rows_this_time pixel rows and expand to rows_in_mem rows. */
    /* Then we have exactly DCTSIZE row groups for downsampling. */   
    rows_this_time = (int) MIN((long) rows_in_mem,
			       cinfo->image_height - cur_pixel_row);
 
    (*cinfo->methods->get_sample_rows) (cinfo, rows_this_time,
					fullsize_data[whichss]);
    (*cinfo->methods->edge_expand) (cinfo,
				    cinfo->image_width, rows_this_time,
				    fullsize_width, rows_in_mem,
				    fullsize_data[whichss]);
    
    /* Downsample the data (all components) */
    /* First time through is a special case */
    
    if (cur_pixel_row) {
      /* Downsample last row group of previous set */
      downsample(cinfo, fullsize_data[whichss], sampled_data, fullsize_width,
		 (short) DCTSIZE, (short) (DCTSIZE+1), (short) 0,
		 (short) (DCTSIZE-1));
      /* and dump the previous set's downsampled data */
      (*cinfo->methods->extract_MCUs) (cinfo, sampled_data, 
				       mcu_rows_per_loop,
				       MCU_output_catcher);
      mcu_rows_output += mcu_rows_per_loop;
      /* Downsample first row group of this set */
      downsample(cinfo, fullsize_data[whichss], sampled_data, fullsize_width,
		 (short) (DCTSIZE+1), (short) 0, (short) 1,
		 (short) 0);
    } else {
      /* Downsample first row group with dummy above-context */
      downsample(cinfo, fullsize_data[whichss], sampled_data, fullsize_width,
		 (short) (-1), (short) 0, (short) 1,
		 (short) 0);
    }
    /* Downsample second through next-to-last row groups of this set */
    for (i = 1; i <= DCTSIZE-2; i++) {
      downsample(cinfo, fullsize_data[whichss], sampled_data, fullsize_width,
		 (short) (i-1), (short) i, (short) (i+1),
		 (short) i);
    }
  } /* end of outer loop */
  
  /* Downsample the last row group with dummy below-context */
  /* Note whichss points to last buffer side used */
  downsample(cinfo, fullsize_data[whichss], sampled_data, fullsize_width,
	     (short) (DCTSIZE-2), (short) (DCTSIZE-1), (short) (-1),
	     (short) (DCTSIZE-1));
  /* Dump the remaining data (may be less than full height if uninterleaved) */
  (*cinfo->methods->extract_MCUs) (cinfo, sampled_data, 
		(int) (cinfo->MCU_rows_in_scan - mcu_rows_output),
		MCU_output_catcher);

  /* Clean up after that stuff, then find the optimal entropy parameters */

  (*cinfo->methods->extract_term) (cinfo);
  (*cinfo->methods->downsample_term) (cinfo);

  cinfo->completed_passes++;

  (*cinfo->methods->entropy_optimize) (cinfo, dump_scan_MCUs);

  /* Emit scan to output file */
  /* Note: we can't do write_scan_header until entropy parameters are set! */

  (*cinfo->methods->write_scan_header) (cinfo);
  cinfo->methods->entropy_output = cinfo->methods->write_jpeg_data;
  (*cinfo->methods->entropy_encode_init) (cinfo);
  dump_scan_MCUs(cinfo, cinfo->methods->entropy_encode);
  (*cinfo->methods->entropy_encode_term) (cinfo);
  (*cinfo->methods->write_scan_trailer) (cinfo);

  /* Release working memory */
  /* (no work -- we let free_all release what's needful) */
}

#endif /* ENTROPY_OPT_SUPPORTED */


/*
 * Compression pipeline controller used for multiple-scan files
 * with no optimization of entropy parameters.
 */

#ifdef C_MULTISCAN_FILES_SUPPORTED

METHODDEF void
multi_ccontroller (compress_info_ptr cinfo)
{
  ERREXIT(cinfo->emethods, "Not implemented yet");
}

#endif /* C_MULTISCAN_FILES_SUPPORTED */


/*
 * Compression pipeline controller used for multiple-scan files
 * with optimization of entropy parameters.
 */

#ifdef C_MULTISCAN_FILES_SUPPORTED
#ifdef ENTROPY_OPT_SUPPORTED

METHODDEF void
multi_eopt_ccontroller (compress_info_ptr cinfo)
{
  ERREXIT(cinfo->emethods, "Not implemented yet");
}

#endif /* ENTROPY_OPT_SUPPORTED */
#endif /* C_MULTISCAN_FILES_SUPPORTED */


/*
 * The method selection routine for compression pipeline controllers.
 */

GLOBAL void
jselcpipeline (compress_info_ptr cinfo)
{
  if (cinfo->interleave || cinfo->num_components == 1) {
    /* single scan needed */
#ifdef ENTROPY_OPT_SUPPORTED
    if (cinfo->optimize_coding)
      cinfo->methods->c_pipeline_controller = single_eopt_ccontroller;
    else
#endif
      cinfo->methods->c_pipeline_controller = single_ccontroller;
  } else {
    /* multiple scans needed */
#ifdef C_MULTISCAN_FILES_SUPPORTED
#ifdef ENTROPY_OPT_SUPPORTED
    if (cinfo->optimize_coding)
      cinfo->methods->c_pipeline_controller = multi_eopt_ccontroller;
    else
#endif
      cinfo->methods->c_pipeline_controller = multi_ccontroller;
#else
    ERREXIT(cinfo->emethods, "Multiple-scan support was not compiled");
#endif
  }
}

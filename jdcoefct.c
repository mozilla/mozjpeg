/*
 * jdcoefct.c
 *
 * Copyright (C) 1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the coefficient buffer controller for decompression.
 * This controller is the top level of the JPEG decompressor proper.
 * The coefficient buffer lies between entropy decoding and inverse-DCT steps.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Private buffer controller object */

typedef struct {
  struct jpeg_d_coef_controller pub; /* public fields */

  JDIMENSION MCU_col_num;	/* saves next MCU column to process */
  JDIMENSION MCU_row_num;	/* keep track of MCU row # within image */

  /* In single-pass modes without block smoothing, it's sufficient to buffer
   * just one MCU (although this may prove a bit slow in practice).
   * We allocate a workspace of MAX_BLOCKS_IN_MCU coefficient blocks,
   * and let the entropy decoder write into that workspace each time.
   * (On 80x86, the workspace is FAR even though it's not really very big;
   * this is to keep the module interfaces unchanged when a large coefficient
   * buffer is necessary.)
   * In multi-pass modes, this array points to the current MCU's blocks
   * within the virtual arrays.
   */
  JBLOCKROW MCU_buffer[MAX_BLOCKS_IN_MCU];

  /* In multi-pass modes, we need a virtual block array for each component. */
  jvirt_barray_ptr whole_image[MAX_COMPONENTS];
} my_coef_controller;

typedef my_coef_controller * my_coef_ptr;


/* Forward declarations */
METHODDEF boolean decompress_data
	JPP((j_decompress_ptr cinfo, JSAMPIMAGE output_buf));
#ifdef D_MULTISCAN_FILES_SUPPORTED
METHODDEF boolean decompress_read
	JPP((j_decompress_ptr cinfo, JSAMPIMAGE output_buf));
METHODDEF boolean decompress_output
	JPP((j_decompress_ptr cinfo, JSAMPIMAGE output_buf));
#endif


/*
 * Initialize for a processing pass.
 */

METHODDEF void
start_pass_coef (j_decompress_ptr cinfo, J_BUF_MODE pass_mode)
{
  my_coef_ptr coef = (my_coef_ptr) cinfo->coef;

  coef->MCU_col_num = 0;
  coef->MCU_row_num = 0;

  switch (pass_mode) {
  case JBUF_PASS_THRU:
    if (coef->whole_image[0] != NULL)
      ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    coef->pub.decompress_data = decompress_data;
    break;
#ifdef D_MULTISCAN_FILES_SUPPORTED
  case JBUF_SAVE_SOURCE:
    if (coef->whole_image[0] == NULL)
      ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    coef->pub.decompress_data = decompress_read;
    break;
  case JBUF_CRANK_DEST:
    if (coef->whole_image[0] == NULL)
      ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    coef->pub.decompress_data = decompress_output;
    break;
#endif
  default:
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    break;
  }
}


/*
 * Process some data in the single-pass case.
 * Always attempts to emit one fully interleaved MCU row ("iMCU" row).
 * Returns TRUE if it completed a row, FALSE if not (suspension).
 *
 * NB: output_buf contains a plane for each component in image.
 * For single pass, this is the same as the components in the scan.
 */

METHODDEF boolean
decompress_data (j_decompress_ptr cinfo, JSAMPIMAGE output_buf)
{
  my_coef_ptr coef = (my_coef_ptr) cinfo->coef;
  JDIMENSION MCU_col_num;	/* index of current MCU within row */
  JDIMENSION last_MCU_col = cinfo->MCUs_per_row - 1;
  JDIMENSION last_MCU_row = cinfo->MCU_rows_in_scan - 1;
  int blkn, ci, xindex, yindex, useful_width;
  JSAMPARRAY output_ptr;
  JDIMENSION start_col, output_col;
  jpeg_component_info *compptr;
  inverse_DCT_method_ptr inverse_DCT;

  /* Loop to process as much as one whole MCU row */

  for (MCU_col_num = coef->MCU_col_num; MCU_col_num <= last_MCU_col;
       MCU_col_num++) {

    /* Try to fetch an MCU.  Entropy decoder expects buffer to be zeroed. */
    jzero_far((void FAR *) coef->MCU_buffer[0],
	      (size_t) (cinfo->blocks_in_MCU * SIZEOF(JBLOCK)));
    if (! (*cinfo->entropy->decode_mcu) (cinfo, coef->MCU_buffer)) {
      /* Suspension forced; return with row unfinished */
      coef->MCU_col_num = MCU_col_num; /* update my state */
      return FALSE;
    }

    /* Determine where data should go in output_buf and do the IDCT thing.
     * We skip dummy blocks at the right and bottom edges (but blkn gets
     * incremented past them!).  Note the inner loop relies on having
     * allocated the MCU_buffer[] blocks sequentially.
     */
    blkn = 0;			/* index of current DCT block within MCU */
    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      /* Don't bother to IDCT an uninteresting component. */
      if (! compptr->component_needed) {
	blkn += compptr->MCU_blocks;
	continue;
      }
      inverse_DCT = cinfo->idct->inverse_DCT[compptr->component_index];
      useful_width = (MCU_col_num < last_MCU_col) ? compptr->MCU_width
						  : compptr->last_col_width;
      output_ptr = output_buf[ci];
      start_col = MCU_col_num * compptr->MCU_sample_width;
      for (yindex = 0; yindex < compptr->MCU_height; yindex++) {
	if (coef->MCU_row_num < last_MCU_row ||
	    yindex < compptr->last_row_height) {
	  output_col = start_col;
	  for (xindex = 0; xindex < useful_width; xindex++) {
	    (*inverse_DCT) (cinfo, compptr,
			    (JCOEFPTR) coef->MCU_buffer[blkn+xindex],
			    output_ptr, output_col);
	    output_col += compptr->DCT_scaled_size;
	  }
	}
	blkn += compptr->MCU_width;
	output_ptr += compptr->DCT_scaled_size;
      }
    }
  }

  /* We finished the row successfully */
  coef->MCU_col_num = 0;	/* prepare for next row */
  coef->MCU_row_num++;
  return TRUE;
}


#ifdef D_MULTISCAN_FILES_SUPPORTED

/*
 * Process some data: handle an input pass for a multiple-scan file.
 * We read the equivalent of one fully interleaved MCU row ("iMCU" row)
 * per call, ie, v_samp_factor block rows for each component in the scan.
 * No data is returned; we just stash it in the virtual arrays.
 *
 * Returns TRUE if it completed a row, FALSE if not (suspension).
 * Currently, the suspension case is not supported.
 */

METHODDEF boolean
decompress_read (j_decompress_ptr cinfo, JSAMPIMAGE output_buf)
{
  my_coef_ptr coef = (my_coef_ptr) cinfo->coef;
  JDIMENSION MCU_col_num;	/* index of current MCU within row */
  int blkn, ci, xindex, yindex, yoffset, num_MCU_rows;
  JDIMENSION total_width, remaining_rows, start_col;
  JBLOCKARRAY buffer[MAX_COMPS_IN_SCAN];
  JBLOCKROW buffer_ptr;
  jpeg_component_info *compptr;

  /* Align the virtual buffers for the components used in this scan. */
  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    buffer[ci] = (*cinfo->mem->access_virt_barray)
      ((j_common_ptr) cinfo, coef->whole_image[compptr->component_index],
       coef->MCU_row_num * compptr->v_samp_factor, TRUE);
    /* Entropy decoder expects buffer to be zeroed. */
    total_width = (JDIMENSION) jround_up((long) compptr->width_in_blocks,
					 (long) compptr->h_samp_factor);
    for (yindex = 0; yindex < compptr->v_samp_factor; yindex++) {
      jzero_far((void FAR *) buffer[ci][yindex], 
		(size_t) (total_width * SIZEOF(JBLOCK)));
    }
  }

  /* In an interleaved scan, we process exactly one MCU row.
   * In a noninterleaved scan, we need to process v_samp_factor MCU rows,
   * each of which contains a single block row.
   */
  if (cinfo->comps_in_scan == 1) {
    compptr = cinfo->cur_comp_info[0];
    num_MCU_rows = compptr->v_samp_factor;
    /* but watch out for the bottom of the image */
    remaining_rows = cinfo->MCU_rows_in_scan -
		     coef->MCU_row_num * compptr->v_samp_factor;
    if (remaining_rows < (JDIMENSION) num_MCU_rows)
      num_MCU_rows = (int) remaining_rows;
  } else {
    num_MCU_rows = 1;
  }

  /* Loop to process one whole iMCU row */
  for (yoffset = 0; yoffset < num_MCU_rows; yoffset++) {
    for (MCU_col_num = 0; MCU_col_num < cinfo->MCUs_per_row; MCU_col_num++) {
      /* Construct list of pointers to DCT blocks belonging to this MCU */
      blkn = 0;			/* index of current DCT block within MCU */
      for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
	compptr = cinfo->cur_comp_info[ci];
	start_col = MCU_col_num * compptr->MCU_width;
	for (yindex = 0; yindex < compptr->MCU_height; yindex++) {
	  buffer_ptr = buffer[ci][yindex+yoffset] + start_col;
	  for (xindex = 0; xindex < compptr->MCU_width; xindex++) {
	    coef->MCU_buffer[blkn++] = buffer_ptr++;
	  }
	}
      }
      /* Try to fetch the MCU. */
      if (! (*cinfo->entropy->decode_mcu) (cinfo, coef->MCU_buffer)) {
	ERREXIT(cinfo, JERR_CANT_SUSPEND); /* not supported */
      }
    }
  }

  coef->MCU_row_num++;
  return TRUE;
}


/*
 * Process some data: output from the virtual arrays after reading is done.
 * Always emits one fully interleaved MCU row ("iMCU" row).
 * Always returns TRUE --- suspension is not possible.
 *
 * NB: output_buf contains a plane for each component in image.
 */

METHODDEF boolean
decompress_output (j_decompress_ptr cinfo, JSAMPIMAGE output_buf)
{
  my_coef_ptr coef = (my_coef_ptr) cinfo->coef;
  JDIMENSION last_MCU_row = cinfo->total_iMCU_rows - 1;
  JDIMENSION block_num;
  int ci, block_row, block_rows;
  JBLOCKARRAY buffer;
  JBLOCKROW buffer_ptr;
  JSAMPARRAY output_ptr;
  JDIMENSION output_col;
  jpeg_component_info *compptr;
  inverse_DCT_method_ptr inverse_DCT;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    /* Don't bother to IDCT an uninteresting component. */
    if (! compptr->component_needed)
      continue;
    /* Align the virtual buffer for this component. */
    buffer = (*cinfo->mem->access_virt_barray)
      ((j_common_ptr) cinfo, coef->whole_image[ci],
       coef->MCU_row_num * compptr->v_samp_factor, FALSE);
    /* Count non-dummy DCT block rows in this iMCU row. */
    if (coef->MCU_row_num < last_MCU_row)
      block_rows = compptr->v_samp_factor;
    else {
      block_rows = (int) (compptr->height_in_blocks % compptr->v_samp_factor);
      if (block_rows == 0) block_rows = compptr->v_samp_factor;
    }
    inverse_DCT = cinfo->idct->inverse_DCT[ci];
    output_ptr = output_buf[ci];
    /* Loop over all DCT blocks to be processed. */
    for (block_row = 0; block_row < block_rows; block_row++) {
      buffer_ptr = buffer[block_row];
      output_col = 0;
      for (block_num = 0; block_num < compptr->width_in_blocks; block_num++) {
	(*inverse_DCT) (cinfo, compptr, (JCOEFPTR) buffer_ptr,
			output_ptr, output_col);
	buffer_ptr++;
	output_col += compptr->DCT_scaled_size;
      }
      output_ptr += compptr->DCT_scaled_size;
    }
  }

  coef->MCU_row_num++;
  return TRUE;
}

#endif /* D_MULTISCAN_FILES_SUPPORTED */


/*
 * Initialize coefficient buffer controller.
 */

GLOBAL void
jinit_d_coef_controller (j_decompress_ptr cinfo, boolean need_full_buffer)
{
  my_coef_ptr coef;
  int ci, i;
  jpeg_component_info *compptr;
  JBLOCKROW buffer;

  coef = (my_coef_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_coef_controller));
  cinfo->coef = (struct jpeg_d_coef_controller *) coef;
  coef->pub.start_pass = start_pass_coef;

  /* Create the coefficient buffer. */
  if (need_full_buffer) {
#ifdef D_MULTISCAN_FILES_SUPPORTED
    /* Allocate a full-image virtual array for each component, */
    /* padded to a multiple of samp_factor DCT blocks in each direction. */
    /* Note memmgr implicitly pads the vertical direction. */
    for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
	 ci++, compptr++) {
      coef->whole_image[ci] = (*cinfo->mem->request_virt_barray)
	((j_common_ptr) cinfo, JPOOL_IMAGE,
	 (JDIMENSION) jround_up((long) compptr->width_in_blocks,
				(long) compptr->h_samp_factor),
	 compptr->height_in_blocks,
	 (JDIMENSION) compptr->v_samp_factor);
    }
#else
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
#endif
  } else {
    /* We only need a single-MCU buffer. */
    buffer = (JBLOCKROW)
      (*cinfo->mem->alloc_large) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  MAX_BLOCKS_IN_MCU * SIZEOF(JBLOCK));
    for (i = 0; i < MAX_BLOCKS_IN_MCU; i++) {
      coef->MCU_buffer[i] = buffer + i;
    }
    coef->whole_image[0] = NULL; /* flag for no virtual arrays */
  }
}

/*
 * jcexpand.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains image edge-expansion routines.
 * These routines are invoked via the edge_expand method.
 */

#include "jinclude.h"


/*
 * Expand an image so that it is a multiple of the MCU dimensions.
 * This is to be accomplished by duplicating the rightmost column
 * and/or bottommost row of pixels.  The image has not yet been
 * downsampled, so all components have the same dimensions.
 */

METHODDEF void
edge_expand (compress_info_ptr cinfo,
	     long input_cols, int input_rows,
	     long output_cols, int output_rows,
	     JSAMPIMAGE image_data)
{
  /* Expand horizontally */
  if (input_cols < output_cols) {
    register JSAMPROW ptr;
    register JSAMPLE pixval;
    register long count;
    register int row;
    short ci;
    long numcols = output_cols - input_cols;

    for (ci = 0; ci < cinfo->num_components; ci++) {
      for (row = 0; row < input_rows; row++) {
	ptr = image_data[ci][row] + (input_cols-1);
	pixval = GETJSAMPLE(*ptr++);
	for (count = numcols; count > 0; count--)
	  *ptr++ = pixval;
      }
    }
  }

  /* Expand vertically */
  /* This happens only once at the bottom of the image, */
  /* so it needn't be super-efficient */
  if (input_rows < output_rows) {
    register int row;
    short ci;
    JSAMPARRAY this_component;

    for (ci = 0; ci < cinfo->num_components; ci++) {
      this_component = image_data[ci];
      for (row = input_rows; row < output_rows; row++) {
	jcopy_sample_rows(this_component, input_rows-1, this_component, row,
			  1, output_cols);
      }
    }
  }
}


/*
 * The method selection routine for edge expansion.
 */

GLOBAL void
jselexpand (compress_info_ptr cinfo)
{
  /* just one implementation for now */
  cinfo->methods->edge_expand = edge_expand;
}

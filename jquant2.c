/*
 * jquant2.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains 2-pass color quantization (color mapping) routines.
 * These routines are invoked via the methods color_quant_prescan,
 * color_quant_doit, and color_quant_init/term.
 */

#include "jinclude.h"

#ifdef QUANT_2PASS_SUPPORTED


/*
 * Initialize for two-pass color quantization.
 */

METHODDEF void
color_quant_init (decompress_info_ptr cinfo)
{
  TRACEMS(cinfo->emethods, 1, "color_quant_init 2 pass");
}


/*
 * Prescan some rows of pixels.
 * Note: this could change the data being written into the big image array,
 * if there were any benefit to doing so.  The doit routine is not allowed
 * to modify the big image array, because the memory manager is not required
 * to support multiple write passes on a big image.
 */

METHODDEF void
color_quant_prescan (decompress_info_ptr cinfo, int num_rows,
		     JSAMPIMAGE image_data)
{
  TRACEMS1(cinfo->emethods, 2, "color_quant_prescan %d rows", num_rows);
}


/*
 * This routine makes the final pass over the image data.
 * output_workspace is a one-component array of pixel dimensions at least
 * as large as the input image strip; it can be used to hold the converted
 * pixels' colormap indexes.
 */

METHODDEF void
final_pass (decompress_info_ptr cinfo, int num_rows,
	    JSAMPIMAGE image_data, JSAMPARRAY output_workspace)
{
  TRACEMS1(cinfo->emethods, 2, "final_pass %d rows", num_rows);
  /* for debug purposes, just emit input data */
  /* NB: this only works for PPM output */
  (*cinfo->methods->put_pixel_rows) (cinfo, num_rows, image_data);
}


/*
 * Perform two-pass quantization: rescan the image data and output the
 * converted data via put_color_map and put_pixel_rows.
 * The source_method is a routine that can scan the image data; it can
 * be called as many times as desired.  The processing routine called by
 * source_method has the same interface as color_quantize does in the
 * one-pass case, except it must call put_pixel_rows itself.  (This allows
 * me to use multiple passes in which earlier passes don't output anything.)
 */

METHODDEF void
color_quant_doit (decompress_info_ptr cinfo, quantize_caller_ptr source_method)
{
  TRACEMS(cinfo->emethods, 1, "color_quant_doit 2 pass");
  (*source_method) (cinfo, final_pass);
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
color_quant_term (decompress_info_ptr cinfo)
{
  TRACEMS(cinfo->emethods, 1, "color_quant_term 2 pass");
}


/*
 * Map some rows of pixels to the output colormapped representation.
 * Not used in two-pass case.
 */

METHODDEF void
color_quantize (decompress_info_ptr cinfo, int num_rows,
		JSAMPIMAGE input_data, JSAMPARRAY output_data)
{
  ERREXIT(cinfo->emethods, "Should not get here!");
}


/*
 * The method selection routine for 2-pass color quantization.
 */

GLOBAL void
jsel2quantize (decompress_info_ptr cinfo)
{
  if (cinfo->two_pass_quantize) {
    /* just one alternative for the moment */
    cinfo->methods->color_quant_init = color_quant_init;
    cinfo->methods->color_quant_prescan = color_quant_prescan;
    cinfo->methods->color_quant_doit = color_quant_doit;
    cinfo->methods->color_quant_term = color_quant_term;
    cinfo->methods->color_quantize = color_quantize;
  }
}

#endif /* QUANT_2PASS_SUPPORTED */

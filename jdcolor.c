/*
 * jdcolor.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains output colorspace conversion routines.
 * These routines are invoked via the methods color_convert
 * and colorout_init/term.
 */

#include "jinclude.h"


/*
 * Initialize for colorspace conversion.
 */

METHODDEF void
colorout_init (decompress_info_ptr cinfo)
{
  /* no work needed */
}


/*
 * Convert some rows of samples to the output colorspace.
 * This version handles YCbCr -> RGB conversion.
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 */

METHODDEF void
ycc_rgb_convert (decompress_info_ptr cinfo, int num_rows,
		 JSAMPIMAGE input_data, JSAMPIMAGE output_data)
{
  register INT32 y, u, v, x;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JSAMPROW outptr0, outptr1, outptr2;
  register long col;
  register long width = cinfo->image_width;
  register int row;
  
  for (row = 0; row < num_rows; row++) {
    inptr0 = input_data[0][row];
    inptr1 = input_data[1][row];
    inptr2 = input_data[2][row];
    outptr0 = output_data[0][row];
    outptr1 = output_data[1][row];
    outptr2 = output_data[2][row];
    for (col = width; col > 0; col--) {
      y = GETJSAMPLE(*inptr0++);
      u = (int) GETJSAMPLE(*inptr1++) - CENTERJSAMPLE;
      v = (int) GETJSAMPLE(*inptr2++) - CENTERJSAMPLE;
      /* Note: if the inputs were computed directly from RGB values,
       * range-limiting would be unnecessary here; but due to possible
       * noise in the DCT/IDCT phase, we do need to apply range limits.
       */
      y *= 1024;	/* in case compiler can't spot common subexpression */
      x = y          + 1436*v + 512; /* red */
      if (x < 0) x = 0;
      if (x > ((INT32) MAXJSAMPLE*1024)) x = (INT32) MAXJSAMPLE*1024;
      *outptr0++ = (JSAMPLE) (x >> 10);
      x = y -  352*u -  731*v + 512; /* green */
      if (x < 0) x = 0;
      if (x > ((INT32) MAXJSAMPLE*1024)) x = (INT32) MAXJSAMPLE*1024;
      *outptr1++ = (JSAMPLE) (x >> 10);
      x = y + 1815*u          + 512; /* blue */
      if (x < 0) x = 0;
      if (x > ((INT32) MAXJSAMPLE*1024)) x = (INT32) MAXJSAMPLE*1024;
      *outptr2++ = (JSAMPLE) (x >> 10);
    }
  }
}


/*
 * Color conversion for no colorspace change: just copy the data.
 */

METHODDEF void
null_convert (decompress_info_ptr cinfo, int num_rows,
	      JSAMPIMAGE input_data, JSAMPIMAGE output_data)
{
  short ci;

  for (ci = 0; ci < cinfo->num_components; ci++) {
    jcopy_sample_rows(input_data[ci], 0, output_data[ci], 0,
		      num_rows, cinfo->image_width);
  }
}


/*
 * Color conversion for grayscale: just copy the data.
 * This also works for YCbCr/YIQ -> grayscale conversion, in which
 * we just copy the Y (luminance) component and ignore chrominance.
 */

METHODDEF void
grayscale_convert (decompress_info_ptr cinfo, int num_rows,
		   JSAMPIMAGE input_data, JSAMPIMAGE output_data)
{
  jcopy_sample_rows(input_data[0], 0, output_data[0], 0,
		    num_rows, cinfo->image_width);
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
colorout_term (decompress_info_ptr cinfo)
{
  /* no work needed */
}


/*
 * The method selection routine for output colorspace conversion.
 */

GLOBAL void
jseldcolor (decompress_info_ptr cinfo)
{
  /* Make sure num_components agrees with jpeg_color_space */
  switch (cinfo->jpeg_color_space) {
  case CS_GRAYSCALE:
    if (cinfo->num_components != 1)
      ERREXIT(cinfo->emethods, "Bogus JPEG colorspace");
    break;

  case CS_RGB:
  case CS_YIQ:
  case CS_YCbCr:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo->emethods, "Bogus JPEG colorspace");
    break;

  case CS_CMYK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo->emethods, "Bogus JPEG colorspace");
    break;

  default:
    ERREXIT(cinfo->emethods, "Unsupported JPEG colorspace");
    break;
  }

  /* Set color_out_comps and conversion method based on requested space */
  switch (cinfo->out_color_space) {
  case CS_GRAYSCALE:
    cinfo->color_out_comps = 1;
    if (cinfo->jpeg_color_space == CS_GRAYSCALE ||
	cinfo->jpeg_color_space == CS_YCbCr ||
	cinfo->jpeg_color_space == CS_YIQ)
      cinfo->methods->color_convert = grayscale_convert;
    else
      ERREXIT(cinfo->emethods, "Unsupported color conversion request");
    break;

  case CS_RGB:
    cinfo->color_out_comps = 3;
    if (cinfo->jpeg_color_space == CS_YCbCr)
      cinfo->methods->color_convert = ycc_rgb_convert;
    else if (cinfo->jpeg_color_space == CS_RGB)
      cinfo->methods->color_convert = null_convert;
    else
      ERREXIT(cinfo->emethods, "Unsupported color conversion request");
    break;

  case CS_CMYK:
    cinfo->color_out_comps = 4;
    if (cinfo->jpeg_color_space == CS_CMYK)
      cinfo->methods->color_convert = null_convert;
    else
      ERREXIT(cinfo->emethods, "Unsupported color conversion request");
    break;

  default:
    ERREXIT(cinfo->emethods, "Unsupported output colorspace");
    break;
  }

  if (cinfo->quantize_colors)
    cinfo->final_out_comps = 1;	/* single colormapped output component */
  else
    cinfo->final_out_comps = cinfo->color_out_comps;

  cinfo->methods->colorout_init = colorout_init;
  cinfo->methods->colorout_term = colorout_term;
}

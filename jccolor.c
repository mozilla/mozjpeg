/*
 * jccolor.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains input colorspace conversion routines.
 * These routines are invoked via the methods get_sample_rows
 * and colorin_init/term.
 */

#include "jinclude.h"


static JSAMPARRAY pixel_row;	/* Workspace for a pixel row in input format */


/*
 * Initialize for colorspace conversion.
 */

METHODDEF void
colorin_init (compress_info_ptr cinfo)
{
  /* Allocate a workspace for the result of get_input_row. */
  pixel_row = (*cinfo->emethods->alloc_small_sarray)
		(cinfo->image_width, (long) cinfo->input_components);
}


/*
 * Fetch some rows of pixels from get_input_row and convert to the
 * JPEG colorspace.
 */


/*
 * This version handles RGB -> YCbCr conversion.
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
 *	Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B
 *	Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B
 * where Cb and Cr must be incremented by MAXJSAMPLE/2 to create a
 * nonnegative output value.
 * (These numbers are derived from TIFF Appendix O, draft of 4/10/91.)
 *
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^14 (about 4 digits precision); we have to divide
 * the products by 2^14, with appropriate rounding, to get the correct answer.
 *
 * For even more speed, we could avoid any multiplications in the inner loop
 * by precalculating the constants times R,G,B for all possible values.
 * This is not currently implemented.
 */

#define SCALEBITS	14
#define ONE_HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))


METHODDEF void
get_rgb_ycc_rows (compress_info_ptr cinfo,
		  int rows_to_read, JSAMPIMAGE image_data)
{
  register INT32 r, g, b;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JSAMPROW outptr0, outptr1, outptr2;
  register long col;
  long width = cinfo->image_width;
  int row;

  for (row = 0; row < rows_to_read; row++) {
    /* Read one row from the source file */
    (*cinfo->methods->get_input_row) (cinfo, pixel_row);
    /* Convert colorspace */
    inptr0 = pixel_row[0];
    inptr1 = pixel_row[1];
    inptr2 = pixel_row[2];
    outptr0 = image_data[0][row];
    outptr1 = image_data[1][row];
    outptr2 = image_data[2][row];
    for (col = width; col > 0; col--) {
      r = GETJSAMPLE(*inptr0++);
      g = GETJSAMPLE(*inptr1++);
      b = GETJSAMPLE(*inptr2++);
      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
       * must be too; we do not need an explicit range-limiting operation.
       * Hence the value being shifted is never negative, and we don't
       * need the general RIGHT_SHIFT macro.
       */
      /* Y */
      *outptr0++ = (JSAMPLE)
	((  FIX(0.29900)*r  + FIX(0.58700)*g + FIX(0.11400)*b
	  + ONE_HALF) >> SCALEBITS);
      /* Cb */
      *outptr1++ = (JSAMPLE)
	(((-FIX(0.16874))*r - FIX(0.33126)*g + FIX(0.50000)*b
	  + ONE_HALF*(MAXJSAMPLE+1)) >> SCALEBITS);
      /* Cr */
      *outptr2++ = (JSAMPLE)
	((  FIX(0.50000)*r  - FIX(0.41869)*g - FIX(0.08131)*b
	  + ONE_HALF*(MAXJSAMPLE+1)) >> SCALEBITS);
    }
  }
}


/*
 * Fetch some rows of pixels from get_input_row and convert to the
 * JPEG colorspace.
 * This version handles grayscale (no conversion).
 */

METHODDEF void
get_grayscale_rows (compress_info_ptr cinfo,
		    int rows_to_read, JSAMPIMAGE image_data)
{
  int row;

  for (row = 0; row < rows_to_read; row++) {
    /* Read one row from the source file */
    (*cinfo->methods->get_input_row) (cinfo, pixel_row);
    /* Convert colorspace (gamma mapping needed here) */
    jcopy_sample_rows(pixel_row, 0, image_data[0], row,
		      1, cinfo->image_width);
  }
}


/*
 * Fetch some rows of pixels from get_input_row and convert to the
 * JPEG colorspace.
 * This version handles multi-component colorspaces without conversion.
 */

METHODDEF void
get_noconvert_rows (compress_info_ptr cinfo,
		    int rows_to_read, JSAMPIMAGE image_data)
{
  int row, ci;

  for (row = 0; row < rows_to_read; row++) {
    /* Read one row from the source file */
    (*cinfo->methods->get_input_row) (cinfo, pixel_row);
    /* Convert colorspace (gamma mapping needed here) */
    for (ci = 0; ci < cinfo->input_components; ci++) {
      jcopy_sample_rows(pixel_row, ci, image_data[ci], row,
			1, cinfo->image_width);
    }
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
colorin_term (compress_info_ptr cinfo)
{
  /* no work (we let free_all release the workspace) */
}


/*
 * The method selection routine for input colorspace conversion.
 */

GLOBAL void
jselccolor (compress_info_ptr cinfo)
{
  /* Make sure input_components agrees with in_color_space */
  switch (cinfo->in_color_space) {
  case CS_GRAYSCALE:
    if (cinfo->input_components != 1)
      ERREXIT(cinfo->emethods, "Bogus input colorspace");
    break;

  case CS_RGB:
  case CS_YCbCr:
  case CS_YIQ:
    if (cinfo->input_components != 3)
      ERREXIT(cinfo->emethods, "Bogus input colorspace");
    break;

  case CS_CMYK:
    if (cinfo->input_components != 4)
      ERREXIT(cinfo->emethods, "Bogus input colorspace");
    break;

  default:
    ERREXIT(cinfo->emethods, "Unsupported input colorspace");
    break;
  }

  /* Check num_components, set conversion method based on requested space */
  switch (cinfo->jpeg_color_space) {
  case CS_GRAYSCALE:
    if (cinfo->num_components != 1)
      ERREXIT(cinfo->emethods, "Bogus JPEG colorspace");
    if (cinfo->in_color_space == CS_GRAYSCALE)
      cinfo->methods->get_sample_rows = get_grayscale_rows;
    else
      ERREXIT(cinfo->emethods, "Unsupported color conversion request");
    break;

  case CS_YCbCr:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo->emethods, "Bogus JPEG colorspace");
    if (cinfo->in_color_space == CS_RGB)
      cinfo->methods->get_sample_rows = get_rgb_ycc_rows;
    else if (cinfo->in_color_space == CS_YCbCr)
      cinfo->methods->get_sample_rows = get_noconvert_rows;
    else
      ERREXIT(cinfo->emethods, "Unsupported color conversion request");
    break;

  case CS_CMYK:
    if (cinfo->num_components != 4)
      ERREXIT(cinfo->emethods, "Bogus JPEG colorspace");
    if (cinfo->in_color_space == CS_CMYK)
      cinfo->methods->get_sample_rows = get_noconvert_rows;
    else
      ERREXIT(cinfo->emethods, "Unsupported color conversion request");
    break;

  default:
    ERREXIT(cinfo->emethods, "Unsupported JPEG colorspace");
    break;
  }

  cinfo->methods->colorin_init = colorin_init;
  cinfo->methods->colorin_term = colorin_term;
}

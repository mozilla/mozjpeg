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


/**************** RGB -> YCbCr conversion: most common case **************/

/*
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
 *	Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B  + MAXJSAMPLE/2
 *	Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B  + MAXJSAMPLE/2
 * (These numbers are derived from TIFF 6.0 section 21, dated 3-June-92.)
 *
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^16 (about 4 digits precision); we have to divide
 * the products by 2^16, with appropriate rounding, to get the correct answer.
 *
 * For even more speed, we avoid doing any multiplications in the inner loop
 * by precalculating the constants times R,G,B for all possible values.
 * For 8-bit JSAMPLEs this is very reasonable (only 256 entries per table);
 * for 12-bit samples it is still acceptable.  It's not very reasonable for
 * 16-bit samples, but if you want lossless storage you shouldn't be changing
 * colorspace anyway.
 * The MAXJSAMPLE/2 offsets and the rounding fudge-factor of 0.5 are included
 * in the tables to save adding them separately in the inner loop.
 */

#ifdef SIXTEEN_BIT_SAMPLES
#define SCALEBITS	14	/* avoid overflow */
#else
#define SCALEBITS	16	/* speedier right-shift on some machines */
#endif
#define ONE_HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))

/* We allocate one big table and divide it up into eight parts, instead of
 * doing eight alloc_small requests.  This lets us use a single table base
 * address, which can be held in a register in the inner loops on many
 * machines (more than can hold all eight addresses, anyway).
 */

static INT32 * rgb_ycc_tab;	/* => table for RGB to YCbCr conversion */
#define R_Y_OFF		0			/* offset to R => Y section */
#define G_Y_OFF		(1*(MAXJSAMPLE+1))	/* offset to G => Y section */
#define B_Y_OFF		(2*(MAXJSAMPLE+1))	/* etc. */
#define R_CB_OFF	(3*(MAXJSAMPLE+1))
#define G_CB_OFF	(4*(MAXJSAMPLE+1))
#define B_CB_OFF	(5*(MAXJSAMPLE+1))
#define R_CR_OFF	B_CB_OFF		/* B=>Cb, R=>Cr are the same */
#define G_CR_OFF	(6*(MAXJSAMPLE+1))
#define B_CR_OFF	(7*(MAXJSAMPLE+1))
#define TABLE_SIZE	(8*(MAXJSAMPLE+1))


/*
 * Initialize for colorspace conversion.
 */

METHODDEF void
rgb_ycc_init (compress_info_ptr cinfo)
{
  INT32 i;

  /* Allocate a workspace for the result of get_input_row. */
  pixel_row = (*cinfo->emethods->alloc_small_sarray)
		(cinfo->image_width, (long) cinfo->input_components);

  /* Allocate and fill in the conversion tables. */
  rgb_ycc_tab = (INT32 *) (*cinfo->emethods->alloc_small)
				(TABLE_SIZE * SIZEOF(INT32));

  for (i = 0; i <= MAXJSAMPLE; i++) {
    rgb_ycc_tab[i+R_Y_OFF] = FIX(0.29900) * i;
    rgb_ycc_tab[i+G_Y_OFF] = FIX(0.58700) * i;
    rgb_ycc_tab[i+B_Y_OFF] = FIX(0.11400) * i     + ONE_HALF;
    rgb_ycc_tab[i+R_CB_OFF] = (-FIX(0.16874)) * i;
    rgb_ycc_tab[i+G_CB_OFF] = (-FIX(0.33126)) * i;
    rgb_ycc_tab[i+B_CB_OFF] = FIX(0.50000) * i    + ONE_HALF*(MAXJSAMPLE+1);
/*  B=>Cb and R=>Cr tables are the same
    rgb_ycc_tab[i+R_CR_OFF] = FIX(0.50000) * i    + ONE_HALF*(MAXJSAMPLE+1);
*/
    rgb_ycc_tab[i+G_CR_OFF] = (-FIX(0.41869)) * i;
    rgb_ycc_tab[i+B_CR_OFF] = (-FIX(0.08131)) * i;
  }
}


/*
 * Fetch some rows of pixels from get_input_row and convert to the
 * JPEG colorspace.
 */

METHODDEF void
get_rgb_ycc_rows (compress_info_ptr cinfo,
		  int rows_to_read, JSAMPIMAGE image_data)
{
#ifdef SIXTEEN_BIT_SAMPLES
  register UINT16 r, g, b;
#else
  register int r, g, b;
#endif
  register INT32 * ctab = rgb_ycc_tab;
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
    for (col = 0; col < width; col++) {
      r = GETJSAMPLE(inptr0[col]);
      g = GETJSAMPLE(inptr1[col]);
      b = GETJSAMPLE(inptr2[col]);
      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
       * must be too; we do not need an explicit range-limiting operation.
       * Hence the value being shifted is never negative, and we don't
       * need the general RIGHT_SHIFT macro.
       */
      /* Y */
      outptr0[col] = (JSAMPLE)
		((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
		 >> SCALEBITS);
      /* Cb */
      outptr1[col] = (JSAMPLE)
		((ctab[r+R_CB_OFF] + ctab[g+G_CB_OFF] + ctab[b+B_CB_OFF])
		 >> SCALEBITS);
      /* Cr */
      outptr2[col] = (JSAMPLE)
		((ctab[r+R_CR_OFF] + ctab[g+G_CR_OFF] + ctab[b+B_CR_OFF])
		 >> SCALEBITS);
    }
  }
}


/**************** Cases other than RGB -> YCbCr **************/


/*
 * Fetch some rows of pixels from get_input_row and convert to the
 * JPEG colorspace.
 * This version handles RGB->grayscale conversion, which is the same
 * as the RGB->Y portion of RGB->YCbCr.
 * We assume rgb_ycc_init has been called (we only use the Y tables).
 */

METHODDEF void
get_rgb_gray_rows (compress_info_ptr cinfo,
		   int rows_to_read, JSAMPIMAGE image_data)
{
#ifdef SIXTEEN_BIT_SAMPLES
  register UINT16 r, g, b;
#else
  register int r, g, b;
#endif
  register INT32 * ctab = rgb_ycc_tab;
  register JSAMPROW inptr0, inptr1, inptr2;
  register JSAMPROW outptr;
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
    outptr = image_data[0][row];
    for (col = 0; col < width; col++) {
      r = GETJSAMPLE(inptr0[col]);
      g = GETJSAMPLE(inptr1[col]);
      b = GETJSAMPLE(inptr2[col]);
      /* If the inputs are 0..MAXJSAMPLE, the outputs of these equations
       * must be too; we do not need an explicit range-limiting operation.
       * Hence the value being shifted is never negative, and we don't
       * need the general RIGHT_SHIFT macro.
       */
      /* Y */
      outptr[col] = (JSAMPLE)
		((ctab[r+R_Y_OFF] + ctab[g+G_Y_OFF] + ctab[b+B_Y_OFF])
		 >> SCALEBITS);
    }
  }
}


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
 * This version handles grayscale output with no conversion.
 * The source can be either plain grayscale or YCbCr (since Y == gray).
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

  /* Standard init/term methods (may override below) */
  cinfo->methods->colorin_init = colorin_init;
  cinfo->methods->colorin_term = colorin_term;

  /* Check num_components, set conversion method based on requested space */
  switch (cinfo->jpeg_color_space) {
  case CS_GRAYSCALE:
    if (cinfo->num_components != 1)
      ERREXIT(cinfo->emethods, "Bogus JPEG colorspace");
    if (cinfo->in_color_space == CS_GRAYSCALE)
      cinfo->methods->get_sample_rows = get_grayscale_rows;
    else if (cinfo->in_color_space == CS_RGB) {
      cinfo->methods->colorin_init = rgb_ycc_init;
      cinfo->methods->get_sample_rows = get_rgb_gray_rows;
    } else if (cinfo->in_color_space == CS_YCbCr)
      cinfo->methods->get_sample_rows = get_grayscale_rows;
    else
      ERREXIT(cinfo->emethods, "Unsupported color conversion request");
    break;

  case CS_YCbCr:
    if (cinfo->num_components != 3)
      ERREXIT(cinfo->emethods, "Bogus JPEG colorspace");
    if (cinfo->in_color_space == CS_RGB) {
      cinfo->methods->colorin_init = rgb_ycc_init;
      cinfo->methods->get_sample_rows = get_rgb_ycc_rows;
    } else if (cinfo->in_color_space == CS_YCbCr)
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
}

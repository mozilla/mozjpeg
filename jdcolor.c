/*
 * jdcolor.c
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains output colorspace conversion routines.
 * These routines are invoked via the methods color_convert
 * and colorout_init/term.
 */

#include "jinclude.h"


/**************** YCbCr -> RGB conversion: most common case **************/

/*
 * YCbCr is defined per CCIR 601-1, except that Cb and Cr are
 * normalized to the range 0..MAXJSAMPLE rather than -0.5 .. 0.5.
 * The conversion equations to be implemented are therefore
 *	R = Y                + 1.40200 * Cr
 *	G = Y - 0.34414 * Cb - 0.71414 * Cr
 *	B = Y + 1.77200 * Cb
 * where Cb and Cr represent the incoming values less MAXJSAMPLE/2.
 * (These numbers are derived from TIFF 6.0 section 21, dated 3-June-92.)
 *
 * To avoid floating-point arithmetic, we represent the fractional constants
 * as integers scaled up by 2^16 (about 4 digits precision); we have to divide
 * the products by 2^16, with appropriate rounding, to get the correct answer.
 * Notice that Y, being an integral input, does not contribute any fraction
 * so it need not participate in the rounding.
 *
 * For even more speed, we avoid doing any multiplications in the inner loop
 * by precalculating the constants times Cb and Cr for all possible values.
 * For 8-bit JSAMPLEs this is very reasonable (only 256 entries per table);
 * for 12-bit samples it is still acceptable.  It's not very reasonable for
 * 16-bit samples, but if you want lossless storage you shouldn't be changing
 * colorspace anyway.
 * The Cr=>R and Cb=>B values can be rounded to integers in advance; the
 * values for the G calculation are left scaled up, since we must add them
 * together before rounding.
 */

#ifdef SIXTEEN_BIT_SAMPLES
#define SCALEBITS	14	/* avoid overflow */
#else
#define SCALEBITS	16	/* speedier right-shift on some machines */
#endif
#define ONE_HALF	((INT32) 1 << (SCALEBITS-1))
#define FIX(x)		((INT32) ((x) * (1L<<SCALEBITS) + 0.5))

static int * Cr_r_tab;		/* => table for Cr to R conversion */
static int * Cb_b_tab;		/* => table for Cb to B conversion */
static INT32 * Cr_g_tab;	/* => table for Cr to G conversion */
static INT32 * Cb_g_tab;	/* => table for Cb to G conversion */


/*
 * Initialize for colorspace conversion.
 */

METHODDEF void
ycc_rgb_init (decompress_info_ptr cinfo)
{
  INT32 i, x2;
  SHIFT_TEMPS

  Cr_r_tab = (int *) (*cinfo->emethods->alloc_small)
				((MAXJSAMPLE+1) * SIZEOF(int));
  Cb_b_tab = (int *) (*cinfo->emethods->alloc_small)
				((MAXJSAMPLE+1) * SIZEOF(int));
  Cr_g_tab = (INT32 *) (*cinfo->emethods->alloc_small)
				((MAXJSAMPLE+1) * SIZEOF(INT32));
  Cb_g_tab = (INT32 *) (*cinfo->emethods->alloc_small)
				((MAXJSAMPLE+1) * SIZEOF(INT32));

  for (i = 0; i <= MAXJSAMPLE; i++) {
    /* i is the actual input pixel value, in the range 0..MAXJSAMPLE */
    /* The Cb or Cr value we are thinking of is x = i - MAXJSAMPLE/2 */
    x2 = 2*i - MAXJSAMPLE;	/* twice x */
    /* Cr=>R value is nearest int to 1.40200 * x */
    Cr_r_tab[i] = (int)
		    RIGHT_SHIFT(FIX(1.40200/2) * x2 + ONE_HALF, SCALEBITS);
    /* Cb=>B value is nearest int to 1.77200 * x */
    Cb_b_tab[i] = (int)
		    RIGHT_SHIFT(FIX(1.77200/2) * x2 + ONE_HALF, SCALEBITS);
    /* Cr=>G value is scaled-up -0.71414 * x */
    Cr_g_tab[i] = (- FIX(0.71414/2)) * x2;
    /* Cb=>G value is scaled-up -0.34414 * x */
    /* We also add in ONE_HALF so that need not do it in inner loop */
    Cb_g_tab[i] = (- FIX(0.34414/2)) * x2 + ONE_HALF;
  }
}


/*
 * Convert some rows of samples to the output colorspace.
 */

METHODDEF void
ycc_rgb_convert (decompress_info_ptr cinfo, int num_rows, long num_cols,
		 JSAMPIMAGE input_data, JSAMPIMAGE output_data)
{
#ifdef SIXTEEN_BIT_SAMPLES
  register INT32 y;
  register UINT16 cb, cr;
#else
  register int y, cb, cr;
#endif
  register JSAMPROW inptr0, inptr1, inptr2;
  register JSAMPROW outptr0, outptr1, outptr2;
  register long col;
  /* copy these pointers into registers if possible */
  register JSAMPLE * range_limit = cinfo->sample_range_limit;
  register int * Crrtab = Cr_r_tab;
  register int * Cbbtab = Cb_b_tab;
  register INT32 * Crgtab = Cr_g_tab;
  register INT32 * Cbgtab = Cb_g_tab;
  int row;
  SHIFT_TEMPS
  
  for (row = 0; row < num_rows; row++) {
    inptr0 = input_data[0][row];
    inptr1 = input_data[1][row];
    inptr2 = input_data[2][row];
    outptr0 = output_data[0][row];
    outptr1 = output_data[1][row];
    outptr2 = output_data[2][row];
    for (col = 0; col < num_cols; col++) {
      y  = GETJSAMPLE(inptr0[col]);
      cb = GETJSAMPLE(inptr1[col]);
      cr = GETJSAMPLE(inptr2[col]);
      /* Note: if the inputs were computed directly from RGB values,
       * range-limiting would be unnecessary here; but due to possible
       * noise in the DCT/IDCT phase, we do need to apply range limits.
       */
      outptr0[col] = range_limit[y + Crrtab[cr]];	/* red */
      outptr1[col] = range_limit[y +			/* green */
				 ((int) RIGHT_SHIFT(Cbgtab[cb] + Crgtab[cr],
						    SCALEBITS))];
      outptr2[col] = range_limit[y + Cbbtab[cb]];	/* blue */
    }
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
ycc_rgb_term (decompress_info_ptr cinfo)
{
  /* no work (we let free_all release the workspace) */
}


/**************** Cases other than YCbCr -> RGB **************/


/*
 * Initialize for colorspace conversion.
 */

METHODDEF void
null_init (decompress_info_ptr cinfo)
/* colorout_init for cases where no setup is needed */
{
  /* no work needed */
}


/*
 * Color conversion for no colorspace change: just copy the data.
 */

METHODDEF void
null_convert (decompress_info_ptr cinfo, int num_rows, long num_cols,
	      JSAMPIMAGE input_data, JSAMPIMAGE output_data)
{
  short ci;

  for (ci = 0; ci < cinfo->num_components; ci++) {
    jcopy_sample_rows(input_data[ci], 0, output_data[ci], 0,
		      num_rows, num_cols);
  }
}


/*
 * Color conversion for grayscale: just copy the data.
 * This also works for YCbCr/YIQ -> grayscale conversion, in which
 * we just copy the Y (luminance) component and ignore chrominance.
 */

METHODDEF void
grayscale_convert (decompress_info_ptr cinfo, int num_rows, long num_cols,
		   JSAMPIMAGE input_data, JSAMPIMAGE output_data)
{
  jcopy_sample_rows(input_data[0], 0, output_data[0], 0,
		    num_rows, num_cols);
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
null_term (decompress_info_ptr cinfo)
/* colorout_term for cases where no teardown is needed */
{
  /* no work needed */
}



/*
 * The method selection routine for output colorspace conversion.
 */

GLOBAL void
jseldcolor (decompress_info_ptr cinfo)
{
  int ci;

  /* Make sure num_components agrees with jpeg_color_space */
  switch (cinfo->jpeg_color_space) {
  case CS_GRAYSCALE:
    if (cinfo->num_components != 1)
      ERREXIT(cinfo->emethods, "Bogus JPEG colorspace");
    break;

  case CS_RGB:
  case CS_YCbCr:
  case CS_YIQ:
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

  /* Set color_out_comps and conversion method based on requested space. */
  /* Also clear the component_needed flags for any unused components, */
  /* so that earlier pipeline stages can avoid useless computation. */

  switch (cinfo->out_color_space) {
  case CS_GRAYSCALE:
    cinfo->color_out_comps = 1;
    if (cinfo->jpeg_color_space == CS_GRAYSCALE ||
	cinfo->jpeg_color_space == CS_YCbCr ||
	cinfo->jpeg_color_space == CS_YIQ) {
      cinfo->methods->color_convert = grayscale_convert;
      cinfo->methods->colorout_init = null_init;
      cinfo->methods->colorout_term = null_term;
      /* For color->grayscale conversion, only the Y (0) component is needed */
      for (ci = 1; ci < cinfo->num_components; ci++)
	cinfo->cur_comp_info[ci]->component_needed = FALSE;
    } else
      ERREXIT(cinfo->emethods, "Unsupported color conversion request");
    break;

  case CS_RGB:
    cinfo->color_out_comps = 3;
    if (cinfo->jpeg_color_space == CS_YCbCr) {
      cinfo->methods->color_convert = ycc_rgb_convert;
      cinfo->methods->colorout_init = ycc_rgb_init;
      cinfo->methods->colorout_term = ycc_rgb_term;
    } else if (cinfo->jpeg_color_space == CS_RGB) {
      cinfo->methods->color_convert = null_convert;
      cinfo->methods->colorout_init = null_init;
      cinfo->methods->colorout_term = null_term;
    } else
      ERREXIT(cinfo->emethods, "Unsupported color conversion request");
    break;

  default:
    /* Permit null conversion from CMYK or YCbCr to same output space */
    if (cinfo->out_color_space == cinfo->jpeg_color_space) {
      cinfo->color_out_comps = cinfo->num_components;
      cinfo->methods->color_convert = null_convert;
      cinfo->methods->colorout_init = null_init;
      cinfo->methods->colorout_term = null_term;
    } else			/* unsupported non-null conversion */
      ERREXIT(cinfo->emethods, "Unsupported color conversion request");
    break;
  }

  if (cinfo->quantize_colors)
    cinfo->final_out_comps = 1;	/* single colormapped output component */
  else
    cinfo->final_out_comps = cinfo->color_out_comps;
}

/*
 * jquant1.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains 1-pass color quantization (color mapping) routines.
 * These routines are invoked via the methods color_quantize
 * and color_quant_init/term.
 */

#include "jinclude.h"

#ifdef QUANT_1PASS_SUPPORTED


/*
 * This implementation is a fairly dumb, quick-and-dirty quantizer;
 * it's here mostly so that we can start working on colormapped output formats.
 *
 * We quantize to a color map that is selected in advance of seeing the image;
 * the map depends only on the requested number of colors (at least 8).
 * The map consists of all combinations of Ncolors[j] color values for each
 * component j; we choose Ncolors[] based on the requested # of colors.
 * We always use 0 and MAXJSAMPLE in each color (hence the minimum 8 colors).
 * Any additional color values are equally spaced between these limits.
 *
 * The result almost always needs dithering to look decent.
 */

#define MAX_COMPONENTS 4	/* max components I can handle */

static int total_colors;	/* Number of distinct output colors */
static int Ncolors[MAX_COMPONENTS]; /* # of values alloced to each component */
/* total_colors is the product of the Ncolors[] values */

static JSAMPARRAY colormap;	/* The actual color map */
/* colormap[i][j] = value of i'th color component for output pixel value j */

static JSAMPARRAY colorindex;	/* Precomputed mapping for speed */
/* colorindex[i][j] = index of color closest to pixel value j in component i,
 * premultiplied so that the correct mapped value for a pixel (r,g,b) is:
 *   colorindex[0][r] + colorindex[1][g] + colorindex[2][b]
 */


/* Declarations for Floyd-Steinberg dithering.
 * Errors are accumulated into the arrays evenrowerrs[] and oddrowerrs[],
 * each of which have #colors * (#columns + 2) entries (so that first/last
 * pixels need not be special cases).  These have resolutions of 1/16th of
 * a pixel count.  The error at a given pixel is propagated to its unprocessed
 * neighbors using the standard F-S fractions,
 *		...	(here)	7/16
 *		3/16	5/16	1/16
 * We work left-to-right on even rows, right-to-left on odd rows.
 *
 * Note: on a wide image, we might not have enough room in a PC's near data
 * segment to hold the error arrays; so they are allocated with alloc_medium.
 */

#ifdef EIGHT_BIT_SAMPLES
typedef INT16 FSERROR;		/* 16 bits should be enough */
#else
typedef INT32 FSERROR;		/* may need more than 16 bits? */
#endif

typedef FSERROR FAR *FSERRPTR;	/* pointer to error array (in FAR storage!) */

static FSERRPTR evenrowerrs, oddrowerrs; /* current-row and next-row errors */
static boolean on_odd_row;	/* flag to remember which row we are on */


/*
 * Initialize for one-pass color quantization.
 */

METHODDEF void
color_quant_init (decompress_info_ptr cinfo)
{
  int max_colors = cinfo->desired_number_of_colors;
  int i,j,k, ntc, nci, blksize, blkdist, ptr, val;

  if (cinfo->color_out_comps > MAX_COMPONENTS)
    ERREXIT1(cinfo->emethods, "Cannot quantize more than %d color components",
	     MAX_COMPONENTS);
  if (max_colors > (MAXJSAMPLE+1))
    ERREXIT1(cinfo->emethods, "Cannot request more than %d quantized colors",
	    MAXJSAMPLE+1);

  /* Initialize to 2 color values for each component */
  total_colors = 1;
  for (i = 0; i < cinfo->color_out_comps; i++) {
    Ncolors[i] = 2;
    total_colors *= 2;
  }
  if (total_colors > max_colors)
    ERREXIT1(cinfo->emethods, "Cannot quantize to fewer than %d colors",
	     total_colors);

  /* Increase the number of color values until requested limit is reached. */
  /* Note that for standard RGB color space, we will have at least as many */
  /* red values as green, and at least as many green values as blue. */
  i = 0;			/* component index to increase next */
  /* test calculates ntc = new total_colors if Ncolors[i] is incremented */
  while ((ntc = (total_colors / Ncolors[i]) * (Ncolors[i]+1)) <= max_colors) {
    Ncolors[i]++;		/* OK, apply the increment */
    total_colors = ntc;
    i++;			/* advance to next component */
    if (i >= cinfo->color_out_comps) i = 0;
  }

  /* Report selected color counts */
  if (cinfo->color_out_comps == 3)
    TRACEMS4(cinfo->emethods, 1, "Quantizing to %d = %d*%d*%d colors",
	     total_colors, Ncolors[0], Ncolors[1], Ncolors[2]);
  else
    TRACEMS1(cinfo->emethods, 1, "Quantizing to %d colors", total_colors);

  /* Allocate and fill in the colormap and color index. */
  /* The colors are ordered in the map in standard row-major order, */
  /* i.e. rightmost (highest-indexed) color changes most rapidly. */

  colormap = (*cinfo->emethods->alloc_small_sarray)
		((long) total_colors, (long) cinfo->color_out_comps);
  colorindex = (*cinfo->emethods->alloc_small_sarray)
		((long) (MAXJSAMPLE+1), (long) cinfo->color_out_comps);

  /* blksize is number of adjacent repeated entries for a component */
  /* blkdist is distance between groups of identical entries for a component */
  blkdist = total_colors;

  for (i = 0; i < cinfo->color_out_comps; i++) {
    /* fill in colormap entries for i'th color component */
    nci = Ncolors[i];		/* # of distinct values for this color */
    blksize = blkdist / nci;
    for (j = 0; j < nci; j++) {
      /* Compute j'th output value (out of nci) for component */
      val = (j * MAXJSAMPLE + (nci-1)/2) / (nci-1);
      /* Fill in all colormap entries that have this value of this component */
      for (ptr = j * blksize; ptr < total_colors; ptr += blkdist) {
	/* fill in blksize entries beginning at ptr */
	for (k = 0; k < blksize; k++)
	  colormap[i][ptr+k] = (JSAMPLE) val;
      }
    }
    blkdist = blksize;		/* blksize of this color is blkdist of next */

    /* fill in colorindex entries for i'th color component */
    for (j = 0; j <= MAXJSAMPLE; j++) {
      /* compute index of color closest to pixel value j */
      val = (j * (nci-1) + CENTERJSAMPLE) / MAXJSAMPLE;
      /* premultiply so that no multiplication needed in main processing */
      colorindex[i][j] = (JSAMPLE) (val * blksize);
    }
  }

  /* Pass the colormap to the output module.  Note that the output */
  /* module is allowed to save this pointer and use the map during */
  /* any put_pixel_rows call! */
  (*cinfo->methods->put_color_map) (cinfo, total_colors, colormap);

  /* Allocate Floyd-Steinberg workspace if necessary */
  if (cinfo->use_dithering) {
    size_t arraysize = (cinfo->image_width + 2L) * cinfo->color_out_comps
		       * SIZEOF(FSERROR);

    evenrowerrs = (FSERRPTR) (*cinfo->emethods->alloc_medium) (arraysize);
    oddrowerrs  = (FSERRPTR) (*cinfo->emethods->alloc_medium) (arraysize);
    /* we only need to zero the forward contribution for current row. */
    jzero_far((void FAR *) evenrowerrs, arraysize);
    on_odd_row = FALSE;
  }

}


/*
 * Map some rows of pixels to the output colormapped representation.
 */

METHODDEF void
color_quantize (decompress_info_ptr cinfo, int num_rows,
		JSAMPIMAGE input_data, JSAMPARRAY output_data)
/* General case, no dithering */
{
  register int pixcode, ci;
  register long col;
  register int row;
  register long widthm1 = cinfo->image_width - 1;
  register int nc = cinfo->color_out_comps;  

  for (row = 0; row < num_rows; row++) {
    for (col = widthm1; col >= 0; col--) {
      pixcode = 0;
      for (ci = 0; ci < nc; ci++) {
	pixcode += GETJSAMPLE(colorindex[ci]
			      [GETJSAMPLE(input_data[ci][row][col])]);
      }
      output_data[row][col] = (JSAMPLE) pixcode;
    }
  }
}


METHODDEF void
color_quantize3 (decompress_info_ptr cinfo, int num_rows,
		 JSAMPIMAGE input_data, JSAMPARRAY output_data)
/* Fast path for color_out_comps==3, no dithering */
{
  register int pixcode;
  register JSAMPROW ptr0, ptr1, ptr2, ptrout;
  register long col;
  register int row;
  register long width = cinfo->image_width;

  for (row = 0; row < num_rows; row++) {
    ptr0 = input_data[0][row];
    ptr1 = input_data[1][row];
    ptr2 = input_data[2][row];
    ptrout = output_data[row];
    for (col = width; col > 0; col--) {
      pixcode  = GETJSAMPLE(colorindex[0][GETJSAMPLE(*ptr0++)]);
      pixcode += GETJSAMPLE(colorindex[1][GETJSAMPLE(*ptr1++)]);
      pixcode += GETJSAMPLE(colorindex[2][GETJSAMPLE(*ptr2++)]);
      *ptrout++ = (JSAMPLE) pixcode;
    }
  }
}


METHODDEF void
color_quantize_dither (decompress_info_ptr cinfo, int num_rows,
		       JSAMPIMAGE input_data, JSAMPARRAY output_data)
/* General case, with Floyd-Steinberg dithering */
{
  register int pixcode, ci;
  register FSERROR val;
  register FSERRPTR thisrowerr, nextrowerr;
  register long col;
  register int row;
  register long width = cinfo->image_width;
  register int nc = cinfo->color_out_comps;  

  for (row = 0; row < num_rows; row++) {
    if (on_odd_row) {
      /* work right to left in this row */
      thisrowerr = oddrowerrs + width*nc;
      nextrowerr = evenrowerrs + width*nc;
      for (ci = 0; ci < nc; ci++) /* need only initialize this one entry */
	nextrowerr[ci] = 0;
      for (col = width - 1; col >= 0; col--) {
	/* select the output pixel value */
	pixcode = 0;
	for (ci = 0; ci < nc; ci++) {
	  /* compute pixel value + accumulated error */
	  val = (((FSERROR) GETJSAMPLE(input_data[ci][row][col])) << 4)
		+ thisrowerr[ci];
	  if (val <= 0) val = 0; /* must watch for range overflow! */
	  else {
	    val += 8;		/* divide by 16 with proper rounding */
	    val >>= 4;
	    if (val > MAXJSAMPLE) val = MAXJSAMPLE;
	  }
	  thisrowerr[ci] = val;	/* save for error propagation */
	  pixcode += GETJSAMPLE(colorindex[ci][val]);
	}
	output_data[row][col] = (JSAMPLE) pixcode;
	/* propagate error to adjacent pixels */
	for (ci = 0; ci < nc; ci++) {
	  val = thisrowerr[ci] - (FSERROR) GETJSAMPLE(colormap[ci][pixcode]);
	  thisrowerr[ci-nc] += val * 7;
	  nextrowerr[ci+nc] += val * 3;
	  nextrowerr[ci   ] += val * 5;
	  nextrowerr[ci-nc]  = val; /* not +=, since not initialized yet */
	}
	thisrowerr -= nc;	/* advance error ptrs to next pixel entry */
	nextrowerr -= nc;
      }
      on_odd_row = FALSE;
    } else {
      /* work left to right in this row */
      thisrowerr = evenrowerrs + nc;
      nextrowerr = oddrowerrs + nc;
      for (ci = 0; ci < nc; ci++) /* need only initialize this one entry */
	nextrowerr[ci] = 0;
      for (col = 0; col < width; col++) {
	/* select the output pixel value */
	pixcode = 0;
	for (ci = 0; ci < nc; ci++) {
	  /* compute pixel value + accumulated error */
	  val = (((FSERROR) GETJSAMPLE(input_data[ci][row][col])) << 4)
		+ thisrowerr[ci];
	  if (val <= 0) val = 0; /* must watch for range overflow! */
	  else {
	    val += 8;		/* divide by 16 with proper rounding */
	    val >>= 4;
	    if (val > MAXJSAMPLE) val = MAXJSAMPLE;
	  }
	  thisrowerr[ci] = val;	/* save for error propagation */
	  pixcode += GETJSAMPLE(colorindex[ci][val]);
	}
	output_data[row][col] = (JSAMPLE) pixcode;
	/* propagate error to adjacent pixels */
	for (ci = 0; ci < nc; ci++) {
	  val = thisrowerr[ci] - (FSERROR) GETJSAMPLE(colormap[ci][pixcode]);
	  thisrowerr[ci+nc] += val * 7;
	  nextrowerr[ci-nc] += val * 3;
	  nextrowerr[ci   ] += val * 5;
	  nextrowerr[ci+nc]  = val; /* not +=, since not initialized yet */
	}
	thisrowerr += nc;	/* advance error ptrs to next pixel entry */
	nextrowerr += nc;
      }
      on_odd_row = TRUE;
    }
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
color_quant_term (decompress_info_ptr cinfo)
{
  /* We can't free the colormap until now, since output module may use it! */
  (*cinfo->emethods->free_small_sarray)
		(colormap, (long) cinfo->color_out_comps);
  (*cinfo->emethods->free_small_sarray)
		(colorindex, (long) cinfo->color_out_comps);
  if (cinfo->use_dithering) {
    (*cinfo->emethods->free_medium) ((void FAR *) evenrowerrs);
    (*cinfo->emethods->free_medium) ((void FAR *) oddrowerrs);
  }
}


/*
 * Prescan some rows of pixels.
 * Not used in one-pass case.
 */

METHODDEF void
color_quant_prescan (decompress_info_ptr cinfo, int num_rows,
		     JSAMPIMAGE image_data)
{
  ERREXIT(cinfo->emethods, "Should not get here!");
}


/*
 * Do two-pass quantization.
 * Not used in one-pass case.
 */

METHODDEF void
color_quant_doit (decompress_info_ptr cinfo, quantize_caller_ptr source_method)
{
  ERREXIT(cinfo->emethods, "Should not get here!");
}


/*
 * The method selection routine for 1-pass color quantization.
 */

GLOBAL void
jsel1quantize (decompress_info_ptr cinfo)
{
  if (! cinfo->two_pass_quantize) {
    cinfo->methods->color_quant_init = color_quant_init;
    if (cinfo->use_dithering) {
      cinfo->methods->color_quantize = color_quantize_dither;
    } else {
      if (cinfo->color_out_comps == 3)
	cinfo->methods->color_quantize = color_quantize3;
      else
	cinfo->methods->color_quantize = color_quantize;
    }
    cinfo->methods->color_quant_prescan = color_quant_prescan;
    cinfo->methods->color_quant_doit = color_quant_doit;
    cinfo->methods->color_quant_term = color_quant_term;
  }
}

#endif /* QUANT_1PASS_SUPPORTED */

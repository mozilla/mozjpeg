/*
 * jquant1.c
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
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
 * The main purpose of 1-pass quantization is to provide a fast, if not very
 * high quality, colormapped output capability.  A 2-pass quantizer usually
 * gives better visual quality; however, for quantized grayscale output this
 * quantizer is perfectly adequate.  Dithering is highly recommended with this
 * quantizer, though you can turn it off if you really want to.
 *
 * This implementation quantizes in the output colorspace.  This has a couple
 * of disadvantages: each pixel must be individually color-converted, and if
 * the color conversion includes gamma correction then quantization is done in
 * a nonlinear space, which is less desirable.  The major advantage is that
 * with the usual output color spaces (RGB, grayscale) an orthogonal grid of
 * representative colors can be used, thus permitting the very simple and fast
 * color lookup scheme used here.  The standard JPEG colorspace (YCbCr) cannot
 * be effectively handled this way, because only about a quarter of an
 * orthogonal grid would fall within the gamut of realizable colors.  Another
 * advantage is that when the user wants quantized grayscale output from a
 * color JPEG file, this quantizer can provide a high-quality result with no
 * special hacking.
 *
 * The gamma-correction problem could be eliminated by adjusting the grid
 * spacing to counteract the gamma correction applied by color_convert.
 * At this writing, gamma correction is not implemented by jdcolor, so
 * nothing is done here.
 *
 * In 1-pass quantization the colormap must be chosen in advance of seeing the
 * image.  We use a map consisting of all combinations of Ncolors[i] color
 * values for the i'th component.  The Ncolors[] values are chosen so that
 * their product, the total number of colors, is no more than that requested.
 * (In most cases, the product will be somewhat less.)
 *
 * Since the colormap is orthogonal, the representative value for each color
 * component can be determined without considering the other components;
 * then these indexes can be combined into a colormap index by a standard
 * N-dimensional-array-subscript calculation.  Most of the arithmetic involved
 * can be precalculated and stored in the lookup table colorindex[].
 * colorindex[i][j] maps pixel value j in component i to the nearest
 * representative value (grid plane) for that component; this index is
 * multiplied by the array stride for component i, so that the
 * index of the colormap entry closest to a given pixel value is just
 *    sum( colorindex[component-number][pixel-component-value] )
 * Aside from being fast, this scheme allows for variable spacing between
 * representative values with no additional lookup cost.
 */


#define MAX_COMPONENTS 4	/* max components I can handle */

static JSAMPARRAY colormap;	/* The actual color map */
/* colormap[i][j] = value of i'th color component for output pixel value j */

static JSAMPARRAY colorindex;	/* Precomputed mapping for speed */
/* colorindex[i][j] = index of color closest to pixel value j in component i,
 * premultiplied as described above.  Since colormap indexes must fit into
 * JSAMPLEs, the entries of this array will too.
 */

static JSAMPARRAY input_buffer;	/* color conversion workspace */
/* Since our input data is presented in the JPEG colorspace, we have to call
 * color_convert to get it into the output colorspace.  input_buffer is a
 * one-row-high workspace for the result of color_convert.
 */


/* Declarations for Floyd-Steinberg dithering.
 *
 * Errors are accumulated into the array fserrors[], at a resolution of
 * 1/16th of a pixel count.  The error at a given pixel is propagated
 * to its not-yet-processed neighbors using the standard F-S fractions,
 *		...	(here)	7/16
 *		3/16	5/16	1/16
 * We work left-to-right on even rows, right-to-left on odd rows.
 *
 * We can get away with a single array (holding one row's worth of errors)
 * by using it to store the current row's errors at pixel columns not yet
 * processed, but the next row's errors at columns already processed.  We
 * need only a few extra variables to hold the errors immediately around the
 * current column.  (If we are lucky, those variables are in registers, but
 * even if not, they're probably cheaper to access than array elements are.)
 *
 * The fserrors[] array is indexed [component#][position].
 * We provide (#columns + 2) entries per component; the extra entry at each
 * end saves us from special-casing the first and last pixels.
 *
 * Note: on a wide image, we might not have enough room in a PC's near data
 * segment to hold the error array; so it is allocated with alloc_medium.
 */

#ifdef EIGHT_BIT_SAMPLES
typedef INT16 FSERROR;		/* 16 bits should be enough */
typedef int LOCFSERROR;		/* use 'int' for calculation temps */
#else
typedef INT32 FSERROR;		/* may need more than 16 bits */
typedef INT32 LOCFSERROR;	/* be sure calculation temps are big enough */
#endif

typedef FSERROR FAR *FSERRPTR;	/* pointer to error array (in FAR storage!) */

static FSERRPTR fserrors[MAX_COMPONENTS]; /* accumulated errors */
static boolean on_odd_row;	/* flag to remember which row we are on */


/*
 * Policy-making subroutines for color_quant_init: these routines determine
 * the colormap to be used.  The rest of the module only assumes that the
 * colormap is orthogonal.
 *
 *  * select_ncolors decides how to divvy up the available colors
 *    among the components.
 *  * output_value defines the set of representative values for a component.
 *  * largest_input_value defines the mapping from input values to
 *    representative values for a component.
 * Note that the latter two routines may impose different policies for
 * different components, though this is not currently done.
 */


LOCAL int
select_ncolors (decompress_info_ptr cinfo, int Ncolors[])
/* Determine allocation of desired colors to components, */
/* and fill in Ncolors[] array to indicate choice. */
/* Return value is total number of colors (product of Ncolors[] values). */
{
  int nc = cinfo->color_out_comps; /* number of color components */
  int max_colors = cinfo->desired_number_of_colors;
  int total_colors, iroot, i;
  long temp;
  boolean changed;

  /* We can allocate at least the nc'th root of max_colors per component. */
  /* Compute floor(nc'th root of max_colors). */
  iroot = 1;
  do {
    iroot++;
    temp = iroot;		/* set temp = iroot ** nc */
    for (i = 1; i < nc; i++)
      temp *= iroot;
  } while (temp <= (long) max_colors); /* repeat till iroot exceeds root */
  iroot--;			/* now iroot = floor(root) */

  /* Must have at least 2 color values per component */
  if (iroot < 2)
    ERREXIT1(cinfo->emethods, "Cannot quantize to fewer than %d colors",
	     (int) temp);

  if (cinfo->out_color_space == CS_RGB && nc == 3) {
    /* We provide a special policy for quantizing in RGB space.
     * If 256 colors are requested, we allocate 8 red, 8 green, 4 blue levels;
     * this corresponds to the common 3/3/2-bit scheme.  For other totals,
     * the counts are set so that the number of colors allocated to each
     * component are roughly in the proportion R 3, G 4, B 2.
     * For low color counts, it's easier to hardwire the optimal choices
     * than try to tweak the algorithm to generate them.
     */
    if (max_colors == 256) {
      Ncolors[0] = 8;  Ncolors[1] = 8;  Ncolors[2] = 4;
      return 256;
    }
    if (max_colors < 12) {
      /* Fixed mapping for 8 colors */
      Ncolors[0] = Ncolors[1] = Ncolors[2] = 2;
    } else if (max_colors < 18) {
      /* Fixed mapping for 12 colors */
      Ncolors[0] = 2;  Ncolors[1] = 3;  Ncolors[2] = 2;
    } else if (max_colors < 24) {
      /* Fixed mapping for 18 colors */
      Ncolors[0] = 3;  Ncolors[1] = 3;  Ncolors[2] = 2;
    } else if (max_colors < 27) {
      /* Fixed mapping for 24 colors */
      Ncolors[0] = 3;  Ncolors[1] = 4;  Ncolors[2] = 2;
    } else if (max_colors < 36) {
      /* Fixed mapping for 27 colors */
      Ncolors[0] = 3;  Ncolors[1] = 3;  Ncolors[2] = 3;
    } else {
      /* these weights are readily derived with a little algebra */
      Ncolors[0] = (iroot * 266) >> 8; /* R weight is 1.0400 */
      Ncolors[1] = (iroot * 355) >> 8; /* G weight is 1.3867 */
      Ncolors[2] = (iroot * 177) >> 8; /* B weight is 0.6934 */
    }
    total_colors = Ncolors[0] * Ncolors[1] * Ncolors[2];
    /* The above computation produces "floor" values, so we may be able to
     * increment the count for one or more components without exceeding
     * max_colors.  We try in the order B, G, R.
     */
    do {
      changed = FALSE;
      for (i = 2; i >= 0; i--) {
	/* calculate new total_colors if Ncolors[i] is incremented */
	temp = total_colors / Ncolors[i];
	temp *= Ncolors[i]+1;	/* done in long arith to avoid oflo */
	if (temp <= (long) max_colors) {
	  Ncolors[i]++;		/* OK, apply the increment */
	  total_colors = (int) temp;
	  changed = TRUE;
	}
      }
    } while (changed);		/* loop until no increment is possible */
  } else {
    /* For any colorspace besides RGB, treat all the components equally. */

    /* Initialize to iroot color values for each component */
    total_colors = 1;
    for (i = 0; i < nc; i++) {
      Ncolors[i] = iroot;
      total_colors *= iroot;
    }
    /* We may be able to increment the count for one or more components without
     * exceeding max_colors, though we know not all can be incremented.
     */
    for (i = 0; i < nc; i++) {
      /* calculate new total_colors if Ncolors[i] is incremented */
      temp = total_colors / Ncolors[i];
      temp *= Ncolors[i]+1;	/* done in long arith to avoid oflo */
      if (temp > (long) max_colors)
	break;			/* won't fit, done */
      Ncolors[i]++;		/* OK, apply the increment */
      total_colors = (int) temp;
    }
  }

  return total_colors;
}


LOCAL int
output_value (decompress_info_ptr cinfo, int ci, int j, int maxj)
/* Return j'th output value, where j will range from 0 to maxj */
/* The output values must fall in 0..MAXJSAMPLE in increasing order */
{
  /* We always provide values 0 and MAXJSAMPLE for each component;
   * any additional values are equally spaced between these limits.
   * (Forcing the upper and lower values to the limits ensures that
   * dithering can't produce a color outside the selected gamut.)
   */
  return (int) (((INT32) j * MAXJSAMPLE + maxj/2) / maxj);
}


LOCAL int
largest_input_value (decompress_info_ptr cinfo, int ci, int j, int maxj)
/* Return largest input value that should map to j'th output value */
/* Must have largest(j=0) >= 0, and largest(j=maxj) >= MAXJSAMPLE */
{
  /* Breakpoints are halfway between values returned by output_value */
  return (int) (((INT32) (2*j + 1) * MAXJSAMPLE + maxj) / (2*maxj));
}


/*
 * Initialize for one-pass color quantization.
 */

METHODDEF void
color_quant_init (decompress_info_ptr cinfo)
{
  int total_colors;		/* Number of distinct output colors */
  int Ncolors[MAX_COMPONENTS];	/* # of values alloced to each component */
  int i,j,k, nci, blksize, blkdist, ptr, val;

  /* Make sure my internal arrays won't overflow */
  if (cinfo->num_components > MAX_COMPONENTS ||
      cinfo->color_out_comps > MAX_COMPONENTS)
    ERREXIT1(cinfo->emethods, "Cannot quantize more than %d color components",
	     MAX_COMPONENTS);
  /* Make sure colormap indexes can be represented by JSAMPLEs */
  if (cinfo->desired_number_of_colors > (MAXJSAMPLE+1))
    ERREXIT1(cinfo->emethods, "Cannot request more than %d quantized colors",
	     MAXJSAMPLE+1);

  /* Select number of colors for each component */
  total_colors = select_ncolors(cinfo, Ncolors);

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
      val = output_value(cinfo, i, j, nci-1);
      /* Fill in all colormap entries that have this value of this component */
      for (ptr = j * blksize; ptr < total_colors; ptr += blkdist) {
	/* fill in blksize entries beginning at ptr */
	for (k = 0; k < blksize; k++)
	  colormap[i][ptr+k] = (JSAMPLE) val;
      }
    }
    blkdist = blksize;		/* blksize of this color is blkdist of next */

    /* fill in colorindex entries for i'th color component */
    /* in loop, val = index of current output value, */
    /* and k = largest j that maps to current val */
    val = 0;
    k = largest_input_value(cinfo, i, 0, nci-1);
    for (j = 0; j <= MAXJSAMPLE; j++) {
      while (j > k)		/* advance val if past boundary */
	k = largest_input_value(cinfo, i, ++val, nci-1);
      /* premultiply so that no multiplication needed in main processing */
      colorindex[i][j] = (JSAMPLE) (val * blksize);
    }
  }

  /* Pass the colormap to the output module. */
  /* NB: the output module may continue to use the colormap until shutdown. */
  cinfo->colormap = colormap;
  cinfo->actual_number_of_colors = total_colors;
  (*cinfo->methods->put_color_map) (cinfo, total_colors, colormap);

  /* Allocate workspace to hold one row of color-converted data */
  input_buffer = (*cinfo->emethods->alloc_small_sarray)
			(cinfo->image_width, (long) cinfo->color_out_comps);

  /* Allocate Floyd-Steinberg workspace if necessary */
  if (cinfo->use_dithering) {
    size_t arraysize = (size_t) ((cinfo->image_width + 2L) * SIZEOF(FSERROR));

    for (i = 0; i < cinfo->color_out_comps; i++) {
      fserrors[i] = (FSERRPTR) (*cinfo->emethods->alloc_medium) (arraysize);
      /* Initialize the propagated errors to zero. */
      jzero_far((void FAR *) fserrors[i], arraysize);
    }
    on_odd_row = FALSE;
  }
}


/*
 * Subroutines for color conversion methods.
 */

LOCAL void
do_color_conversion (decompress_info_ptr cinfo, JSAMPIMAGE input_data, int row)
/* Convert the indicated row of the input data into output colorspace */
/* in input_buffer.  This requires a little trickery since color_convert */
/* expects to deal with 3-D arrays; fortunately we can fake it out */
/* at fairly low cost. */
{
  short ci;
  JSAMPARRAY input_hack[MAX_COMPONENTS];
  JSAMPARRAY output_hack[MAX_COMPONENTS];

  /* create JSAMPIMAGE pointing at specified row of input_data */
  for (ci = 0; ci < cinfo->num_components; ci++)
    input_hack[ci] = input_data[ci] + row;
  /* Create JSAMPIMAGE pointing at input_buffer */
  for (ci = 0; ci < cinfo->color_out_comps; ci++)
    output_hack[ci] = &(input_buffer[ci]);

  (*cinfo->methods->color_convert) (cinfo, 1, cinfo->image_width,
				    input_hack, output_hack);
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
  register JSAMPROW ptrout;
  register long col;
  int row;
  long width = cinfo->image_width;
  register int nc = cinfo->color_out_comps;  

  for (row = 0; row < num_rows; row++) {
    do_color_conversion(cinfo, input_data, row);
    ptrout = output_data[row];
    for (col = 0; col < width; col++) {
      pixcode = 0;
      for (ci = 0; ci < nc; ci++) {
	pixcode += GETJSAMPLE(colorindex[ci]
			      [GETJSAMPLE(input_buffer[ci][col])]);
      }
      *ptrout++ = (JSAMPLE) pixcode;
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
  int row;
  JSAMPROW colorindex0 = colorindex[0];
  JSAMPROW colorindex1 = colorindex[1];
  JSAMPROW colorindex2 = colorindex[2];
  long width = cinfo->image_width;

  for (row = 0; row < num_rows; row++) {
    do_color_conversion(cinfo, input_data, row);
    ptr0 = input_buffer[0];
    ptr1 = input_buffer[1];
    ptr2 = input_buffer[2];
    ptrout = output_data[row];
    for (col = width; col > 0; col--) {
      pixcode  = GETJSAMPLE(colorindex0[GETJSAMPLE(*ptr0++)]);
      pixcode += GETJSAMPLE(colorindex1[GETJSAMPLE(*ptr1++)]);
      pixcode += GETJSAMPLE(colorindex2[GETJSAMPLE(*ptr2++)]);
      *ptrout++ = (JSAMPLE) pixcode;
    }
  }
}


METHODDEF void
color_quantize_dither (decompress_info_ptr cinfo, int num_rows,
		       JSAMPIMAGE input_data, JSAMPARRAY output_data)
/* General case, with Floyd-Steinberg dithering */
{
  register LOCFSERROR cur;	/* current error or pixel value */
  LOCFSERROR belowerr;		/* error for pixel below cur */
  LOCFSERROR bpreverr;		/* error for below/prev col */
  LOCFSERROR bnexterr;		/* error for below/next col */
  LOCFSERROR delta;
  register FSERRPTR errorptr;	/* => fserrors[] at column before current */
  register JSAMPROW input_ptr;
  register JSAMPROW output_ptr;
  JSAMPROW colorindex_ci;
  JSAMPROW colormap_ci;
  int pixcode;
  int dir;			/* 1 for left-to-right, -1 for right-to-left */
  int ci;
  int nc = cinfo->color_out_comps;
  int row;
  long col_counter;
  long width = cinfo->image_width;
  JSAMPLE *range_limit = cinfo->sample_range_limit;
  SHIFT_TEMPS

  for (row = 0; row < num_rows; row++) {
    do_color_conversion(cinfo, input_data, row);
    /* Initialize output values to 0 so can process components separately */
    jzero_far((void FAR *) output_data[row],
	      (size_t) (width * SIZEOF(JSAMPLE)));
    for (ci = 0; ci < nc; ci++) {
      input_ptr = input_buffer[ci];
      output_ptr = output_data[row];
      if (on_odd_row) {
	/* work right to left in this row */
	input_ptr += width - 1;	/* so point to rightmost pixel */
	output_ptr += width - 1;
	dir = -1;
	errorptr = fserrors[ci] + (width+1); /* point to entry after last column */
      } else {
	/* work left to right in this row */
	dir = 1;
	errorptr = fserrors[ci]; /* point to entry before first real column */
      }
      colorindex_ci = colorindex[ci];
      colormap_ci = colormap[ci];
      /* Preset error values: no error propagated to first pixel from left */
      cur = 0;
      /* and no error propagated to row below yet */
      belowerr = bpreverr = 0;

      for (col_counter = width; col_counter > 0; col_counter--) {
	/* cur holds the error propagated from the previous pixel on the
	 * current line.  Add the error propagated from the previous line
	 * to form the complete error correction term for this pixel, and
	 * round the error term (which is expressed * 16) to an integer.
	 * RIGHT_SHIFT rounds towards minus infinity, so adding 8 is correct
	 * for either sign of the error value.
	 * Note: errorptr points to *previous* column's array entry.
	 */
	cur = RIGHT_SHIFT(cur + errorptr[dir] + 8, 4);
	/* Form pixel value + error, and range-limit to 0..MAXJSAMPLE.
	 * The maximum error is +- MAXJSAMPLE; this sets the required size
	 * of the range_limit array.
	 */
	cur += GETJSAMPLE(*input_ptr);
	cur = GETJSAMPLE(range_limit[cur]);
	/* Select output value, accumulate into output code for this pixel */
	pixcode = GETJSAMPLE(colorindex_ci[cur]);
	*output_ptr += (JSAMPLE) pixcode;
	/* Compute actual representation error at this pixel */
	/* Note: we can do this even though we don't have the final */
	/* pixel code, because the colormap is orthogonal. */
	cur -= GETJSAMPLE(colormap_ci[pixcode]);
	/* Compute error fractions to be propagated to adjacent pixels.
	 * Add these into the running sums, and simultaneously shift the
	 * next-line error sums left by 1 column.
	 */
	bnexterr = cur;
	delta = cur * 2;
	cur += delta;		/* form error * 3 */
	errorptr[0] = (FSERROR) (bpreverr + cur);
	cur += delta;		/* form error * 5 */
	bpreverr = belowerr + cur;
	belowerr = bnexterr;
	cur += delta;		/* form error * 7 */
	/* At this point cur contains the 7/16 error value to be propagated
	 * to the next pixel on the current line, and all the errors for the
	 * next line have been shifted over. We are therefore ready to move on.
	 */
	input_ptr += dir;	/* advance input ptr to next column */
	output_ptr += dir;	/* advance output ptr to next column */
	errorptr += dir;	/* advance errorptr to current column */
      }
      /* Post-loop cleanup: we must unload the final error value into the
       * final fserrors[] entry.  Note we need not unload belowerr because
       * it is for the dummy column before or after the actual array.
       */
      errorptr[0] = (FSERROR) bpreverr; /* unload prev err into array */
    }
    on_odd_row = (on_odd_row ? FALSE : TRUE);
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
color_quant_term (decompress_info_ptr cinfo)
{
  /* no work (we let free_all release the workspace) */
  /* Note that we *mustn't* free the colormap before free_all, */
  /* since output module may use it! */
}


/*
 * Prescan some rows of pixels.
 * Not used in one-pass case.
 */

METHODDEF void
color_quant_prescan (decompress_info_ptr cinfo, int num_rows,
		     JSAMPIMAGE image_data, JSAMPARRAY workspace)
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

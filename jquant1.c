/*
 * jquant1.c
 *
 * Copyright (C) 1991-1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains 1-pass color quantization (color mapping) routines.
 * These routines provide mapping to a fixed color map using equally spaced
 * color values.  Optional Floyd-Steinberg or ordered dithering is available.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

#ifdef QUANT_1PASS_SUPPORTED


/*
 * The main purpose of 1-pass quantization is to provide a fast, if not very
 * high quality, colormapped output capability.  A 2-pass quantizer usually
 * gives better visual quality; however, for quantized grayscale output this
 * quantizer is perfectly adequate.  Dithering is highly recommended with this
 * quantizer, though you can turn it off if you really want to.
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
 *
 * If gamma correction has been applied in color conversion, it might be wise
 * to adjust the color grid spacing so that the representative colors are
 * equidistant in linear space.  At this writing, gamma correction is not
 * implemented by jdcolor, so nothing is done here.
 */


/* Declarations for ordered dithering.
 *
 * We use a standard 4x4 ordered dither array.  The basic concept of ordered
 * dithering is described in many references, for instance Dale Schumacher's
 * chapter II.2 of Graphics Gems II (James Arvo, ed. Academic Press, 1991).
 * In place of Schumacher's comparisons against a "threshold" value, we add a
 * "dither" value to the input pixel and then round the result to the nearest
 * output value.  The dither value is equivalent to (0.5 - threshold) times
 * the distance between output values.  For ordered dithering, we assume that
 * the output colors are equally spaced; if not, results will probably be
 * worse, since the dither may be too much or too little at a given point.
 *
 * The normal calculation would be to form pixel value + dither, range-limit
 * this to 0..MAXJSAMPLE, and then index into the colorindex table as usual.
 * We can skip the separate range-limiting step by extending the colorindex
 * table in both directions.
 */

#define ODITHER_SIZE  4		/* dimension of dither matrix */
#define ODITHER_CELLS (4*4)	/* number of cells in dither matrix */
#define ODITHER_MASK  3		/* mask for wrapping around dither counters */

typedef int ODITHER_MATRIX[ODITHER_SIZE][ODITHER_SIZE];


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
 * segment to hold the error array; so it is allocated with alloc_large.
 */

#if BITS_IN_JSAMPLE == 8
typedef INT16 FSERROR;		/* 16 bits should be enough */
typedef int LOCFSERROR;		/* use 'int' for calculation temps */
#else
typedef INT32 FSERROR;		/* may need more than 16 bits */
typedef INT32 LOCFSERROR;	/* be sure calculation temps are big enough */
#endif

typedef FSERROR FAR *FSERRPTR;	/* pointer to error array (in FAR storage!) */


/* Private subobject */

#define MAX_Q_COMPS 4		/* max components I can handle */

typedef struct {
  struct jpeg_color_quantizer pub; /* public fields */

  JSAMPARRAY colorindex;	/* Precomputed mapping for speed */
  /* colorindex[i][j] = index of color closest to pixel value j in component i,
   * premultiplied as described above.  Since colormap indexes must fit into
   * JSAMPLEs, the entries of this array will too.
   */

  /* Variables for ordered dithering */
  int row_index;		/* cur row's vertical index in dither matrix */
  ODITHER_MATRIX *odither;	/* one dither array per component */

  /* Variables for Floyd-Steinberg dithering */
  FSERRPTR fserrors[MAX_Q_COMPS]; /* accumulated errors */
  boolean on_odd_row;		/* flag to remember which row we are on */
} my_cquantizer;

typedef my_cquantizer * my_cquantize_ptr;


/*
 * Policy-making subroutines for create_colormap: these routines determine
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
select_ncolors (j_decompress_ptr cinfo, int Ncolors[])
/* Determine allocation of desired colors to components, */
/* and fill in Ncolors[] array to indicate choice. */
/* Return value is total number of colors (product of Ncolors[] values). */
{
  int nc = cinfo->out_color_components; /* number of color components */
  int max_colors = cinfo->desired_number_of_colors;
  int total_colors, iroot, i, j;
  long temp;
  static const int RGB_order[3] = { RGB_GREEN, RGB_RED, RGB_BLUE };

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
    ERREXIT1(cinfo, JERR_QUANT_FEW_COLORS, (int) temp);

  /* Initialize to iroot color values for each component */
  total_colors = 1;
  for (i = 0; i < nc; i++) {
    Ncolors[i] = iroot;
    total_colors *= iroot;
  }
  /* We may be able to increment the count for one or more components without
   * exceeding max_colors, though we know not all can be incremented.
   * In RGB colorspace, try to increment G first, then R, then B.
   */
  for (i = 0; i < nc; i++) {
    j = (cinfo->out_color_space == JCS_RGB ? RGB_order[i] : i);
    /* calculate new total_colors if Ncolors[j] is incremented */
    temp = total_colors / Ncolors[j];
    temp *= Ncolors[j]+1;	/* done in long arith to avoid oflo */
    if (temp > (long) max_colors)
      break;			/* won't fit, done */
    Ncolors[j]++;		/* OK, apply the increment */
    total_colors = (int) temp;
  }

  return total_colors;
}


LOCAL int
output_value (j_decompress_ptr cinfo, int ci, int j, int maxj)
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
largest_input_value (j_decompress_ptr cinfo, int ci, int j, int maxj)
/* Return largest input value that should map to j'th output value */
/* Must have largest(j=0) >= 0, and largest(j=maxj) >= MAXJSAMPLE */
{
  /* Breakpoints are halfway between values returned by output_value */
  return (int) (((INT32) (2*j + 1) * MAXJSAMPLE + maxj) / (2*maxj));
}


/*
 * Create the colormap and color index table.
 * Also creates the ordered-dither tables, if required.
 */

LOCAL void
create_colormap (j_decompress_ptr cinfo)
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  JSAMPARRAY colormap;		/* Created colormap */
  JSAMPROW indexptr;
  int total_colors;		/* Number of distinct output colors */
  int Ncolors[MAX_Q_COMPS];	/* # of values alloced to each component */
  ODITHER_MATRIX *odither;
  int i,j,k, nci, blksize, blkdist, ptr, val, pad;

  /* Select number of colors for each component */
  total_colors = select_ncolors(cinfo, Ncolors);

  /* Report selected color counts */
  if (cinfo->out_color_components == 3)
    TRACEMS4(cinfo, 1, JTRC_QUANT_3_NCOLORS,
	     total_colors, Ncolors[0], Ncolors[1], Ncolors[2]);
  else
    TRACEMS1(cinfo, 1, JTRC_QUANT_NCOLORS, total_colors);

  /* For ordered dither, we pad the color index tables by MAXJSAMPLE in
   * each direction (input index values can be -MAXJSAMPLE .. 2*MAXJSAMPLE).
   * This is not necessary in the other dithering modes.
   */
  pad = (cinfo->dither_mode == JDITHER_ORDERED) ? MAXJSAMPLE*2 : 0;

  /* Allocate and fill in the colormap and color index. */
  /* The colors are ordered in the map in standard row-major order, */
  /* i.e. rightmost (highest-indexed) color changes most rapidly. */

  colormap = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE,
     (JDIMENSION) total_colors, (JDIMENSION) cinfo->out_color_components);
  cquantize->colorindex = (*cinfo->mem->alloc_sarray)
    ((j_common_ptr) cinfo, JPOOL_IMAGE,
     (JDIMENSION) (MAXJSAMPLE+1 + pad),
     (JDIMENSION) cinfo->out_color_components);

  /* blksize is number of adjacent repeated entries for a component */
  /* blkdist is distance between groups of identical entries for a component */
  blkdist = total_colors;

  for (i = 0; i < cinfo->out_color_components; i++) {
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

    /* adjust colorindex pointers to provide padding at negative indexes. */
    if (pad)
      cquantize->colorindex[i] += MAXJSAMPLE;

    /* fill in colorindex entries for i'th color component */
    /* in loop, val = index of current output value, */
    /* and k = largest j that maps to current val */
    indexptr = cquantize->colorindex[i];
    val = 0;
    k = largest_input_value(cinfo, i, 0, nci-1);
    for (j = 0; j <= MAXJSAMPLE; j++) {
      while (j > k)		/* advance val if past boundary */
	k = largest_input_value(cinfo, i, ++val, nci-1);
      /* premultiply so that no multiplication needed in main processing */
      indexptr[j] = (JSAMPLE) (val * blksize);
    }
    /* Pad at both ends if necessary */
    if (pad)
      for (j = 1; j <= MAXJSAMPLE; j++) {
	indexptr[-j] = indexptr[0];
	indexptr[MAXJSAMPLE+j] = indexptr[MAXJSAMPLE];
      }
  }

  /* Make the colormap available to the application. */
  cinfo->colormap = colormap;
  cinfo->actual_number_of_colors = total_colors;

  if (cinfo->dither_mode == JDITHER_ORDERED) {
    /* Allocate and fill in the ordered-dither tables. */
    odither = (ODITHER_MATRIX *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
			cinfo->out_color_components * SIZEOF(ODITHER_MATRIX));
    cquantize->odither = odither;
    for (i = 0; i < cinfo->out_color_components; i++) {
      nci = Ncolors[i];		/* # of distinct values for this color */
      /* The inter-value distance for this color is MAXJSAMPLE/(nci-1).
       * Hence the dither value for the matrix cell with fill order j
       * (j=1..N) should be (N+1-2*j)/(2*(N+1)) * MAXJSAMPLE/(nci-1).
       */
      val = 2 * (ODITHER_CELLS + 1) * (nci - 1); /* denominator */
      /* Macro is coded to ensure round towards zero despite C's
       * lack of consistency in integer division...
       */
#define ODITHER_DIV(num,den)  ((num)<0 ? -((-(num))/(den)) : (num)/(den))
#define ODITHER_VAL(j)  ODITHER_DIV((ODITHER_CELLS+1-2*j)*MAXJSAMPLE, val)
      /* Traditional fill order for 4x4 dither; see Schumacher's figure 4. */
      odither[0][0][0] = ODITHER_VAL(1);
      odither[0][0][1] = ODITHER_VAL(9);
      odither[0][0][2] = ODITHER_VAL(3);
      odither[0][0][3] = ODITHER_VAL(11);
      odither[0][1][0] = ODITHER_VAL(13);
      odither[0][1][1] = ODITHER_VAL(5);
      odither[0][1][2] = ODITHER_VAL(15);
      odither[0][1][3] = ODITHER_VAL(7);
      odither[0][2][0] = ODITHER_VAL(4);
      odither[0][2][1] = ODITHER_VAL(12);
      odither[0][2][2] = ODITHER_VAL(2);
      odither[0][2][3] = ODITHER_VAL(10);
      odither[0][3][0] = ODITHER_VAL(16);
      odither[0][3][1] = ODITHER_VAL(8);
      odither[0][3][2] = ODITHER_VAL(14);
      odither[0][3][3] = ODITHER_VAL(6);
      odither++;		/* advance to next matrix */
    }
  }
}


/*
 * Map some rows of pixels to the output colormapped representation.
 */

METHODDEF void
color_quantize (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
		JSAMPARRAY output_buf, int num_rows)
/* General case, no dithering */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  JSAMPARRAY colorindex = cquantize->colorindex;
  register int pixcode, ci;
  register JSAMPROW ptrin, ptrout;
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;
  register int nc = cinfo->out_color_components;

  for (row = 0; row < num_rows; row++) {
    ptrin = input_buf[row];
    ptrout = output_buf[row];
    for (col = width; col > 0; col--) {
      pixcode = 0;
      for (ci = 0; ci < nc; ci++) {
	pixcode += GETJSAMPLE(colorindex[ci][GETJSAMPLE(*ptrin++)]);
      }
      *ptrout++ = (JSAMPLE) pixcode;
    }
  }
}


METHODDEF void
color_quantize3 (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
		 JSAMPARRAY output_buf, int num_rows)
/* Fast path for out_color_components==3, no dithering */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  register int pixcode;
  register JSAMPROW ptrin, ptrout;
  JSAMPROW colorindex0 = cquantize->colorindex[0];
  JSAMPROW colorindex1 = cquantize->colorindex[1];
  JSAMPROW colorindex2 = cquantize->colorindex[2];
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;

  for (row = 0; row < num_rows; row++) {
    ptrin = input_buf[row];
    ptrout = output_buf[row];
    for (col = width; col > 0; col--) {
      pixcode  = GETJSAMPLE(colorindex0[GETJSAMPLE(*ptrin++)]);
      pixcode += GETJSAMPLE(colorindex1[GETJSAMPLE(*ptrin++)]);
      pixcode += GETJSAMPLE(colorindex2[GETJSAMPLE(*ptrin++)]);
      *ptrout++ = (JSAMPLE) pixcode;
    }
  }
}


METHODDEF void
quantize_ord_dither (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
		     JSAMPARRAY output_buf, int num_rows)
/* General case, with ordered dithering */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  register JSAMPROW input_ptr;
  register JSAMPROW output_ptr;
  JSAMPROW colorindex_ci;
  int * dither;			/* points to active row of dither matrix */
  int row_index, col_index;	/* current indexes into dither matrix */
  int nc = cinfo->out_color_components;
  int ci;
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;

  for (row = 0; row < num_rows; row++) {
    /* Initialize output values to 0 so can process components separately */
    jzero_far((void FAR *) output_buf[row],
	      (size_t) (width * SIZEOF(JSAMPLE)));
    row_index = cquantize->row_index;
    for (ci = 0; ci < nc; ci++) {
      input_ptr = input_buf[row] + ci;
      output_ptr = output_buf[row];
      colorindex_ci = cquantize->colorindex[ci];
      dither = cquantize->odither[ci][row_index];
      col_index = 0;

      for (col = width; col > 0; col--) {
	/* Form pixel value + dither, range-limit to 0..MAXJSAMPLE,
	 * select output value, accumulate into output code for this pixel.
	 * Range-limiting need not be done explicitly, as we have extended
	 * the colorindex table to produce the right answers for out-of-range
	 * inputs.  The maximum dither is +- MAXJSAMPLE; this sets the
	 * required amount of padding.
	 */
	*output_ptr += colorindex_ci[GETJSAMPLE(*input_ptr)+dither[col_index]];
	input_ptr += nc;
	output_ptr++;
	col_index = (col_index + 1) & ODITHER_MASK;
      }
    }
    /* Advance row index for next row */
    row_index = (row_index + 1) & ODITHER_MASK;
    cquantize->row_index = row_index;
  }
}


METHODDEF void
quantize3_ord_dither (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
		      JSAMPARRAY output_buf, int num_rows)
/* Fast path for out_color_components==3, with ordered dithering */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
  register int pixcode;
  register JSAMPROW input_ptr;
  register JSAMPROW output_ptr;
  JSAMPROW colorindex0 = cquantize->colorindex[0];
  JSAMPROW colorindex1 = cquantize->colorindex[1];
  JSAMPROW colorindex2 = cquantize->colorindex[2];
  int * dither0;		/* points to active row of dither matrix */
  int * dither1;
  int * dither2;
  int row_index, col_index;	/* current indexes into dither matrix */
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;

  for (row = 0; row < num_rows; row++) {
    row_index = cquantize->row_index;
    input_ptr = input_buf[row];
    output_ptr = output_buf[row];
    dither0 = cquantize->odither[0][row_index];
    dither1 = cquantize->odither[1][row_index];
    dither2 = cquantize->odither[2][row_index];
    col_index = 0;

    for (col = width; col > 0; col--) {
      pixcode  = GETJSAMPLE(colorindex0[GETJSAMPLE(*input_ptr++) +
					dither0[col_index]]);
      pixcode += GETJSAMPLE(colorindex1[GETJSAMPLE(*input_ptr++) +
					dither1[col_index]]);
      pixcode += GETJSAMPLE(colorindex2[GETJSAMPLE(*input_ptr++) +
					dither2[col_index]]);
      *output_ptr++ = (JSAMPLE) pixcode;
      col_index = (col_index + 1) & ODITHER_MASK;
    }
    row_index = (row_index + 1) & ODITHER_MASK;
    cquantize->row_index = row_index;
  }
}


METHODDEF void
quantize_fs_dither (j_decompress_ptr cinfo, JSAMPARRAY input_buf,
		    JSAMPARRAY output_buf, int num_rows)
/* General case, with Floyd-Steinberg dithering */
{
  my_cquantize_ptr cquantize = (my_cquantize_ptr) cinfo->cquantize;
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
  int nc = cinfo->out_color_components;
  int dir;			/* 1 for left-to-right, -1 for right-to-left */
  int dirnc;			/* dir * nc */
  int ci;
  int row;
  JDIMENSION col;
  JDIMENSION width = cinfo->output_width;
  JSAMPLE *range_limit = cinfo->sample_range_limit;
  SHIFT_TEMPS

  for (row = 0; row < num_rows; row++) {
    /* Initialize output values to 0 so can process components separately */
    jzero_far((void FAR *) output_buf[row],
	      (size_t) (width * SIZEOF(JSAMPLE)));
    for (ci = 0; ci < nc; ci++) {
      input_ptr = input_buf[row] + ci;
      output_ptr = output_buf[row];
      if (cquantize->on_odd_row) {
	/* work right to left in this row */
	input_ptr += (width-1) * nc; /* so point to rightmost pixel */
	output_ptr += width-1;
	dir = -1;
	dirnc = -nc;
	errorptr = cquantize->fserrors[ci] + (width+1); /* => entry after last column */
      } else {
	/* work left to right in this row */
	dir = 1;
	dirnc = nc;
	errorptr = cquantize->fserrors[ci]; /* => entry before first column */
      }
      colorindex_ci = cquantize->colorindex[ci];
      colormap_ci = cinfo->colormap[ci];
      /* Preset error values: no error propagated to first pixel from left */
      cur = 0;
      /* and no error propagated to row below yet */
      belowerr = bpreverr = 0;

      for (col = width; col > 0; col--) {
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
	input_ptr += dirnc;	/* advance input ptr to next column */
	output_ptr += dir;	/* advance output ptr to next column */
	errorptr += dir;	/* advance errorptr to current column */
      }
      /* Post-loop cleanup: we must unload the final error value into the
       * final fserrors[] entry.  Note we need not unload belowerr because
       * it is for the dummy column before or after the actual array.
       */
      errorptr[0] = (FSERROR) bpreverr; /* unload prev err into array */
    }
    cquantize->on_odd_row = (cquantize->on_odd_row ? FALSE : TRUE);
  }
}


/*
 * Initialize for one-pass color quantization.
 */

METHODDEF void
start_pass_1_quant (j_decompress_ptr cinfo, boolean is_pre_scan)
{
  /* no work in 1-pass case */
}


/*
 * Finish up at the end of the pass.
 */

METHODDEF void
finish_pass_1_quant (j_decompress_ptr cinfo)
{
  /* no work in 1-pass case */
}


/*
 * Module initialization routine for 1-pass color quantization.
 */

GLOBAL void
jinit_1pass_quantizer (j_decompress_ptr cinfo)
{
  my_cquantize_ptr cquantize;
  size_t arraysize;
  int i;

  cquantize = (my_cquantize_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				SIZEOF(my_cquantizer));
  cinfo->cquantize = (struct jpeg_color_quantizer *) cquantize;
  cquantize->pub.start_pass = start_pass_1_quant;
  cquantize->pub.finish_pass = finish_pass_1_quant;

  /* Make sure my internal arrays won't overflow */
  if (cinfo->out_color_components > MAX_Q_COMPS)
    ERREXIT1(cinfo, JERR_QUANT_COMPONENTS, MAX_Q_COMPS);
  /* Make sure colormap indexes can be represented by JSAMPLEs */
  if (cinfo->desired_number_of_colors > (MAXJSAMPLE+1))
    ERREXIT1(cinfo, JERR_QUANT_MANY_COLORS, MAXJSAMPLE+1);

  /* Initialize for desired dithering mode. */
  switch (cinfo->dither_mode) {
  case JDITHER_NONE:
    if (cinfo->out_color_components == 3)
      cquantize->pub.color_quantize = color_quantize3;
    else
      cquantize->pub.color_quantize = color_quantize;
    break;
  case JDITHER_ORDERED:
    if (cinfo->out_color_components == 3)
      cquantize->pub.color_quantize = quantize3_ord_dither;
    else
      cquantize->pub.color_quantize = quantize_ord_dither;
    cquantize->row_index = 0;	/* initialize state for ordered dither */
    break;
  case JDITHER_FS:
    cquantize->pub.color_quantize = quantize_fs_dither;
    cquantize->on_odd_row = FALSE; /* initialize state for F-S dither */
    /* Allocate Floyd-Steinberg workspace if necessary. */
    /* We do this now since it is FAR storage and may affect the memory */
    /* manager's space calculations. */
    arraysize = (size_t) ((cinfo->output_width + 2) * SIZEOF(FSERROR));
    for (i = 0; i < cinfo->out_color_components; i++) {
      cquantize->fserrors[i] = (FSERRPTR) (*cinfo->mem->alloc_large)
	((j_common_ptr) cinfo, JPOOL_IMAGE, arraysize);
      /* Initialize the propagated errors to zero. */
      jzero_far((void FAR *) cquantize->fserrors[i], arraysize);
    }
    break;
  default:
    ERREXIT(cinfo, JERR_NOT_COMPILED);
    break;
  }

  /* Create the colormap. */
  create_colormap(cinfo);
}

#endif /* QUANT_1PASS_SUPPORTED */

/*
 * jdsample.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains upsampling routines.
 * These routines are invoked via the upsample and
 * upsample_init/term methods.
 *
 * An excellent reference for image resampling is
 *   Digital Image Warping, George Wolberg, 1990.
 *   Pub. by IEEE Computer Society Press, Los Alamitos, CA. ISBN 0-8186-8944-7.
 */

#include "jinclude.h"


/*
 * Initialize for upsampling a scan.
 */

METHODDEF void
upsample_init (decompress_info_ptr cinfo)
{
  /* no work for now */
}


/*
 * Upsample pixel values of a single component.
 * This version handles any integral sampling ratios.
 *
 * This is not used for typical JPEG files, so it need not be fast.
 * Nor, for that matter, is it particularly accurate: the algorithm is
 * simple replication of the input pixel onto the corresponding output
 * pixels.  The hi-falutin sampling literature refers to this as a
 * "box filter".  A box filter tends to introduce visible artifacts,
 * so if you are actually going to use 3:1 or 4:1 sampling ratios
 * you would be well advised to improve this code.
 */

METHODDEF void
int_upsample (decompress_info_ptr cinfo, int which_component,
	      long input_cols, int input_rows,
	      long output_cols, int output_rows,
	      JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
	      JSAMPARRAY output_data)
{
  jpeg_component_info * compptr = cinfo->cur_comp_info[which_component];
  register JSAMPROW inptr, outptr;
  register JSAMPLE invalue;
  register short h_expand, h;
  short v_expand, v;
  int inrow, outrow;
  register long incol;

#ifdef DEBUG			/* for debugging pipeline controller */
  if (input_rows != compptr->v_samp_factor ||
      output_rows != cinfo->max_v_samp_factor ||
      (input_cols % compptr->h_samp_factor) != 0 ||
      (output_cols % cinfo->max_h_samp_factor) != 0 ||
      output_cols*compptr->h_samp_factor != input_cols*cinfo->max_h_samp_factor)
    ERREXIT(cinfo->emethods, "Bogus upsample parameters");
#endif

  h_expand = cinfo->max_h_samp_factor / compptr->h_samp_factor;
  v_expand = cinfo->max_v_samp_factor / compptr->v_samp_factor;

  outrow = 0;
  for (inrow = 0; inrow < input_rows; inrow++) {
    for (v = 0; v < v_expand; v++) {
      inptr = input_data[inrow];
      outptr = output_data[outrow++];
      for (incol = 0; incol < input_cols; incol++) {
	invalue = GETJSAMPLE(*inptr++);
	for (h = 0; h < h_expand; h++) {
	  *outptr++ = invalue;
	}
      }
    }
  }
}


/*
 * Upsample pixel values of a single component.
 * This version handles the common case of 2:1 horizontal and 1:1 vertical.
 *
 * The upsampling algorithm is linear interpolation between pixel centers,
 * also known as a "triangle filter".  This is a good compromise between
 * speed and visual quality.  The centers of the output pixels are 1/4 and 3/4
 * of the way between input pixel centers.
 */

METHODDEF void
h2v1_upsample (decompress_info_ptr cinfo, int which_component,
	       long input_cols, int input_rows,
	       long output_cols, int output_rows,
	       JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
	       JSAMPARRAY output_data)
{
  register JSAMPROW inptr, outptr;
  register int invalue;
  int inrow;
  register long colctr;

#ifdef DEBUG			/* for debugging pipeline controller */
  jpeg_component_info * compptr = cinfo->cur_comp_info[which_component];
  if (input_rows != compptr->v_samp_factor ||
      output_rows != cinfo->max_v_samp_factor ||
      (input_cols % compptr->h_samp_factor) != 0 ||
      (output_cols % cinfo->max_h_samp_factor) != 0 ||
      output_cols*compptr->h_samp_factor != input_cols*cinfo->max_h_samp_factor)
    ERREXIT(cinfo->emethods, "Bogus upsample parameters");
#endif

  for (inrow = 0; inrow < input_rows; inrow++) {
    inptr = input_data[inrow];
    outptr = output_data[inrow];
    /* Special case for first column */
    invalue = GETJSAMPLE(*inptr++);
    *outptr++ = (JSAMPLE) invalue;
    *outptr++ = (JSAMPLE) ((invalue * 3 + GETJSAMPLE(*inptr) + 2) >> 2);

    for (colctr = input_cols - 2; colctr > 0; colctr--) {
      /* General case: 3/4 * nearer pixel + 1/4 * further pixel */
      invalue = GETJSAMPLE(*inptr++) * 3;
      *outptr++ = (JSAMPLE) ((invalue + GETJSAMPLE(inptr[-2]) + 2) >> 2);
      *outptr++ = (JSAMPLE) ((invalue + GETJSAMPLE(*inptr) + 2) >> 2);
    }

    /* Special case for last column */
    invalue = GETJSAMPLE(*inptr);
    *outptr++ = (JSAMPLE) ((invalue * 3 + GETJSAMPLE(inptr[-1]) + 2) >> 2);
    *outptr++ = (JSAMPLE) invalue;
  }
}


/*
 * Upsample pixel values of a single component.
 * This version handles the common case of 2:1 horizontal and 2:1 vertical.
 *
 * The upsampling algorithm is linear interpolation between pixel centers,
 * also known as a "triangle filter".  This is a good compromise between
 * speed and visual quality.  The centers of the output pixels are 1/4 and 3/4
 * of the way between input pixel centers.
 */

METHODDEF void
h2v2_upsample (decompress_info_ptr cinfo, int which_component,
	       long input_cols, int input_rows,
	       long output_cols, int output_rows,
	       JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
	       JSAMPARRAY output_data)
{
  register JSAMPROW inptr0, inptr1, outptr;
#ifdef EIGHT_BIT_SAMPLES
  register int thiscolsum, lastcolsum, nextcolsum;
#else
  register INT32 thiscolsum, lastcolsum, nextcolsum;
#endif
  int inrow, outrow, v;
  register long colctr;

#ifdef DEBUG			/* for debugging pipeline controller */
  jpeg_component_info * compptr = cinfo->cur_comp_info[which_component];
  if (input_rows != compptr->v_samp_factor ||
      output_rows != cinfo->max_v_samp_factor ||
      (input_cols % compptr->h_samp_factor) != 0 ||
      (output_cols % cinfo->max_h_samp_factor) != 0 ||
      output_cols*compptr->h_samp_factor != input_cols*cinfo->max_h_samp_factor)
    ERREXIT(cinfo->emethods, "Bogus upsample parameters");
#endif

  outrow = 0;
  for (inrow = 0; inrow < input_rows; inrow++) {
    for (v = 0; v < 2; v++) {
      /* inptr0 points to nearest input row, inptr1 points to next nearest */
      inptr0 = input_data[inrow];
      if (v == 0) {		/* next nearest is row above */
	if (inrow == 0)
	  inptr1 = above[input_rows-1];
	else
	  inptr1 = input_data[inrow-1];
      } else {			/* next nearest is row below */
	if (inrow == input_rows-1)
	  inptr1 = below[0];
	else
	  inptr1 = input_data[inrow+1];
      }
      outptr = output_data[outrow++];

      /* Special case for first column */
      thiscolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
      nextcolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
      *outptr++ = (JSAMPLE) ((thiscolsum * 4 + 8) >> 4);
      *outptr++ = (JSAMPLE) ((thiscolsum * 3 + nextcolsum + 8) >> 4);
      lastcolsum = thiscolsum; thiscolsum = nextcolsum;

      for (colctr = input_cols - 2; colctr > 0; colctr--) {
	/* General case: 3/4 * nearer pixel + 1/4 * further pixel in each */
	/* dimension, thus 9/16, 3/16, 3/16, 1/16 overall */
	nextcolsum = GETJSAMPLE(*inptr0++) * 3 + GETJSAMPLE(*inptr1++);
	*outptr++ = (JSAMPLE) ((thiscolsum * 3 + lastcolsum + 8) >> 4);
	*outptr++ = (JSAMPLE) ((thiscolsum * 3 + nextcolsum + 8) >> 4);
	lastcolsum = thiscolsum; thiscolsum = nextcolsum;
      }

      /* Special case for last column */
      *outptr++ = (JSAMPLE) ((thiscolsum * 3 + lastcolsum + 8) >> 4);
      *outptr++ = (JSAMPLE) ((thiscolsum * 4 + 8) >> 4);
    }
  }
}


/*
 * Upsample pixel values of a single component.
 * This version handles the special case of a full-size component.
 */

METHODDEF void
fullsize_upsample (decompress_info_ptr cinfo, int which_component,
		   long input_cols, int input_rows,
		   long output_cols, int output_rows,
		   JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
		   JSAMPARRAY output_data)
{
#ifdef DEBUG			/* for debugging pipeline controller */
  if (input_cols != output_cols || input_rows != output_rows)
    ERREXIT(cinfo->emethods, "Pipeline controller messed up");
#endif

  jcopy_sample_rows(input_data, 0, output_data, 0, output_rows, output_cols);
}



/*
 * Clean up after a scan.
 */

METHODDEF void
upsample_term (decompress_info_ptr cinfo)
{
  /* no work for now */
}



/*
 * The method selection routine for upsampling.
 * Note that we must select a routine for each component.
 */

GLOBAL void
jselupsample (decompress_info_ptr cinfo)
{
  short ci;
  jpeg_component_info * compptr;

  if (cinfo->CCIR601_sampling)
    ERREXIT(cinfo->emethods, "CCIR601 upsampling not implemented yet");

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    if (compptr->h_samp_factor == cinfo->max_h_samp_factor &&
	compptr->v_samp_factor == cinfo->max_v_samp_factor)
      cinfo->methods->upsample[ci] = fullsize_upsample;
    else if (compptr->h_samp_factor * 2 == cinfo->max_h_samp_factor &&
	     compptr->v_samp_factor == cinfo->max_v_samp_factor)
      cinfo->methods->upsample[ci] = h2v1_upsample;
    else if (compptr->h_samp_factor * 2 == cinfo->max_h_samp_factor &&
	     compptr->v_samp_factor * 2 == cinfo->max_v_samp_factor)
      cinfo->methods->upsample[ci] = h2v2_upsample;
    else if ((cinfo->max_h_samp_factor % compptr->h_samp_factor) == 0 &&
	     (cinfo->max_v_samp_factor % compptr->v_samp_factor) == 0)
      cinfo->methods->upsample[ci] = int_upsample;
    else
      ERREXIT(cinfo->emethods, "Fractional upsampling not implemented yet");
  }

  cinfo->methods->upsample_init = upsample_init;
  cinfo->methods->upsample_term = upsample_term;
}

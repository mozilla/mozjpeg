/*
 * jcsample.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains downsampling routines.
 * These routines are invoked via the downsample and
 * downsample_init/term methods.
 *
 * An excellent reference for image resampling is
 *   Digital Image Warping, George Wolberg, 1990.
 *   Pub. by IEEE Computer Society Press, Los Alamitos, CA. ISBN 0-8186-8944-7.
 *
 * The downsampling algorithm used here is a simple average of the source
 * pixels covered by the output pixel.  The hi-falutin sampling literature
 * refers to this as a "box filter".  In general the characteristics of a box
 * filter are not very good, but for the specific cases we normally use (1:1
 * and 2:1 ratios) the box is equivalent to a "triangle filter" which is not
 * nearly so bad.  If you intend to use other sampling ratios, you'd be well
 * advised to improve this code.
 *
 * A simple input-smoothing capability is provided.  This is mainly intended
 * for cleaning up color-dithered GIF input files (if you find it inadequate,
 * we suggest using an external filtering program such as pnmconvol).  When
 * enabled, each input pixel P is replaced by a weighted sum of itself and its
 * eight neighbors.  P's weight is 1-8*SF and each neighbor's weight is SF,
 * where SF = (smoothing_factor / 1024).
 * Currently, smoothing is only supported for 2h2v sampling factors.
 */

#include "jinclude.h"


/*
 * Initialize for downsampling a scan.
 */

METHODDEF void
downsample_init (compress_info_ptr cinfo)
{
  /* no work for now */
}


/*
 * Downsample pixel values of a single component.
 * This version handles arbitrary integral sampling ratios, without smoothing.
 * Note that this version is not actually used for customary sampling ratios.
 */

METHODDEF void
int_downsample (compress_info_ptr cinfo, int which_component,
		long input_cols, int input_rows,
		long output_cols, int output_rows,
		JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
		JSAMPARRAY output_data)
{
  jpeg_component_info * compptr = cinfo->cur_comp_info[which_component];
  int inrow, outrow, h_expand, v_expand, numpix, numpix2, h, v;
  long outcol, outcol_h;	/* outcol_h == outcol*h_expand */
  JSAMPROW inptr, outptr;
  INT32 outvalue;

#ifdef DEBUG			/* for debugging pipeline controller */
  if (output_rows != compptr->v_samp_factor ||
      input_rows != cinfo->max_v_samp_factor ||
      (output_cols % compptr->h_samp_factor) != 0 ||
      (input_cols % cinfo->max_h_samp_factor) != 0 ||
      input_cols*compptr->h_samp_factor != output_cols*cinfo->max_h_samp_factor)
    ERREXIT(cinfo->emethods, "Bogus downsample parameters");
#endif

  h_expand = cinfo->max_h_samp_factor / compptr->h_samp_factor;
  v_expand = cinfo->max_v_samp_factor / compptr->v_samp_factor;
  numpix = h_expand * v_expand;
  numpix2 = numpix/2;

  inrow = 0;
  for (outrow = 0; outrow < output_rows; outrow++) {
    outptr = output_data[outrow];
    for (outcol = 0, outcol_h = 0; outcol < output_cols;
	 outcol++, outcol_h += h_expand) {
      outvalue = 0;
      for (v = 0; v < v_expand; v++) {
	inptr = input_data[inrow+v] + outcol_h;
	for (h = 0; h < h_expand; h++) {
	  outvalue += (INT32) GETJSAMPLE(*inptr++);
	}
      }
      *outptr++ = (JSAMPLE) ((outvalue + numpix2) / numpix);
    }
    inrow += v_expand;
  }
}


/*
 * Downsample pixel values of a single component.
 * This version handles the common case of 2:1 horizontal and 1:1 vertical,
 * without smoothing.
 */

METHODDEF void
h2v1_downsample (compress_info_ptr cinfo, int which_component,
		 long input_cols, int input_rows,
		 long output_cols, int output_rows,
		 JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
		 JSAMPARRAY output_data)
{
  int outrow;
  long outcol;
  register JSAMPROW inptr, outptr;

#ifdef DEBUG			/* for debugging pipeline controller */
  jpeg_component_info * compptr = cinfo->cur_comp_info[which_component];
  if (output_rows != compptr->v_samp_factor ||
      input_rows != cinfo->max_v_samp_factor ||
      (output_cols % compptr->h_samp_factor) != 0 ||
      (input_cols % cinfo->max_h_samp_factor) != 0 ||
      input_cols*compptr->h_samp_factor != output_cols*cinfo->max_h_samp_factor)
    ERREXIT(cinfo->emethods, "Bogus downsample parameters");
#endif

  for (outrow = 0; outrow < output_rows; outrow++) {
    outptr = output_data[outrow];
    inptr = input_data[outrow];
    for (outcol = 0; outcol < output_cols; outcol++) {
      *outptr++ = (JSAMPLE) ((GETJSAMPLE(*inptr) + GETJSAMPLE(inptr[1])
			      + 1) >> 1);
      inptr += 2;
    }
  }
}


/*
 * Downsample pixel values of a single component.
 * This version handles the standard case of 2:1 horizontal and 2:1 vertical,
 * without smoothing.
 */

METHODDEF void
h2v2_downsample (compress_info_ptr cinfo, int which_component,
		 long input_cols, int input_rows,
		 long output_cols, int output_rows,
		 JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
		 JSAMPARRAY output_data)
{
  int inrow, outrow;
  long outcol;
  register JSAMPROW inptr0, inptr1, outptr;

#ifdef DEBUG			/* for debugging pipeline controller */
  jpeg_component_info * compptr = cinfo->cur_comp_info[which_component];
  if (output_rows != compptr->v_samp_factor ||
      input_rows != cinfo->max_v_samp_factor ||
      (output_cols % compptr->h_samp_factor) != 0 ||
      (input_cols % cinfo->max_h_samp_factor) != 0 ||
      input_cols*compptr->h_samp_factor != output_cols*cinfo->max_h_samp_factor)
    ERREXIT(cinfo->emethods, "Bogus downsample parameters");
#endif

  inrow = 0;
  for (outrow = 0; outrow < output_rows; outrow++) {
    outptr = output_data[outrow];
    inptr0 = input_data[inrow];
    inptr1 = input_data[inrow+1];
    for (outcol = 0; outcol < output_cols; outcol++) {
      *outptr++ = (JSAMPLE) ((GETJSAMPLE(*inptr0) + GETJSAMPLE(inptr0[1]) +
			      GETJSAMPLE(*inptr1) + GETJSAMPLE(inptr1[1])
			      + 2) >> 2);
      inptr0 += 2; inptr1 += 2;
    }
    inrow += 2;
  }
}


/*
 * Downsample pixel values of a single component.
 * This version handles the special case of a full-size component,
 * without smoothing.
 */

METHODDEF void
fullsize_downsample (compress_info_ptr cinfo, int which_component,
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


#ifdef INPUT_SMOOTHING_SUPPORTED

/*
 * Downsample pixel values of a single component.
 * This version handles the standard case of 2:1 horizontal and 2:1 vertical,
 * with smoothing.
 */

METHODDEF void
h2v2_smooth_downsample (compress_info_ptr cinfo, int which_component,
			long input_cols, int input_rows,
			long output_cols, int output_rows,
			JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
			JSAMPARRAY output_data)
{
  int inrow, outrow;
  long colctr;
  register JSAMPROW inptr0, inptr1, above_ptr, below_ptr, outptr;
  INT32 membersum, neighsum, memberscale, neighscale;

#ifdef DEBUG			/* for debugging pipeline controller */
  jpeg_component_info * compptr = cinfo->cur_comp_info[which_component];
  if (output_rows != compptr->v_samp_factor ||
      input_rows != cinfo->max_v_samp_factor ||
      (output_cols % compptr->h_samp_factor) != 0 ||
      (input_cols % cinfo->max_h_samp_factor) != 0 ||
      input_cols*compptr->h_samp_factor != output_cols*cinfo->max_h_samp_factor)
    ERREXIT(cinfo->emethods, "Bogus downsample parameters");
#endif

  /* We don't bother to form the individual "smoothed" input pixel values;
   * we can directly compute the output which is the average of the four
   * smoothed values.  Each of the four member pixels contributes a fraction
   * (1-8*SF) to its own smoothed image and a fraction SF to each of the three
   * other smoothed pixels, therefore a total fraction (1-5*SF)/4 to the final
   * output.  The four corner-adjacent neighbor pixels contribute a fraction
   * SF to just one smoothed pixel, or SF/4 to the final output; while the
   * eight edge-adjacent neighbors contribute SF to each of two smoothed
   * pixels, or SF/2 overall.  In order to use integer arithmetic, these
   * factors are scaled by 2^16 = 65536.
   * Also recall that SF = smoothing_factor / 1024.
   */

  memberscale = 16384 - cinfo->smoothing_factor * 80; /* scaled (1-5*SF)/4 */
  neighscale = cinfo->smoothing_factor * 16; /* scaled SF/4 */

  inrow = 0;
  for (outrow = 0; outrow < output_rows; outrow++) {
    outptr = output_data[outrow];
    inptr0 = input_data[inrow];
    inptr1 = input_data[inrow+1];
    if (inrow == 0)
      above_ptr = above[input_rows-1];
    else
      above_ptr = input_data[inrow-1];
    if (inrow >= input_rows-2)
      below_ptr = below[0];
    else
      below_ptr = input_data[inrow+2];

    /* Special case for first column: pretend column -1 is same as column 0 */
    membersum = GETJSAMPLE(*inptr0) + GETJSAMPLE(inptr0[1]) +
		GETJSAMPLE(*inptr1) + GETJSAMPLE(inptr1[1]);
    neighsum = GETJSAMPLE(*above_ptr) + GETJSAMPLE(above_ptr[1]) +
	       GETJSAMPLE(*below_ptr) + GETJSAMPLE(below_ptr[1]) +
	       GETJSAMPLE(*inptr0) + GETJSAMPLE(inptr0[2]) +
	       GETJSAMPLE(*inptr1) + GETJSAMPLE(inptr1[2]);
    neighsum += neighsum;
    neighsum += GETJSAMPLE(*above_ptr) + GETJSAMPLE(above_ptr[2]) +
		GETJSAMPLE(*below_ptr) + GETJSAMPLE(below_ptr[2]);
    membersum = membersum * memberscale + neighsum * neighscale;
    *outptr++ = (JSAMPLE) ((membersum + 32768L) >> 16);
    inptr0 += 2; inptr1 += 2; above_ptr += 2; below_ptr += 2;

    for (colctr = output_cols - 2; colctr > 0; colctr--) {
      /* sum of pixels directly mapped to this output element */
      membersum = GETJSAMPLE(*inptr0) + GETJSAMPLE(inptr0[1]) +
		  GETJSAMPLE(*inptr1) + GETJSAMPLE(inptr1[1]);
      /* sum of edge-neighbor pixels */
      neighsum = GETJSAMPLE(*above_ptr) + GETJSAMPLE(above_ptr[1]) +
		 GETJSAMPLE(*below_ptr) + GETJSAMPLE(below_ptr[1]) +
		 GETJSAMPLE(inptr0[-1]) + GETJSAMPLE(inptr0[2]) +
		 GETJSAMPLE(inptr1[-1]) + GETJSAMPLE(inptr1[2]);
      /* The edge-neighbors count twice as much as corner-neighbors */
      neighsum += neighsum;
      /* Add in the corner-neighbors */
      neighsum += GETJSAMPLE(above_ptr[-1]) + GETJSAMPLE(above_ptr[2]) +
		  GETJSAMPLE(below_ptr[-1]) + GETJSAMPLE(below_ptr[2]);
      /* form final output scaled up by 2^16 */
      membersum = membersum * memberscale + neighsum * neighscale;
      /* round, descale and output it */
      *outptr++ = (JSAMPLE) ((membersum + 32768L) >> 16);
      inptr0 += 2; inptr1 += 2; above_ptr += 2; below_ptr += 2;
    }

    /* Special case for last column */
    membersum = GETJSAMPLE(*inptr0) + GETJSAMPLE(inptr0[1]) +
		GETJSAMPLE(*inptr1) + GETJSAMPLE(inptr1[1]);
    neighsum = GETJSAMPLE(*above_ptr) + GETJSAMPLE(above_ptr[1]) +
	       GETJSAMPLE(*below_ptr) + GETJSAMPLE(below_ptr[1]) +
	       GETJSAMPLE(inptr0[-1]) + GETJSAMPLE(inptr0[1]) +
	       GETJSAMPLE(inptr1[-1]) + GETJSAMPLE(inptr1[1]);
    neighsum += neighsum;
    neighsum += GETJSAMPLE(above_ptr[-1]) + GETJSAMPLE(above_ptr[1]) +
		GETJSAMPLE(below_ptr[-1]) + GETJSAMPLE(below_ptr[1]);
    membersum = membersum * memberscale + neighsum * neighscale;
    *outptr = (JSAMPLE) ((membersum + 32768L) >> 16);

    inrow += 2;
  }
}


/*
 * Downsample pixel values of a single component.
 * This version handles the special case of a full-size component,
 * with smoothing.
 */

METHODDEF void
fullsize_smooth_downsample (compress_info_ptr cinfo, int which_component,
			    long input_cols, int input_rows,
			    long output_cols, int output_rows,
			    JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
			    JSAMPARRAY output_data)
{
  int outrow;
  long colctr;
  register JSAMPROW inptr, above_ptr, below_ptr, outptr;
  INT32 membersum, neighsum, memberscale, neighscale;
  int colsum, lastcolsum, nextcolsum;

#ifdef DEBUG			/* for debugging pipeline controller */
  if (input_cols != output_cols || input_rows != output_rows)
    ERREXIT(cinfo->emethods, "Pipeline controller messed up");
#endif

  /* Each of the eight neighbor pixels contributes a fraction SF to the
   * smoothed pixel, while the main pixel contributes (1-8*SF).  In order
   * to use integer arithmetic, these factors are multiplied by 2^16 = 65536.
   * Also recall that SF = smoothing_factor / 1024.
   */

  memberscale = 65536L - cinfo->smoothing_factor * 512L; /* scaled 1-8*SF */
  neighscale = cinfo->smoothing_factor * 64; /* scaled SF */

  for (outrow = 0; outrow < output_rows; outrow++) {
    outptr = output_data[outrow];
    inptr = input_data[outrow];
    if (outrow == 0)
      above_ptr = above[input_rows-1];
    else
      above_ptr = input_data[outrow-1];
    if (outrow >= input_rows-1)
      below_ptr = below[0];
    else
      below_ptr = input_data[outrow+1];

    /* Special case for first column */
    colsum = GETJSAMPLE(*above_ptr++) + GETJSAMPLE(*below_ptr++) +
	     GETJSAMPLE(*inptr);
    membersum = GETJSAMPLE(*inptr++);
    nextcolsum = GETJSAMPLE(*above_ptr) + GETJSAMPLE(*below_ptr) +
		 GETJSAMPLE(*inptr);
    neighsum = colsum + (colsum - membersum) + nextcolsum;
    membersum = membersum * memberscale + neighsum * neighscale;
    *outptr++ = (JSAMPLE) ((membersum + 32768L) >> 16);
    lastcolsum = colsum; colsum = nextcolsum;

    for (colctr = output_cols - 2; colctr > 0; colctr--) {
      membersum = GETJSAMPLE(*inptr++);
      above_ptr++; below_ptr++;
      nextcolsum = GETJSAMPLE(*above_ptr) + GETJSAMPLE(*below_ptr) +
		   GETJSAMPLE(*inptr);
      neighsum = lastcolsum + (colsum - membersum) + nextcolsum;
      membersum = membersum * memberscale + neighsum * neighscale;
      *outptr++ = (JSAMPLE) ((membersum + 32768L) >> 16);
      lastcolsum = colsum; colsum = nextcolsum;
    }

    /* Special case for last column */
    membersum = GETJSAMPLE(*inptr);
    neighsum = lastcolsum + (colsum - membersum) + colsum;
    membersum = membersum * memberscale + neighsum * neighscale;
    *outptr = (JSAMPLE) ((membersum + 32768L) >> 16);

  }
}

#endif /* INPUT_SMOOTHING_SUPPORTED */


/*
 * Clean up after a scan.
 */

METHODDEF void
downsample_term (compress_info_ptr cinfo)
{
  /* no work for now */
}



/*
 * The method selection routine for downsampling.
 * Note that we must select a routine for each component.
 */

GLOBAL void
jseldownsample (compress_info_ptr cinfo)
{
  short ci;
  jpeg_component_info * compptr;
  boolean smoothok = TRUE;

  if (cinfo->CCIR601_sampling)
    ERREXIT(cinfo->emethods, "CCIR601 downsampling not implemented yet");

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    if (compptr->h_samp_factor == cinfo->max_h_samp_factor &&
	compptr->v_samp_factor == cinfo->max_v_samp_factor) {
#ifdef INPUT_SMOOTHING_SUPPORTED
      if (cinfo->smoothing_factor)
	cinfo->methods->downsample[ci] = fullsize_smooth_downsample;
      else
#endif
	cinfo->methods->downsample[ci] = fullsize_downsample;
    } else if (compptr->h_samp_factor * 2 == cinfo->max_h_samp_factor &&
	     compptr->v_samp_factor == cinfo->max_v_samp_factor) {
      smoothok = FALSE;
      cinfo->methods->downsample[ci] = h2v1_downsample;
    } else if (compptr->h_samp_factor * 2 == cinfo->max_h_samp_factor &&
	     compptr->v_samp_factor * 2 == cinfo->max_v_samp_factor) {
#ifdef INPUT_SMOOTHING_SUPPORTED
      if (cinfo->smoothing_factor)
	cinfo->methods->downsample[ci] = h2v2_smooth_downsample;
      else
#endif
	cinfo->methods->downsample[ci] = h2v2_downsample;
    } else if ((cinfo->max_h_samp_factor % compptr->h_samp_factor) == 0 &&
	     (cinfo->max_v_samp_factor % compptr->v_samp_factor) == 0) {
      smoothok = FALSE;
      cinfo->methods->downsample[ci] = int_downsample;
    } else
      ERREXIT(cinfo->emethods, "Fractional downsampling not implemented yet");
  }

#ifdef INPUT_SMOOTHING_SUPPORTED
  if (cinfo->smoothing_factor && !smoothok)
    TRACEMS(cinfo->emethods, 0,
	    "Smoothing not supported with nonstandard sampling ratios");
#endif

  cinfo->methods->downsample_init = downsample_init;
  cinfo->methods->downsample_term = downsample_term;
}

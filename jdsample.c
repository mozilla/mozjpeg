/*
 * jdsample.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains un-subsampling routines.
 * These routines are invoked via the unsubsample and
 * unsubsample_init/term methods.
 */

#include "jinclude.h"


/*
 * Initialize for un-subsampling a scan.
 */

METHODDEF void
unsubsample_init (decompress_info_ptr cinfo)
{
  /* no work for now */
}


/*
 * Un-subsample pixel values of a single component.
 * This version handles any integral sampling ratios.
 * This is not used for typical JPEG files, so it need not be fast.
 */

METHODDEF void
int_unsubsample (decompress_info_ptr cinfo, int which_component,
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
    ERREXIT(cinfo->emethods, "Bogus unsubsample parameters");
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
 * Un-subsample pixel values of a single component.
 * This version handles the extremely common case of
 * horizontal expansion by 2 and any integral vertical expansion.
 */

METHODDEF void
h2_unsubsample (decompress_info_ptr cinfo, int which_component,
		long input_cols, int input_rows,
		long output_cols, int output_rows,
		JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
		JSAMPARRAY output_data)
{
  jpeg_component_info * compptr = cinfo->cur_comp_info[which_component];
  register JSAMPROW inptr, outptr;
  register JSAMPLE invalue;
  short v_expand, v;
  int inrow, outrow;
  register long incol;

#ifdef DEBUG			/* for debugging pipeline controller */
  if (input_rows != compptr->v_samp_factor ||
      output_rows != cinfo->max_v_samp_factor ||
      (input_cols % compptr->h_samp_factor) != 0 ||
      (output_cols % cinfo->max_h_samp_factor) != 0 ||
      output_cols*compptr->h_samp_factor != input_cols*cinfo->max_h_samp_factor)
    ERREXIT(cinfo->emethods, "Bogus unsubsample parameters");
#endif

  v_expand = cinfo->max_v_samp_factor / compptr->v_samp_factor;

/* The subsampled image width will always be a multiple of DCTSIZE,
 * so we can unroll the inner loop.
 */

  outrow = 0;
  for (inrow = 0; inrow < input_rows; inrow++) {
    for (v = 0; v < v_expand; v++) {
      inptr = input_data[inrow];
      outptr = output_data[outrow++];
#if DCTSIZE == 8
      for (incol = 0; incol < input_cols; incol += DCTSIZE) {
	invalue = GETJSAMPLE(*inptr++);
	*outptr++ = invalue;
	*outptr++ = invalue;
	invalue = GETJSAMPLE(*inptr++);
	*outptr++ = invalue;
	*outptr++ = invalue;
	invalue = GETJSAMPLE(*inptr++);
	*outptr++ = invalue;
	*outptr++ = invalue;
	invalue = GETJSAMPLE(*inptr++);
	*outptr++ = invalue;
	*outptr++ = invalue;
	invalue = GETJSAMPLE(*inptr++);
	*outptr++ = invalue;
	*outptr++ = invalue;
	invalue = GETJSAMPLE(*inptr++);
	*outptr++ = invalue;
	*outptr++ = invalue;
	invalue = GETJSAMPLE(*inptr++);
	*outptr++ = invalue;
	*outptr++ = invalue;
	invalue = GETJSAMPLE(*inptr++);
	*outptr++ = invalue;
	*outptr++ = invalue;
      }
#else /* nonstandard DCTSIZE */
      for (incol = 0; incol < input_cols; incol++) {
	invalue = GETJSAMPLE(*inptr++);
	*outptr++ = invalue;
	*outptr++ = invalue;
      }
#endif
    }
  }
}


/*
 * Un-subsample pixel values of a single component.
 * This version handles the special case of a full-size component.
 */

METHODDEF void
fullsize_unsubsample (decompress_info_ptr cinfo, int which_component,
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
unsubsample_term (decompress_info_ptr cinfo)
{
  /* no work for now */
}



/*
 * The method selection routine for unsubsampling.
 * Note that we must select a routine for each component.
 */

GLOBAL void
jselunsubsample (decompress_info_ptr cinfo)
{
  short ci;
  jpeg_component_info * compptr;

  if (cinfo->CCIR601_sampling)
    ERREXIT(cinfo->emethods, "CCIR601 subsampling not implemented yet");

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    if (compptr->h_samp_factor == cinfo->max_h_samp_factor &&
	compptr->v_samp_factor == cinfo->max_v_samp_factor)
      cinfo->methods->unsubsample[ci] = fullsize_unsubsample;
    else if (compptr->h_samp_factor * 2 == cinfo->max_h_samp_factor &&
	     (cinfo->max_v_samp_factor % compptr->v_samp_factor) == 0)
      cinfo->methods->unsubsample[ci] = h2_unsubsample;
    else if ((cinfo->max_h_samp_factor % compptr->h_samp_factor) == 0 &&
	     (cinfo->max_v_samp_factor % compptr->v_samp_factor) == 0)
      cinfo->methods->unsubsample[ci] = int_unsubsample;
    else
      ERREXIT(cinfo->emethods, "Fractional subsampling not implemented yet");
  }

  cinfo->methods->unsubsample_init = unsubsample_init;
  cinfo->methods->unsubsample_term = unsubsample_term;
}

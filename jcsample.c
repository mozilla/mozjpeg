/*
 * jcsample.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains subsampling routines.
 * These routines are invoked via the subsample and
 * subsample_init/term methods.
 */

#include "jinclude.h"


/*
 * Initialize for subsampling a scan.
 */

METHODDEF void
subsample_init (compress_info_ptr cinfo)
{
  /* no work for now */
}


/*
 * Subsample pixel values of a single component.
 * This version only handles integral sampling ratios.
 */

METHODDEF void
subsample (compress_info_ptr cinfo, int which_component,
	   long input_cols, int input_rows,
	   long output_cols, int output_rows,
	   JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
	   JSAMPARRAY output_data)
{
  jpeg_component_info * compptr = cinfo->cur_comp_info[which_component];
  int inrow, outrow, h_expand, v_expand, numpix, numpix2, h, v;
  long outcol;
  JSAMPROW inptr, outptr;
  INT32 outvalue;

  /* TEMP FOR DEBUGGING PIPELINE CONTROLLER */
  if (output_rows != compptr->v_samp_factor ||
      input_rows != cinfo->max_v_samp_factor ||
      (output_cols % compptr->h_samp_factor) != 0 ||
      (input_cols % cinfo->max_h_samp_factor) != 0 ||
      input_cols*compptr->h_samp_factor != output_cols*cinfo->max_h_samp_factor)
    ERREXIT(cinfo->emethods, "Bogus subsample parameters");

  h_expand = cinfo->max_h_samp_factor / compptr->h_samp_factor;
  v_expand = cinfo->max_v_samp_factor / compptr->v_samp_factor;
  numpix = h_expand * v_expand;
  numpix2 = numpix/2;

  inrow = 0;
  for (outrow = 0; outrow < output_rows; outrow++) {
    outptr = output_data[outrow];
    for (outcol = 0; outcol < output_cols; outcol++) {
      outvalue = 0;
      for (v = 0; v < v_expand; v++) {
	inptr = input_data[inrow+v] + (outcol*h_expand);
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
 * Subsample pixel values of a single component.
 * This version handles the special case of a full-size component.
 */

METHODDEF void
fullsize_subsample (compress_info_ptr cinfo, int which_component,
		    long input_cols, int input_rows,
		    long output_cols, int output_rows,
		    JSAMPARRAY above, JSAMPARRAY input_data, JSAMPARRAY below,
		    JSAMPARRAY output_data)
{
  if (input_cols != output_cols || input_rows != output_rows) /* DEBUG */
    ERREXIT(cinfo->emethods, "Pipeline controller messed up");

  jcopy_sample_rows(input_data, 0, output_data, 0, output_rows, output_cols);
}


/*
 * Clean up after a scan.
 */

METHODDEF void
subsample_term (compress_info_ptr cinfo)
{
  /* no work for now */
}



/*
 * The method selection routine for subsampling.
 * Note that we must select a routine for each component.
 */

GLOBAL void
jselsubsample (compress_info_ptr cinfo)
{
  short ci;
  jpeg_component_info * compptr;

  if (cinfo->CCIR601_sampling)
    ERREXIT(cinfo->emethods, "CCIR601 subsampling not implemented yet");

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    if (compptr->h_samp_factor == cinfo->max_h_samp_factor &&
	compptr->v_samp_factor == cinfo->max_v_samp_factor)
      cinfo->methods->subsample[ci] = fullsize_subsample;
    else if ((cinfo->max_h_samp_factor % compptr->h_samp_factor) == 0 &&
	     (cinfo->max_v_samp_factor % compptr->v_samp_factor) == 0)
      cinfo->methods->subsample[ci] = subsample;
    else
      ERREXIT(cinfo->emethods, "Fractional subsampling not implemented yet");
  }

  cinfo->methods->subsample_init = subsample_init;
  cinfo->methods->subsample_term = subsample_term;
}

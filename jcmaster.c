/*
 * jcmaster.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains the main control for the JPEG compressor.
 * The system-dependent (user interface) code should call jpeg_compress()
 * after doing appropriate setup of the compress_info_struct parameter.
 */

#include "jinclude.h"


METHODDEF void
c_per_scan_method_selection (compress_info_ptr cinfo)
/* Central point for per-scan method selection */
{
  /* Edge expansion */
  jselexpand(cinfo);
  /* Downsampling of pixels */
  jseldownsample(cinfo);
  /* MCU extraction */
  jselcmcu(cinfo);
}


LOCAL void
c_initial_method_selection (compress_info_ptr cinfo)
/* Central point for initial method selection */
{
  /* Input image reading method selection is already done. */
  /* So is output file header formatting (both are done by user interface). */

  /* Gamma and color space conversion */
  jselccolor(cinfo);
  /* Entropy encoding: either Huffman or arithmetic coding. */
#ifdef C_ARITH_CODING_SUPPORTED
  jselcarithmetic(cinfo);
#else
  cinfo->arith_code = FALSE;	/* force Huffman mode */
#endif
  jselchuffman(cinfo);
  /* Pipeline control */
  jselcpipeline(cinfo);
  /* Overall control (that's me!) */
  cinfo->methods->c_per_scan_method_selection = c_per_scan_method_selection;
}


LOCAL void
initial_setup (compress_info_ptr cinfo)
/* Do computations that are needed before initial method selection */
{
  short ci;
  jpeg_component_info *compptr;

  /* Compute maximum sampling factors; check factor validity */
  cinfo->max_h_samp_factor = 1;
  cinfo->max_v_samp_factor = 1;
  for (ci = 0; ci < cinfo->num_components; ci++) {
    compptr = &cinfo->comp_info[ci];
    if (compptr->h_samp_factor<=0 || compptr->h_samp_factor>MAX_SAMP_FACTOR ||
	compptr->v_samp_factor<=0 || compptr->v_samp_factor>MAX_SAMP_FACTOR)
      ERREXIT(cinfo->emethods, "Bogus sampling factors");
    cinfo->max_h_samp_factor = MAX(cinfo->max_h_samp_factor,
				   compptr->h_samp_factor);
    cinfo->max_v_samp_factor = MAX(cinfo->max_v_samp_factor,
				   compptr->v_samp_factor);

  }

  /* Compute logical downsampled dimensions of components */
  for (ci = 0; ci < cinfo->num_components; ci++) {
    compptr = &cinfo->comp_info[ci];
    compptr->true_comp_width = (cinfo->image_width * compptr->h_samp_factor
				+ cinfo->max_h_samp_factor - 1)
				/ cinfo->max_h_samp_factor;
    compptr->true_comp_height = (cinfo->image_height * compptr->v_samp_factor
				 + cinfo->max_v_samp_factor - 1)
				 / cinfo->max_v_samp_factor;
  }
}


/*
 * This is the main entry point to the JPEG compressor.
 */


GLOBAL void
jpeg_compress (compress_info_ptr cinfo)
{
  /* Init pass counts to 0 --- total_passes is adjusted in method selection */
  cinfo->total_passes = 0;
  cinfo->completed_passes = 0;

  /* Read the input file header: determine image size & component count.
   * NOTE: the user interface must have initialized the input_init method
   * pointer (eg, by calling jselrppm) before calling me.
   * The other file reading methods (get_input_row etc.) were probably
   * set at the same time, but could be set up by input_init itself,
   * or by c_ui_method_selection.
   */
  (*cinfo->methods->input_init) (cinfo);

  /* Give UI a chance to adjust compression parameters and select */
  /* output file format based on results of input_init. */
  (*cinfo->methods->c_ui_method_selection) (cinfo);

  /* Now select methods for compression steps. */
  initial_setup(cinfo);
  c_initial_method_selection(cinfo);

  /* Initialize the output file & other modules as needed */
  /* (entropy_encoder is inited by pipeline controller) */

  (*cinfo->methods->colorin_init) (cinfo);
  (*cinfo->methods->write_file_header) (cinfo);

  /* And let the pipeline controller do the rest. */
  (*cinfo->methods->c_pipeline_controller) (cinfo);

  /* Finish output file, release working storage, etc */
  (*cinfo->methods->write_file_trailer) (cinfo);
  (*cinfo->methods->colorin_term) (cinfo);
  (*cinfo->methods->input_term) (cinfo);

  (*cinfo->emethods->free_all) ();

  /* My, that was easy, wasn't it? */
}

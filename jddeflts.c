/*
 * jddeflts.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains optional default-setting code for the JPEG decompressor.
 * User interfaces do not have to use this file, but those that don't use it
 * must know more about the innards of the JPEG code.
 */

#include "jinclude.h"


/*
 * Reload the input buffer after it's been emptied, and return the next byte.
 * See the JGETC macro for calling conditions.
 *
 * This routine is used only if the system-dependent user interface passes
 * standard_buffering = TRUE to j_d_defaults.  Otherwise, the UI must supply
 * a corresponding routine.  Note that in any case, this routine is likely
 * to be used only for JFIF or similar serial-access JPEG file formats.
 * The input file control module for a random-access format such as TIFF/JPEG
 * would need to override the read_jpeg_data method with its own routine.
 *
 * This routine would need to be replaced if reading JPEG data from something
 * other than a stdio stream.
 */

METHODDEF int
read_jpeg_data (decompress_info_ptr cinfo)
{
  cinfo->next_input_byte = cinfo->input_buffer + MIN_UNGET;

  cinfo->bytes_in_buffer = (int) FREAD(cinfo->input_file,
				       cinfo->next_input_byte,
				       JPEG_BUF_SIZE);
  
  if (cinfo->bytes_in_buffer <= 0)
    ERREXIT(cinfo->emethods, "Unexpected EOF in JPEG file");

  return JGETC(cinfo);
}



/* Default parameter setup for decompression.
 *
 * User interfaces that don't choose to use this routine must do their
 * own setup of all these parameters.  Alternately, you can call this
 * to establish defaults and then alter parameters selectively.  This
 * is the recommended approach since, if we add any new parameters,
 * your code will still work (they'll be set to reasonable defaults).
 *
 * standard_buffering should be TRUE if the JPEG data is to come from
 * a stdio stream and the user interface isn't interested in changing
 * the normal input-buffering logic.  If FALSE is passed, the user
 * interface must provide its own read_jpeg_data method and must
 * set up its own input buffer.  (Alternately, you can pass TRUE to
 * let the buffer be allocated here, then override read_jpeg_data with
 * your own routine.)
 */

GLOBAL void
j_d_defaults (decompress_info_ptr cinfo, boolean standard_buffering)
/* NB: the external methods must already be set up. */
{
  /* Default to RGB output */
  /* UI can override by changing out_color_space */
  cinfo->out_color_space = CS_RGB;
  cinfo->jpeg_color_space = CS_UNKNOWN;
  /* Setting any other value in jpeg_color_space overrides heuristics in */
  /* jrdjfif.c.  That might be useful when reading non-JFIF JPEG files, */
  /* but ordinarily the UI shouldn't change it. */
  
  /* Default to no gamma correction of output */
  cinfo->output_gamma = 1.0;
  
  /* Default to no color quantization */
  cinfo->quantize_colors = FALSE;
  /* but set reasonable default parameters for quantization, */
  /* so that turning on quantize_colors is sufficient to do something useful */
  cinfo->two_pass_quantize = FALSE; /* may change to TRUE later */
  cinfo->use_dithering = TRUE;
  cinfo->desired_number_of_colors = 256;
  
  /* Default to no smoothing */
  cinfo->do_block_smoothing = FALSE;
  cinfo->do_pixel_smoothing = FALSE;
  
  if (standard_buffering) {
    /* Allocate memory for input buffer. */
    cinfo->input_buffer = (char *) (*cinfo->emethods->alloc_small)
					((size_t) (JPEG_BUF_SIZE + MIN_UNGET));
    cinfo->bytes_in_buffer = 0;	/* initialize buffer to empty */

    /* Install standard buffer-reloading method. */
    cinfo->methods->read_jpeg_data = read_jpeg_data;
  }
}


/* This routine releases storage allocated by j_d_defaults.
 * Note that freeing the method pointer structs and the decompress_info_struct
 * itself are the responsibility of the user interface.
 *
 * standard_buffering must agree with what was passed to j_d_defaults.
 */

GLOBAL void
j_d_free_defaults (decompress_info_ptr cinfo, boolean standard_buffering)
{
  if (standard_buffering) {
    (*cinfo->emethods->free_small) ((void *) cinfo->input_buffer);
  }
}

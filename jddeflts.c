/*
 * jddeflts.c
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains optional default-setting code for the JPEG decompressor.
 * User interfaces do not have to use this file, but those that don't use it
 * must know more about the innards of the JPEG code.
 */

#include "jinclude.h"


/* Default do-nothing progress monitoring routine.
 * This can be overridden by a user interface that wishes to
 * provide progress monitoring; just set methods->progress_monitor
 * after j_d_defaults is done.  The routine will be called periodically
 * during the decompression process.
 *
 * During any one pass, loopcounter increases from 0 up to (not including)
 * looplimit; the step size is not necessarily 1.  Both the step size and
 * the limit may differ between passes.  The expected total number of passes
 * is in cinfo->total_passes, and the number of passes already completed is
 * in cinfo->completed_passes.  Thus the fraction of work completed may be
 * estimated as
 *		completed_passes + (loopcounter/looplimit)
 *		------------------------------------------
 *				total_passes
 * ignoring the fact that the passes may not be equal amounts of work.
 *
 * When decompressing, the total_passes figure is an estimate that may be
 * on the high side; completed_passes will jump by more than one if some
 * passes are skipped.
 */

METHODDEF void
progress_monitor (decompress_info_ptr cinfo, long loopcounter, long looplimit)
{
  /* do nothing */
}


/* Default comment-block processing routine.
 * This can be overridden by an application that wishes to examine
 * COM blocks found in the JPEG file.  The default routine does nothing.
 * CAUTION: the comment processing routine MUST call JGETC() exactly
 * comment_length times to read the comment data, whether it intends
 * to do anything with the data or not!
 * Keep in mind that (a) there may be more than one COM block in a file;
 * (b) there's no guarantee that what's in the block is ASCII data.
 */

METHODDEF void
process_comment (decompress_info_ptr cinfo, long comment_length)
{
  while (comment_length-- > 0) {
    (void) JGETC(cinfo);
  }
}


/*
 * Reload the input buffer after it's been emptied, and return the next byte.
 * See the JGETC macro for calling conditions.  Note in particular that
 * read_jpeg_data may NOT return EOF.  If no more data is available, it must
 * exit via ERREXIT, or perhaps synthesize fake data (such as an RST marker).
 * In the present implementation, we insert an EOI marker; this might not be
 * appropriate for non-JFIF file formats, but it usually allows us to handle
 * a truncated JFIF file.
 *
 * This routine can be overridden by the system-dependent user interface,
 * in case the data source is not a stdio stream or some other special
 * condition applies.  Note, however, that this capability only applies for
 * JFIF or similar serial-access JPEG file formats.  The input file control
 * module for a random-access format such as TIFF/JPEG would most likely
 * override the read_jpeg_data method with its own routine.
 */

METHODDEF int
read_jpeg_data (decompress_info_ptr cinfo)
{
  cinfo->next_input_byte = cinfo->input_buffer + MIN_UNGET;

  cinfo->bytes_in_buffer = (int) JFREAD(cinfo->input_file,
					cinfo->next_input_byte,
					JPEG_BUF_SIZE);
  
  if (cinfo->bytes_in_buffer <= 0) {
    WARNMS(cinfo->emethods, "Premature EOF in JPEG file");
    cinfo->next_input_byte[0] = (char) 0xFF;
    cinfo->next_input_byte[1] = (char) 0xD9; /* EOI marker */
    cinfo->bytes_in_buffer = 2;
  }

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
 * standard_buffering should be TRUE to cause an input buffer to be allocated
 * (the normal case); if FALSE, the user interface must provide a buffer.
 * This option is most useful in the case that the buffer must not be freed
 * at the end of an image.  (For example, when reading a sequence of images
 * from a single file, the remaining data in the buffer represents the
 * start of the next image and mustn't be discarded.)  To handle this,
 * allocate the input buffer yourself at startup, WITHOUT using alloc_small
 * (probably a direct call to malloc() instead).  Then pass FALSE on each
 * call to j_d_defaults to ensure the buffer state is not modified.
 *
 * If the source of the JPEG data is not a stdio stream, override the
 * read_jpeg_data method with your own routine after calling j_d_defaults.
 * You can still use the standard buffer if it's appropriate.
 *
 * CAUTION: if you want to decompress multiple images per run, it's necessary
 * to call j_d_defaults before *each* call to jpeg_decompress, since subsidiary
 * structures like the quantization tables are automatically freed during
 * cleanup.
 */

GLOBAL void
j_d_defaults (decompress_info_ptr cinfo, boolean standard_buffering)
/* NB: the external methods must already be set up. */
{
  short i;

  /* Initialize pointers as needed to mark stuff unallocated. */
  /* Outer application may fill in default tables for abbreviated files... */
  cinfo->comp_info = NULL;
  for (i = 0; i < NUM_QUANT_TBLS; i++)
    cinfo->quant_tbl_ptrs[i] = NULL;
  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    cinfo->dc_huff_tbl_ptrs[i] = NULL;
    cinfo->ac_huff_tbl_ptrs[i] = NULL;
  }
  cinfo->colormap = NULL;

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
  cinfo->two_pass_quantize = TRUE;
  cinfo->use_dithering = TRUE;
  cinfo->desired_number_of_colors = 256;
  
  /* Default to no smoothing */
  cinfo->do_block_smoothing = FALSE;
  cinfo->do_pixel_smoothing = FALSE;
  
  /* Allocate memory for input buffer, unless outer application provides it. */
  if (standard_buffering) {
    cinfo->input_buffer = (char *) (*cinfo->emethods->alloc_small)
					((size_t) (JPEG_BUF_SIZE + MIN_UNGET));
    cinfo->bytes_in_buffer = 0;	/* initialize buffer to empty */
  }

  /* Install standard buffer-reloading method (outer code may override). */
  cinfo->methods->read_jpeg_data = read_jpeg_data;

  /* Install default do-nothing progress monitoring method. */
  cinfo->methods->progress_monitor = progress_monitor;

  /* Install default comment-block processing method. */
  cinfo->methods->process_comment = process_comment;
}

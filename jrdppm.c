/*
 * jrdppm.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in PPM format.
 * The PBMPLUS library is required (well, it will be in the real version).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume input from
 * an ordinary stdio stream.  They further assume that reading begins
 * at the start of the file; input_init may need work if the
 * user interface has already read some data (e.g., to determine that
 * the file is indeed PPM format).
 *
 * These routines are invoked via the methods get_input_row
 * and input_init/term.
 */

#include "jinclude.h"

#ifdef PPM_SUPPORTED


/*
 * Read the file header; return image size and component count.
 */

METHODDEF void
input_init (compress_info_ptr cinfo)
{
  int c, w, h, prec;

  if (getc(cinfo->input_file) != 'P')
    ERREXIT(cinfo->emethods, "Not a PPM file");

  c = getc(cinfo->input_file);
  switch (c) {
  case '5':			/* it's a PGM file */
    cinfo->input_components = 1;
    cinfo->in_color_space = CS_GRAYSCALE;
    break;

  case '6':			/* it's a PPM file */
    cinfo->input_components = 3;
    cinfo->in_color_space = CS_RGB;
    break;

  default:
    ERREXIT(cinfo->emethods, "Not a PPM file");
    break;
  }

  if (fscanf(cinfo->input_file, " %d %d %d", &w, &h, &prec) != 3)
    ERREXIT(cinfo->emethods, "Not a PPM file");

  if (getc(cinfo->input_file) != '\n' || w <= 0 || h <= 0 || prec != 255)
    ERREXIT(cinfo->emethods, "Not a PPM file");

  cinfo->image_width = w;
  cinfo->image_height = h;
  cinfo->data_precision = 8;
}


/*
 * Read one row of pixels.
 */

METHODDEF void
get_input_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
{
  register FILE * infile = cinfo->input_file;
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  
  if (cinfo->input_components == 1) {
    ptr0 = pixel_row[0];
    for (col = cinfo->image_width; col > 0; col--) {
      *ptr0++ = getc(infile);
    }
  } else {
    ptr0 = pixel_row[0];
    ptr1 = pixel_row[1];
    ptr2 = pixel_row[2];
    for (col = cinfo->image_width; col > 0; col--) {
      *ptr0++ = getc(infile);
      *ptr1++ = getc(infile);
      *ptr2++ = getc(infile);
    }
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
input_term (compress_info_ptr cinfo)
{
  /* no work required */
}


/*
 * The method selection routine for PPM format input.
 * Note that this must be called by the user interface before calling
 * jpeg_compress.  If multiple input formats are supported, the
 * user interface is responsible for discovering the file format and
 * calling the appropriate method selection routine.
 */

GLOBAL void
jselrppm (compress_info_ptr cinfo)
{
  cinfo->methods->input_init = input_init;
  cinfo->methods->get_input_row = get_input_row;
  cinfo->methods->input_term = input_term;
}

#endif /* PPM_SUPPORTED */

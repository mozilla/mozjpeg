/*
 * jrdppm.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in PPM format.
 * The PBMPLUS library is NOT required to compile this software,
 * but it is highly useful as a set of PPM image manipulation programs.
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


/* Portions of this code are based on the PBMPLUS library, which is:
**
** Copyright (C) 1988 by Jef Poskanzer.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/


/* Macros to deal with unsigned chars as efficiently as compiler allows */

#ifdef HAVE_UNSIGNED_CHAR
typedef unsigned char U_CHAR;
#define UCH(x)	((int) (x))
#else /* !HAVE_UNSIGNED_CHAR */
#ifdef CHAR_IS_UNSIGNED
typedef char U_CHAR;
#define UCH(x)	((int) (x))
#else
typedef char U_CHAR;
#define UCH(x)	((int) (x) & 0xFF)
#endif
#endif /* HAVE_UNSIGNED_CHAR */


#define	ReadOK(file,buffer,len)	(JFREAD(file,buffer,len) == ((size_t) (len)))


/*
 * On most systems, reading individual bytes with getc() is drastically less
 * efficient than buffering a row at a time with fread().  But we must
 * allocate the row buffer in near data space on PCs, because we are assuming
 * small-data memory model, wherein fread() can't reach far memory.  If you
 * need to process very wide images on a PC, you may have to use the getc()
 * approach.  In that case, define USE_GETC_INPUT.
 */

#ifndef USE_GETC_INPUT
static U_CHAR * row_buffer;	/* holds 1 pixel row's worth of raw input */
#endif

static JSAMPLE * rescale;	/* => maxval-remapping array, or NULL */


LOCAL int
pbm_getc (FILE * file)
/* Read next char, skipping over any comments */
/* A comment/newline sequence is returned as a newline */
{
  register int ch;
  
  ch = getc(file);
  if (ch == '#') {
    do {
      ch = getc(file);
    } while (ch != '\n' && ch != EOF);
  }
  return ch;
}


LOCAL unsigned int
read_pbm_integer (compress_info_ptr cinfo)
/* Read an unsigned decimal integer from the PPM file */
/* Swallows one trailing character after the integer */
/* Note that on a 16-bit-int machine, only values up to 64k can be read. */
/* This should not be a problem in practice. */
{
  register int ch;
  register unsigned int val;
  
  /* Skip any leading whitespace */
  do {
    ch = pbm_getc(cinfo->input_file);
    if (ch == EOF)
      ERREXIT(cinfo->emethods, "Premature EOF in PPM file");
  } while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
  
  if (ch < '0' || ch > '9')
    ERREXIT(cinfo->emethods, "Bogus data in PPM file");
  
  val = ch - '0';
  while ((ch = pbm_getc(cinfo->input_file)) >= '0' && ch <= '9') {
    val *= 10;
    val += ch - '0';
  }
  return val;
}


/*
 * Read one row of pixels.
 *
 * We provide several different versions depending on input file format.
 * In all cases, input is scaled to the size of JSAMPLE; it's possible that
 * when JSAMPLE is 12 bits, this would not really be desirable.
 *
 * Note that a really fast path is provided for reading raw files with
 * maxval = MAXJSAMPLE, which is the normal case (at least for 8-bit JSAMPLEs).
 */


METHODDEF void
get_text_gray_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading text-format PGM files with any maxval */
{
  register JSAMPROW ptr0;
  register unsigned int val;
  register long col;
  
  ptr0 = pixel_row[0];
  for (col = cinfo->image_width; col > 0; col--) {
    val = read_pbm_integer(cinfo);
    if (rescale != NULL)
      val = rescale[val];
    *ptr0++ = (JSAMPLE) val;
  }
}


METHODDEF void
get_text_rgb_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading text-format PPM files with any maxval */
{
  register JSAMPROW ptr0, ptr1, ptr2;
  register unsigned int val;
  register long col;
  
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  for (col = cinfo->image_width; col > 0; col--) {
    val = read_pbm_integer(cinfo);
    if (rescale != NULL)
      val = rescale[val];
    *ptr0++ = (JSAMPLE) val;
    val = read_pbm_integer(cinfo);
    if (rescale != NULL)
      val = rescale[val];
    *ptr1++ = (JSAMPLE) val;
    val = read_pbm_integer(cinfo);
    if (rescale != NULL)
      val = rescale[val];
    *ptr2++ = (JSAMPLE) val;
  }
}


#ifdef USE_GETC_INPUT


METHODDEF void
get_scaled_gray_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading raw-format PGM files with any maxval */
{
  register FILE * infile = cinfo->input_file;
  register JSAMPROW ptr0;
  register long col;
  
  ptr0 = pixel_row[0];
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr0++ = rescale[getc(infile)];
  }
}


METHODDEF void
get_scaled_rgb_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading raw-format PPM files with any maxval */
{
  register FILE * infile = cinfo->input_file;
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr0++ = rescale[getc(infile)];
    *ptr1++ = rescale[getc(infile)];
    *ptr2++ = rescale[getc(infile)];
  }
}


METHODDEF void
get_raw_gray_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading raw-format PGM files with maxval = MAXJSAMPLE */
{
  register FILE * infile = cinfo->input_file;
  register JSAMPROW ptr0;
  register long col;
  
  ptr0 = pixel_row[0];
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr0++ = (JSAMPLE) getc(infile);
  }
}


METHODDEF void
get_raw_rgb_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading raw-format PPM files with maxval = MAXJSAMPLE */
{
  register FILE * infile = cinfo->input_file;
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr0++ = (JSAMPLE) getc(infile);
    *ptr1++ = (JSAMPLE) getc(infile);
    *ptr2++ = (JSAMPLE) getc(infile);
  }
}


#else /* use row buffering */


METHODDEF void
get_scaled_gray_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading raw-format PGM files with any maxval */
{
  register JSAMPROW ptr0;
  register U_CHAR * row_bufferptr;
  register long col;
  
  if (! ReadOK(cinfo->input_file, row_buffer, cinfo->image_width))
    ERREXIT(cinfo->emethods, "Premature EOF in PPM file");
  ptr0 = pixel_row[0];
  row_bufferptr = row_buffer;
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr0++ = rescale[UCH(*row_bufferptr++)];
  }
}


METHODDEF void
get_scaled_rgb_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading raw-format PPM files with any maxval */
{
  register JSAMPROW ptr0, ptr1, ptr2;
  register U_CHAR * row_bufferptr;
  register long col;
  
  if (! ReadOK(cinfo->input_file, row_buffer, 3 * cinfo->image_width))
    ERREXIT(cinfo->emethods, "Premature EOF in PPM file");
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  row_bufferptr = row_buffer;
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr0++ = rescale[UCH(*row_bufferptr++)];
    *ptr1++ = rescale[UCH(*row_bufferptr++)];
    *ptr2++ = rescale[UCH(*row_bufferptr++)];
  }
}


METHODDEF void
get_raw_gray_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading raw-format PGM files with maxval = MAXJSAMPLE */
{
  register JSAMPROW ptr0;
  register U_CHAR * row_bufferptr;
  register long col;
  
  if (! ReadOK(cinfo->input_file, row_buffer, cinfo->image_width))
    ERREXIT(cinfo->emethods, "Premature EOF in PPM file");
  ptr0 = pixel_row[0];
  row_bufferptr = row_buffer;
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr0++ = (JSAMPLE) UCH(*row_bufferptr++);
  }
}


METHODDEF void
get_raw_rgb_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading raw-format PPM files with maxval = MAXJSAMPLE */
{
  register JSAMPROW ptr0, ptr1, ptr2;
  register U_CHAR * row_bufferptr;
  register long col;
  
  if (! ReadOK(cinfo->input_file, row_buffer, 3 * cinfo->image_width))
    ERREXIT(cinfo->emethods, "Premature EOF in PPM file");
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  row_bufferptr = row_buffer;
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr0++ = (JSAMPLE) UCH(*row_bufferptr++);
    *ptr1++ = (JSAMPLE) UCH(*row_bufferptr++);
    *ptr2++ = (JSAMPLE) UCH(*row_bufferptr++);
  }
}


#endif /* USE_GETC_INPUT */


/*
 * Read the file header; return image size and component count.
 */

METHODDEF void
input_init (compress_info_ptr cinfo)
{
  int c;
  unsigned int w, h, maxval;

  if (getc(cinfo->input_file) != 'P')
    ERREXIT(cinfo->emethods, "Not a PPM file");

  c = getc(cinfo->input_file);	/* save format discriminator for a sec */

  w = read_pbm_integer(cinfo);	/* while we fetch the header info */
  h = read_pbm_integer(cinfo);
  maxval = read_pbm_integer(cinfo);

  if (w <= 0 || h <= 0 || maxval <= 0) /* error check */
    ERREXIT(cinfo->emethods, "Not a PPM file");

  switch (c) {
  case '2':			/* it's a text-format PGM file */
    cinfo->methods->get_input_row = get_text_gray_row;
    cinfo->input_components = 1;
    cinfo->in_color_space = CS_GRAYSCALE;
    TRACEMS2(cinfo->emethods, 1, "%ux%u text PGM image", w, h);
    break;

  case '3':			/* it's a text-format PPM file */
    cinfo->methods->get_input_row = get_text_rgb_row;
    cinfo->input_components = 3;
    cinfo->in_color_space = CS_RGB;
    TRACEMS2(cinfo->emethods, 1, "%ux%u text PPM image", w, h);
    break;

  case '5':			/* it's a raw-format PGM file */
    if (maxval == MAXJSAMPLE)
      cinfo->methods->get_input_row = get_raw_gray_row;
    else
      cinfo->methods->get_input_row = get_scaled_gray_row;
    cinfo->input_components = 1;
    cinfo->in_color_space = CS_GRAYSCALE;
#ifndef USE_GETC_INPUT
    /* allocate space for row buffer: 1 byte/pixel */
    row_buffer = (U_CHAR *) (*cinfo->emethods->alloc_small)
			((size_t) (SIZEOF(U_CHAR) * (long) w));
#endif
    TRACEMS2(cinfo->emethods, 1, "%ux%u PGM image", w, h);
    break;

  case '6':			/* it's a raw-format PPM file */
    if (maxval == MAXJSAMPLE)
      cinfo->methods->get_input_row = get_raw_rgb_row;
    else
      cinfo->methods->get_input_row = get_scaled_rgb_row;
    cinfo->input_components = 3;
    cinfo->in_color_space = CS_RGB;
#ifndef USE_GETC_INPUT
    /* allocate space for row buffer: 3 bytes/pixel */
    row_buffer = (U_CHAR *) (*cinfo->emethods->alloc_small)
			((size_t) (3 * SIZEOF(U_CHAR) * (long) w));
#endif
    TRACEMS2(cinfo->emethods, 1, "%ux%u PPM image", w, h);
    break;

  default:
    ERREXIT(cinfo->emethods, "Not a PPM file");
    break;
  }

  /* Compute the rescaling array if necessary */
  /* This saves per-pixel calculation */
  if (maxval == MAXJSAMPLE)
    rescale = NULL;		/* no rescaling required */
  else {
    INT32 val, half_maxval;

    /* On 16-bit-int machines we have to be careful of maxval = 65535 */
    rescale = (JSAMPLE *) (*cinfo->emethods->alloc_small)
			((size_t) (((long) maxval + 1L) * SIZEOF(JSAMPLE)));
    half_maxval = maxval / 2;
    for (val = 0; val <= (INT32) maxval; val++) {
      /* The multiplication here must be done in 32 bits to avoid overflow */
      rescale[val] = (JSAMPLE) ((val * MAXJSAMPLE + half_maxval) / maxval);
    }
  }

  cinfo->image_width = w;
  cinfo->image_height = h;
  cinfo->data_precision = BITS_IN_JSAMPLE;
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
input_term (compress_info_ptr cinfo)
{
  /* no work (we let free_all release the workspace) */
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
  /* cinfo->methods->get_input_row is set by input_init */
  cinfo->methods->input_term = input_term;
}

#endif /* PPM_SUPPORTED */

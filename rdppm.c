/*
 * rdppm.c
 *
 * Copyright (C) 1991-1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in PPM/PGM format.
 * The PBMPLUS library is NOT required to compile this software
 * (but it is highly useful as a set of PPM image manipulation programs).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume input from
 * an ordinary stdio stream.  They further assume that reading begins
 * at the start of the file; start_input may need work if the
 * user interface has already read some data (e.g., to determine that
 * the file is indeed PPM format).
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */

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
 * efficient than buffering a row at a time with fread().  On PCs, we must
 * allocate the buffer in near data space, because we are assuming small-data
 * memory model, wherein fread() can't reach far memory.  If you need to
 * process very wide images on a PC, you might have to compile in large-memory
 * model, or else replace fread() with a getc() loop --- which will be much
 * slower.
 */


/* Private version of data source object */

typedef struct {
  struct cjpeg_source_struct pub; /* public fields */

  U_CHAR *iobuffer;		/* non-FAR pointer to I/O buffer */
  JSAMPROW pixrow;		/* FAR pointer to same */
  JDIMENSION buffer_width;	/* width of one row */
  JSAMPLE *rescale;		/* => maxval-remapping array, or NULL */
} ppm_source_struct;

typedef ppm_source_struct * ppm_source_ptr;


LOCAL int
pbm_getc (FILE * infile)
/* Read next char, skipping over any comments */
/* A comment/newline sequence is returned as a newline */
{
  register int ch;
  
  ch = getc(infile);
  if (ch == '#') {
    do {
      ch = getc(infile);
    } while (ch != '\n' && ch != EOF);
  }
  return ch;
}


LOCAL unsigned int
read_pbm_integer (j_compress_ptr cinfo, FILE * infile)
/* Read an unsigned decimal integer from the PPM file */
/* Swallows one trailing character after the integer */
/* Note that on a 16-bit-int machine, only values up to 64k can be read. */
/* This should not be a problem in practice. */
{
  register int ch;
  register unsigned int val;
  
  /* Skip any leading whitespace */
  do {
    ch = pbm_getc(infile);
    if (ch == EOF)
      ERREXIT(cinfo, JERR_INPUT_EOF);
  } while (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
  
  if (ch < '0' || ch > '9')
    ERREXIT(cinfo, JERR_PPM_NONNUMERIC);
  
  val = ch - '0';
  while ((ch = pbm_getc(infile)) >= '0' && ch <= '9') {
    val *= 10;
    val += ch - '0';
  }
  return val;
}


/*
 * Read one row of pixels.
 *
 * We provide several different versions depending on input file format.
 * In all cases, input is scaled to the size of JSAMPLE.
 *
 * Note that a really fast path is provided for reading raw files with
 * maxval = MAXJSAMPLE, which is the normal case (at least for 8-bit JSAMPLEs).
 */


METHODDEF JDIMENSION
get_text_gray_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
/* This version is for reading text-format PGM files with any maxval */
{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  FILE * infile = source->pub.input_file;
  register JSAMPROW ptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;
  
  ptr = source->pub.buffer[0];
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr++ = rescale[read_pbm_integer(cinfo, infile)];
  }
  return 1;
}


METHODDEF JDIMENSION
get_text_rgb_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
/* This version is for reading text-format PPM files with any maxval */
{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  FILE * infile = source->pub.input_file;
  register JSAMPROW ptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;
  
  ptr = source->pub.buffer[0];
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr++ = rescale[read_pbm_integer(cinfo, infile)];
    *ptr++ = rescale[read_pbm_integer(cinfo, infile)];
    *ptr++ = rescale[read_pbm_integer(cinfo, infile)];
  }
  return 1;
}


METHODDEF JDIMENSION
get_scaled_gray_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
/* This version is for reading raw-format PGM files with any maxval */
{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  register JSAMPROW ptr;
  register U_CHAR * bufferptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  if (! ReadOK(source->pub.input_file, source->iobuffer, source->buffer_width))
    ERREXIT(cinfo, JERR_INPUT_EOF);
  ptr = source->pub.buffer[0];
  bufferptr = source->iobuffer;
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr++ = rescale[UCH(*bufferptr++)];
  }
  return 1;
}


METHODDEF JDIMENSION
get_scaled_rgb_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
/* This version is for reading raw-format PPM files with any maxval */
{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  register JSAMPROW ptr;
  register U_CHAR * bufferptr;
  register JSAMPLE *rescale = source->rescale;
  JDIMENSION col;

  if (! ReadOK(source->pub.input_file, source->iobuffer, source->buffer_width))
    ERREXIT(cinfo, JERR_INPUT_EOF);
  ptr = source->pub.buffer[0];
  bufferptr = source->iobuffer;
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr++ = rescale[UCH(*bufferptr++)];
    *ptr++ = rescale[UCH(*bufferptr++)];
    *ptr++ = rescale[UCH(*bufferptr++)];
  }
  return 1;
}


METHODDEF JDIMENSION
get_raw_row (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
/* This version is for reading raw-format files with maxval = MAXJSAMPLE. */
/* In this case we just read right into the JSAMPLE buffer! */
/* Note that same code works for PPM and PGM files. */
{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;

  if (! ReadOK(source->pub.input_file, source->iobuffer, source->buffer_width))
    ERREXIT(cinfo, JERR_INPUT_EOF);
  return 1;
}


/*
 * Read the file header; return image size and component count.
 */

METHODDEF void
start_input_ppm (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  ppm_source_ptr source = (ppm_source_ptr) sinfo;
  int c;
  unsigned int w, h, maxval;
  boolean can_use_raw;

  if (getc(source->pub.input_file) != 'P')
    ERREXIT(cinfo, JERR_PPM_NOT);

  c = getc(source->pub.input_file); /* save format discriminator for a sec */

  /* while we fetch the remaining header info */
  w = read_pbm_integer(cinfo, source->pub.input_file);
  h = read_pbm_integer(cinfo, source->pub.input_file);
  maxval = read_pbm_integer(cinfo, source->pub.input_file);

  if (w <= 0 || h <= 0 || maxval <= 0) /* error check */
    ERREXIT(cinfo, JERR_PPM_NOT);

  cinfo->data_precision = BITS_IN_JSAMPLE; /* we always rescale data to this */
  cinfo->image_width = (JDIMENSION) w;
  cinfo->image_height = (JDIMENSION) h;

  /* Raw PPM/PGM matches JSAMPLE representation iff: */
  can_use_raw = (maxval == MAXJSAMPLE && SIZEOF(JSAMPLE) == SIZEOF(U_CHAR));

  switch (c) {
  case '2':			/* it's a text-format PGM file */
    cinfo->input_components = 1;
    cinfo->in_color_space = JCS_GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC_PGM_TEXT, w, h);
    source->pub.get_pixel_rows = get_text_gray_row;
    can_use_raw = FALSE;	/* force a rescale array to be made */
    break;

  case '3':			/* it's a text-format PPM file */
    cinfo->input_components = 3;
    cinfo->in_color_space = JCS_RGB;
    TRACEMS2(cinfo, 1, JTRC_PPM_TEXT, w, h);
    source->pub.get_pixel_rows = get_text_rgb_row;
    can_use_raw = FALSE;	/* force a rescale array to be made */
    break;

  case '5':			/* it's a raw-format PGM file */
    cinfo->input_components = 1;
    cinfo->in_color_space = JCS_GRAYSCALE;
    TRACEMS2(cinfo, 1, JTRC_PGM, w, h);
    if (can_use_raw)
      source->pub.get_pixel_rows = get_raw_row;
    else
      source->pub.get_pixel_rows = get_scaled_gray_row;
    /* allocate space for I/O buffer: 1 byte/pixel */
    source->iobuffer = (U_CHAR *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  (size_t) (SIZEOF(U_CHAR) * (size_t) w));
    break;

  case '6':			/* it's a raw-format PPM file */
    cinfo->input_components = 3;
    cinfo->in_color_space = JCS_RGB;
    TRACEMS2(cinfo, 1, JTRC_PPM, w, h);
    if (can_use_raw)
      source->pub.get_pixel_rows = get_raw_row;
    else
      source->pub.get_pixel_rows = get_scaled_rgb_row;
    /* allocate space for I/O buffer: 3 bytes/pixel */
    source->iobuffer = (U_CHAR *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  (size_t) (3 * SIZEOF(U_CHAR) * (size_t) w));
    break;

  default:
    ERREXIT(cinfo, JERR_PPM_NOT);
    break;
  }

  /* Create compressor input buffer. */
  source->buffer_width = (JDIMENSION) w * cinfo->input_components;
  if (can_use_raw) {
    /* For unscaled raw-input case, we can just map it onto the I/O buffer. */
    /* Synthesize a JSAMPARRAY pointer structure */
    /* Cast here implies near->far pointer conversion on PCs */
    source->pixrow = (JSAMPROW) source->iobuffer;
    source->pub.buffer = & source->pixrow;
    source->pub.buffer_height = 1;
  } else {
    /* Need to translate anyway, so make a separate sample buffer. */
    source->pub.buffer = (*cinfo->mem->alloc_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE,
       source->buffer_width, (JDIMENSION) 1);
    source->pub.buffer_height = 1;
  }

  /* Compute the rescaling array if required (we use it for all but raw) */
  if (can_use_raw) {
    source->rescale = NULL;	/* no rescaling required */
  } else {
    INT32 val, half_maxval;

    /* On 16-bit-int machines we have to be careful of maxval = 65535 */
    source->rescale = (JSAMPLE *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  (size_t) (((long) maxval + 1L) * SIZEOF(JSAMPLE)));
    half_maxval = maxval / 2;
    for (val = 0; val <= (INT32) maxval; val++) {
      /* The multiplication here must be done in 32 bits to avoid overflow */
      source->rescale[val] = (JSAMPLE) ((val*MAXJSAMPLE + half_maxval)/maxval);
    }
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
finish_input_ppm (j_compress_ptr cinfo, cjpeg_source_ptr sinfo)
{
  /* no work */
}


/*
 * The module selection routine for PPM format input.
 */

GLOBAL cjpeg_source_ptr
jinit_read_ppm (j_compress_ptr cinfo)
{
  ppm_source_ptr source;

  /* Create module interface object */
  source = (ppm_source_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(ppm_source_struct));
  /* Fill in method ptrs, except get_pixel_rows which start_input sets */
  source->pub.start_input = start_input_ppm;
  source->pub.finish_input = finish_input_ppm;

  return (cjpeg_source_ptr) source;
}

#endif /* PPM_SUPPORTED */

/*
 * wrppm.c
 *
 * Copyright (C) 1991-1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write output images in PPM/PGM format.
 * The PBMPLUS library is NOT required to compile this software
 * (but it is highly useful as a set of PPM image manipulation programs).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume output to
 * an ordinary stdio stream.
 */

#include "cdjpeg.h"		/* Common decls for cjpeg/djpeg applications */

#ifdef PPM_SUPPORTED


/*
 * Currently, this code only knows how to write raw PPM or PGM format,
 * which can be no more than 8 bits/sample.  As an expedient for testing
 * 12-bit JPEG mode, we support writing 12-bit data to an 8-bit file by
 * downscaling the values.  Of course this implies loss of precision.
 * (When the core library supports data precision reduction, a cleaner
 * implementation will be to ask for that instead.)
 */

#if BITS_IN_JSAMPLE == 8
#define DOWNSCALE(x)  (x)
#else
#define DOWNSCALE(x)  ((x) >> (BITS_IN_JSAMPLE-8))
#endif


/*
 * When JSAMPLE is the same size as char, we can just fwrite() the
 * decompressed data to the PPM or PGM file.  On PCs, in order to make this
 * work the output buffer must be allocated in near data space, because we are
 * assuming small-data memory model wherein fwrite() can't reach far memory.
 * If you need to process very wide images on a PC, you might have to compile
 * in large-memory model, or else replace fwrite() with a putc() loop ---
 * which will be much slower.
 */


/* Private version of data destination object */

typedef struct {
  struct djpeg_dest_struct pub;	/* public fields */

  /* Usually these two pointers point to the same place: */
  char *iobuffer;		/* fwrite's I/O buffer */
  JSAMPROW pixrow;		/* decompressor output buffer */

  JDIMENSION buffer_width;	/* width of one row */
} ppm_dest_struct;

typedef ppm_dest_struct * ppm_dest_ptr;


/*
 * Write some pixel data.
 * In this module rows_supplied will always be 1.
 *
 * put_pixel_rows handles the "normal" 8-bit case where the decompressor
 * output buffer is physically the same as the fwrite buffer.
 */

METHODDEF void
put_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		JDIMENSION rows_supplied)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;

  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


/*
 * This code is used when we have to copy the data because JSAMPLE is not
 * the same size as char.  Typically this only happens in 12-bit mode.
 */

METHODDEF void
copy_pixel_rows (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		 JDIMENSION rows_supplied)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;
  register char * bufferptr;
  register JSAMPROW ptr;
  register JDIMENSION col;

  ptr = dest->pub.buffer[0];
  bufferptr = dest->iobuffer;
  for (col = dest->buffer_width; col > 0; col--) {
    *bufferptr++ = (char) DOWNSCALE(GETJSAMPLE(*ptr++));
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


/*
 * Write some pixel data when color quantization is in effect.
 * We have to demap the color index values to straight data.
 */

METHODDEF void
put_demapped_rgb (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		  JDIMENSION rows_supplied)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;
  register char * bufferptr;
  register int pixval;
  register JSAMPROW ptr;
  register JSAMPROW color_map0 = cinfo->colormap[0];
  register JSAMPROW color_map1 = cinfo->colormap[1];
  register JSAMPROW color_map2 = cinfo->colormap[2];
  register JDIMENSION col;

  ptr = dest->pub.buffer[0];
  bufferptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    pixval = GETJSAMPLE(*ptr++);
    *bufferptr++ = (char) DOWNSCALE(GETJSAMPLE(color_map0[pixval]));
    *bufferptr++ = (char) DOWNSCALE(GETJSAMPLE(color_map1[pixval]));
    *bufferptr++ = (char) DOWNSCALE(GETJSAMPLE(color_map2[pixval]));
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


METHODDEF void
put_demapped_gray (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo,
		   JDIMENSION rows_supplied)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;
  register char * bufferptr;
  register JSAMPROW ptr;
  register JSAMPROW color_map = cinfo->colormap[0];
  register JDIMENSION col;

  ptr = dest->pub.buffer[0];
  bufferptr = dest->iobuffer;
  for (col = cinfo->output_width; col > 0; col--) {
    *bufferptr++ = (char) DOWNSCALE(GETJSAMPLE(color_map[GETJSAMPLE(*ptr++)]));
  }
  (void) JFWRITE(dest->pub.output_file, dest->iobuffer, dest->buffer_width);
}


/*
 * Startup: write the file header.
 */

METHODDEF void
start_output_ppm (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  ppm_dest_ptr dest = (ppm_dest_ptr) dinfo;

  /* Emit file header */
  switch (cinfo->out_color_space) {
  case JCS_GRAYSCALE:
    /* emit header for raw PGM format */
    fprintf(dest->pub.output_file, "P5\n%ld %ld\n%d\n",
	    (long) cinfo->output_width, (long) cinfo->output_height, 255);
    break;
  case JCS_RGB:
    /* emit header for raw PPM format */
    fprintf(dest->pub.output_file, "P6\n%ld %ld\n%d\n",
	    (long) cinfo->output_width, (long) cinfo->output_height, 255);
    break;
  default:
    ERREXIT(cinfo, JERR_PPM_COLORSPACE);
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
finish_output_ppm (j_decompress_ptr cinfo, djpeg_dest_ptr dinfo)
{
  /* Make sure we wrote the output file OK */
  fflush(dinfo->output_file);
  if (ferror(dinfo->output_file))
    ERREXIT(cinfo, JERR_FILE_WRITE);
}


/*
 * The module selection routine for PPM format output.
 */

GLOBAL djpeg_dest_ptr
jinit_write_ppm (j_decompress_ptr cinfo)
{
  ppm_dest_ptr dest;

  /* Create module interface object, fill in method pointers */
  dest = (ppm_dest_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(ppm_dest_struct));
  dest->pub.start_output = start_output_ppm;
  dest->pub.finish_output = finish_output_ppm;

  /* Calculate output image dimensions so we can allocate space */
  jpeg_calc_output_dimensions(cinfo);

  /* Create physical I/O buffer.  Note we make this near on a PC. */
  dest->buffer_width = cinfo->output_width * cinfo->out_color_components;
  dest->iobuffer = (char *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				(size_t) (dest->buffer_width * SIZEOF(char)));

  if (cinfo->quantize_colors || SIZEOF(JSAMPLE) != SIZEOF(char)) {
    /* When quantizing, we need an output buffer for colormap indexes
     * that's separate from the physical I/O buffer.  We also need a
     * separate buffer if JSAMPLE and char are not the same size.
     */
    dest->pub.buffer = (*cinfo->mem->alloc_sarray)
      ((j_common_ptr) cinfo, JPOOL_IMAGE,
       cinfo->output_width * cinfo->output_components, (JDIMENSION) 1);
    dest->pub.buffer_height = 1;
    if (! cinfo->quantize_colors)
      dest->pub.put_pixel_rows = copy_pixel_rows;
    else if (cinfo->out_color_space == JCS_GRAYSCALE)
      dest->pub.put_pixel_rows = put_demapped_gray;
    else
      dest->pub.put_pixel_rows = put_demapped_rgb;
  } else {
    /* We will fwrite() directly from decompressor output buffer. */
    /* Synthesize a JSAMPARRAY pointer structure */
    /* Cast here implies near->far pointer conversion on PCs */
    dest->pixrow = (JSAMPROW) dest->iobuffer;
    dest->pub.buffer = & dest->pixrow;
    dest->pub.buffer_height = 1;
    dest->pub.put_pixel_rows = put_pixel_rows;
  }

  return (djpeg_dest_ptr) dest;
}

#endif /* PPM_SUPPORTED */

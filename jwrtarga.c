/*
 * jwrtarga.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write output images in Targa format.
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume output to
 * an ordinary stdio stream.
 *
 * These routines are invoked via the methods put_pixel_rows, put_color_map,
 * and output_init/term.
 *
 * Based on code contributed by Lee Daniel Crocker.
 */

#include "jinclude.h"

#ifdef TARGA_SUPPORTED


/*
 * To support 12-bit JPEG data, we'd have to scale output down to 8 bits.
 * This is not yet implemented.
 */

#ifndef EIGHT_BIT_SAMPLES
  Sorry, this code only copes with 8-bit JSAMPLEs. /* deliberate syntax err */
#endif


static JSAMPARRAY color_map;	/* saves color map passed by quantizer */


LOCAL void
write_header (decompress_info_ptr cinfo, int num_colors)
/* Create and write a Targa header */
{
  char targaheader[18];

  /* Set unused fields of header to 0 */
  MEMZERO((void *) targaheader, SIZEOF(targaheader));

  if (num_colors > 0) {
    targaheader[1] = 1;		/* color map type 1 */
    targaheader[5] = (char) (num_colors & 0xFF);
    targaheader[6] = (char) (num_colors >> 8);
    targaheader[7] = 24;	/* 24 bits per cmap entry */
  }

  targaheader[12] = (char) (cinfo->image_width & 0xFF);
  targaheader[13] = (char) (cinfo->image_width >> 8);
  targaheader[14] = (char) (cinfo->image_height & 0xFF);
  targaheader[15] = (char) (cinfo->image_height >> 8);
  targaheader[17] = 0x20;	/* Top-down, non-interlaced */

  if (cinfo->out_color_space == CS_GRAYSCALE) {
    targaheader[2] = 3;		/* image type = uncompressed gray-scale */
    targaheader[16] = 8;	/* bits per pixel */
  } else {			/* must be RGB */
    if (num_colors > 0) {
      targaheader[2] = 1;	/* image type = colormapped RGB */
      targaheader[16] = 8;
    } else {
      targaheader[2] = 2;	/* image type = uncompressed RGB */
      targaheader[16] = 24;
    }
  }

  if (FWRITE(cinfo->output_file, targaheader, 18) != (size_t) 18)
    ERREXIT(cinfo->emethods, "Could not write Targa header");
}


/*
 * Write the file header.
 */

METHODDEF void
output_init (decompress_info_ptr cinfo)
{
  if (cinfo->out_color_space == CS_GRAYSCALE) {
    /* Targa doesn't have a mapped grayscale format, so we will */
    /* demap quantized gray output.  Never emit a colormap. */
    write_header(cinfo, 0);
  } else if (cinfo->out_color_space == CS_RGB) {
    /* For quantized output, defer writing header until put_color_map time. */
    if (! cinfo->quantize_colors)
      write_header(cinfo, 0);
  } else {
    ERREXIT(cinfo->emethods, "Targa output must be grayscale or RGB");
  }
}


/*
 * Write some pixel data.
 */

METHODDEF void
put_pixel_rows (decompress_info_ptr cinfo, int num_rows,
		JSAMPIMAGE pixel_data)
{
  register FILE * outfile = cinfo->output_file;
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  register long width = cinfo->image_width;
  register int row;
  
  if (cinfo->final_out_comps == 1) {
    /* here for grayscale or quantized color output */
    for (row = 0; row < num_rows; row++) {
      ptr0 = pixel_data[0][row];
      for (col = width; col > 0; col--) {
	putc(GETJSAMPLE(*ptr0), outfile);
	ptr0++;
      }
    }
  } else {
    /* here for unquantized color output */
    for (row = 0; row < num_rows; row++) {
      ptr0 = pixel_data[0][row];
      ptr1 = pixel_data[1][row];
      ptr2 = pixel_data[2][row];
      for (col = width; col > 0; col--) {
	putc(GETJSAMPLE(*ptr2), outfile); /* write in BGR order */
	ptr2++;
	putc(GETJSAMPLE(*ptr1), outfile);
	ptr1++;
	putc(GETJSAMPLE(*ptr0), outfile);
	ptr0++;
      }
    }
  }
}


/*
 * Write some demapped pixel data when color quantization is in effect.
 * For Targa, this is only applied to grayscale data.
 */

METHODDEF void
put_demapped_rows (decompress_info_ptr cinfo, int num_rows,
		   JSAMPIMAGE pixel_data)
{
  register FILE * outfile = cinfo->output_file;
  register JSAMPROW ptr;
  register long col;
  register long width = cinfo->image_width;
  register int row;
  
  for (row = 0; row < num_rows; row++) {
    ptr = pixel_data[0][row];
    for (col = width; col > 0; col--) {
      putc(GETJSAMPLE(color_map[0][GETJSAMPLE(*ptr)]), outfile);
      ptr++;
    }
  }
}


/*
 * Write the color map.
 */

METHODDEF void
put_color_map (decompress_info_ptr cinfo, int num_colors, JSAMPARRAY colormap)
{
  register FILE * outfile = cinfo->output_file;
  int i;

  if (cinfo->out_color_space == CS_RGB) {
    /* We only support 8-bit colormap indexes, so only 256 colors */
    if (num_colors > 256)
      ERREXIT(cinfo->emethods, "Too many colors for Targa output");
    /* Time to write the header */
    write_header(cinfo, num_colors);
    /* Write the colormap.  Note Targa uses BGR byte order */
    for (i = 0; i < num_colors; i++) {
      putc(GETJSAMPLE(colormap[2][i]), outfile);
      putc(GETJSAMPLE(colormap[1][i]), outfile);
      putc(GETJSAMPLE(colormap[0][i]), outfile);
    }
  } else {
    color_map = colormap;	/* save for use in output */
    cinfo->methods->put_pixel_rows = put_demapped_rows;
  }
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
output_term (decompress_info_ptr cinfo)
{
  /* No work except to make sure we wrote the output file OK */
  fflush(cinfo->output_file);
  if (ferror(cinfo->output_file))
    ERREXIT(cinfo->emethods, "Output file write error");
}


/*
 * The method selection routine for Targa format output.
 * This should be called from d_ui_method_selection if Targa output is wanted.
 */

GLOBAL void
jselwtarga (decompress_info_ptr cinfo)
{
  cinfo->methods->output_init = output_init;
  cinfo->methods->put_color_map = put_color_map;
  cinfo->methods->put_pixel_rows = put_pixel_rows;
  cinfo->methods->output_term = output_term;
}

#endif /* TARGA_SUPPORTED */

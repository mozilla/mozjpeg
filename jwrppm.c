/*
 * jwrppm.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write output images in PPM format.
 * The PBMPLUS library is required (well, it will be in the real version).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume output to
 * an ordinary stdio stream.
 *
 * These routines are invoked via the methods put_pixel_rows, put_color_map,
 * and output_init/term.
 */

#include "jinclude.h"

#ifdef PPM_SUPPORTED


static JSAMPARRAY color_map;	/* saves color map passed by quantizer */


/*
 * Write the file header.
 */

METHODDEF void
output_init (decompress_info_ptr cinfo)
{
  if (cinfo->out_color_space == CS_GRAYSCALE) {
    /* emit header for raw PGM format */
    fprintf(cinfo->output_file, "P5\n%ld %ld\n%d\n",
	    cinfo->image_width, cinfo->image_height, 255);
  } else if (cinfo->out_color_space == CS_RGB) {
    /* emit header for raw PPM format */
    fprintf(cinfo->output_file, "P6\n%ld %ld\n%d\n",
	    cinfo->image_width, cinfo->image_height, 255);
  } else {
    ERREXIT(cinfo->emethods, "PPM output must be grayscale or RGB");
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
  
  if (cinfo->out_color_space == CS_GRAYSCALE) {
    for (row = 0; row < num_rows; row++) {
      ptr0 = pixel_data[0][row];
      for (col = width; col > 0; col--) {
	putc(GETJSAMPLE(*ptr0), outfile);
	ptr0++;
      }
    }
  } else {
    for (row = 0; row < num_rows; row++) {
      ptr0 = pixel_data[0][row];
      ptr1 = pixel_data[1][row];
      ptr2 = pixel_data[2][row];
      for (col = width; col > 0; col--) {
	putc(GETJSAMPLE(*ptr0), outfile);
	ptr0++;
	putc(GETJSAMPLE(*ptr1), outfile);
	ptr1++;
	putc(GETJSAMPLE(*ptr2), outfile);
	ptr2++;
      }
    }
  }
}


/*
 * Write some pixel data when color quantization is in effect.
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
  
  if (cinfo->out_color_space == CS_GRAYSCALE) {
    for (row = 0; row < num_rows; row++) {
      ptr = pixel_data[0][row];
      for (col = width; col > 0; col--) {
	putc(GETJSAMPLE(color_map[0][GETJSAMPLE(*ptr)]), outfile);
	ptr++;
      }
    }
  } else {
    for (row = 0; row < num_rows; row++) {
      ptr = pixel_data[0][row];
      for (col = width; col > 0; col--) {
	register int pixval = GETJSAMPLE(*ptr);

	putc(GETJSAMPLE(color_map[0][pixval]), outfile);
	putc(GETJSAMPLE(color_map[1][pixval]), outfile);
	putc(GETJSAMPLE(color_map[2][pixval]), outfile);
	ptr++;
      }
    }
  }
}


/*
 * Write the color map.
 * For PPM output, we just demap the output data!
 */

METHODDEF void
put_color_map (decompress_info_ptr cinfo, int num_colors, JSAMPARRAY colormap)
{
  color_map = colormap;		/* save for use in output */
  cinfo->methods->put_pixel_rows = put_demapped_rows;
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
 * The method selection routine for PPM format output.
 * This should be called from d_ui_method_selection if PPM output is wanted.
 */

GLOBAL void
jselwppm (decompress_info_ptr cinfo)
{
  cinfo->methods->output_init = output_init;
  cinfo->methods->put_color_map = put_color_map;
  cinfo->methods->put_pixel_rows = put_pixel_rows;
  cinfo->methods->output_term = output_term;
}

#endif /* PPM_SUPPORTED */

/*
 * jrdrle.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in Utah RLE format.
 * The Utah Raster Toolkit library is required (version 3.0).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume input from
 * an ordinary stdio stream.  They further assume that reading begins
 * at the start of the file; input_init may need work if the
 * user interface has already read some data (e.g., to determine that
 * the file is indeed RLE format).
 *
 * These routines are invoked via the methods get_input_row
 * and input_init/term.
 *
 * Based on code contributed by Mike Lijewski.
 */

#include "jinclude.h"

#ifdef RLE_SUPPORTED

/* rle.h is provided by the Utah Raster Toolkit. */

#include <rle.h>


/*
 * load_image assumes that JSAMPLE has the same representation as rle_pixel,
 * to wit, "unsigned char".  Hence we can't cope with 12- or 16-bit samples.
 */

#ifndef EIGHT_BIT_SAMPLES
  Sorry, this code only copes with 8-bit JSAMPLEs. /* deliberate syntax err */
#endif


/*
 * We support the following types of RLE files:
 *   
 *   GRAYSCALE   - 8 bits, no colormap
 *   PSEUDOCOLOR - 8 bits, colormap
 *   TRUECOLOR   - 24 bits, colormap
 *   DIRECTCOLOR - 24 bits, no colormap
 *
 * For now, we ignore any alpha channel in the image.
 */

typedef enum { GRAYSCALE, PSEUDOCOLOR, TRUECOLOR, DIRECTCOLOR } rle_kind;

static rle_kind visual;		/* actual type of input file */

/*
 * Since RLE stores scanlines bottom-to-top, we have to invert the image
 * to conform to JPEG's top-to-bottom order.  To do this, we read the
 * incoming image into a virtual array on the first get_input_row call,
 * then fetch the required row from the virtual array on subsequent calls.
 */

static big_sarray_ptr image;	/* single array for GRAYSCALE/PSEUDOCOLOR */
static big_sarray_ptr red_channel; /* three arrays for TRUECOLOR/DIRECTCOLOR */
static big_sarray_ptr green_channel;
static big_sarray_ptr blue_channel;
static long cur_row_number;	/* last row# read from virtual array */

static rle_hdr header;		/* Input file information */
static rle_map *colormap;	/* RLE colormap, if any */


/*
 * Read the file header; return image size and component count.
 */

METHODDEF void
input_init (compress_info_ptr cinfo)
{
  long width, height;

  /* Use RLE library routine to get the header info */
  header.rle_file = cinfo->input_file;
  switch (rle_get_setup(&header)) {
  case RLE_SUCCESS:
    /* A-OK */
    break;
  case RLE_NOT_RLE:
    ERREXIT(cinfo->emethods, "Not an RLE file");
    break;
  case RLE_NO_SPACE:
    ERREXIT(cinfo->emethods, "Insufficient memory for RLE header");
    break;
  case RLE_EMPTY:
    ERREXIT(cinfo->emethods, "Empty RLE file");
    break;
  case RLE_EOF:
    ERREXIT(cinfo->emethods, "Premature EOF in RLE header");
    break;
  default:
    ERREXIT(cinfo->emethods, "Bogus RLE error code");
    break;
  }

  /* Figure out what we have, set private vars and return values accordingly */
  
  width  = header.xmax - header.xmin + 1;
  height = header.ymax - header.ymin + 1;
  header.xmin = 0;		/* realign horizontally */
  header.xmax = width-1;

  cinfo->image_width      = width;
  cinfo->image_height     = height;
  cinfo->data_precision   = 8;  /* we can only handle 8 bit data */

  if (header.ncolors == 1 && header.ncmap == 0) {
    visual     = GRAYSCALE;
    TRACEMS(cinfo->emethods, 1, "Gray-scale RLE file");
  } else if (header.ncolors == 1 && header.ncmap == 3) {
    visual     = PSEUDOCOLOR;
    colormap   = header.cmap;
    TRACEMS1(cinfo->emethods, 1, "Colormapped RLE file with map of length %d",
	     1 << header.cmaplen);
  } else if (header.ncolors == 3 && header.ncmap == 3) {
    visual     = TRUECOLOR;
    colormap   = header.cmap;
    TRACEMS1(cinfo->emethods, 1, "Full-color RLE file with map of length %d",
	     1 << header.cmaplen);
  } else if (header.ncolors == 3 && header.ncmap == 0) {
    visual     = DIRECTCOLOR;
    TRACEMS(cinfo->emethods, 1, "Full-color RLE file");
  } else
    ERREXIT(cinfo->emethods, "Can't handle this RLE setup");
  
  switch (visual) {
  case GRAYSCALE:
    /* request one big array to hold the grayscale image */
    image = (*cinfo->emethods->request_big_sarray) (width, height, 1L);
    cinfo->in_color_space   = CS_GRAYSCALE;
    cinfo->input_components = 1;
    break;
  case PSEUDOCOLOR:
    /* request one big array to hold the pseudocolor image */
    image = (*cinfo->emethods->request_big_sarray) (width, height, 1L);
    cinfo->in_color_space   = CS_RGB;
    cinfo->input_components = 3;
    break;
  case TRUECOLOR:
  case DIRECTCOLOR:
    /* request three big arrays to hold the RGB channels */
    red_channel   = (*cinfo->emethods->request_big_sarray) (width, height, 1L);
    green_channel = (*cinfo->emethods->request_big_sarray) (width, height, 1L);
    blue_channel  = (*cinfo->emethods->request_big_sarray) (width, height, 1L);
    cinfo->in_color_space   = CS_RGB;
    cinfo->input_components = 3;
    break;
  }

  cinfo->total_passes++;	/* count file reading as separate pass */
}


/*
 * Read one row of pixels.
 * These are called only after load_image has read the image into
 * the virtual array(s).
 */


METHODDEF void
get_grayscale_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This is used for GRAYSCALE images */
{
  JSAMPROW inputrows[1];	/* a pseudo JSAMPARRAY structure */

  cur_row_number--;		/* work down in array */
  
  inputrows[0] = *((*cinfo->emethods->access_big_sarray)
			(image, cur_row_number, FALSE));

  jcopy_sample_rows(inputrows, 0, pixel_row, 0, 1, cinfo->image_width);
}


METHODDEF void
get_pseudocolor_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This is used for PSEUDOCOLOR images */
{
  long col;
  JSAMPROW image_ptr, ptr0, ptr1, ptr2;
  int val;

  cur_row_number--;		/* work down in array */
  
  image_ptr = *((*cinfo->emethods->access_big_sarray)
		(image, cur_row_number, FALSE));

  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  
  for (col = cinfo->image_width; col > 0; col--) {
    val = GETJSAMPLE(*image_ptr++);
    *ptr0++ = colormap[val      ] >> 8;
    *ptr1++ = colormap[val + 256] >> 8;
    *ptr2++ = colormap[val + 512] >> 8;
  }
}


METHODDEF void
get_truecolor_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This is used for TRUECOLOR images */
/* The colormap consists of 3 independent lookup tables */
{
  long col;
  JSAMPROW red_ptr, green_ptr, blue_ptr, ptr0, ptr1, ptr2;
  
  cur_row_number--;		/* work down in array */
  
  red_ptr   = *((*cinfo->emethods->access_big_sarray)
		(red_channel, cur_row_number, FALSE));
  green_ptr = *((*cinfo->emethods->access_big_sarray)
		(green_channel, cur_row_number, FALSE));
  blue_ptr  = *((*cinfo->emethods->access_big_sarray)
		(blue_channel, cur_row_number, FALSE));
  
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  
  for (col = cinfo->image_width; col > 0; col--) {
    *ptr0++ = colormap[GETJSAMPLE(*red_ptr++)        ] >> 8;
    *ptr1++ = colormap[GETJSAMPLE(*green_ptr++) + 256] >> 8;
    *ptr2++ = colormap[GETJSAMPLE(*blue_ptr++)  + 512] >> 8;
  }
}


METHODDEF void
get_directcolor_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This is used for DIRECTCOLOR images */
{
  JSAMPROW inputrows[3];	/* a pseudo JSAMPARRAY structure */

  cur_row_number--;		/* work down in array */
  
  inputrows[0] = *((*cinfo->emethods->access_big_sarray)
			(red_channel, cur_row_number, FALSE));
  inputrows[1] = *((*cinfo->emethods->access_big_sarray)
			(green_channel, cur_row_number, FALSE));
  inputrows[2] = *((*cinfo->emethods->access_big_sarray)
			(blue_channel, cur_row_number, FALSE));

  jcopy_sample_rows(inputrows, 0, pixel_row, 0, 3, cinfo->image_width);
}


/*
 * Load the color channels into separate arrays.  We have to do
 * this because RLE files start at the lower left while the JPEG standard
 * has them starting in the upper left.  This is called the first time
 * we want to get a row of input.  What we do is load the RLE data into
 * big arrays and then call the appropriate routine to read one row from
 * the big arrays.  We also change cinfo->methods->get_input_row so that
 * subsequent calls go straight to the row-reading routine.
 */

METHODDEF void
load_image (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
{
  long row;
  rle_pixel *rle_row[3];
  
  /* Read the RLE data into our virtual array(s).
   * We assume here that (a) rle_pixel is represented the same as JSAMPLE,
   * and (b) we are not on a machine where FAR pointers differ from regular.
   */
  RLE_CLR_BIT(header, RLE_ALPHA); /* don't read the alpha channel */

  switch (visual) {
  case GRAYSCALE:
  case PSEUDOCOLOR:
    for (row = 0; row < cinfo->image_height; row++) {
      (*cinfo->methods->progress_monitor) (cinfo, row, cinfo->image_height);
      /*
       * Read a row of the image directly into our big array.
       * Too bad this doesn't seem to return any indication of errors :-(.
       */
      rle_row[0] = (rle_pixel *) *((*cinfo->emethods->access_big_sarray)
					(image, row, TRUE));
      rle_getrow(&header, rle_row);
    }
    break;
  case TRUECOLOR:
  case DIRECTCOLOR:
    for (row = 0; row < cinfo->image_height; row++) {
      (*cinfo->methods->progress_monitor) (cinfo, row, cinfo->image_height);
      /*
       * Read a row of the image directly into our big arrays.
       * Too bad this doesn't seem to return any indication of errors :-(.
       */
      rle_row[0] = (rle_pixel *) *((*cinfo->emethods->access_big_sarray)
					(red_channel, row, TRUE));
      rle_row[1] = (rle_pixel *) *((*cinfo->emethods->access_big_sarray)
					(green_channel, row, TRUE));
      rle_row[2] = (rle_pixel *) *((*cinfo->emethods->access_big_sarray)
					(blue_channel, row, TRUE));
      rle_getrow(&header, rle_row);
    }
    break;
  }
  cinfo->completed_passes++;
  
  /* Set up to call proper row-extraction routine in future */
  switch (visual) {
  case GRAYSCALE:
    cinfo->methods->get_input_row = get_grayscale_row;
    break;
  case PSEUDOCOLOR:
    cinfo->methods->get_input_row = get_pseudocolor_row;
    break;
  case TRUECOLOR:
    cinfo->methods->get_input_row = get_truecolor_row;
    break;
  case DIRECTCOLOR:
    cinfo->methods->get_input_row = get_directcolor_row;
    break;
  }

  /* And fetch the topmost (bottommost) row */
  cur_row_number = cinfo->image_height;
  (*cinfo->methods->get_input_row) (cinfo, pixel_row);   
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
 * The method selection routine for RLE format input.
 * Note that this must be called by the user interface before calling
 * jpeg_compress.  If multiple input formats are supported, the
 * user interface is responsible for discovering the file format and
 * calling the appropriate method selection routine.
 */

GLOBAL void
jselrrle (compress_info_ptr cinfo)
{
  cinfo->methods->input_init    = input_init;
  cinfo->methods->get_input_row = load_image; /* until first call */
  cinfo->methods->input_term    = input_term;
}

#endif /* RLE_SUPPORTED */

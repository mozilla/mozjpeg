/*
 * jwrrle.c
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write output images in RLE format.
 * The Utah Raster Toolkit library is required (version 3.0).
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume output to
 * an ordinary stdio stream.
 *
 * These routines are invoked via the methods put_pixel_rows, put_color_map,
 * and output_init/term.
 *
 * Based on code contributed by Mike Lijewski.
 */

#include "jinclude.h"

#ifdef RLE_SUPPORTED

/* rle.h is provided by the Utah Raster Toolkit. */

#include <rle.h>


/*
 * output_term assumes that JSAMPLE has the same representation as rle_pixel,
 * to wit, "unsigned char".  Hence we can't cope with 12- or 16-bit samples.
 */

#ifndef EIGHT_BIT_SAMPLES
  Sorry, this code only copes with 8-bit JSAMPLEs. /* deliberate syntax err */
#endif


/*
 * Since RLE stores scanlines bottom-to-top, we have to invert the image
 * from JPEG's top-to-bottom order.  To do this, we save the outgoing data
 * in virtual array(s) during put_pixel_row calls, then actually emit the
 * RLE file during output_term.  We use one virtual array if the output is
 * grayscale or colormapped, more if it is full color.
 */

#define MAX_CHANS	4	/* allow up to four color components */
static big_sarray_ptr channels[MAX_CHANS]; /* Virtual arrays for saved data */

static long cur_output_row;	/* next row# to write to virtual array(s) */


/*
 * For now, if we emit an RLE color map then it is always 256 entries long,
 * though not all of the entries need be used.
 */

#define CMAPBITS	8
#define CMAPLENGTH	(1<<(CMAPBITS))

static rle_map *output_colormap; /* RLE-style color map, or NULL if none */
static int number_colors;	/* Number of colors actually used */


/*
 * Write the file header.
 *
 * In this module it's easier to wait till output_term to actually write
 * anything; here we just request the big arrays we'll need.
 */

METHODDEF void
output_init (decompress_info_ptr cinfo)
{
  short ci;
  
  if (cinfo->final_out_comps > MAX_CHANS)
    ERREXIT1(cinfo->emethods, "Cannot handle %d output channels for RLE",
	     cinfo->final_out_comps);
  
  for (ci = 0; ci < cinfo->final_out_comps; ci++) {
    channels[ci] = (*cinfo->emethods->request_big_sarray)
			(cinfo->image_width, cinfo->image_height, 1L);
  }
  
  output_colormap = NULL;	/* No output colormap as yet */
  number_colors = 0;
  cur_output_row = 0;		/* Start filling virtual arrays at row 0 */

  cinfo->total_passes++;	/* count file writing as separate pass */
}


/*
 * Write some pixel data.
 *
 * This routine just saves the data away in virtual arrays.
 */

METHODDEF void
put_pixel_rows (decompress_info_ptr cinfo, int num_rows,
		JSAMPIMAGE pixel_data)
{
  JSAMPROW outputrow[1];	/* a pseudo JSAMPARRAY structure */
  int row;
  short ci;
  
  for (row = 0; row < num_rows; row++) {
    for (ci = 0; ci < cinfo->final_out_comps; ci++) {
      outputrow[0] = *((*cinfo->emethods->access_big_sarray)
			(channels[ci], cur_output_row, TRUE));
      jcopy_sample_rows(pixel_data[ci], row, outputrow, 0,
			1, cinfo->image_width);
    }
    cur_output_row++;
  }
}


/*
 * Write the color map.
 *
 *  For RLE output we just save the colormap for the output stage.
 */

METHODDEF void
put_color_map (decompress_info_ptr cinfo, int num_colors, JSAMPARRAY colormap)
{
  size_t cmapsize;
  short ci;
  int i;

  if (num_colors > CMAPLENGTH)
    ERREXIT1(cinfo->emethods, "Cannot handle %d colormap entries for RLE",
	     num_colors);

  /* Allocate storage for RLE-style cmap, zero any extra entries */
  cmapsize = cinfo->color_out_comps * CMAPLENGTH * SIZEOF(rle_map);
  output_colormap = (rle_map *) (*cinfo->emethods->alloc_small) (cmapsize);
  MEMZERO(output_colormap, cmapsize);

  /* Save away data in RLE format --- note 8-bit left shift! */
  /* Shifting would need adjustment for JSAMPLEs wider than 8 bits. */
  for (ci = 0; ci < cinfo->color_out_comps; ci++) {
    for (i = 0; i < num_colors; i++) {
      output_colormap[ci * CMAPLENGTH + i] = GETJSAMPLE(colormap[ci][i]) << 8;
    }
  }
  number_colors = num_colors;
}


/*
 * Finish up at the end of the file.
 *
 * Here is where we really output the RLE file.
 */

METHODDEF void
output_term (decompress_info_ptr cinfo)
{
  rle_hdr header;		/* Output file information */
  rle_pixel *output_rows[MAX_CHANS];
  char cmapcomment[80];
  short ci;
  long row;

  /* Initialize the header info */
  MEMZERO(&header, SIZEOF(rle_hdr)); /* make sure all bits are 0 */
  header.rle_file = cinfo->output_file;
  header.xmin     = 0;
  header.xmax     = cinfo->image_width  - 1;
  header.ymin     = 0;
  header.ymax     = cinfo->image_height - 1;
  header.alpha    = 0;
  header.ncolors  = cinfo->final_out_comps;
  for (ci = 0; ci < cinfo->final_out_comps; ci++) {
    RLE_SET_BIT(header, ci);
  }
  if (number_colors > 0) {
    header.ncmap   = cinfo->color_out_comps;
    header.cmaplen = CMAPBITS;
    header.cmap    = output_colormap;
    /* Add a comment to the output image with the true colormap length. */
    sprintf(cmapcomment, "color_map_length=%d", number_colors);
    rle_putcom(cmapcomment, &header);
  }
  /* Emit the RLE header and color map (if any) */
  rle_put_setup(&header);

  /* Now output the RLE data from our virtual array(s).
   * We assume here that (a) rle_pixel is represented the same as JSAMPLE,
   * and (b) we are not on a machine where FAR pointers differ from regular.
   */
  for (row = cinfo->image_height-1; row >= 0; row--) {
    (*cinfo->methods->progress_monitor) (cinfo, cinfo->image_height-row-1,
					 cinfo->image_height);
    for (ci = 0; ci < cinfo->final_out_comps; ci++) {
      output_rows[ci] = (rle_pixel *) *((*cinfo->emethods->access_big_sarray)
					(channels[ci], row, FALSE));
    }
    rle_putrow(output_rows, (int) cinfo->image_width, &header);
  }
  cinfo->completed_passes++;

  /* Emit file trailer */
  rle_puteof(&header);
  fflush(cinfo->output_file);
  if (ferror(cinfo->output_file))
    ERREXIT(cinfo->emethods, "Output file write error --- out of disk space?");

  /* Release memory */
  /* no work (we let free_all release the workspace) */
}


/*
 * The method selection routine for RLE format output.
 * This should be called from d_ui_method_selection if RLE output is wanted.
 */

GLOBAL void
jselwrle (decompress_info_ptr cinfo)
{
  cinfo->methods->output_init    = output_init;
  cinfo->methods->put_color_map  = put_color_map;
  cinfo->methods->put_pixel_rows = put_pixel_rows;
  cinfo->methods->output_term    = output_term;
}

#endif /* RLE_SUPPORTED */

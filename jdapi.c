/*
 * jdapi.c
 *
 * Copyright (C) 1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains application interface code for the decompression half of
 * the JPEG library.  Most of the routines intended to be called directly by
 * an application are in this file.  But also see jcomapi.c for routines
 * shared by compression and decompression.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/*
 * Initialization of a JPEG decompression object.
 * The error manager must already be set up (in case memory manager fails).
 */

GLOBAL void
jpeg_create_decompress (j_decompress_ptr cinfo)
{
  int i;

  /* For debugging purposes, zero the whole master structure.
   * But error manager pointer is already there, so save and restore it.
   */
  {
    struct jpeg_error_mgr * err = cinfo->err;
    MEMZERO(cinfo, SIZEOF(struct jpeg_decompress_struct));
    cinfo->err = err;
  }
  cinfo->is_decompressor = TRUE;

  /* Initialize a memory manager instance for this object */
  jinit_memory_mgr((j_common_ptr) cinfo);

  /* Zero out pointers to permanent structures. */
  cinfo->progress = NULL;
  cinfo->src = NULL;

  for (i = 0; i < NUM_QUANT_TBLS; i++)
    cinfo->quant_tbl_ptrs[i] = NULL;

  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    cinfo->dc_huff_tbl_ptrs[i] = NULL;
    cinfo->ac_huff_tbl_ptrs[i] = NULL;
  }

  cinfo->sample_range_limit = NULL;

  /* Initialize marker processor so application can override methods
   * for COM, APPn markers before calling jpeg_read_header.
   */
  cinfo->marker = NULL;
  jinit_marker_reader(cinfo);

  /* OK, I'm ready */
  cinfo->global_state = DSTATE_START;
}


/*
 * Destruction of a JPEG decompression object
 */

GLOBAL void
jpeg_destroy_decompress (j_decompress_ptr cinfo)
{
  jpeg_destroy((j_common_ptr) cinfo); /* use common routine */
}


/*
 * Install a special processing method for COM or APPn markers.
 */

GLOBAL void
jpeg_set_marker_processor (j_decompress_ptr cinfo, int marker_code,
			   jpeg_marker_parser_method routine)
{
  if (marker_code == JPEG_COM)
    cinfo->marker->process_COM = routine;
  else if (marker_code >= JPEG_APP0 && marker_code <= JPEG_APP0+15)
    cinfo->marker->process_APPn[marker_code-JPEG_APP0] = routine;
  else
    ERREXIT1(cinfo, JERR_UNKNOWN_MARKER, marker_code);
}


/*
 * Set default decompression parameters.
 */

LOCAL void
default_decompress_parms (j_decompress_ptr cinfo)
{
  /* Guess the input colorspace, and set output colorspace accordingly. */
  /* (Wish JPEG committee had provided a real way to specify this...) */
  /* Note application may override our guesses. */
  switch (cinfo->num_components) {
  case 1:
    cinfo->jpeg_color_space = JCS_GRAYSCALE;
    cinfo->out_color_space = JCS_GRAYSCALE;
    break;
    
  case 3:
    if (cinfo->saw_JFIF_marker) {
      cinfo->jpeg_color_space = JCS_YCbCr; /* JFIF implies YCbCr */
    } else if (cinfo->saw_Adobe_marker) {
      switch (cinfo->Adobe_transform) {
      case 0:
	cinfo->jpeg_color_space = JCS_RGB;
	break;
      case 1:
	cinfo->jpeg_color_space = JCS_YCbCr;
	break;
      default:
	WARNMS1(cinfo, JWRN_ADOBE_XFORM, cinfo->Adobe_transform);
	cinfo->jpeg_color_space = JCS_YCbCr; /* assume it's YCbCr */
	break;
      }
    } else {
      /* Saw no special markers, try to guess from the component IDs */
      int cid0 = cinfo->comp_info[0].component_id;
      int cid1 = cinfo->comp_info[1].component_id;
      int cid2 = cinfo->comp_info[2].component_id;

      if (cid0 == 1 && cid1 == 2 && cid2 == 3)
	cinfo->jpeg_color_space = JCS_YCbCr; /* assume JFIF w/out marker */
      else if (cid0 == 82 && cid1 == 71 && cid2 == 66)
	cinfo->jpeg_color_space = JCS_RGB; /* ASCII 'R', 'G', 'B' */
      else {
	TRACEMS3(cinfo, 1, JTRC_UNKNOWN_IDS, cid0, cid1, cid2);
	cinfo->jpeg_color_space = JCS_YCbCr; /* assume it's YCbCr */
      }
    }
    /* Always guess RGB is proper output colorspace. */
    cinfo->out_color_space = JCS_RGB;
    break;
    
  case 4:
    if (cinfo->saw_Adobe_marker) {
      switch (cinfo->Adobe_transform) {
      case 0:
	cinfo->jpeg_color_space = JCS_CMYK;
	break;
      case 2:
	cinfo->jpeg_color_space = JCS_YCCK;
	break;
      default:
	WARNMS1(cinfo, JWRN_ADOBE_XFORM, cinfo->Adobe_transform);
	cinfo->jpeg_color_space = JCS_YCCK; /* assume it's YCCK */
	break;
      }
    } else {
      /* No special markers, assume straight CMYK. */
      cinfo->jpeg_color_space = JCS_CMYK;
    }
    cinfo->out_color_space = JCS_CMYK;
    break;
    
  default:
    cinfo->jpeg_color_space = JCS_UNKNOWN;
    cinfo->out_color_space = JCS_UNKNOWN;
    break;
  }

  /* Set defaults for other decompression parameters. */
  cinfo->scale_num = 1;		/* 1:1 scaling */
  cinfo->scale_denom = 1;
  cinfo->output_gamma = 1.0;
  cinfo->raw_data_out = FALSE;
  cinfo->quantize_colors = FALSE;
  /* We set these in case application only sets quantize_colors. */
  cinfo->two_pass_quantize = TRUE;
  cinfo->dither_mode = JDITHER_FS;
  cinfo->desired_number_of_colors = 256;
  cinfo->colormap = NULL;
  /* DCT algorithm preference */
  cinfo->dct_method = JDCT_DEFAULT;
  cinfo->do_fancy_upsampling = TRUE;
}


/*
 * Decompression startup: read start of JPEG datastream to see what's there.
 * Need only initialize JPEG object and supply a data source before calling.
 *
 * This routine will read as far as the first SOS marker (ie, actual start of
 * compressed data), and will save all tables and parameters in the JPEG
 * object.  It will also initialize the decompression parameters to default
 * values, and finally return JPEG_HEADER_OK.  On return, the application may
 * adjust the decompression parameters and then call jpeg_start_decompress.
 * (Or, if the application only wanted to determine the image parameters,
 * the data need not be decompressed.  In that case, call jpeg_abort or
 * jpeg_destroy to release any temporary space.)
 * If an abbreviated (tables only) datastream is presented, the routine will
 * return JPEG_HEADER_TABLES_ONLY upon reaching EOI.  The application may then
 * re-use the JPEG object to read the abbreviated image datastream(s).
 * It is unnecessary (but OK) to call jpeg_abort in this case.
 * The JPEG_SUSPENDED return code only occurs if the data source module
 * requests suspension of the decompressor.  In this case the application
 * should load more source data and then re-call jpeg_read_header to resume
 * processing.
 * If a non-suspending data source is used and require_image is TRUE, then the
 * return code need not be inspected since only JPEG_HEADER_OK is possible.
 */

GLOBAL int
jpeg_read_header (j_decompress_ptr cinfo, boolean require_image)
{
  int retcode;

  if (cinfo->global_state == DSTATE_START) {
    /* First-time actions: reset appropriate modules */
    (*cinfo->err->reset_error_mgr) ((j_common_ptr) cinfo);
    (*cinfo->marker->reset_marker_reader) (cinfo);
    (*cinfo->src->init_source) (cinfo);
    cinfo->global_state = DSTATE_INHEADER;
  } else if (cinfo->global_state != DSTATE_INHEADER) {
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  }

  retcode = (*cinfo->marker->read_markers) (cinfo);

  switch (retcode) {
  case JPEG_HEADER_OK:		/* Found SOS, prepare to decompress */
    /* Set up default parameters based on header data */
    default_decompress_parms(cinfo);
    /* Set global state: ready for start_decompress */
    cinfo->global_state = DSTATE_READY;
    break;

  case JPEG_HEADER_TABLES_ONLY:	/* Found EOI before any SOS */
    if (cinfo->marker->saw_SOF)
      ERREXIT(cinfo, JERR_SOF_NO_SOS);
    if (require_image)		/* Complain if application wants an image */
      ERREXIT(cinfo, JERR_NO_IMAGE);
    /* We need not do any cleanup since only permanent storage (for DQT, DHT)
     * has been allocated.
     */
    /* Set global state: ready for a new datastream */
    cinfo->global_state = DSTATE_START;
    break;

  case JPEG_SUSPENDED:		/* Had to suspend before end of headers */
    /* no work */
    break;
  }

  return retcode;
}


/*
 * Decompression initialization.
 * jpeg_read_header must be completed before calling this.
 *
 * If a multipass operating mode was selected, this will do all but the
 * last pass, and thus may take a great deal of time.
 */

GLOBAL void
jpeg_start_decompress (j_decompress_ptr cinfo)
{
  JDIMENSION chunk_ctr, last_chunk_ctr;

  if (cinfo->global_state != DSTATE_READY)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  /* Perform master selection of active modules */
  jinit_master_decompress(cinfo);
  /* Do all but the final (output) pass, and set up for that one. */
  for (;;) {
    (*cinfo->master->prepare_for_pass) (cinfo);
    if (cinfo->master->is_last_pass)
      break;
    chunk_ctr = 0;
    while (chunk_ctr < cinfo->main->num_chunks) {
      /* Call progress monitor hook if present */
      if (cinfo->progress != NULL) {
	cinfo->progress->pass_counter = (long) chunk_ctr;
	cinfo->progress->pass_limit = (long) cinfo->main->num_chunks;
	(*cinfo->progress->progress_monitor) ((j_common_ptr) cinfo);
      }
      /* Process some data */
      last_chunk_ctr = chunk_ctr;
      (*cinfo->main->process_data) (cinfo, (JSAMPARRAY) NULL,
				    &chunk_ctr, (JDIMENSION) 0);
      if (chunk_ctr == last_chunk_ctr) /* check for failure to make progress */
	ERREXIT(cinfo, JERR_CANT_SUSPEND);
    }
    (*cinfo->master->finish_pass) (cinfo);
  }
  /* Ready for application to drive last pass through jpeg_read_scanlines
   * or jpeg_read_raw_data.
   */
  cinfo->output_scanline = 0;
  cinfo->global_state = (cinfo->raw_data_out ? DSTATE_RAW_OK : DSTATE_SCANNING);
}


/*
 * Read some scanlines of data from the JPEG decompressor.
 *
 * The return value will be the number of lines actually read.
 * This may be less than the number requested in several cases,
 * including bottom of image, data source suspension, and operating
 * modes that emit multiple scanlines at a time.
 *
 * Note: we warn about excess calls to jpeg_read_scanlines() since
 * this likely signals an application programmer error.  However,
 * an oversize buffer (max_lines > scanlines remaining) is not an error.
 */

GLOBAL JDIMENSION
jpeg_read_scanlines (j_decompress_ptr cinfo, JSAMPARRAY scanlines,
		     JDIMENSION max_lines)
{
  JDIMENSION row_ctr;

  if (cinfo->global_state != DSTATE_SCANNING)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  if (cinfo->output_scanline >= cinfo->output_height)
    WARNMS(cinfo, JWRN_TOO_MUCH_DATA);

  /* Call progress monitor hook if present */
  if (cinfo->progress != NULL) {
    cinfo->progress->pass_counter = (long) cinfo->output_scanline;
    cinfo->progress->pass_limit = (long) cinfo->output_height;
    (*cinfo->progress->progress_monitor) ((j_common_ptr) cinfo);
  }

  /* Process some data */
  row_ctr = 0;
  (*cinfo->main->process_data) (cinfo, scanlines, &row_ctr, max_lines);
  cinfo->output_scanline += row_ctr;
  return row_ctr;
}


/*
 * Alternate entry point to read raw data.
 * Processes exactly one MCU row per call.
 */

GLOBAL JDIMENSION
jpeg_read_raw_data (j_decompress_ptr cinfo, JSAMPIMAGE data,
		    JDIMENSION max_lines)
{
  JDIMENSION lines_per_MCU_row;

  if (cinfo->global_state != DSTATE_RAW_OK)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  if (cinfo->output_scanline >= cinfo->output_height) {
    WARNMS(cinfo, JWRN_TOO_MUCH_DATA);
    return 0;
  }

  /* Call progress monitor hook if present */
  if (cinfo->progress != NULL) {
    cinfo->progress->pass_counter = (long) cinfo->output_scanline;
    cinfo->progress->pass_limit = (long) cinfo->output_height;
    (*cinfo->progress->progress_monitor) ((j_common_ptr) cinfo);
  }

  /* Verify that at least one MCU row can be returned. */
  lines_per_MCU_row = cinfo->max_v_samp_factor * cinfo->min_DCT_scaled_size;
  if (max_lines < lines_per_MCU_row)
    ERREXIT(cinfo, JERR_BUFFER_SIZE);

  /* Decompress directly into user's buffer. */
  if (! (*cinfo->coef->decompress_data) (cinfo, data))
    return 0;			/* suspension forced, can do nothing more */

  /* OK, we processed one MCU row. */
  cinfo->output_scanline += lines_per_MCU_row;
  return lines_per_MCU_row;
}


/*
 * Finish JPEG decompression.
 *
 * This will normally just verify the file trailer and release temp storage.
 *
 * Returns FALSE if suspended.  The return value need be inspected only if
 * a suspending data source is used.
 */

GLOBAL boolean
jpeg_finish_decompress (j_decompress_ptr cinfo)
{
  if (cinfo->global_state == DSTATE_SCANNING ||
      cinfo->global_state == DSTATE_RAW_OK) {
    /* Terminate final pass */
    if (cinfo->output_scanline < cinfo->output_height)
      ERREXIT(cinfo, JERR_TOO_LITTLE_DATA);
    (*cinfo->master->finish_pass) (cinfo);
    cinfo->global_state = DSTATE_STOPPING;
  } else if (cinfo->global_state != DSTATE_STOPPING) {
    /* Repeat call after a suspension? */
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  }
  /* Check for EOI in source file, unless master control already read it */
  if (! cinfo->master->eoi_processed) {
    switch ((*cinfo->marker->read_markers) (cinfo)) {
    case JPEG_HEADER_OK:	/* Found SOS!? */
      ERREXIT(cinfo, JERR_EOI_EXPECTED);
      break;
    case JPEG_HEADER_TABLES_ONLY: /* Found EOI, A-OK */
      break;
    case JPEG_SUSPENDED:	/* Suspend, come back later */
      return FALSE;
    }
  }
  /* Do final cleanup */
  (*cinfo->src->term_source) (cinfo);
  /* We can use jpeg_abort to release memory and reset global_state */
  jpeg_abort((j_common_ptr) cinfo);
  return TRUE;
}


/*
 * Abort processing of a JPEG decompression operation,
 * but don't destroy the object itself.
 */

GLOBAL void
jpeg_abort_decompress (j_decompress_ptr cinfo)
{
  jpeg_abort((j_common_ptr) cinfo); /* use common routine */
}

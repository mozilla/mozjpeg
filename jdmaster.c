/*
 * jdmaster.c
 *
 * Copyright (C) 1991-1994, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains master control logic for the JPEG decompressor.
 * These routines are concerned with selecting the modules to be executed
 * and with determining the number of passes and the work to be done in each
 * pass.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/* Private state */

typedef enum {
	main_pass,		/* read and process a single-scan file */
	preread_pass,		/* read one scan of a multi-scan file */
	output_pass,		/* primary processing pass for multi-scan */
	post_pass		/* optional post-pass for 2-pass quant. */
} D_PASS_TYPE;

typedef struct {
  struct jpeg_decomp_master pub; /* public fields */

  boolean using_merged_upsample; /* TRUE if using merged upsample/cconvert */

  D_PASS_TYPE pass_type;	/* the type of the current pass */

  int pass_number;		/* # of passes completed */
  int total_passes;		/* estimated total # of passes needed */

  boolean need_post_pass;	/* are we using full two-pass quantization? */
} my_decomp_master;

typedef my_decomp_master * my_master_ptr;


/*
 * Determine whether merged upsample/color conversion should be used.
 * CRUCIAL: this must match the actual capabilities of jdmerge.c!
 */

LOCAL boolean
use_merged_upsample (j_decompress_ptr cinfo)
{
#ifdef UPSAMPLE_MERGING_SUPPORTED
  /* Merging is the equivalent of plain box-filter upsampling */
  if (cinfo->do_fancy_upsampling || cinfo->CCIR601_sampling)
    return FALSE;
  /* jdmerge.c only supports YCC=>RGB color conversion */
  if (cinfo->jpeg_color_space != JCS_YCbCr || cinfo->num_components != 3 ||
      cinfo->out_color_space != JCS_RGB ||
      cinfo->out_color_components != RGB_PIXELSIZE)
    return FALSE;
  /* and it only handles 2h1v or 2h2v sampling ratios */
  if (cinfo->comp_info[0].h_samp_factor != 2 ||
      cinfo->comp_info[1].h_samp_factor != 1 ||
      cinfo->comp_info[2].h_samp_factor != 1 ||
      cinfo->comp_info[0].v_samp_factor >  2 ||
      cinfo->comp_info[1].v_samp_factor != 1 ||
      cinfo->comp_info[2].v_samp_factor != 1)
    return FALSE;
  /* furthermore, it doesn't work if we've scaled the IDCTs differently */
  if (cinfo->comp_info[0].DCT_scaled_size != cinfo->min_DCT_scaled_size ||
      cinfo->comp_info[1].DCT_scaled_size != cinfo->min_DCT_scaled_size ||
      cinfo->comp_info[2].DCT_scaled_size != cinfo->min_DCT_scaled_size)
    return FALSE;
  /* ??? also need to test for upsample-time rescaling, when & if supported */
  /* by golly, it'll work... */
  return TRUE;
#else
  return FALSE;
#endif
}


/*
 * Support routines that do various essential calculations.
 *
 * jpeg_calc_output_dimensions is exported for possible use by application.
 * Hence it mustn't do anything that can't be done twice.
 */

GLOBAL void
jpeg_calc_output_dimensions (j_decompress_ptr cinfo)
/* Do computations that are needed before master selection phase */
{
  int ci;
  jpeg_component_info *compptr;

  /* Compute maximum sampling factors; check factor validity */
  cinfo->max_h_samp_factor = 1;
  cinfo->max_v_samp_factor = 1;
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    if (compptr->h_samp_factor<=0 || compptr->h_samp_factor>MAX_SAMP_FACTOR ||
	compptr->v_samp_factor<=0 || compptr->v_samp_factor>MAX_SAMP_FACTOR)
      ERREXIT(cinfo, JERR_BAD_SAMPLING);
    cinfo->max_h_samp_factor = MAX(cinfo->max_h_samp_factor,
				   compptr->h_samp_factor);
    cinfo->max_v_samp_factor = MAX(cinfo->max_v_samp_factor,
				   compptr->v_samp_factor);
  }

  /* Compute actual output image dimensions and DCT scaling choices. */
#ifdef IDCT_SCALING_SUPPORTED
  if (cinfo->scale_num * 8 <= cinfo->scale_denom) {
    /* Provide 1/8 scaling */
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width, 8L);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height, 8L);
    cinfo->min_DCT_scaled_size = 1;
  } else if (cinfo->scale_num * 4 <= cinfo->scale_denom) {
    /* Provide 1/4 scaling */
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width, 4L);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height, 4L);
    cinfo->min_DCT_scaled_size = 2;
  } else if (cinfo->scale_num * 2 <= cinfo->scale_denom) {
    /* Provide 1/2 scaling */
    cinfo->output_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width, 2L);
    cinfo->output_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height, 2L);
    cinfo->min_DCT_scaled_size = 4;
  } else {
    /* Provide 1/1 scaling */
    cinfo->output_width = cinfo->image_width;
    cinfo->output_height = cinfo->image_height;
    cinfo->min_DCT_scaled_size = DCTSIZE;
  }
  /* In selecting the actual DCT scaling for each component, we try to
   * scale up the chroma components via IDCT scaling rather than upsampling.
   * This saves time if the upsampler gets to use 1:1 scaling.
   * Note this code assumes that the supported DCT scalings are powers of 2.
   */
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    int ssize = cinfo->min_DCT_scaled_size;
    while (ssize < DCTSIZE &&
	   (compptr->h_samp_factor * ssize * 2 <=
	    cinfo->max_h_samp_factor * cinfo->min_DCT_scaled_size) &&
	   (compptr->v_samp_factor * ssize * 2 <=
	    cinfo->max_v_samp_factor * cinfo->min_DCT_scaled_size)) {
      ssize = ssize * 2;
    }
    compptr->DCT_scaled_size = ssize;
  }
#else /* !IDCT_SCALING_SUPPORTED */
  /* Hardwire it to "no scaling" */
  cinfo->output_width = cinfo->image_width;
  cinfo->output_height = cinfo->image_height;
  cinfo->min_DCT_scaled_size = DCTSIZE;
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    compptr->DCT_scaled_size = DCTSIZE;
  }
#endif /* IDCT_SCALING_SUPPORTED */

  /* Report number of components in selected colorspace. */
  /* Probably this should be in the color conversion module... */
  switch (cinfo->out_color_space) {
  case JCS_GRAYSCALE:
    cinfo->out_color_components = 1;
    break;
  case JCS_RGB:
#if RGB_PIXELSIZE != 3
    cinfo->out_color_components = RGB_PIXELSIZE;
    break;
#endif /* else share code with YCbCr */
  case JCS_YCbCr:
    cinfo->out_color_components = 3;
    break;
  case JCS_CMYK:
  case JCS_YCCK:
    cinfo->out_color_components = 4;
    break;
  default:			/* else must be same colorspace as in file */
    cinfo->out_color_components = cinfo->num_components;
    break;
  }
  cinfo->output_components = (cinfo->quantize_colors ? 1 :
			      cinfo->out_color_components);

  /* See if upsampler will want to emit more than one row at a time */
  if (use_merged_upsample(cinfo))
    cinfo->rec_outbuf_height = cinfo->max_v_samp_factor;
  else
    cinfo->rec_outbuf_height = 1;

  /* Compute various sampling-related dimensions.
   * Some of these are of interest to the application if it is dealing with
   * "raw" (not upsampled) output, so we do the calculations here.
   */

  /* Compute dimensions of components */
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    /* Size in DCT blocks */
    compptr->width_in_blocks = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width * (long) compptr->h_samp_factor,
		    (long) (cinfo->max_h_samp_factor * DCTSIZE));
    compptr->height_in_blocks = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height * (long) compptr->v_samp_factor,
		    (long) (cinfo->max_v_samp_factor * DCTSIZE));
    /* Size in samples, after IDCT scaling */
    compptr->downsampled_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width *
		    (long) (compptr->h_samp_factor * compptr->DCT_scaled_size),
		    (long) (cinfo->max_h_samp_factor * DCTSIZE));
    compptr->downsampled_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height *
		    (long) (compptr->v_samp_factor * compptr->DCT_scaled_size),
		    (long) (cinfo->max_v_samp_factor * DCTSIZE));
    /* Mark component needed, until color conversion says otherwise */
    compptr->component_needed = TRUE;
  }

  /* Compute number of fully interleaved MCU rows (number of times that
   * main controller will call coefficient controller).
   */
  cinfo->total_iMCU_rows = (JDIMENSION)
    jdiv_round_up((long) cinfo->image_height,
		  (long) (cinfo->max_v_samp_factor*DCTSIZE));
}


LOCAL void
per_scan_setup (j_decompress_ptr cinfo)
/* Do computations that are needed before processing a JPEG scan */
/* cinfo->comps_in_scan and cinfo->cur_comp_info[] were set from SOS marker */
{
  int ci, mcublks, tmp;
  jpeg_component_info *compptr;
  
  if (cinfo->comps_in_scan == 1) {
    
    /* Noninterleaved (single-component) scan */
    compptr = cinfo->cur_comp_info[0];
    
    /* Overall image size in MCUs */
    cinfo->MCUs_per_row = compptr->width_in_blocks;
    cinfo->MCU_rows_in_scan = compptr->height_in_blocks;
    
    /* For noninterleaved scan, always one block per MCU */
    compptr->MCU_width = 1;
    compptr->MCU_height = 1;
    compptr->MCU_blocks = 1;
    compptr->MCU_sample_width = compptr->DCT_scaled_size;
    compptr->last_col_width = 1;
    compptr->last_row_height = 1;
    
    /* Prepare array describing MCU composition */
    cinfo->blocks_in_MCU = 1;
    cinfo->MCU_membership[0] = 0;
    
  } else {
    
    /* Interleaved (multi-component) scan */
    if (cinfo->comps_in_scan <= 0 || cinfo->comps_in_scan > MAX_COMPS_IN_SCAN)
      ERREXIT2(cinfo, JERR_COMPONENT_COUNT, cinfo->comps_in_scan,
	       MAX_COMPS_IN_SCAN);
    
    /* Overall image size in MCUs */
    cinfo->MCUs_per_row = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_width,
		    (long) (cinfo->max_h_samp_factor*DCTSIZE));
    cinfo->MCU_rows_in_scan = (JDIMENSION)
      jdiv_round_up((long) cinfo->image_height,
		    (long) (cinfo->max_v_samp_factor*DCTSIZE));
    
    cinfo->blocks_in_MCU = 0;
    
    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      /* Sampling factors give # of blocks of component in each MCU */
      compptr->MCU_width = compptr->h_samp_factor;
      compptr->MCU_height = compptr->v_samp_factor;
      compptr->MCU_blocks = compptr->MCU_width * compptr->MCU_height;
      compptr->MCU_sample_width = compptr->MCU_width * compptr->DCT_scaled_size;
      /* Figure number of non-dummy blocks in last MCU column & row */
      tmp = (int) (compptr->width_in_blocks % compptr->MCU_width);
      if (tmp == 0) tmp = compptr->MCU_width;
      compptr->last_col_width = tmp;
      tmp = (int) (compptr->height_in_blocks % compptr->MCU_height);
      if (tmp == 0) tmp = compptr->MCU_height;
      compptr->last_row_height = tmp;
      /* Prepare array describing MCU composition */
      mcublks = compptr->MCU_blocks;
      if (cinfo->blocks_in_MCU + mcublks > MAX_BLOCKS_IN_MCU)
	ERREXIT(cinfo, JERR_BAD_MCU_SIZE);
      while (mcublks-- > 0) {
	cinfo->MCU_membership[cinfo->blocks_in_MCU++] = ci;
      }
    }
    
  }
}


/*
 * Several decompression processes need to range-limit values to the range
 * 0..MAXJSAMPLE; the input value may fall somewhat outside this range
 * due to noise introduced by quantization, roundoff error, etc.  These
 * processes are inner loops and need to be as fast as possible.  On most
 * machines, particularly CPUs with pipelines or instruction prefetch,
 * a (subscript-check-less) C table lookup
 *		x = sample_range_limit[x];
 * is faster than explicit tests
 *		if (x < 0)  x = 0;
 *		else if (x > MAXJSAMPLE)  x = MAXJSAMPLE;
 * These processes all use a common table prepared by the routine below.
 *
 * For most steps we can mathematically guarantee that the initial value
 * of x is within MAXJSAMPLE+1 of the legal range, so a table running from
 * -(MAXJSAMPLE+1) to 2*MAXJSAMPLE+1 is sufficient.  But for the initial
 * limiting step (just after the IDCT), a wildly out-of-range value is 
 * possible if the input data is corrupt.  To avoid any chance of indexing
 * off the end of memory and getting a bad-pointer trap, we perform the
 * post-IDCT limiting thus:
 *		x = range_limit[x & MASK];
 * where MASK is 2 bits wider than legal sample data, ie 10 bits for 8-bit
 * samples.  Under normal circumstances this is more than enough range and
 * a correct output will be generated; with bogus input data the mask will
 * cause wraparound, and we will safely generate a bogus-but-in-range output.
 * For the post-IDCT step, we want to convert the data from signed to unsigned
 * representation by adding CENTERJSAMPLE at the same time that we limit it.
 * So the post-IDCT limiting table ends up looking like this:
 *   CENTERJSAMPLE,CENTERJSAMPLE+1,...,MAXJSAMPLE,
 *   MAXJSAMPLE (repeat 2*(MAXJSAMPLE+1)-CENTERJSAMPLE times),
 *   0          (repeat 2*(MAXJSAMPLE+1)-CENTERJSAMPLE times),
 *   0,1,...,CENTERJSAMPLE-1
 * Negative inputs select values from the upper half of the table after
 * masking.
 *
 * We can save some space by overlapping the start of the post-IDCT table
 * with the simpler range limiting table.  The post-IDCT table begins at
 * sample_range_limit + CENTERJSAMPLE.
 *
 * Note that the table is allocated in near data space on PCs; it's small
 * enough and used often enough to justify this.
 */

LOCAL void
prepare_range_limit_table (j_decompress_ptr cinfo)
/* Allocate and fill in the sample_range_limit table */
{
  JSAMPLE * table;
  int i;

  table = (JSAMPLE *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
		(5 * (MAXJSAMPLE+1) + CENTERJSAMPLE) * SIZEOF(JSAMPLE));
  table += (MAXJSAMPLE+1);	/* allow negative subscripts of simple table */
  cinfo->sample_range_limit = table;
  /* First segment of "simple" table: limit[x] = 0 for x < 0 */
  MEMZERO(table - (MAXJSAMPLE+1), (MAXJSAMPLE+1) * SIZEOF(JSAMPLE));
  /* Main part of "simple" table: limit[x] = x */
  for (i = 0; i <= MAXJSAMPLE; i++)
    table[i] = (JSAMPLE) i;
  table += CENTERJSAMPLE;	/* Point to where post-IDCT table starts */
  /* End of simple table, rest of first half of post-IDCT table */
  for (i = CENTERJSAMPLE; i < 2*(MAXJSAMPLE+1); i++)
    table[i] = MAXJSAMPLE;
  /* Second half of post-IDCT table */
  MEMZERO(table + (2 * (MAXJSAMPLE+1)),
	  (2 * (MAXJSAMPLE+1) - CENTERJSAMPLE) * SIZEOF(JSAMPLE));
  MEMCOPY(table + (4 * (MAXJSAMPLE+1) - CENTERJSAMPLE),
	  cinfo->sample_range_limit, CENTERJSAMPLE * SIZEOF(JSAMPLE));
}


/*
 * Master selection of decompression modules.
 * This is done once at the start of processing an image.  We determine
 * which modules will be used and give them appropriate initialization calls.
 *
 * Note that this is called only after jpeg_read_header has finished.
 * We therefore know what is in the SOF and (first) SOS markers.
 */

LOCAL void
master_selection (j_decompress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;
  long samplesperrow;
  JDIMENSION jd_samplesperrow;

  /* Initialize dimensions and other stuff */
  jpeg_calc_output_dimensions(cinfo);
  prepare_range_limit_table(cinfo);

  /* Width of an output scanline must be representable as JDIMENSION. */
  samplesperrow = (long) cinfo->output_width * (long) cinfo->out_color_components;
  jd_samplesperrow = (JDIMENSION) samplesperrow;
  if ((long) jd_samplesperrow != samplesperrow)
    ERREXIT(cinfo, JERR_WIDTH_OVERFLOW);

  /* Initialize my private state */
  master->pub.eoi_processed = FALSE;
  master->pass_number = 0;
  master->need_post_pass = FALSE;
  if (cinfo->comps_in_scan == cinfo->num_components) {
    master->pass_type = main_pass;
    master->total_passes = 1;
  } else {
#ifdef D_MULTISCAN_FILES_SUPPORTED
    master->pass_type = preread_pass;
    /* Assume there is a separate scan for each component; */
    /* if partially interleaved, we'll increment pass_number appropriately */
    master->total_passes = cinfo->num_components + 1;
#else
    ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
  }
  master->using_merged_upsample = use_merged_upsample(cinfo);

  /* There's not a lot of smarts here right now, but it'll get more
   * complicated when we have multiple implementations available...
   */

  /* Color quantizer selection */
  if (cinfo->quantize_colors) {
    if (cinfo->raw_data_out)
      ERREXIT(cinfo, JERR_NOTIMPL);
#ifdef QUANT_2PASS_SUPPORTED
    /* 2-pass quantizer only works in 3-component color space.
     * We use the "2-pass" code in a single pass if a colormap is given.
     */
    if (cinfo->out_color_components != 3)
      cinfo->two_pass_quantize = FALSE;
    else if (cinfo->colormap != NULL)
      cinfo->two_pass_quantize = TRUE;
#else
    /* Force 1-pass quantize if we don't have 2-pass code compiled. */
    cinfo->two_pass_quantize = FALSE;
#endif

    if (cinfo->two_pass_quantize) {
#ifdef QUANT_2PASS_SUPPORTED
      if (cinfo->colormap == NULL) {
	master->need_post_pass = TRUE;
	master->total_passes++;
      }
      jinit_2pass_quantizer(cinfo);
#else
      ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
    } else {
#ifdef QUANT_1PASS_SUPPORTED
      jinit_1pass_quantizer(cinfo);
#else
      ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
    }
  }

  /* Post-processing: in particular, color conversion first */
  if (! cinfo->raw_data_out) {
    if (master->using_merged_upsample) {
#ifdef UPSAMPLE_MERGING_SUPPORTED
      jinit_merged_upsampler(cinfo); /* does color conversion too */
#else
      ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
    } else {
      jinit_color_deconverter(cinfo);
      jinit_upsampler(cinfo);
    }
    jinit_d_post_controller(cinfo, master->need_post_pass);
  }
  /* Inverse DCT */
  jinit_inverse_dct(cinfo);
  /* Entropy decoding: either Huffman or arithmetic coding. */
  if (cinfo->arith_code) {
#ifdef D_ARITH_CODING_SUPPORTED
    jinit_arith_decoder(cinfo);
#else
    ERREXIT(cinfo, JERR_ARITH_NOTIMPL);
#endif
  } else
    jinit_huff_decoder(cinfo);

  jinit_d_coef_controller(cinfo, (master->pass_type == preread_pass));
  jinit_d_main_controller(cinfo, FALSE /* never need full buffer here */);
  /* Note that main controller is initialized even in raw-data mode. */

  /* We can now tell the memory manager to allocate virtual arrays. */
  (*cinfo->mem->realize_virt_arrays) ((j_common_ptr) cinfo);
}


/*
 * Per-pass setup.
 * This is called at the beginning of each pass.  We determine which modules
 * will be active during this pass and give them appropriate start_pass calls.
 * We also set is_last_pass to indicate whether any more passes will be
 * required.
 */

METHODDEF void
prepare_for_pass (j_decompress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;

  switch (master->pass_type) {
  case main_pass:
    /* Set up to read and decompress single-scan file in one pass */
    per_scan_setup(cinfo);
    master->pub.is_last_pass = ! master->need_post_pass;
    if (! cinfo->raw_data_out) {
      if (! master->using_merged_upsample)
	(*cinfo->cconvert->start_pass) (cinfo);
      (*cinfo->upsample->start_pass) (cinfo);
      if (cinfo->quantize_colors)
	(*cinfo->cquantize->start_pass) (cinfo, master->need_post_pass);
      (*cinfo->post->start_pass) (cinfo,
	    (master->need_post_pass ? JBUF_SAVE_AND_PASS : JBUF_PASS_THRU));
    }
    (*cinfo->idct->start_input_pass) (cinfo);
    (*cinfo->idct->start_output_pass) (cinfo);
    (*cinfo->entropy->start_pass) (cinfo);
    (*cinfo->coef->start_pass) (cinfo, JBUF_PASS_THRU);
    (*cinfo->main->start_pass) (cinfo, JBUF_PASS_THRU);
    break;
#ifdef D_MULTISCAN_FILES_SUPPORTED
  case preread_pass:
    /* Read (another) scan of a multi-scan file */
    per_scan_setup(cinfo);
    master->pub.is_last_pass = FALSE;
    (*cinfo->idct->start_input_pass) (cinfo);
    (*cinfo->entropy->start_pass) (cinfo);
    (*cinfo->coef->start_pass) (cinfo, JBUF_SAVE_SOURCE);
    (*cinfo->main->start_pass) (cinfo, JBUF_CRANK_SOURCE);
    break;
  case output_pass:
    /* All scans read, now do the IDCT and subsequent processing */
    master->pub.is_last_pass = ! master->need_post_pass;
    if (! cinfo->raw_data_out) {
      if (! master->using_merged_upsample)
	(*cinfo->cconvert->start_pass) (cinfo);
      (*cinfo->upsample->start_pass) (cinfo);
      if (cinfo->quantize_colors)
	(*cinfo->cquantize->start_pass) (cinfo, master->need_post_pass);
      (*cinfo->post->start_pass) (cinfo,
	    (master->need_post_pass ? JBUF_SAVE_AND_PASS : JBUF_PASS_THRU));
    }
    (*cinfo->idct->start_output_pass) (cinfo);
    (*cinfo->coef->start_pass) (cinfo, JBUF_CRANK_DEST);
    (*cinfo->main->start_pass) (cinfo, JBUF_PASS_THRU);
    break;
#endif /* D_MULTISCAN_FILES_SUPPORTED */
#ifdef QUANT_2PASS_SUPPORTED
  case post_pass:
    /* Final pass of 2-pass quantization */
    master->pub.is_last_pass = TRUE;
    (*cinfo->cquantize->start_pass) (cinfo, FALSE);
    (*cinfo->post->start_pass) (cinfo, JBUF_CRANK_DEST);
    (*cinfo->main->start_pass) (cinfo, JBUF_CRANK_DEST);
    break;
#endif /* QUANT_2PASS_SUPPORTED */
  default:
    ERREXIT(cinfo, JERR_NOT_COMPILED);
  }

  /* Set up progress monitor's pass info if present */
  if (cinfo->progress != NULL) {
    cinfo->progress->completed_passes = master->pass_number;
    cinfo->progress->total_passes = master->total_passes;
  }
}


/*
 * Finish up at end of pass.
 * In multi-scan mode, we must read next scan header and set the next
 * pass_type correctly for prepare_for_pass.
 */

METHODDEF void
finish_pass_master (j_decompress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;

  switch (master->pass_type) {
  case main_pass:
  case output_pass:
    if (cinfo->quantize_colors)
      (*cinfo->cquantize->finish_pass) (cinfo);
    master->pass_number++;
    master->pass_type = post_pass; /* in case need_post_pass is true */
    break;
#ifdef D_MULTISCAN_FILES_SUPPORTED
  case preread_pass:
    /* Count one pass done for each component in this scan */
    master->pass_number += cinfo->comps_in_scan;
    switch ((*cinfo->marker->read_markers) (cinfo)) {
    case JPEG_HEADER_OK:	/* Found SOS, do another preread pass */
      break;
    case JPEG_HEADER_TABLES_ONLY: /* Found EOI, no more preread passes */
      master->pub.eoi_processed = TRUE;
      master->pass_type = output_pass;
      break;
    case JPEG_SUSPENDED:
      ERREXIT(cinfo, JERR_CANT_SUSPEND);
    }
    break;
#endif /* D_MULTISCAN_FILES_SUPPORTED */
#ifdef QUANT_2PASS_SUPPORTED
  case post_pass:
    (*cinfo->cquantize->finish_pass) (cinfo);
    /* there will be no more passes, don't bother to change state */
    break;
#endif /* QUANT_2PASS_SUPPORTED */
  default:
    ERREXIT(cinfo, JERR_NOT_COMPILED);
  }
}


/*
 * Initialize master decompression control.
 * This creates my own subrecord and also performs the master selection phase,
 * which causes other modules to create their subrecords.
 */

GLOBAL void
jinit_master_decompress (j_decompress_ptr cinfo)
{
  my_master_ptr master;

  master = (my_master_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(my_decomp_master));
  cinfo->master = (struct jpeg_decomp_master *) master;
  master->pub.prepare_for_pass = prepare_for_pass;
  master->pub.finish_pass = finish_pass_master;

  master_selection(cinfo);
}

/*
 * jcmaster.c
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * Modified 2003-2010 by Guido Vollbeding.
 * libjpeg-turbo Modifications:
 * Copyright (C) 2010, D. R. Commander.
 * mozjpeg Modifications:
 * Copyright (C) 2014, Mozilla Corporation.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains master control logic for the JPEG compressor.
 * These routines are concerned with parameter validation, initial setup,
 * and inter-pass control (determining the number of passes and the work 
 * to be done in each pass).
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jpegcomp.h"


/* Private state */

typedef enum {
	main_pass,		/* input data, also do first output step */
	huff_opt_pass,		/* Huffman code optimization pass */
	output_pass		/* data output pass */
} c_pass_type;

typedef struct {
  struct jpeg_comp_master pub;	/* public fields */

  c_pass_type pass_type;	/* the type of the current pass */

  int pass_number;		/* # of passes completed */
  int total_passes;		/* total # of passes needed */

  int scan_number;		/* current index in scan_info[] */
} my_comp_master;

typedef my_comp_master * my_master_ptr;


/*
 * Support routines that do various essential calculations.
 */

#if JPEG_LIB_VERSION >= 70
/*
 * Compute JPEG image dimensions and related values.
 * NOTE: this is exported for possible use by application.
 * Hence it mustn't do anything that can't be done twice.
 */

GLOBAL(void)
jpeg_calc_jpeg_dimensions (j_compress_ptr cinfo)
/* Do computations that are needed before master selection phase */
{
  /* Hardwire it to "no scaling" */
  cinfo->jpeg_width = cinfo->image_width;
  cinfo->jpeg_height = cinfo->image_height;
  cinfo->min_DCT_h_scaled_size = DCTSIZE;
  cinfo->min_DCT_v_scaled_size = DCTSIZE;
}
#endif


LOCAL(void)
initial_setup (j_compress_ptr cinfo, boolean transcode_only)
/* Do computations that are needed before master selection phase */
{
  int ci;
  jpeg_component_info *compptr;
  long samplesperrow;
  JDIMENSION jd_samplesperrow;

#if JPEG_LIB_VERSION >= 70
#if JPEG_LIB_VERSION >= 80
  if (!transcode_only)
#endif
    jpeg_calc_jpeg_dimensions(cinfo);
#endif

  /* Sanity check on image dimensions */
  if (cinfo->_jpeg_height <= 0 || cinfo->_jpeg_width <= 0
      || cinfo->num_components <= 0 || cinfo->input_components <= 0)
    ERREXIT(cinfo, JERR_EMPTY_IMAGE);

  /* Make sure image isn't bigger than I can handle */
  if ((long) cinfo->_jpeg_height > (long) JPEG_MAX_DIMENSION ||
      (long) cinfo->_jpeg_width > (long) JPEG_MAX_DIMENSION)
    ERREXIT1(cinfo, JERR_IMAGE_TOO_BIG, (unsigned int) JPEG_MAX_DIMENSION);

  /* Width of an input scanline must be representable as JDIMENSION. */
  samplesperrow = (long) cinfo->image_width * (long) cinfo->input_components;
  jd_samplesperrow = (JDIMENSION) samplesperrow;
  if ((long) jd_samplesperrow != samplesperrow)
    ERREXIT(cinfo, JERR_WIDTH_OVERFLOW);

  /* For now, precision must match compiled-in value... */
  if (cinfo->data_precision != BITS_IN_JSAMPLE)
    ERREXIT1(cinfo, JERR_BAD_PRECISION, cinfo->data_precision);

  /* Check that number of components won't exceed internal array sizes */
  if (cinfo->num_components > MAX_COMPONENTS)
    ERREXIT2(cinfo, JERR_COMPONENT_COUNT, cinfo->num_components,
	     MAX_COMPONENTS);

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

  /* Compute dimensions of components */
  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    /* Fill in the correct component_index value; don't rely on application */
    compptr->component_index = ci;
    /* For compression, we never do DCT scaling. */
#if JPEG_LIB_VERSION >= 70
    compptr->DCT_h_scaled_size = compptr->DCT_v_scaled_size = DCTSIZE;
#else
    compptr->DCT_scaled_size = DCTSIZE;
#endif
    /* Size in DCT blocks */
    compptr->width_in_blocks = (JDIMENSION)
      jdiv_round_up((long) cinfo->_jpeg_width * (long) compptr->h_samp_factor,
		    (long) (cinfo->max_h_samp_factor * DCTSIZE));
    compptr->height_in_blocks = (JDIMENSION)
      jdiv_round_up((long) cinfo->_jpeg_height * (long) compptr->v_samp_factor,
		    (long) (cinfo->max_v_samp_factor * DCTSIZE));
    /* Size in samples */
    compptr->downsampled_width = (JDIMENSION)
      jdiv_round_up((long) cinfo->_jpeg_width * (long) compptr->h_samp_factor,
		    (long) cinfo->max_h_samp_factor);
    compptr->downsampled_height = (JDIMENSION)
      jdiv_round_up((long) cinfo->_jpeg_height * (long) compptr->v_samp_factor,
		    (long) cinfo->max_v_samp_factor);
    /* Mark component needed (this flag isn't actually used for compression) */
    compptr->component_needed = TRUE;
  }

  /* Compute number of fully interleaved MCU rows (number of times that
   * main controller will call coefficient controller).
   */
  cinfo->total_iMCU_rows = (JDIMENSION)
    jdiv_round_up((long) cinfo->_jpeg_height,
		  (long) (cinfo->max_v_samp_factor*DCTSIZE));
}


#ifdef C_MULTISCAN_FILES_SUPPORTED

LOCAL(void)
validate_script (j_compress_ptr cinfo)
/* Verify that the scan script in cinfo->scan_info[] is valid; also
 * determine whether it uses progressive JPEG, and set cinfo->progressive_mode.
 */
{
  const jpeg_scan_info * scanptr;
  int scanno, ncomps, ci, coefi, thisi;
  int Ss, Se, Ah, Al;
  boolean component_sent[MAX_COMPONENTS];
#ifdef C_PROGRESSIVE_SUPPORTED
  int * last_bitpos_ptr;
  int last_bitpos[MAX_COMPONENTS][DCTSIZE2];
  /* -1 until that coefficient has been seen; then last Al for it */
#endif

  if (cinfo->optimize_scans) {
    cinfo->progressive_mode = TRUE;
    /* When we optimize scans, there is redundancy in the scan list
     * and this function will fail. Therefore skip all this checking
     */
    return;
  }
  
  if (cinfo->num_scans <= 0)
    ERREXIT1(cinfo, JERR_BAD_SCAN_SCRIPT, 0);

  /* For sequential JPEG, all scans must have Ss=0, Se=DCTSIZE2-1;
   * for progressive JPEG, no scan can have this.
   */
  scanptr = cinfo->scan_info;
  if (scanptr->Ss != 0 || scanptr->Se != DCTSIZE2-1) {
#ifdef C_PROGRESSIVE_SUPPORTED
    cinfo->progressive_mode = TRUE;
    last_bitpos_ptr = & last_bitpos[0][0];
    for (ci = 0; ci < cinfo->num_components; ci++) 
      for (coefi = 0; coefi < DCTSIZE2; coefi++)
	*last_bitpos_ptr++ = -1;
#else
    ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
  } else {
    cinfo->progressive_mode = FALSE;
    for (ci = 0; ci < cinfo->num_components; ci++) 
      component_sent[ci] = FALSE;
  }

  for (scanno = 1; scanno <= cinfo->num_scans; scanptr++, scanno++) {
    /* Validate component indexes */
    ncomps = scanptr->comps_in_scan;
    if (ncomps <= 0 || ncomps > MAX_COMPS_IN_SCAN)
      ERREXIT2(cinfo, JERR_COMPONENT_COUNT, ncomps, MAX_COMPS_IN_SCAN);
    for (ci = 0; ci < ncomps; ci++) {
      thisi = scanptr->component_index[ci];
      if (thisi < 0 || thisi >= cinfo->num_components)
	ERREXIT1(cinfo, JERR_BAD_SCAN_SCRIPT, scanno);
      /* Components must appear in SOF order within each scan */
      if (ci > 0 && thisi <= scanptr->component_index[ci-1])
	ERREXIT1(cinfo, JERR_BAD_SCAN_SCRIPT, scanno);
    }
    /* Validate progression parameters */
    Ss = scanptr->Ss;
    Se = scanptr->Se;
    Ah = scanptr->Ah;
    Al = scanptr->Al;
    if (cinfo->progressive_mode) {
#ifdef C_PROGRESSIVE_SUPPORTED
      /* The JPEG spec simply gives the ranges 0..13 for Ah and Al, but that
       * seems wrong: the upper bound ought to depend on data precision.
       * Perhaps they really meant 0..N+1 for N-bit precision.
       * Here we allow 0..10 for 8-bit data; Al larger than 10 results in
       * out-of-range reconstructed DC values during the first DC scan,
       * which might cause problems for some decoders.
       */
#if BITS_IN_JSAMPLE == 8
#define MAX_AH_AL 10
#else
#define MAX_AH_AL 13
#endif
      if (Ss < 0 || Ss >= DCTSIZE2 || Se < Ss || Se >= DCTSIZE2 ||
	  Ah < 0 || Ah > MAX_AH_AL || Al < 0 || Al > MAX_AH_AL)
	ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
      if (Ss == 0) {
	if (Se != 0)		/* DC and AC together not OK */
	  ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
      } else {
	if (ncomps != 1)	/* AC scans must be for only one component */
	  ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
      }
      for (ci = 0; ci < ncomps; ci++) {
	last_bitpos_ptr = & last_bitpos[scanptr->component_index[ci]][0];
	if (Ss != 0 && last_bitpos_ptr[0] < 0) /* AC without prior DC scan */
	  ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
	for (coefi = Ss; coefi <= Se; coefi++) {
	  if (last_bitpos_ptr[coefi] < 0) {
	    /* first scan of this coefficient */
	    if (Ah != 0)
	      ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
	  } else {
	    /* not first scan */
	    if (Ah != last_bitpos_ptr[coefi] || Al != Ah-1)
	      ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
	  }
	  last_bitpos_ptr[coefi] = Al;
	}
      }
#endif
    } else {
      /* For sequential JPEG, all progression parameters must be these: */
      if (Ss != 0 || Se != DCTSIZE2-1 || Ah != 0 || Al != 0)
	ERREXIT1(cinfo, JERR_BAD_PROG_SCRIPT, scanno);
      /* Make sure components are not sent twice */
      for (ci = 0; ci < ncomps; ci++) {
	thisi = scanptr->component_index[ci];
	if (component_sent[thisi])
	  ERREXIT1(cinfo, JERR_BAD_SCAN_SCRIPT, scanno);
	component_sent[thisi] = TRUE;
      }
    }
  }

  /* Now verify that everything got sent. */
  if (cinfo->progressive_mode) {
#ifdef C_PROGRESSIVE_SUPPORTED
    /* For progressive mode, we only check that at least some DC data
     * got sent for each component; the spec does not require that all bits
     * of all coefficients be transmitted.  Would it be wiser to enforce
     * transmission of all coefficient bits??
     */
    for (ci = 0; ci < cinfo->num_components; ci++) {
      if (last_bitpos[ci][0] < 0)
	ERREXIT(cinfo, JERR_MISSING_DATA);
    }
#endif
  } else {
    for (ci = 0; ci < cinfo->num_components; ci++) {
      if (! component_sent[ci])
	ERREXIT(cinfo, JERR_MISSING_DATA);
    }
  }
}

#endif /* C_MULTISCAN_FILES_SUPPORTED */


LOCAL(void)
select_scan_parameters (j_compress_ptr cinfo)
/* Set up the scan parameters for the current scan */
{
  int ci;

#ifdef C_MULTISCAN_FILES_SUPPORTED
  if (cinfo->scan_info != NULL) {
    /* Prepare for current scan --- the script is already validated */
    my_master_ptr master = (my_master_ptr) cinfo->master;
    const jpeg_scan_info * scanptr = cinfo->scan_info + master->scan_number;

    cinfo->comps_in_scan = scanptr->comps_in_scan;
    for (ci = 0; ci < scanptr->comps_in_scan; ci++) {
      cinfo->cur_comp_info[ci] =
	&cinfo->comp_info[scanptr->component_index[ci]];
    }
    cinfo->Ss = scanptr->Ss;
    cinfo->Se = scanptr->Se;
    cinfo->Ah = scanptr->Ah;
    cinfo->Al = scanptr->Al;
  }
  else
#endif
  {
    /* Prepare for single sequential-JPEG scan containing all components */
    if (cinfo->num_components > MAX_COMPS_IN_SCAN)
      ERREXIT2(cinfo, JERR_COMPONENT_COUNT, cinfo->num_components,
	       MAX_COMPS_IN_SCAN);
    cinfo->comps_in_scan = cinfo->num_components;
    for (ci = 0; ci < cinfo->num_components; ci++) {
      cinfo->cur_comp_info[ci] = &cinfo->comp_info[ci];
    }
    cinfo->Ss = 0;
    cinfo->Se = DCTSIZE2-1;
    cinfo->Ah = 0;
    cinfo->Al = 0;
  }
}


LOCAL(void)
per_scan_setup (j_compress_ptr cinfo)
/* Do computations that are needed before processing a JPEG scan */
/* cinfo->comps_in_scan and cinfo->cur_comp_info[] are already set */
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
    compptr->MCU_sample_width = DCTSIZE;
    compptr->last_col_width = 1;
    /* For noninterleaved scans, it is convenient to define last_row_height
     * as the number of block rows present in the last iMCU row.
     */
    tmp = (int) (compptr->height_in_blocks % compptr->v_samp_factor);
    if (tmp == 0) tmp = compptr->v_samp_factor;
    compptr->last_row_height = tmp;
    
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
      jdiv_round_up((long) cinfo->_jpeg_width,
		    (long) (cinfo->max_h_samp_factor*DCTSIZE));
    cinfo->MCU_rows_in_scan = (JDIMENSION)
      jdiv_round_up((long) cinfo->_jpeg_height,
		    (long) (cinfo->max_v_samp_factor*DCTSIZE));
    
    cinfo->blocks_in_MCU = 0;
    
    for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
      compptr = cinfo->cur_comp_info[ci];
      /* Sampling factors give # of blocks of component in each MCU */
      compptr->MCU_width = compptr->h_samp_factor;
      compptr->MCU_height = compptr->v_samp_factor;
      compptr->MCU_blocks = compptr->MCU_width * compptr->MCU_height;
      compptr->MCU_sample_width = compptr->MCU_width * DCTSIZE;
      /* Figure number of non-dummy blocks in last MCU column & row */
      tmp = (int) (compptr->width_in_blocks % compptr->MCU_width);
      if (tmp == 0) tmp = compptr->MCU_width;
      compptr->last_col_width = tmp;
      tmp = (int) (compptr->height_in_blocks % compptr->MCU_height);
      if (tmp == 0) tmp = compptr->MCU_height;
      compptr->last_row_height = tmp;
      /* Prepare array describing MCU composition */
      mcublks = compptr->MCU_blocks;
      if (cinfo->blocks_in_MCU + mcublks > C_MAX_BLOCKS_IN_MCU)
	ERREXIT(cinfo, JERR_BAD_MCU_SIZE);
      while (mcublks-- > 0) {
	cinfo->MCU_membership[cinfo->blocks_in_MCU++] = ci;
      }
    }
    
  }

  /* Convert restart specified in rows to actual MCU count. */
  /* Note that count must fit in 16 bits, so we provide limiting. */
  if (cinfo->restart_in_rows > 0) {
    long nominal = (long) cinfo->restart_in_rows * (long) cinfo->MCUs_per_row;
    cinfo->restart_interval = (unsigned int) MIN(nominal, 65535L);
  }
}


/*
 * Per-pass setup.
 * This is called at the beginning of each pass.  We determine which modules
 * will be active during this pass and give them appropriate start_pass calls.
 * We also set is_last_pass to indicate whether any more passes will be
 * required.
 */

METHODDEF(void)
prepare_for_pass (j_compress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;

  switch (master->pass_type) {
  case main_pass:
    /* Initial pass: will collect input data, and do either Huffman
     * optimization or data output for the first scan.
     */
    select_scan_parameters(cinfo);
    per_scan_setup(cinfo);
    if (! cinfo->raw_data_in) {
      (*cinfo->cconvert->start_pass) (cinfo);
      (*cinfo->downsample->start_pass) (cinfo);
      (*cinfo->prep->start_pass) (cinfo, JBUF_PASS_THRU);
    }
    (*cinfo->fdct->start_pass) (cinfo);
    (*cinfo->entropy->start_pass) (cinfo, cinfo->optimize_coding);
    (*cinfo->coef->start_pass) (cinfo,
				(master->total_passes > 1 ?
				 JBUF_SAVE_AND_PASS : JBUF_PASS_THRU));
    (*cinfo->main->start_pass) (cinfo, JBUF_PASS_THRU);
    if (cinfo->optimize_coding) {
      /* No immediate data output; postpone writing frame/scan headers */
      master->pub.call_pass_startup = FALSE;
    } else {
      /* Will write frame/scan headers at first jpeg_write_scanlines call */
      master->pub.call_pass_startup = TRUE;
    }
    break;
#ifdef ENTROPY_OPT_SUPPORTED
  case huff_opt_pass:
    /* Do Huffman optimization for a scan after the first one. */
    select_scan_parameters(cinfo);
    per_scan_setup(cinfo);
    if (cinfo->Ss != 0 || cinfo->Ah == 0 || cinfo->arith_code) {
      (*cinfo->entropy->start_pass) (cinfo, TRUE);
      (*cinfo->coef->start_pass) (cinfo, JBUF_CRANK_DEST);
      master->pub.call_pass_startup = FALSE;
      break;
    }
    /* Special case: Huffman DC refinement scans need no Huffman table
     * and therefore we can skip the optimization pass for them.
     */
    master->pass_type = output_pass;
    master->pass_number++;
    /*FALLTHROUGH*/
#endif
  case output_pass:
    /* Do a data-output pass. */
    /* We need not repeat per-scan setup if prior optimization pass did it. */
    if (! cinfo->optimize_coding) {
      select_scan_parameters(cinfo);
      per_scan_setup(cinfo);
    }
      if (cinfo->optimize_scans) {
        cinfo->saved_dest = cinfo->dest;
        cinfo->dest = NULL;
        cinfo->scan_buffer[master->scan_number] = NULL;
        cinfo->scan_size[master->scan_number] = 0;
        jpeg_mem_dest(cinfo, &cinfo->scan_buffer[master->scan_number], &cinfo->scan_size[master->scan_number]);
        (*cinfo->dest->init_destination)(cinfo);
      }
    (*cinfo->entropy->start_pass) (cinfo, FALSE);
    (*cinfo->coef->start_pass) (cinfo, JBUF_CRANK_DEST);
    /* We emit frame/scan headers now */
    if (master->scan_number == 0)
      (*cinfo->marker->write_frame_header) (cinfo);
    (*cinfo->marker->write_scan_header) (cinfo);
    master->pub.call_pass_startup = FALSE;
    break;
  default:
    ERREXIT(cinfo, JERR_NOT_COMPILED);
  }

  master->pub.is_last_pass = (master->pass_number == master->total_passes-1);

  /* Set up progress monitor's pass info if present */
  if (cinfo->progress != NULL) {
    cinfo->progress->completed_passes = master->pass_number;
    cinfo->progress->total_passes = master->total_passes;
  }
}


/*
 * Special start-of-pass hook.
 * This is called by jpeg_write_scanlines if call_pass_startup is TRUE.
 * In single-pass processing, we need this hook because we don't want to
 * write frame/scan headers during jpeg_start_compress; we want to let the
 * application write COM markers etc. between jpeg_start_compress and the
 * jpeg_write_scanlines loop.
 * In multi-pass processing, this routine is not used.
 */

METHODDEF(void)
pass_startup (j_compress_ptr cinfo)
{
  cinfo->master->call_pass_startup = FALSE; /* reset flag so call only once */

  (*cinfo->marker->write_frame_header) (cinfo);
  (*cinfo->marker->write_scan_header) (cinfo);
}


LOCAL(void)
copy_buffer (j_compress_ptr cinfo, int scan_idx)
{
  int size = cinfo->scan_size[scan_idx];
  unsigned char * src = cinfo->scan_buffer[scan_idx];
  
  while (size >= cinfo->dest->free_in_buffer)
  {
    MEMCOPY(cinfo->dest->next_output_byte, src, cinfo->dest->free_in_buffer);
    src += cinfo->dest->free_in_buffer;
    size -= cinfo->dest->free_in_buffer;
    cinfo->dest->next_output_byte += cinfo->dest->free_in_buffer;
    cinfo->dest->free_in_buffer = 0;
    (*cinfo->dest->empty_output_buffer)(cinfo);
  }

  MEMCOPY(cinfo->dest->next_output_byte, src, size);
  cinfo->dest->next_output_byte += size;
  cinfo->dest->free_in_buffer -= size;
}

/*
 * Finish up at end of pass.
 */

METHODDEF(void)
finish_pass_master (j_compress_ptr cinfo)
{
  my_master_ptr master = (my_master_ptr) cinfo->master;

  /* The entropy coder always needs an end-of-pass call,
   * either to analyze statistics or to flush its output buffer.
   */
  (*cinfo->entropy->finish_pass) (cinfo);

  /* Update state for next pass */
  switch (master->pass_type) {
  case main_pass:
    /* next pass is either output of scan 0 (after optimization)
     * or output of scan 1 (if no optimization).
     */
    master->pass_type = output_pass;
    if (! cinfo->optimize_coding)
      master->scan_number++;
    break;
  case huff_opt_pass:
    /* next pass is always output of current scan */
    master->pass_type = output_pass;
    break;
  case output_pass:
    /* next pass is either optimization or output of next scan */
    if (cinfo->optimize_coding)
      master->pass_type = huff_opt_pass;
      if (cinfo->optimize_scans) {
        (*cinfo->dest->term_destination)(cinfo);
        cinfo->dest = cinfo->saved_dest;
        if (master->scan_number == 0) {
          copy_buffer(cinfo, 0);
          
        } else if (master->scan_number == 3*cinfo->Al_max_luma+2) {
          int Al;
          int size[4];
          int i;
          
          cinfo->best_Al = 0;
          
          for (Al = 0; Al <= cinfo->Al_max_luma; Al++) {
            size[Al]  = cinfo->scan_size[1 + 2 * Al];
            size[Al] += cinfo->scan_size[2 + 2 * Al];
            
            for (i = 0; i < Al; i++)
              size[Al] += cinfo->scan_size[3 + 2*cinfo->Al_max_luma + i];
            
            if (size[Al] < size[cinfo->best_Al])
              cinfo->best_Al = Al;
          }
          
          /* HACK! casting a const to a non-const :-( */
          jpeg_scan_info *scanptr = (jpeg_scan_info *)&cinfo->scan_info[3*cinfo->Al_max_luma+3];
          
          for (i = 0; i < 11; i++)
            scanptr[i].Al = cinfo->best_Al;
          
        } else if (master->scan_number == cinfo->num_scans_luma-1) {
          int size[6];
          int i;
          int best = 0;
          int scan_number_base = 3*cinfo->Al_max_luma+3;
          int Al;
          
          size[0] = cinfo->scan_size[scan_number_base];
          for (i = 1; i < 6; i++) {
            size[i]  = cinfo->scan_size[scan_number_base+2*i-1];
            size[i] += cinfo->scan_size[scan_number_base+2*i];
          }
          
          if (size[1] < size[best])
            best = 1;
          if (size[2] < size[best])
            best = 2;
          if (best != 0) {
            if (size[3] < size[best])
              best = 3;
            if (best == 2) {
              if (size[4] < size[best]) {
                best = 4;
                if (size[5] < size[best])
                  best = 5;
              }
            }
          }
          
          if (best == 0)
            copy_buffer(cinfo, scan_number_base);
          else {
            copy_buffer(cinfo, scan_number_base+2*best-1);
            copy_buffer(cinfo, scan_number_base+2*best);
          }
          
          /* copy the LSB refinements as well */
          for (Al = cinfo->best_Al-1; Al >= 0; Al--)
            copy_buffer(cinfo, 2*cinfo->Al_max_luma + 3 + Al);
          
          /* free the memory allocated for the luma buffers */
          for (i = 0; i < cinfo->num_scans_luma; i++)
            free(cinfo->scan_buffer[i]);
          
        } else if (master->scan_number == cinfo->num_scans_luma+2) {
          if (cinfo->scan_size[cinfo->num_scans_luma] <= cinfo->scan_size[cinfo->num_scans_luma+1] + cinfo->scan_size[cinfo->num_scans_luma+2])
            copy_buffer(cinfo, cinfo->num_scans_luma);
          else {
            copy_buffer(cinfo, cinfo->num_scans_luma+1);
            copy_buffer(cinfo, cinfo->num_scans_luma+2);
          }
          
        } else if (master->scan_number == 41) {
          int size[3];
          size[0]  = cinfo->scan_size[26] + cinfo->scan_size[27];
          size[0] += cinfo->scan_size[28] + cinfo->scan_size[29];
          size[1]  = cinfo->scan_size[30] + cinfo->scan_size[31] + cinfo->scan_size[32];
          size[1] += cinfo->scan_size[33] + cinfo->scan_size[34] + cinfo->scan_size[35];
          size[2]  = cinfo->scan_size[36] + cinfo->scan_size[37] + cinfo->scan_size[38] + cinfo->scan_size[39];
          size[2] += cinfo->scan_size[40] + cinfo->scan_size[41] + cinfo->scan_size[34] + cinfo->scan_size[35];
          
          int Al = 0;
          int i;
          for (i = 1; i <= 3; i++)
            if (size[i] < size[Al])
              Al = i;
          cinfo->best_Al = Al;
          jpeg_scan_info *scanptr = (jpeg_scan_info *)&cinfo->scan_info[42];
          
          for (i = 0; i < 22; i++)
            scanptr[i].Al = Al;

        } else if (master->scan_number == 63) {
          int size[6];
          size[0]  = cinfo->scan_size[42];
          size[0] += cinfo->scan_size[43];
          size[1]  = cinfo->scan_size[44] + cinfo->scan_size[45];
          size[1] += cinfo->scan_size[46] + cinfo->scan_size[47];
          size[2]  = cinfo->scan_size[48] + cinfo->scan_size[49];
          size[2] += cinfo->scan_size[50] + cinfo->scan_size[51];
          size[3]  = cinfo->scan_size[52] + cinfo->scan_size[53];
          size[3] += cinfo->scan_size[54] + cinfo->scan_size[55];
          size[4]  = cinfo->scan_size[56] + cinfo->scan_size[57];
          size[4] += cinfo->scan_size[58] + cinfo->scan_size[59];
          size[5]  = cinfo->scan_size[60] + cinfo->scan_size[61];
          size[5] += cinfo->scan_size[62] + cinfo->scan_size[63];
          
          int best = 0;
          if (size[1] < size[best])
            best = 1;
          if (size[2] < size[best])
            best = 2;
          if (best != 0) {
            if (size[3] < size[best])
              best = 3;
            if (best == 2) {
              if (size[4] < size[best]) {
                best = 4;
                if (size[5] < size[best])
                  best = 5;
              }
            }
          }
          
          if (best == 0) {
            copy_buffer(cinfo, 42);
            copy_buffer(cinfo, 43);
          }
          else {
            copy_buffer(cinfo, 40+4*best);
            copy_buffer(cinfo, 41+4*best);
            copy_buffer(cinfo, 42+4*best);
            copy_buffer(cinfo, 43+4*best);
          }
          
          if (cinfo->best_Al >= 2) {
            copy_buffer(cinfo, 40);
            copy_buffer(cinfo, 41);
          }
          if (cinfo->best_Al >= 1) {
            copy_buffer(cinfo, 34);
            copy_buffer(cinfo, 35);
          }
          int i;
          for (i = 23; i < 64; i++)
            free(cinfo->scan_buffer[i]);
        }
      }

    master->scan_number++;
    break;
  }

  master->pass_number++;
}


/*
 * Initialize master compression control.
 */

GLOBAL(void)
jinit_c_master_control (j_compress_ptr cinfo, boolean transcode_only)
{
  my_master_ptr master;

  master = (my_master_ptr)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  SIZEOF(my_comp_master));
  cinfo->master = (struct jpeg_comp_master *) master;
  master->pub.prepare_for_pass = prepare_for_pass;
  master->pub.pass_startup = pass_startup;
  master->pub.finish_pass = finish_pass_master;
  master->pub.is_last_pass = FALSE;

  /* Validate parameters, determine derived values */
  initial_setup(cinfo, transcode_only);

  if (cinfo->scan_info != NULL) {
#ifdef C_MULTISCAN_FILES_SUPPORTED
    validate_script(cinfo);
#else
    ERREXIT(cinfo, JERR_NOT_COMPILED);
#endif
  } else {
    cinfo->progressive_mode = FALSE;
    cinfo->num_scans = 1;
  }

  if (cinfo->progressive_mode && !cinfo->arith_code)	/*  TEMPORARY HACK ??? */
    cinfo->optimize_coding = TRUE; /* assume default tables no good for progressive mode */

  /* Initialize my private state */
  if (transcode_only) {
    /* no main pass in transcoding */
    if (cinfo->optimize_coding)
      master->pass_type = huff_opt_pass;
    else
      master->pass_type = output_pass;
  } else {
    /* for normal compression, first pass is always this type: */
    master->pass_type = main_pass;
  }
  master->scan_number = 0;
  master->pass_number = 0;
  if (cinfo->optimize_coding)
    master->total_passes = cinfo->num_scans * 2;
  else
    master->total_passes = cinfo->num_scans;
}

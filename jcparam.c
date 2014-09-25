/*
 * jcparam.c
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1991-1998, Thomas G. Lane.
 * Modified 2003-2008 by Guido Vollbeding.
 * libjpeg-turbo Modifications:
 * Copyright (C) 2009-2011, D. R. Commander.
 * mozjpeg Modifications:
 * Copyright (C) 2014, Mozilla Corporation.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains optional default-setting code for the JPEG compressor.
 * Applications do not have to use this file, but those that don't use it
 * must know a lot more about the innards of the JPEG code.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jstdhuff.c"


/*
 * Quantization table setup routines
 */

GLOBAL(void)
jpeg_add_quant_table (j_compress_ptr cinfo, int which_tbl,
                      const unsigned int *basic_table,
                      int scale_factor, boolean force_baseline)
/* Define a quantization table equal to the basic_table times
 * a scale factor (given as a percentage).
 * If force_baseline is TRUE, the computed quantization table entries
 * are limited to 1..255 for JPEG baseline compatibility.
 */
{
  JQUANT_TBL ** qtblptr;
  int i;
  long temp;

  /* Safety check to ensure start_compress not called yet. */
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  if (which_tbl < 0 || which_tbl >= NUM_QUANT_TBLS)
    ERREXIT1(cinfo, JERR_DQT_INDEX, which_tbl);

  qtblptr = & cinfo->quant_tbl_ptrs[which_tbl];

  if (*qtblptr == NULL)
    *qtblptr = jpeg_alloc_quant_table((j_common_ptr) cinfo);

  for (i = 0; i < DCTSIZE2; i++) {
    temp = ((long) basic_table[i] * scale_factor + 50L) / 100L;
    /* limit the values to the valid range */
    if (temp <= 0L) temp = 1L;
    if (temp > 32767L) temp = 32767L; /* max quantizer needed for 12 bits */
    if (force_baseline && temp > 255L)
      temp = 255L;              /* limit to baseline range if requested */
    (*qtblptr)->quantval[i] = (UINT16) temp;
  }

  /* Initialize sent_table FALSE so table will be written to JPEG file. */
  (*qtblptr)->sent_table = FALSE;
}


/* These are the sample quantization tables given in JPEG spec section K.1.
 * The spec says that the values given produce "good" quality, and
 * when divided by 2, "very good" quality.
 */
static const unsigned int std_luminance_quant_tbl[DCTSIZE2] = {
  16,  11,  10,  16,  24,  40,  51,  61,
  12,  12,  14,  19,  26,  58,  60,  55,
  14,  13,  16,  24,  40,  57,  69,  56,
  14,  17,  22,  29,  51,  87,  80,  62,
  18,  22,  37,  56,  68, 109, 103,  77,
  24,  35,  55,  64,  81, 104, 113,  92,
  49,  64,  78,  87, 103, 121, 120, 101,
  72,  92,  95,  98, 112, 100, 103,  99
};
static const unsigned int std_chrominance_quant_tbl[DCTSIZE2] = {
  17,  18,  24,  47,  99,  99,  99,  99,
  18,  21,  26,  66,  99,  99,  99,  99,
  24,  26,  56,  99,  99,  99,  99,  99,
  47,  66,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99
};

static const unsigned int flat_quant_tbl[DCTSIZE2] = {
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16,
  16,  16,  16,  16,  16,  16,  16,  16
};

#if JPEG_LIB_VERSION >= 70
GLOBAL(void)
jpeg_default_qtables (j_compress_ptr cinfo, boolean force_baseline)
/* Set or change the 'quality' (quantization) setting, using default tables
 * and straight percentage-scaling quality scales.
 * This entry point allows different scalings for luminance and chrominance.
 */
{
  /* Set up two quantization tables using the specified scaling */
  if (cinfo->use_flat_quant_tbl) {
    jpeg_add_quant_table(cinfo, 0, flat_quant_tbl,
                         cinfo->q_scale_factor[0], force_baseline);
    jpeg_add_quant_table(cinfo, 1, flat_quant_tbl,
                         cinfo->q_scale_factor[1], force_baseline);
  } else {
  jpeg_add_quant_table(cinfo, 0, std_luminance_quant_tbl,
                       cinfo->q_scale_factor[0], force_baseline);
  jpeg_add_quant_table(cinfo, 1, std_chrominance_quant_tbl,
                       cinfo->q_scale_factor[1], force_baseline);
}
}
#endif


GLOBAL(void)
jpeg_set_linear_quality (j_compress_ptr cinfo, int scale_factor,
                         boolean force_baseline)
/* Set or change the 'quality' (quantization) setting, using default tables
 * and a straight percentage-scaling quality scale.  In most cases it's better
 * to use jpeg_set_quality (below); this entry point is provided for
 * applications that insist on a linear percentage scaling.
 */
{
  /* Set up two quantization tables using the specified scaling */
  if (cinfo->use_flat_quant_tbl) {
    jpeg_add_quant_table(cinfo, 0, flat_quant_tbl,
                         scale_factor, force_baseline);
    jpeg_add_quant_table(cinfo, 1, flat_quant_tbl,
                         scale_factor, force_baseline);
  } else {
  jpeg_add_quant_table(cinfo, 0, std_luminance_quant_tbl,
                       scale_factor, force_baseline);
  jpeg_add_quant_table(cinfo, 1, std_chrominance_quant_tbl,
                       scale_factor, force_baseline);
}
}


GLOBAL(int)
jpeg_quality_scaling (int quality)
{
  return jpeg_float_quality_scaling(quality);
}

GLOBAL(float)
jpeg_float_quality_scaling(float quality)
/* Convert a user-specified quality rating to a percentage scaling factor
 * for an underlying quantization table, using our recommended scaling curve.
 * The input 'quality' factor should be 0 (terrible) to 100 (very good).
 */
{
  /* Safety limit on quality factor.  Convert 0 to 1 to avoid zero divide. */
  if (quality <= 0.f) quality = 1.f;
  if (quality > 100.f) quality = 100.f;

  /* The basic table is used as-is (scaling 100) for a quality of 50.
   * Qualities 50..100 are converted to scaling percentage 200 - 2*Q;
   * note that at Q=100 the scaling is 0, which will cause jpeg_add_quant_table
   * to make all the table entries 1 (hence, minimum quantization loss).
   * Qualities 1..50 are converted to scaling percentage 5000/Q.
   */
  if (quality < 50.f)
    quality = 5000.f / quality;
  else
    quality = 200.f - quality*2.f;

  return quality;
}


GLOBAL(void)
jpeg_set_quality (j_compress_ptr cinfo, int quality, boolean force_baseline)
/* Set or change the 'quality' (quantization) setting, using default tables.
 * This is the standard quality-adjusting entry point for typical user
 * interfaces; only those who want detailed control over quantization tables
 * would use the preceding three routines directly.
 */
{
  /* Convert user 0-100 rating to percentage scaling */
  quality = jpeg_quality_scaling(quality);

  /* Set up standard quality tables */
  jpeg_set_linear_quality(cinfo, quality, force_baseline);
}


/*
 * Default parameter setup for compression.
 *
 * Applications that don't choose to use this routine must do their
 * own setup of all these parameters.  Alternately, you can call this
 * to establish defaults and then alter parameters selectively.  This
 * is the recommended approach since, if we add any new parameters,
 * your code will still work (they'll be set to reasonable defaults).
 */

GLOBAL(void)
jpeg_set_defaults (j_compress_ptr cinfo)
{
  int i;

  /* Safety check to ensure start_compress not called yet. */
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  /* Allocate comp_info array large enough for maximum component count.
   * Array is made permanent in case application wants to compress
   * multiple images at same param settings.
   */
  if (cinfo->comp_info == NULL)
    cinfo->comp_info = (jpeg_component_info *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                  MAX_COMPONENTS * sizeof(jpeg_component_info));

  /* Initialize everything not dependent on the color space */

#if JPEG_LIB_VERSION >= 70
  cinfo->scale_num = 1;         /* 1:1 scaling */
  cinfo->scale_denom = 1;
#endif
  cinfo->data_precision = BITS_IN_JSAMPLE;
  /* Set up two quantization tables using default quality of 75 */
  jpeg_set_quality(cinfo, 75, TRUE);
  /* Set up two Huffman tables */
  std_huff_tables((j_common_ptr) cinfo);

  /* Initialize default arithmetic coding conditioning */
  for (i = 0; i < NUM_ARITH_TBLS; i++) {
    cinfo->arith_dc_L[i] = 0;
    cinfo->arith_dc_U[i] = 1;
    cinfo->arith_ac_K[i] = 5;
  }

#ifdef C_PROGRESSIVE_SUPPORTED
  cinfo->scan_info = NULL;
  cinfo->num_scans = 0;
  if (!cinfo->use_moz_defaults) {
  /* Default is no multiple-scan output */
  cinfo->scan_info = NULL;
  cinfo->num_scans = 0;
  }
#else
  /* Default is no multiple-scan output */
  cinfo->scan_info = NULL;
  cinfo->num_scans = 0;
#endif

  /* Expect normal source image, not raw downsampled data */
  cinfo->raw_data_in = FALSE;

  /* Use Huffman coding, not arithmetic coding, by default */
  cinfo->arith_code = FALSE;

#ifdef ENTROPY_OPT_SUPPORTED
  if (cinfo->use_moz_defaults)
    /* By default, do extra passes to optimize entropy coding */
    cinfo->optimize_coding = TRUE;
  else
  /* By default, don't do extra passes to optimize entropy coding */
  cinfo->optimize_coding = FALSE;
#else
  /* By default, don't do extra passes to optimize entropy coding */
  cinfo->optimize_coding = FALSE;
#endif
  
  /* The standard Huffman tables are only valid for 8-bit data precision.
   * If the precision is higher, force optimization on so that usable
   * tables will be computed.  This test can be removed if default tables
   * are supplied that are valid for the desired precision.
   */
  if (cinfo->data_precision > 8)
    cinfo->optimize_coding = TRUE;

  /* By default, use the simpler non-cosited sampling alignment */
  cinfo->CCIR601_sampling = FALSE;

#if JPEG_LIB_VERSION >= 70
  /* By default, apply fancy downsampling */
  cinfo->do_fancy_downsampling = TRUE;
#endif

  cinfo->overshoot_deringing = cinfo->use_moz_defaults;

  /* No input smoothing */
  cinfo->smoothing_factor = 0;

  /* DCT algorithm preference */
  cinfo->dct_method = JDCT_DEFAULT;

  /* No restart markers */
  cinfo->restart_interval = 0;
  cinfo->restart_in_rows = 0;

  /* Fill in default JFIF marker parameters.  Note that whether the marker
   * will actually be written is determined by jpeg_set_colorspace.
   *
   * By default, the library emits JFIF version code 1.01.
   * An application that wants to emit JFIF 1.02 extension markers should set
   * JFIF_minor_version to 2.  We could probably get away with just defaulting
   * to 1.02, but there may still be some decoders in use that will complain
   * about that; saying 1.01 should minimize compatibility problems.
   */
  cinfo->JFIF_major_version = 1; /* Default JFIF version = 1.01 */
  cinfo->JFIF_minor_version = 1;
  cinfo->density_unit = 0;      /* Pixel size is unknown by default */
  cinfo->X_density = 1;         /* Pixel aspect ratio is square by default */
  cinfo->Y_density = 1;

  /* Choose JPEG colorspace based on input space, set defaults accordingly */

  jpeg_default_colorspace(cinfo);
  
  cinfo->one_dc_scan = TRUE;
  
#ifdef C_PROGRESSIVE_SUPPORTED
  if (cinfo->use_moz_defaults) {
    cinfo->optimize_scans = TRUE;
    jpeg_simple_progression(cinfo);
  } else
    cinfo->optimize_scans = FALSE;
#endif
  
  cinfo->trellis_quant = cinfo->use_moz_defaults;
  cinfo->lambda_log_scale1 = 16.0;
  cinfo->lambda_log_scale2 = 15.5;
  
  cinfo->use_lambda_weight_tbl = TRUE;
  cinfo->use_scans_in_trellis = FALSE;
  cinfo->trellis_freq_split = 8;
  cinfo->trellis_num_loops = 1;
  cinfo->trellis_q_opt = FALSE;
  cinfo->trellis_quant_dc = TRUE;
}


/*
 * Select an appropriate JPEG colorspace for in_color_space.
 */

GLOBAL(void)
jpeg_default_colorspace (j_compress_ptr cinfo)
{
  switch (cinfo->in_color_space) {
  case JCS_GRAYSCALE:
    jpeg_set_colorspace(cinfo, JCS_GRAYSCALE);
    break;
  case JCS_RGB:
  case JCS_EXT_RGB:
  case JCS_EXT_RGBX:
  case JCS_EXT_BGR:
  case JCS_EXT_BGRX:
  case JCS_EXT_XBGR:
  case JCS_EXT_XRGB:
  case JCS_EXT_RGBA:
  case JCS_EXT_BGRA:
  case JCS_EXT_ABGR:
  case JCS_EXT_ARGB:
    jpeg_set_colorspace(cinfo, JCS_YCbCr);
    break;
  case JCS_YCbCr:
    jpeg_set_colorspace(cinfo, JCS_YCbCr);
    break;
  case JCS_CMYK:
    jpeg_set_colorspace(cinfo, JCS_CMYK); /* By default, no translation */
    break;
  case JCS_YCCK:
    jpeg_set_colorspace(cinfo, JCS_YCCK);
    break;
  case JCS_UNKNOWN:
    jpeg_set_colorspace(cinfo, JCS_UNKNOWN);
    break;
  default:
    ERREXIT(cinfo, JERR_BAD_IN_COLORSPACE);
  }
}


/*
 * Set the JPEG colorspace, and choose colorspace-dependent default values.
 */

GLOBAL(void)
jpeg_set_colorspace (j_compress_ptr cinfo, J_COLOR_SPACE colorspace)
{
  jpeg_component_info * compptr;
  int ci;

#define SET_COMP(index,id,hsamp,vsamp,quant,dctbl,actbl)  \
  (compptr = &cinfo->comp_info[index], \
   compptr->component_id = (id), \
   compptr->h_samp_factor = (hsamp), \
   compptr->v_samp_factor = (vsamp), \
   compptr->quant_tbl_no = (quant), \
   compptr->dc_tbl_no = (dctbl), \
   compptr->ac_tbl_no = (actbl) )

  /* Safety check to ensure start_compress not called yet. */
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  /* For all colorspaces, we use Q and Huff tables 0 for luminance components,
   * tables 1 for chrominance components.
   */

  cinfo->jpeg_color_space = colorspace;

  cinfo->write_JFIF_header = FALSE; /* No marker for non-JFIF colorspaces */
  cinfo->write_Adobe_marker = FALSE; /* write no Adobe marker by default */

  switch (colorspace) {
  case JCS_GRAYSCALE:
    cinfo->write_JFIF_header = TRUE; /* Write a JFIF marker */
    cinfo->num_components = 1;
    /* JFIF specifies component ID 1 */
    SET_COMP(0, 1, 1,1, 0, 0,0);
    break;
  case JCS_RGB:
    cinfo->write_Adobe_marker = TRUE; /* write Adobe marker to flag RGB */
    cinfo->num_components = 3;
    SET_COMP(0, 0x52 /* 'R' */, 1,1, 0, 0,0);
    SET_COMP(1, 0x47 /* 'G' */, 1,1, 0, 0,0);
    SET_COMP(2, 0x42 /* 'B' */, 1,1, 0, 0,0);
    break;
  case JCS_YCbCr:
    cinfo->write_JFIF_header = TRUE; /* Write a JFIF marker */
    cinfo->num_components = 3;
    /* JFIF specifies component IDs 1,2,3 */
    /* We default to 2x2 subsamples of chrominance */
    SET_COMP(0, 1, 2,2, 0, 0,0);
    SET_COMP(1, 2, 1,1, 1, 1,1);
    SET_COMP(2, 3, 1,1, 1, 1,1);
    break;
  case JCS_CMYK:
    cinfo->write_Adobe_marker = TRUE; /* write Adobe marker to flag CMYK */
    cinfo->num_components = 4;
    SET_COMP(0, 0x43 /* 'C' */, 1,1, 0, 0,0);
    SET_COMP(1, 0x4D /* 'M' */, 1,1, 0, 0,0);
    SET_COMP(2, 0x59 /* 'Y' */, 1,1, 0, 0,0);
    SET_COMP(3, 0x4B /* 'K' */, 1,1, 0, 0,0);
    break;
  case JCS_YCCK:
    cinfo->write_Adobe_marker = TRUE; /* write Adobe marker to flag YCCK */
    cinfo->num_components = 4;
    SET_COMP(0, 1, 2,2, 0, 0,0);
    SET_COMP(1, 2, 1,1, 1, 1,1);
    SET_COMP(2, 3, 1,1, 1, 1,1);
    SET_COMP(3, 4, 2,2, 0, 0,0);
    break;
  case JCS_UNKNOWN:
    cinfo->num_components = cinfo->input_components;
    if (cinfo->num_components < 1 || cinfo->num_components > MAX_COMPONENTS)
      ERREXIT2(cinfo, JERR_COMPONENT_COUNT, cinfo->num_components,
               MAX_COMPONENTS);
    for (ci = 0; ci < cinfo->num_components; ci++) {
      SET_COMP(ci, ci, 1,1, 0, 0,0);
    }
    break;
  default:
    ERREXIT(cinfo, JERR_BAD_J_COLORSPACE);
  }
}


#ifdef C_PROGRESSIVE_SUPPORTED

LOCAL(jpeg_scan_info *)
fill_a_scan (jpeg_scan_info * scanptr, int ci,
             int Ss, int Se, int Ah, int Al)
/* Support routine: generate one scan for specified component */
{
  scanptr->comps_in_scan = 1;
  scanptr->component_index[0] = ci;
  scanptr->Ss = Ss;
  scanptr->Se = Se;
  scanptr->Ah = Ah;
  scanptr->Al = Al;
  scanptr++;
  return scanptr;
}

LOCAL(jpeg_scan_info *)
fill_a_scan_pair (jpeg_scan_info * scanptr, int ci,
             int Ss, int Se, int Ah, int Al)
/* Support routine: generate one scan for pair of components */
{
  scanptr->comps_in_scan = 2;
  scanptr->component_index[0] = ci;
  scanptr->component_index[1] = ci + 1;
  scanptr->Ss = Ss;
  scanptr->Se = Se;
  scanptr->Ah = Ah;
  scanptr->Al = Al;
  scanptr++;
  return scanptr;
}

LOCAL(jpeg_scan_info *)
fill_scans (jpeg_scan_info * scanptr, int ncomps,
            int Ss, int Se, int Ah, int Al)
/* Support routine: generate one scan for each component */
{
  int ci;

  for (ci = 0; ci < ncomps; ci++) {
    scanptr->comps_in_scan = 1;
    scanptr->component_index[0] = ci;
    scanptr->Ss = Ss;
    scanptr->Se = Se;
    scanptr->Ah = Ah;
    scanptr->Al = Al;
    scanptr++;
  }
  return scanptr;
}

LOCAL(jpeg_scan_info *)
fill_dc_scans (jpeg_scan_info * scanptr, int ncomps, int Ah, int Al)
/* Support routine: generate interleaved DC scan if possible, else N scans */
{
  int ci;

  if (ncomps <= MAX_COMPS_IN_SCAN) {
    /* Single interleaved DC scan */
    scanptr->comps_in_scan = ncomps;
    for (ci = 0; ci < ncomps; ci++)
      scanptr->component_index[ci] = ci;
    scanptr->Ss = scanptr->Se = 0;
    scanptr->Ah = Ah;
    scanptr->Al = Al;
    scanptr++;
  } else {
    /* Noninterleaved DC scan for each component */
    scanptr = fill_scans(scanptr, ncomps, 0, 0, Ah, Al);
  }
  return scanptr;
}


/*
 * List of scans to be tested
 * cinfo->num_components and cinfo->jpeg_color_space must be correct.
 */

LOCAL(boolean)
jpeg_search_progression (j_compress_ptr cinfo)
{
  int ncomps = cinfo->num_components;
  int nscans;
  jpeg_scan_info * scanptr;
  int Al;
  int frequency_split[] = { 2, 8, 5, 12, 18 };
  int i;
  
  /* Safety check to ensure start_compress not called yet. */
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);
  
  /* Figure space needed for script.  Calculation must match code below! */
  if (ncomps == 3 && cinfo->jpeg_color_space == JCS_YCbCr) {
    /* Custom script for YCbCr color images. */
    nscans = 64;
  } else if (ncomps == 1) {
    nscans = 23;
  } else {
    cinfo->num_scans_luma = 0;
    return FALSE;
  }
  
  /* Allocate space for script.
   * We need to put it in the permanent pool in case the application performs
   * multiple compressions without changing the settings.  To avoid a memory
   * leak if jpeg_simple_progression is called repeatedly for the same JPEG
   * object, we try to re-use previously allocated space, and we allocate
   * enough space to handle YCbCr even if initially asked for grayscale.
   */
  if (cinfo->script_space == NULL || cinfo->script_space_size < nscans) {
    cinfo->script_space_size = MAX(nscans, 64);
    cinfo->script_space = (jpeg_scan_info *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                                cinfo->script_space_size * sizeof(jpeg_scan_info));
  }
  scanptr = cinfo->script_space;
  cinfo->scan_info = scanptr;
  cinfo->num_scans = nscans;
  
  cinfo->Al_max_luma = 3;
  cinfo->num_scans_luma_dc = 1;
  cinfo->num_frequency_splits = 5;
  cinfo->num_scans_luma = cinfo->num_scans_luma_dc + (3 * cinfo->Al_max_luma + 2) + (2 * cinfo->num_frequency_splits + 1);
  
  /* 23 scans for luma */
  /* 1 scan for DC */
  /* 11 scans to determine successive approximation */
  /* 11 scans to determine frequency approximation */
  /* after 12 scans need to update following 11 */
  /* after 23 scans need to determine which to keep */
  /* last 4 done conditionally */
  
  /* luma DC by itself */
  if (cinfo->one_dc_scan)
    scanptr = fill_dc_scans(scanptr, ncomps, 0, 0);
  else
    scanptr = fill_dc_scans(scanptr, 1, 0, 0);
  
  scanptr = fill_a_scan(scanptr, 0, 1, 8, 0, 0);
  scanptr = fill_a_scan(scanptr, 0, 9, 63, 0, 0);
  
  for (Al = 0; Al < cinfo->Al_max_luma; Al++) {
    scanptr = fill_a_scan(scanptr, 0, 1, 63, Al+1, Al);
    scanptr = fill_a_scan(scanptr, 0, 1, 8, 0, Al+1);
    scanptr = fill_a_scan(scanptr, 0, 9, 63, 0, Al+1);
  }
  
  scanptr = fill_a_scan(scanptr, 0, 1, 63, 0, 0);
  
  for (i = 0; i < cinfo->num_frequency_splits; i++) {
    scanptr = fill_a_scan(scanptr, 0, 1, frequency_split[i], 0, 0);
    scanptr = fill_a_scan(scanptr, 0, frequency_split[i]+1, 63, 0, 0);
  }
  
  if (ncomps == 1) {
    cinfo->Al_max_chroma = 0;
    cinfo->num_scans_chroma_dc = 0;
  } else {
    cinfo->Al_max_chroma = 2;
    cinfo->num_scans_chroma_dc = 3;
    /* 41 scans for chroma */
    
    /* chroma DC combined */
    scanptr = fill_a_scan_pair(scanptr, 1, 0, 0, 0, 0);
    /* chroma DC separate */
    scanptr = fill_a_scan(scanptr, 1, 0, 0, 0, 0);
    scanptr = fill_a_scan(scanptr, 2, 0, 0, 0, 0);
    
    scanptr = fill_a_scan(scanptr, 1, 1, 8, 0, 0);
    scanptr = fill_a_scan(scanptr, 1, 9, 63, 0, 0);
    scanptr = fill_a_scan(scanptr, 2, 1, 8, 0, 0);
    scanptr = fill_a_scan(scanptr, 2, 9, 63, 0, 0);

    for (Al = 0; Al < cinfo->Al_max_chroma; Al++) {
      scanptr = fill_a_scan(scanptr, 1, 1, 63, Al+1, Al);
      scanptr = fill_a_scan(scanptr, 2, 1, 63, Al+1, Al);
      scanptr = fill_a_scan(scanptr, 1, 1, 8, 0, Al+1);
      scanptr = fill_a_scan(scanptr, 1, 9, 63, 0, Al+1);
      scanptr = fill_a_scan(scanptr, 2, 1, 8, 0, Al+1);
      scanptr = fill_a_scan(scanptr, 2, 9, 63, 0, Al+1);
    }
    
    scanptr = fill_a_scan(scanptr, 1, 1, 63, 0, 0);
    scanptr = fill_a_scan(scanptr, 2, 1, 63, 0, 0);

    for (i = 0; i < cinfo->num_frequency_splits; i++) {
      scanptr = fill_a_scan(scanptr, 1, 1, frequency_split[i], 0, 0);
      scanptr = fill_a_scan(scanptr, 1, frequency_split[i]+1, 63, 0, 0);
      scanptr = fill_a_scan(scanptr, 2, 1, frequency_split[i], 0, 0);
      scanptr = fill_a_scan(scanptr, 2, frequency_split[i]+1, 63, 0, 0);
    }
  }
  
  return TRUE;
}

/*
 * Create a recommended progressive-JPEG script.
 * cinfo->num_components and cinfo->jpeg_color_space must be correct.
 */

GLOBAL(void)
jpeg_simple_progression (j_compress_ptr cinfo)
{
  int ncomps;
  int nscans;
  jpeg_scan_info * scanptr;

  if (cinfo->optimize_scans) {
    if (jpeg_search_progression(cinfo) == TRUE)
      return;
  }

  /* Safety check to ensure start_compress not called yet. */
  if (cinfo->global_state != CSTATE_START)
    ERREXIT1(cinfo, JERR_BAD_STATE, cinfo->global_state);

  /* Figure space needed for script.  Calculation must match code below! */
  ncomps = cinfo->num_components;
  if (ncomps == 3 && cinfo->jpeg_color_space == JCS_YCbCr) {
    /* Custom script for YCbCr color images. */
    nscans = 10;
  } else {
    /* All-purpose script for other color spaces. */
    if (cinfo->use_moz_defaults == TRUE) {
    if (ncomps > MAX_COMPS_IN_SCAN)
        nscans = 5 * ncomps;	/* 2 DC + 4 AC scans per component */
      else
        nscans = 1 + 4 * ncomps;	/* 2 DC scans; 4 AC scans per component */
    } else {
      if (ncomps > MAX_COMPS_IN_SCAN)
      nscans = 6 * ncomps;      /* 2 DC + 4 AC scans per component */
    else
      nscans = 2 + 4 * ncomps;  /* 2 DC scans; 4 AC scans per component */
  }
  }

  /* Allocate space for script.
   * We need to put it in the permanent pool in case the application performs
   * multiple compressions without changing the settings.  To avoid a memory
   * leak if jpeg_simple_progression is called repeatedly for the same JPEG
   * object, we try to re-use previously allocated space, and we allocate
   * enough space to handle YCbCr even if initially asked for grayscale.
   */
  if (cinfo->script_space == NULL || cinfo->script_space_size < nscans) {
    cinfo->script_space_size = MAX(nscans, 10);
    cinfo->script_space = (jpeg_scan_info *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
                        cinfo->script_space_size * sizeof(jpeg_scan_info));
  }
  scanptr = cinfo->script_space;
  cinfo->scan_info = scanptr;
  cinfo->num_scans = nscans;

  if (ncomps == 3 && cinfo->jpeg_color_space == JCS_YCbCr) {
    /* Custom script for YCbCr color images. */
    if (cinfo->use_moz_defaults == TRUE) {
      /* scan defined in jpeg_scan_rgb.txt in jpgcrush */
    /* Initial DC scan */
      if (cinfo->one_dc_scan)
        scanptr = fill_dc_scans(scanptr, ncomps, 0, 0);
      else if (cinfo->sep_dc_scan) {
        scanptr = fill_a_scan(scanptr, 0, 0, 0, 0, 0);
        scanptr = fill_a_scan(scanptr, 1, 0, 0, 0, 0);
        scanptr = fill_a_scan(scanptr, 2, 0, 0, 0, 0);
      }
      else {
        scanptr = fill_dc_scans(scanptr, 1, 0, 0);
        scanptr = fill_a_scan_pair(scanptr, 1, 0, 0, 0, 0);
      }
      /* Low frequency AC scans */
      scanptr = fill_a_scan(scanptr, 0, 1, 8, 0, 2);
      scanptr = fill_a_scan(scanptr, 1, 1, 8, 0, 0);
      scanptr = fill_a_scan(scanptr, 2, 1, 8, 0, 0);
      /* Complete spectral selection for luma AC */
      scanptr = fill_a_scan(scanptr, 0, 9, 63, 0, 2);
      /* Finish luma AC successive approximation */
      scanptr = fill_a_scan(scanptr, 0, 1, 63, 2, 1);
      scanptr = fill_a_scan(scanptr, 0, 1, 63, 1, 0);
      /* Complete spectral selection for chroma AC */
      scanptr = fill_a_scan(scanptr, 1, 9, 63, 0, 0);
      scanptr = fill_a_scan(scanptr, 2, 9, 63, 0, 0);
    } else {
      /* Initial DC scan */
    scanptr = fill_dc_scans(scanptr, ncomps, 0, 1);
    /* Initial AC scan: get some luma data out in a hurry */
    scanptr = fill_a_scan(scanptr, 0, 1, 5, 0, 2);
    /* Chroma data is too small to be worth expending many scans on */
    scanptr = fill_a_scan(scanptr, 2, 1, 63, 0, 1);
    scanptr = fill_a_scan(scanptr, 1, 1, 63, 0, 1);
    /* Complete spectral selection for luma AC */
    scanptr = fill_a_scan(scanptr, 0, 6, 63, 0, 2);
    /* Refine next bit of luma AC */
    scanptr = fill_a_scan(scanptr, 0, 1, 63, 2, 1);
    /* Finish DC successive approximation */
    scanptr = fill_dc_scans(scanptr, ncomps, 1, 0);
    /* Finish AC successive approximation */
    scanptr = fill_a_scan(scanptr, 2, 1, 63, 1, 0);
    scanptr = fill_a_scan(scanptr, 1, 1, 63, 1, 0);
    /* Luma bottom bit comes last since it's usually largest scan */
    scanptr = fill_a_scan(scanptr, 0, 1, 63, 1, 0);
    }
  } else {
    /* All-purpose script for other color spaces. */
    if (cinfo->use_moz_defaults == TRUE) {
      /* scan defined in jpeg_scan_bw.txt in jpgcrush */
      /* DC component, no successive approximation */
      scanptr = fill_dc_scans(scanptr, ncomps, 0, 0);
    /* Successive approximation first pass */
      scanptr = fill_scans(scanptr, ncomps, 1, 8, 0, 2);
      scanptr = fill_scans(scanptr, ncomps, 9, 63, 0, 2);
      /* Successive approximation second pass */
      scanptr = fill_scans(scanptr, ncomps, 1, 63, 2, 1);
      /* Successive approximation final pass */
      scanptr = fill_scans(scanptr, ncomps, 1, 63, 1, 0);
    } else {
      /* Successive approximation first pass */
    scanptr = fill_dc_scans(scanptr, ncomps, 0, 1);
    scanptr = fill_scans(scanptr, ncomps, 1, 5, 0, 2);
    scanptr = fill_scans(scanptr, ncomps, 6, 63, 0, 2);
    /* Successive approximation second pass */
    scanptr = fill_scans(scanptr, ncomps, 1, 63, 2, 1);
    /* Successive approximation final pass */
    scanptr = fill_dc_scans(scanptr, ncomps, 1, 0);
    scanptr = fill_scans(scanptr, ncomps, 1, 63, 1, 0);
  }
}
}


#endif /* C_PROGRESSIVE_SUPPORTED */

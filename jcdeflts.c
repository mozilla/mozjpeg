/*
 * jcdeflts.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains optional default-setting code for the JPEG compressor.
 */

#include "jinclude.h"


LOCAL void
add_huff_table (compress_info_ptr cinfo,
		HUFF_TBL **htblptr, const UINT8 *bits, const UINT8 *val)
/* Define a Huffman table */
{
  if (*htblptr == NULL)
    *htblptr = (*cinfo->emethods->alloc_small) (SIZEOF(HUFF_TBL));
  
  memcpy((void *) (*htblptr)->bits, (void *) bits,
	 SIZEOF((*htblptr)->bits));
  memcpy((void *) (*htblptr)->huffval, (void *) val,
	 SIZEOF((*htblptr)->huffval));

  /* Initialize sent_table FALSE so table will be written to JPEG file.
   * In an application where we are writing non-interchange JPEG files,
   * it might be desirable to save space by leaving default Huffman tables
   * out of the file.  To do that, just initialize sent_table = TRUE...
   */

  (*htblptr)->sent_table = FALSE;
}


LOCAL void
std_huff_tables (compress_info_ptr cinfo)
/* Set up the standard Huffman tables (cf. JPEG-8-R8 section 13.3) */
{
  static const UINT8 dc_luminance_bits[17] =
    { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  static const UINT8 dc_luminance_val[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const UINT8 dc_chrominance_bits[17] =
    { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  static const UINT8 dc_chrominance_val[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
  
  static const UINT8 ac_luminance_bits[17] =
    { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  static const UINT8 ac_luminance_val[] =
    { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
      0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
      0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
      0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
      0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
      0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
      0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
      0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
      0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
      0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
      0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
      0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
  static const UINT8 ac_chrominance_bits[17] =
    { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  static const UINT8 ac_chrominance_val[] =
    { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
      0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
      0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
      0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
      0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
      0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
      0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
      0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
      0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
      0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
      0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
      0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
      0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
      0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
      0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
      0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
      0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
      0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
      0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  
  add_huff_table(cinfo, &cinfo->dc_huff_tbl_ptrs[0],
		 dc_luminance_bits, dc_luminance_val);
  add_huff_table(cinfo, &cinfo->ac_huff_tbl_ptrs[0],
		 ac_luminance_bits, ac_luminance_val);
  add_huff_table(cinfo, &cinfo->dc_huff_tbl_ptrs[1],
		 dc_chrominance_bits, dc_chrominance_val);
  add_huff_table(cinfo, &cinfo->ac_huff_tbl_ptrs[1],
		 ac_chrominance_bits, ac_chrominance_val);
}


/* This is the sample quantization table given in JPEG-8-R8 sec 13.1,
 * but expressed in zigzag order (as are all of our quant. tables).
 * The spec says that the values given produce "good" quality, and
 * when divided by 2, "very good" quality.  (These two settings are
 * selected by quality=50 and quality=75 in j_set_quality, below.)
 */


static const QUANT_VAL std_luminance_quant_tbl[DCTSIZE2] = {
  16,  11,  12,  14,  12,  10,  16,  14,
  13,  14,  18,  17,  16,  19,  24,  40,
  26,  24,  22,  22,  24,  49,  35,  37,
  29,  40,  58,  51,  61,  60,  57,  51,
  56,  55,  64,  72,  92,  78,  64,  68,
  87,  69,  55,  56,  80, 109,  81,  87,
  95,  98, 103, 104, 103,  62,  77, 113,
 121, 112, 100, 120,  92, 101, 103,  99
};

static const QUANT_VAL std_chrominance_quant_tbl[DCTSIZE2] = {
  17,  18,  18,  24,  21,  24,  47,  26,
  26,  47,  99,  66,  56,  66,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99,
  99,  99,  99,  99,  99,  99,  99,  99
};


LOCAL void
add_quant_table (compress_info_ptr cinfo, int which_tbl,
		 const QUANT_VAL *basic_table, int scale_factor,
		 boolean force_baseline)
/* Define a quantization table equal to the basic_table times */
/* a scale factor (given as a percentage) */
{
  QUANT_TBL_PTR * qtblptr = & cinfo->quant_tbl_ptrs[which_tbl];
  int i;
  long temp;

  if (*qtblptr == NULL)
    *qtblptr = (*cinfo->emethods->alloc_small) (SIZEOF(QUANT_TBL));

  for (i = 0; i < DCTSIZE2; i++) {
    temp = ((long) basic_table[i] * scale_factor + 50L) / 100L;
    /* limit the values to the valid range */
    if (temp <= 0L) temp = 1L;
#ifdef EIGHT_BIT_SAMPLES
    if (temp > 32767L) temp = 32767L; /* QUANT_VALs are 'short' */
#else
    if (temp > 65535L) temp = 65535L; /* QUANT_VALs are 'UINT16' */
#endif
    if (force_baseline && temp > 255L)
      temp = 255L;		/* limit to baseline range if requested */
    (*qtblptr)[i] = (QUANT_VAL) temp;
  }
}


GLOBAL void
j_set_quality (compress_info_ptr cinfo, int quality, boolean force_baseline)
/* Set or change the 'quality' (quantization) setting. */
/* The 'quality' factor should be 0 (terrible) to 100 (very good). */
/* Quality 50 corresponds to the JPEG basic tables given above; */
/* quality 100 results in no quantization scaling at all. */
/* If force_baseline is TRUE, quantization table entries are limited */
/* to 0..255 for JPEG baseline compatibility; this is only an issue */
/* for quality settings below 24. */
{
  /* Safety limit on quality factor.  Convert 0 to 1 to avoid zero divide. */
  if (quality <= 0) quality = 1;
  if (quality > 100) quality = 100;

  /* Convert quality rating to a percentage scaling of the basic tables.
   * The basic table is used as-is (scaling 100) for a quality of 50.
   * Qualities 50..100 are converted to scaling percentage 200 - 2*Q;
   * note that at Q=100 the scaling is 0, which will cause add_quant_table
   * to make all the table entries 1 (hence, no quantization loss).
   * Qualities 1..50 are converted to scaling percentage 5000/Q.
   */
  if (quality < 50)
    quality = 5000 / quality;
  else
    quality = 200 - quality*2;

  /* Set up two quantization tables using the specified quality scaling */
  add_quant_table(cinfo, 0, std_luminance_quant_tbl, quality, force_baseline);
  add_quant_table(cinfo, 1, std_chrominance_quant_tbl, quality, force_baseline);
}



/* Default parameter setup for compression.
 *
 * User interfaces that don't choose to use this routine must do their
 * own setup of all these parameters.  Alternately, you can call this
 * to establish defaults and then alter parameters selectively.
 *
 * See above for the meaning of the 'quality' parameter.  Typically,
 * the application's default quality setting will be passed to this
 * routine.  A later call on j_set_quality() can be used to change to
 * a user-specified quality setting.
 *
 * This sets up for a color image; to output a grayscale image,
 * do this first and call j_monochrome_default() afterwards.
 * (The latter can be called within c_ui_method_selection, so the
 * choice can depend on the input file header.)
 * Note that if you want a JPEG colorspace other than GRAYSCALE or YCbCr,
 * you should also change the component ID codes, and you should NOT emit
 * a JFIF header (set write_JFIF_header = FALSE).
 *
 * CAUTION: if you want to compress multiple images per run, it's safest
 * to call j_default_compression before *each* call to jpeg_compress (and
 * j_free_defaults afterwards).  If this isn't practical, you'll have to
 * be careful to reset any individual parameters that may change during
 * the compression run.  The main thing you need to worry about as this
 * is written is that the sent_table boolean in each Huffman table must
 * be reset to FALSE before each compression; otherwise, Huffman tables
 * won't get emitted for the second and subsequent images.
 */

GLOBAL void
j_default_compression (compress_info_ptr cinfo, int quality)
/* NB: the external methods must already be set up. */
{
  short i;
  jpeg_component_info * compptr;

  /* Initialize pointers as needed to mark stuff unallocated. */
  cinfo->comp_info = NULL;
  for (i = 0; i < NUM_QUANT_TBLS; i++)
    cinfo->quant_tbl_ptrs[i] = NULL;
  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    cinfo->dc_huff_tbl_ptrs[i] = NULL;
    cinfo->ac_huff_tbl_ptrs[i] = NULL;
  }

  cinfo->data_precision = 8;	/* default; can be overridden by input_init */
  cinfo->density_unit = 0;	/* Pixel size is unknown by default */
  cinfo->X_density = 1;		/* Pixel aspect ratio is square by default */
  cinfo->Y_density = 1;

  cinfo->input_gamma = 1.0;	/* no gamma correction by default */

  /* Prepare three color components; first is luminance which is also usable */
  /* for grayscale.  The others are assumed to be UV or similar chrominance. */
  cinfo->write_JFIF_header = TRUE;
  cinfo->jpeg_color_space = CS_YCbCr;
  cinfo->num_components = 3;
  cinfo->comp_info = (*cinfo->emethods->alloc_small)
			(4 * SIZEOF(jpeg_component_info));
  /* Note: we allocate a 4-entry comp_info array so that user interface can
   * easily change over to CMYK color space if desired.
   */

  compptr = &cinfo->comp_info[0];
  compptr->component_index = 0;
  compptr->component_id = 1;	/* JFIF specifies IDs 1,2,3 */
  compptr->h_samp_factor = 2;	/* default to 2x2 subsamples of chrominance */
  compptr->v_samp_factor = 2;
  compptr->quant_tbl_no = 0;	/* use tables 0 for luminance */
  compptr->dc_tbl_no = 0;
  compptr->ac_tbl_no = 0;

  compptr = &cinfo->comp_info[1];
  compptr->component_index = 1;
  compptr->component_id = 2;
  compptr->h_samp_factor = 1;
  compptr->v_samp_factor = 1;
  compptr->quant_tbl_no = 1;	/* use tables 1 for chrominance */
  compptr->dc_tbl_no = 1;
  compptr->ac_tbl_no = 1;

  compptr = &cinfo->comp_info[2];
  compptr->component_index = 2;
  compptr->component_id = 3;
  compptr->h_samp_factor = 1;
  compptr->v_samp_factor = 1;
  compptr->quant_tbl_no = 1;	/* use tables 1 for chrominance */
  compptr->dc_tbl_no = 1;
  compptr->ac_tbl_no = 1;

  /* Set up two quantization tables using the specified quality scaling */
  /* Baseline compatibility is forced (a nonissue for reasonable defaults) */
  j_set_quality(cinfo, quality, TRUE);

  /* Set up two Huffman tables in case user interface wants Huffman coding */
  std_huff_tables(cinfo);

  /* Initialize default arithmetic coding conditioning */
  for (i = 0; i < NUM_ARITH_TBLS; i++) {
    cinfo->arith_dc_L[i] = 0;
    cinfo->arith_dc_U[i] = 1;
    cinfo->arith_ac_K[i] = 5;
  }

  /* Use Huffman coding, not arithmetic coding, by default */
  cinfo->arith_code = FALSE;

  /* Color images are interleaved by default */
  cinfo->interleave = TRUE;

  /* By default, don't do extra passes to optimize entropy coding */
  cinfo->optimize_coding = FALSE;

  /* By default, use the simpler non-cosited sampling alignment */
  cinfo->CCIR601_sampling = FALSE;

  /* No restart markers */
  cinfo->restart_interval = 0;
}



GLOBAL void
j_monochrome_default (compress_info_ptr cinfo)
/* Change the j_default_compression() values to emit a monochrome JPEG file. */
{
  jpeg_component_info * compptr;

  cinfo->jpeg_color_space = CS_GRAYSCALE;
  cinfo->num_components = 1;
  /* Set single component to 1x1 subsampling */
  compptr = &cinfo->comp_info[0];
  compptr->h_samp_factor = 1;
  compptr->v_samp_factor = 1;
}



/* This routine releases storage allocated by j_default_compression.
 * Note that freeing the method pointer structs and the compress_info_struct
 * itself are the responsibility of the user interface.
 */

GLOBAL void
j_free_defaults (compress_info_ptr cinfo)
{
  short i;

#define FREE(ptr)  if ((ptr) != NULL) \
			(*cinfo->emethods->free_small) ((void *) ptr)

  FREE(cinfo->comp_info);
  for (i = 0; i < NUM_QUANT_TBLS; i++)
    FREE(cinfo->quant_tbl_ptrs[i]);
  for (i = 0; i < NUM_HUFF_TBLS; i++) {
    FREE(cinfo->dc_huff_tbl_ptrs[i]);
    FREE(cinfo->ac_huff_tbl_ptrs[i]);
  }
}

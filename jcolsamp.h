/*
 * jcolsamp.h - private declarations for color conversion & up/downsampling
 *
 * x86 SIMD extension for IJG JPEG library
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 * For conditions of distribution and use, see copyright notice in jsimdext.inc
 *
 * Last Modified : February 4, 2006
 *
 * [TAB8]
 */


/* configuration check: BITS_IN_JSAMPLE==8 (8-bit sample values) is the only
 * valid setting on this SIMD extension.
 */
#if BITS_IN_JSAMPLE != 8
#error "Sorry, this SIMD code only copes with 8-bit sample values."
#endif

/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jpeg_rgb_ycc_convert_mmx	jMRgbYccCnv	/* jccolmmx.asm */
#define jpeg_rgb_ycc_convert_sse2	jSRgbYccCnv	/* jccolss2.asm */
#define jpeg_h2v1_downsample_mmx	jM21Downsample	/* jcsammmx.asm */
#define jpeg_h2v2_downsample_mmx	jM22Downsample	/* jcsammmx.asm */
#define jpeg_h2v1_downsample_sse2	jS21Downsample	/* jcsamss2.asm */
#define jpeg_h2v2_downsample_sse2	jS22Downsample	/* jcsamss2.asm */
#define jpeg_ycc_rgb_convert_mmx	jMYccRgbCnv	/* jdcolmmx.asm */
#define jpeg_ycc_rgb_convert_sse2	jSYccRgbCnv	/* jdcolss2.asm */
#define jpeg_h2v1_merged_upsample_mmx	jM21MerUpsample	/* jdmermmx.asm */
#define jpeg_h2v2_merged_upsample_mmx	jM22MerUpsample	/* jdmermmx.asm */
#define jpeg_h2v1_merged_upsample_sse2	jS21MerUpsample	/* jdmerss2.asm */
#define jpeg_h2v2_merged_upsample_sse2	jS22MerUpsample	/* jdmerss2.asm */
#define jpeg_h2v1_fancy_upsample_mmx	jM21FanUpsample	/* jdsammmx.asm */
#define jpeg_h2v2_fancy_upsample_mmx	jM22FanUpsample	/* jdsammmx.asm */
#define jpeg_h1v2_fancy_upsample_mmx	jM12FanUpsample	/* jdsammmx.asm */
#define jpeg_h2v1_upsample_mmx		jM21Upsample	/* jdsammmx.asm */
#define jpeg_h2v2_upsample_mmx		jM22Upsample	/* jdsammmx.asm */
#define jpeg_h2v1_fancy_upsample_sse2	jS21FanUpsample	/* jdsamss2.asm */
#define jpeg_h2v2_fancy_upsample_sse2	jS22FanUpsample	/* jdsamss2.asm */
#define jpeg_h1v2_fancy_upsample_sse2	jS12FanUpsample	/* jdsamss2.asm */
#define jpeg_h2v1_upsample_sse2		jS21Upsample	/* jdsamss2.asm */
#define jpeg_h2v2_upsample_sse2		jS22Upsample	/* jdsamss2.asm */
#define jconst_rgb_ycc_convert_mmx	jMCRgbYccCnv	/* jccolmmx.asm */
#define jconst_rgb_ycc_convert_sse2	jSCRgbYccCnv	/* jccolss2.asm */
#define jconst_ycc_rgb_convert_mmx	jMCYccRgbCnv	/* jdcolmmx.asm */
#define jconst_ycc_rgb_convert_sse2	jSCYccRgbCnv	/* jdcolss2.asm */
#define jconst_merged_upsample_mmx	jMCMerUpsample	/* jdmermmx.asm */
#define jconst_merged_upsample_sse2	jSCMerUpsample	/* jdmerss2.asm */
#define jconst_fancy_upsample_mmx	jMCFanUpsample	/* jdsammmx.asm */
#define jconst_fancy_upsample_sse2	jSCFanUpsample	/* jdsamss2.asm */
#ifndef JSIMD_MODEINFO_NOT_SUPPORTED
#define jpeg_simd_merged_upsampler	jSiMUpsampler	/* jdmerge.c    */
#endif
#endif /* NEED_SHORT_EXTERNAL_NAMES */

/* Extern declarations for color conversion & up/downsampling routines. */

EXTERN(void) jpeg_rgb_ycc_convert_mmx
    JPP((j_compress_ptr cinfo, JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
	 JDIMENSION output_row, int num_rows));
EXTERN(void) jpeg_rgb_ycc_convert_sse2
    JPP((j_compress_ptr cinfo, JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
	 JDIMENSION output_row, int num_rows));

EXTERN(void) jpeg_h2v1_downsample_mmx
    JPP((j_compress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY output_data));
EXTERN(void) jpeg_h2v2_downsample_mmx
    JPP((j_compress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY output_data));
EXTERN(void) jpeg_h2v1_downsample_sse2
    JPP((j_compress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY output_data));
EXTERN(void) jpeg_h2v2_downsample_sse2
    JPP((j_compress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY output_data));

EXTERN(void) jpeg_ycc_rgb_convert_mmx
    JPP((j_decompress_ptr cinfo, JSAMPIMAGE input_buf, JDIMENSION input_row,
	 JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jpeg_ycc_rgb_convert_sse2
    JPP((j_decompress_ptr cinfo, JSAMPIMAGE input_buf, JDIMENSION input_row,
	 JSAMPARRAY output_buf, int num_rows));

EXTERN(void) jpeg_h2v1_merged_upsample_mmx
    JPP((j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
	 JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jpeg_h2v2_merged_upsample_mmx
    JPP((j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
	 JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jpeg_h2v1_merged_upsample_sse2
    JPP((j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
	 JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jpeg_h2v2_merged_upsample_sse2
    JPP((j_decompress_ptr cinfo, JSAMPIMAGE input_buf,
	 JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));

EXTERN(void) jpeg_h2v1_fancy_upsample_mmx
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jpeg_h2v2_fancy_upsample_mmx
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jpeg_h1v2_fancy_upsample_mmx
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jpeg_h2v1_upsample_mmx
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jpeg_h2v2_upsample_mmx
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jpeg_h2v1_fancy_upsample_sse2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jpeg_h2v2_fancy_upsample_sse2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jpeg_h1v2_fancy_upsample_sse2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jpeg_h2v1_upsample_sse2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jpeg_h2v2_upsample_sse2
    JPP((j_decompress_ptr cinfo, jpeg_component_info * compptr,
	 JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

extern const int jconst_rgb_ycc_convert_mmx[];
extern const int jconst_rgb_ycc_convert_sse2[];
extern const int jconst_ycc_rgb_convert_mmx[];
extern const int jconst_ycc_rgb_convert_sse2[];
extern const int jconst_merged_upsample_mmx[];
extern const int jconst_merged_upsample_sse2[];
extern const int jconst_fancy_upsample_mmx[];
extern const int jconst_fancy_upsample_sse2[];

#ifndef JSIMD_MODEINFO_NOT_SUPPORTED
EXTERN(unsigned int) jpeg_simd_merged_upsampler JPP((j_decompress_ptr cinfo));
#endif

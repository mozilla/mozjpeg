/*
 * simd/jsimd.h
 *
 * Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright 2011 D. R. Commander
 * Copyright (C) 2013-2014, MIPS Technologies, Inc., California
 *
 * Based on the x86 SIMD extension for IJG JPEG library,
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 * For conditions of distribution and use, see copyright notice in jsimdext.inc
 *
 */

/* Bitmask for supported acceleration methods */

#define JSIMD_NONE       0x00
#define JSIMD_MMX        0x01
#define JSIMD_3DNOW      0x02
#define JSIMD_SSE        0x04
#define JSIMD_SSE2       0x08
#define JSIMD_ARM_NEON   0x10
#define JSIMD_MIPS_DSPR2 0x20

/* SIMD Ext: retrieve SIMD/CPU information */
EXTERN(unsigned int) jpeg_simd_cpu_support JPP((void));

/* SIMD Color Space Conversion */
EXTERN(void) jsimd_rgb_ycc_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgb_ycc_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgbx_ycc_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgr_ycc_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgrx_ycc_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxbgr_ycc_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxrgb_ycc_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));

EXTERN(void) jsimd_rgb_gray_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgb_gray_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgbx_gray_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgr_gray_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgrx_gray_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxbgr_gray_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxrgb_gray_convert_mmx
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));

EXTERN(void) jsimd_ycc_rgb_convert_mmx
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extrgb_convert_mmx
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extrgbx_convert_mmx
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extbgr_convert_mmx
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extbgrx_convert_mmx
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extxbgr_convert_mmx
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extxrgb_convert_mmx
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));

extern const int jconst_rgb_ycc_convert_sse2[];
EXTERN(void) jsimd_rgb_ycc_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgb_ycc_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgbx_ycc_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgr_ycc_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgrx_ycc_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxbgr_ycc_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxrgb_ycc_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));

extern const int jconst_rgb_gray_convert_sse2[];
EXTERN(void) jsimd_rgb_gray_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgb_gray_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgbx_gray_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgr_gray_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgrx_gray_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxbgr_gray_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxrgb_gray_convert_sse2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));

extern const int jconst_ycc_rgb_convert_sse2[];
EXTERN(void) jsimd_ycc_rgb_convert_sse2
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extrgb_convert_sse2
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extrgbx_convert_sse2
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extbgr_convert_sse2
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extbgrx_convert_sse2
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extxbgr_convert_sse2
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extxrgb_convert_sse2
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));

EXTERN(void) jsimd_rgb_ycc_convert_neon
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgb_ycc_convert_neon
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgbx_ycc_convert_neon
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgr_ycc_convert_neon
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgrx_ycc_convert_neon
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxbgr_ycc_convert_neon
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxrgb_ycc_convert_neon
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));

EXTERN(void) jsimd_ycc_rgb_convert_neon
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extrgb_convert_neon
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extrgbx_convert_neon
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extbgr_convert_neon
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extbgrx_convert_neon
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extxbgr_convert_neon
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extxrgb_convert_neon
        JPP((JDIMENSION out_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));

EXTERN(void) jsimd_rgb_ycc_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgb_ycc_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgbx_ycc_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgr_ycc_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgrx_ycc_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxbgr_ycc_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxrgb_ycc_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));

EXTERN(void) jsimd_rgb_gray_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgb_gray_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extrgbx_gray_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgr_gray_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extbgrx_gray_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxbgr_gray_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));
EXTERN(void) jsimd_extxrgb_gray_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
             JDIMENSION output_row, int num_rows));

EXTERN (void) jsimd_ycc_rgb_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extrgb_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extrgbx_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extbgr_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extbgrx_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extxbgr_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));
EXTERN(void) jsimd_ycc_extxrgb_convert_mips_dspr2
        JPP((JDIMENSION img_width,
             JSAMPIMAGE input_buf, JDIMENSION input_row,
             JSAMPARRAY output_buf, int num_rows));

EXTERN(void) jsimd_c_null_convert_mips_dspr2
        JPP((JDIMENSION img_width, JSAMPARRAY input_buf,
             JSAMPIMAGE output_buf, JDIMENSION output_row,
             int num_rows, int num_components));

/* SIMD Downsample */
EXTERN(void) jsimd_h2v2_downsample_mmx
        JPP((JDIMENSION image_width, int max_v_samp_factor,
             JDIMENSION v_samp_factor, JDIMENSION width_blocks,
             JSAMPARRAY input_data, JSAMPARRAY output_data));
EXTERN(void) jsimd_h2v1_downsample_mmx
        JPP((JDIMENSION image_width, int max_v_samp_factor,
             JDIMENSION v_samp_factor, JDIMENSION width_blocks,
             JSAMPARRAY input_data, JSAMPARRAY output_data));

EXTERN(void) jsimd_h2v2_downsample_sse2
        JPP((JDIMENSION image_width, int max_v_samp_factor,
             JDIMENSION v_samp_factor, JDIMENSION width_blocks,
             JSAMPARRAY input_data, JSAMPARRAY output_data));
EXTERN(void) jsimd_h2v1_downsample_sse2
        JPP((JDIMENSION image_width, int max_v_samp_factor,
             JDIMENSION v_samp_factor, JDIMENSION width_blocks,
             JSAMPARRAY input_data, JSAMPARRAY output_data));

EXTERN(void) jsimd_h2v2_downsample_mips_dspr2
        JPP((JDIMENSION image_width, int max_v_samp_factor,
             JDIMENSION v_samp_factor, JDIMENSION width_blocks,
             JSAMPARRAY input_data, JSAMPARRAY output_data));
EXTERN(void) jsimd_h2v2_smooth_downsample_mips_dspr2
        JPP((JSAMPARRAY input_data, JSAMPARRAY output_data,
             JDIMENSION v_samp_factor, int max_v_samp_factor,
             int smoothing_factor, JDIMENSION width_blocks,
             JDIMENSION image_width));
EXTERN(void) jsimd_h2v1_downsample_mips_dspr2
        JPP((JDIMENSION image_width, int max_v_samp_factor,
             JDIMENSION v_samp_factor, JDIMENSION width_blocks,
             JSAMPARRAY input_data, JSAMPARRAY output_data));

/* SIMD Upsample */
EXTERN(void) jsimd_h2v2_upsample_mmx
        JPP((int max_v_samp_factor, JDIMENSION output_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jsimd_h2v1_upsample_mmx
        JPP((int max_v_samp_factor, JDIMENSION output_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

EXTERN(void) jsimd_h2v2_fancy_upsample_mmx
        JPP((int max_v_samp_factor, JDIMENSION downsampled_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jsimd_h2v1_fancy_upsample_mmx
        JPP((int max_v_samp_factor, JDIMENSION downsampled_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

EXTERN(void) jsimd_h2v2_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extrgb_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extrgbx_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extbgr_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extbgrx_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extxbgr_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extxrgb_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extrgb_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extrgbx_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extbgr_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extbgrx_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extxbgr_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extxrgb_merged_upsample_mmx
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));

EXTERN(void) jsimd_h2v2_upsample_sse2
        JPP((int max_v_samp_factor, JDIMENSION output_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jsimd_h2v1_upsample_sse2
        JPP((int max_v_samp_factor, JDIMENSION output_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

extern const int jconst_fancy_upsample_sse2[];
EXTERN(void) jsimd_h2v2_fancy_upsample_sse2
        JPP((int max_v_samp_factor, JDIMENSION downsampled_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jsimd_h2v1_fancy_upsample_sse2
        JPP((int max_v_samp_factor, JDIMENSION downsampled_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

extern const int jconst_merged_upsample_sse2[];
EXTERN(void) jsimd_h2v2_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extrgb_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extrgbx_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extbgr_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extbgrx_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extxbgr_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v2_extxrgb_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extrgb_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extrgbx_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extbgr_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extbgrx_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extxbgr_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));
EXTERN(void) jsimd_h2v1_extxrgb_merged_upsample_sse2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf));

EXTERN(void) jsimd_h2v1_fancy_upsample_neon
        JPP((int max_v_samp_factor, JDIMENSION downsampled_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

EXTERN(void) jsimd_h2v1_fancy_upsample_mips_dspr2
        JPP((int max_v_samp_factor, JDIMENSION downsampled_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jsimd_h2v2_fancy_upsample_mips_dspr2
        JPP((int max_v_samp_factor, JDIMENSION downsampled_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

EXTERN(void) jsimd_h2v2_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v2_extrgb_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v2_extrgbx_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v2_extbgr_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v2_extbgrx_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v2_extxbgr_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v2_extxrgb_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v1_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v1_extrgb_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v1_extrgbx_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v1_extbgr_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v1_extbgrx_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v1_extxbgr_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));
EXTERN(void) jsimd_h2v1_extxrgb_merged_upsample_mips_dspr2
        JPP((JDIMENSION output_width, JSAMPIMAGE input_buf,
             JDIMENSION in_row_group_ctr, JSAMPARRAY output_buf,
             JSAMPLE* range));

EXTERN(void) jsimd_h2v2_upsample_mips_dspr2
        JPP((int max_v_samp_factor, JDIMENSION output_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));
EXTERN(void) jsimd_h2v1_upsample_mips_dspr2
        JPP((int max_v_samp_factor, JDIMENSION output_width,
             JSAMPARRAY input_data, JSAMPARRAY * output_data_ptr));

/* SIMD Sample Conversion */
EXTERN(void) jsimd_convsamp_mmx JPP((JSAMPARRAY sample_data,
                                     JDIMENSION start_col,
                                     DCTELEM * workspace));

EXTERN(void) jsimd_convsamp_sse2 JPP((JSAMPARRAY sample_data,
                                      JDIMENSION start_col,
                                      DCTELEM * workspace));

EXTERN(void) jsimd_convsamp_neon JPP((JSAMPARRAY sample_data,
                                      JDIMENSION start_col,
                                      DCTELEM * workspace));

EXTERN(void) jsimd_convsamp_mips_dspr2 JPP((JSAMPARRAY sample_data,
                                            JDIMENSION start_col,
                                            DCTELEM * workspace));

EXTERN(void) jsimd_convsamp_float_3dnow JPP((JSAMPARRAY sample_data,
                                             JDIMENSION start_col,
                                             FAST_FLOAT * workspace));

EXTERN(void) jsimd_convsamp_float_sse JPP((JSAMPARRAY sample_data,
                                           JDIMENSION start_col,
                                           FAST_FLOAT * workspace));

EXTERN(void) jsimd_convsamp_float_sse2 JPP((JSAMPARRAY sample_data,
                                            JDIMENSION start_col,
                                            FAST_FLOAT * workspace));

EXTERN(void) jsimd_convsamp_float_mips_dspr2 JPP((JSAMPARRAY sample_data,
                                                  JDIMENSION start_col,
                                                  FAST_FLOAT * workspace));

/* SIMD Forward DCT */
EXTERN(void) jsimd_fdct_islow_mmx JPP((DCTELEM * data));
EXTERN(void) jsimd_fdct_ifast_mmx JPP((DCTELEM * data));

extern const int jconst_fdct_ifast_sse2[];
EXTERN(void) jsimd_fdct_islow_sse2 JPP((DCTELEM * data));
extern const int jconst_fdct_islow_sse2[];
EXTERN(void) jsimd_fdct_ifast_sse2 JPP((DCTELEM * data));

EXTERN(void) jsimd_fdct_ifast_neon JPP((DCTELEM * data));

EXTERN(void) jsimd_fdct_islow_mips_dspr2 JPP((DCTELEM * data));
EXTERN(void) jsimd_fdct_ifast_mips_dspr2 JPP((DCTELEM * data));

EXTERN(void) jsimd_fdct_float_3dnow JPP((FAST_FLOAT * data));

extern const int jconst_fdct_float_sse[];
EXTERN(void) jsimd_fdct_float_sse JPP((FAST_FLOAT * data));

/* SIMD Quantization */
EXTERN(void) jsimd_quantize_mmx JPP((JCOEFPTR coef_block,
                                     DCTELEM * divisors,
                                     DCTELEM * workspace));

EXTERN(void) jsimd_quantize_sse2 JPP((JCOEFPTR coef_block,
                                      DCTELEM * divisors,
                                      DCTELEM * workspace));

EXTERN(void) jsimd_quantize_neon JPP((JCOEFPTR coef_block,
                                      DCTELEM * divisors,
                                      DCTELEM * workspace));

EXTERN(void) jsimd_quantize_mips_dspr2 JPP((JCOEFPTR coef_block,
                                            DCTELEM * divisors,
                                            DCTELEM * workspace));

EXTERN(void) jsimd_quantize_float_3dnow JPP((JCOEFPTR coef_block,
                                             FAST_FLOAT * divisors,
                                             FAST_FLOAT * workspace));

EXTERN(void) jsimd_quantize_float_sse JPP((JCOEFPTR coef_block,
                                           FAST_FLOAT * divisors,
                                           FAST_FLOAT * workspace));

EXTERN(void) jsimd_quantize_float_sse2 JPP((JCOEFPTR coef_block,
                                            FAST_FLOAT * divisors,
                                            FAST_FLOAT * workspace));

EXTERN(void) jsimd_quantize_float_mips_dspr2 JPP((JCOEFPTR coef_block,
                                                  FAST_FLOAT * divisors,
                                                  FAST_FLOAT * workspace));

/* SIMD Reduced Inverse DCT */
EXTERN(void) jsimd_idct_2x2_mmx JPP((void * dct_table,
                                     JCOEFPTR coef_block,
                                     JSAMPARRAY output_buf,
                                     JDIMENSION output_col));
EXTERN(void) jsimd_idct_4x4_mmx JPP((void * dct_table,
                                     JCOEFPTR coef_block,
                                     JSAMPARRAY output_buf,
                                     JDIMENSION output_col));

extern const int jconst_idct_red_sse2[];
EXTERN(void) jsimd_idct_2x2_sse2 JPP((void * dct_table,
                                      JCOEFPTR coef_block,
                                      JSAMPARRAY output_buf,
                                      JDIMENSION output_col));
EXTERN(void) jsimd_idct_4x4_sse2 JPP((void * dct_table,
                                      JCOEFPTR coef_block,
                                      JSAMPARRAY output_buf,
                                      JDIMENSION output_col));

EXTERN(void) jsimd_idct_2x2_neon JPP((void * dct_table,
                                      JCOEFPTR coef_block,
                                      JSAMPARRAY output_buf,
                                      JDIMENSION output_col));
EXTERN(void) jsimd_idct_4x4_neon JPP((void * dct_table,
                                      JCOEFPTR coef_block,
                                      JSAMPARRAY output_buf,
                                      JDIMENSION output_col));

EXTERN(void) jsimd_idct_2x2_mips_dspr2 JPP((void * dct_table,
                                            JCOEFPTR coef_block,
                                            JSAMPARRAY output_buf,
                                            JDIMENSION output_col));
EXTERN(void) jsimd_idct_4x4_mips_dspr2 JPP((void * dct_table,
                                            JCOEFPTR coef_block,
                                            JSAMPARRAY output_buf,
                                            JDIMENSION output_col,
                                            int * workspace));
EXTERN(void) jsimd_idct_6x6_mips_dspr2 JPP((void * dct_table,
                                            JCOEFPTR coef_block,
                                            JSAMPARRAY output_buf,
                                            JDIMENSION output_col));
EXTERN(void) jsimd_idct_12x12_pass1_mips_dspr2 JPP((JCOEFPTR coef_block,
                                                    void * dct_table,
                                                    int * workspace));
EXTERN(void) jsimd_idct_12x12_pass2_mips_dspr2 JPP((int * workspace,
                                                    int * output));

/* SIMD Inverse DCT */
EXTERN(void) jsimd_idct_islow_mmx JPP((void * dct_table,
                                       JCOEFPTR coef_block,
                                       JSAMPARRAY output_buf,
                                       JDIMENSION output_col));
EXTERN(void) jsimd_idct_ifast_mmx JPP((void * dct_table,
                                       JCOEFPTR coef_block,
                                       JSAMPARRAY output_buf,
                                       JDIMENSION output_col));

extern const int jconst_idct_islow_sse2[];
EXTERN(void) jsimd_idct_islow_sse2 JPP((void * dct_table,
                                        JCOEFPTR coef_block,
                                        JSAMPARRAY output_buf,
                                        JDIMENSION output_col));
extern const int jconst_idct_ifast_sse2[];
EXTERN(void) jsimd_idct_ifast_sse2 JPP((void * dct_table,
                                        JCOEFPTR coef_block,
                                        JSAMPARRAY output_buf,
                                        JDIMENSION output_col));

EXTERN(void) jsimd_idct_islow_neon JPP((void * dct_table,
                                        JCOEFPTR coef_block,
                                        JSAMPARRAY output_buf,
                                        JDIMENSION output_col));
EXTERN(void) jsimd_idct_ifast_neon JPP((void * dct_table,
                                        JCOEFPTR coef_block,
                                        JSAMPARRAY output_buf,
                                        JDIMENSION output_col));

EXTERN(void) jsimd_idct_islow_mips_dspr2 JPP((void * dct_table,
                                              JCOEFPTR coef_block,
                                              int * output_buf,
                                              JSAMPLE * output_col));
EXTERN(void) jsimd_idct_ifast_cols_mips_dspr2 JPP((JCOEF * inptr,
                                                   IFAST_MULT_TYPE * quantptr,
                                                   DCTELEM * wsptr,
                                                   const int * idct_coefs));
EXTERN(void) jsimd_idct_ifast_rows_mips_dspr2 JPP((DCTELEM * wsptr,
                                                   JSAMPARRAY output_buf,
                                                   JDIMENSION output_col,
                                                   const int * idct_coefs));

EXTERN(void) jsimd_idct_float_3dnow JPP((void * dct_table,
                                         JCOEFPTR coef_block,
                                         JSAMPARRAY output_buf,
                                         JDIMENSION output_col));

extern const int jconst_idct_float_sse[];
EXTERN(void) jsimd_idct_float_sse JPP((void * dct_table,
                                       JCOEFPTR coef_block,
                                       JSAMPARRAY output_buf,
                                       JDIMENSION output_col));

extern const int jconst_idct_float_sse2[];
EXTERN(void) jsimd_idct_float_sse2 JPP((void * dct_table,
                                        JCOEFPTR coef_block,
                                        JSAMPARRAY output_buf,
                                        JDIMENSION output_col));


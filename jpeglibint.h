/*
 * jpeglibint.h
 *
 * Copyright (C) 2022, D. R. Commander.
 * For conditions of distribution and use, see the accompanying README.ijg
 * file.
 */

#ifndef JPEGLIBINT_H
#define JPEGLIBINT_H


#if BITS_IN_JSAMPLE == 12

#include "jpeg12lib.h"


/* Rename all external types and functions that are affected by JSAMPLE. */

#define JSAMPLE  J12SAMPLE

#undef MAXJSAMPLE
#define MAXJSAMPLE  MAXJ12SAMPLE
#undef CENTERJSAMPLE
#define CENTERJSAMPLE   CENTERJ12SAMPLE

#define JSAMPROW  J12SAMPROW
#define JSAMPARRAY  J12SAMPARRAY
#define JSAMPIMAGE  J12SAMPIMAGE

#define jpeg_common_struct  jpeg12_common_struct

#define j_common_ptr  j12_common_ptr
#define j_compress_ptr  j12_compress_ptr
#define j_decompress_ptr  j12_decompress_ptr

#define jpeg_compress_struct  jpeg12_compress_struct

#define jpeg_decompress_struct  jpeg12_decompress_struct

#define jpeg_error_mgr  jpeg12_error_mgr

#define jpeg_progress_mgr  jpeg12_progress_mgr

#define jpeg_destination_mgr  jpeg12_destination_mgr

#define jpeg_source_mgr  jpeg12_source_mgr

#define jpeg_memory_mgr  jpeg12_memory_mgr

#define jpeg_marker_parser_method  jpeg12_marker_parser_method

#define jpeg_std_error  jpeg12_std_error

#define jpeg_create_compress  jpeg12_create_compress
#define jpeg_create_decompress  jpeg12_create_decompress
#define jpeg_CreateCompress  jpeg12_CreateCompress
#define jpeg_CreateDecompress  jpeg12_CreateDecompress
#define jpeg_destroy_compress  jpeg12_destroy_compress
#define jpeg_destroy_decompress  jpeg12_destroy_decompress

#define jpeg_stdio_dest  jpeg12_stdio_dest
#define jpeg_stdio_src  jpeg12_stdio_src

#if JPEG_LIB_VERSION >= 80 || defined(MEM_SRCDST_SUPPORTED)
#define jpeg_mem_dest  jpeg12_mem_dest
#define jpeg_mem_src  jpeg12_mem_src
#endif

#define jpeg_set_defaults  jpeg12_set_defaults
#define jpeg_set_colorspace  jpeg12_set_colorspace
#define jpeg_default_colorspace  jpeg12_default_colorspace
#define jpeg_set_quality  jpeg12_set_quality
#define jpeg_set_linear_quality  jpeg12_set_linear_quality
#if JPEG_LIB_VERSION >= 70
#define jpeg_default_qtables  jpeg12_default_qtables
#endif
#define jpeg_add_quant_table  jpeg12_add_quant_table
#define jpeg_quality_scaling  jpeg12_quality_scaling
#define jpeg_simple_progression  jpeg12_simple_progression
#define jpeg_suppress_tables  jpeg12_suppress_tables
#define jpeg_alloc_quant_table  jpeg12_alloc_quant_table
#define jpeg_alloc_huff_table  jpeg12_alloc_huff_table

#define jpeg_start_compress  jpeg12_start_compress
#define jpeg_write_scanlines  jpeg12_write_scanlines
#define jpeg_finish_compress  jpeg12_finish_compress

#if JPEG_LIB_VERSION >= 70
#define jpeg_calc_jpeg_dimensions  jpeg12_calc_jpeg_dimensions
#endif

#define jpeg_write_raw_data  jpeg12_write_raw_data

#define jpeg_write_marker  jpeg12_write_marker
#define jpeg_write_m_header  jpeg12_write_m_header
#define jpeg_write_m_byte  jpeg12_write_m_byte

#define jpeg_write_tables  jpeg12_write_tables

#define jpeg_write_icc_profile  jpeg12_write_icc_profile

#define jpeg_read_header  jpeg12_read_header

#define jpeg_start_decompress  jpeg12_start_decompress
#define jpeg_read_scanlines  jpeg12_read_scanlines
#define jpeg_skip_scanlines  jpeg12_skip_scanlines
#define jpeg_crop_scanline  jpeg12_crop_scanline
#define jpeg_finish_decompress  jpeg12_finish_decompress

#define jpeg_read_raw_data  jpeg12_read_raw_data

#define jpeg_has_multiple_scans  jpeg12_has_multiple_scans
#define jpeg_start_output  jpeg12_start_output
#define jpeg_finish_output  jpeg12_finish_output
#define jpeg_input_complete  jpeg12_input_complete
#define jpeg_new_colormap  jpeg12_new_colormap
#define jpeg_consume_input  jpeg12_consume_input

#if JPEG_LIB_VERSION >= 80
#define jpeg_core_output_dimensions  jpeg12_core_output_dimensions
#endif
#define jpeg_calc_output_dimensions  jpeg12_calc_output_dimensions

#define jpeg_save_markers  jpeg12_save_markers

#define jpeg_set_marker_processor  jpeg12_set_marker_processor

#define jpeg_read_coefficients  jpeg12_read_coefficients
#define jpeg_write_coefficients  jpeg12_write_coefficients
#define jpeg_copy_critical_parameters  jpeg12_copy_critical_parameters

#define jpeg_abort_compress  jpeg12_abort_compress
#define jpeg_abort_decompress  jpeg12_abort_decompress

#define jpeg_abort  jpeg12_abort
#define jpeg_destroy  jpeg12_destroy

#define jpeg_resync_to_restart  jpeg12_resync_to_restart

#define jpeg_read_icc_profile  jpeg12_read_icc_profile


/* Rename all internal types and functions that are affected by JSAMPLE. */

#ifdef JPEG_INTERNALS

#define jpeg_comp_master  jpeg12_comp_master
#define jpeg_c_main_controller  jpeg12_c_main_controller
#define jpeg_c_prep_controller  jpeg12_c_prep_controller
#define jpeg_c_coef_controller  jpeg12_c_coef_controller
#define jpeg_color_converter  jpeg12_color_converter
#define jpeg_downsampler  jpeg12_downsampler
#define jpeg_forward_dct  jpeg12_forward_dct
#define jpeg_entropy_encoder  jpeg12_entropy_encoder
#define jpeg_marker_writer  jpeg12_marker_writer
#define jpeg_decomp_master  jpeg12_decomp_master
#define jpeg_input_controller  jpeg12_input_controller
#define jpeg_d_main_controller  jpeg12_d_main_controller
#define jpeg_d_coef_controller  jpeg12_d_coef_controller
#define jpeg_d_post_controller  jpeg12_d_post_controller
#define jpeg_marker_reader  jpeg12_marker_reader
#define jpeg_entropy_decoder  jpeg12_entropy_decoder
#define inverse_DCT_method_ptr  inverse_DCT_12_method_ptr
#define jpeg_inverse_dct  jpeg12_inverse_dct
#define jpeg_upsampler  jpeg12_upsampler
#define jpeg_color_deconverter  jpeg12_color_deconverter
#define jpeg_color_quantizer  jpeg12_color_quantizer

#define jinit_compress_master  j12init_compress_master
#define jinit_c_master_control  j12init_c_master_control
#define jinit_c_main_controller  j12init_c_main_controller
#define jinit_c_prep_controller  j12init_c_prep_controller
#define jinit_c_coef_controller  j12init_c_coef_controller
#define jinit_color_converter  j12init_color_converter
#define jinit_downsampler  j12init_downsampler
#define jinit_forward_dct  j12init_forward_dct
#define jinit_huff_encoder  j12init_huff_encoder
#define jinit_phuff_encoder  j12init_phuff_encoder
#define jinit_arith_encoder  j12init_arith_encoder
#define jinit_marker_writer  j12init_marker_writer
#define jinit_master_decompress  j12init_master_decompress
#define jinit_d_main_controller  j12init_d_main_controller
#define jinit_d_coef_controller  j12init_d_coef_controller
#define jinit_d_post_controller  j12init_d_post_controller
#define jinit_input_controller  j12init_input_controller
#define jinit_marker_reader  j12init_marker_reader
#define jinit_huff_decoder  j12init_huff_decoder
#define jinit_phuff_decoder  j12init_phuff_decoder
#define jinit_arith_decoder  j12init_arith_decoder
#define jinit_inverse_dct  j12init_inverse_dct
#define jinit_upsampler  j12init_upsampler
#define jinit_color_deconverter  j12init_color_deconverter
#define jinit_1pass_quantizer  j12init_1pass_quantizer
#define jinit_2pass_quantizer  j12init_2pass_quantizer
#define jinit_merged_upsampler  j12init_merged_upsampler
#define jinit_memory_mgr  j12init_memory_mgr
#define jdiv_round_up  j12div_round_up
#define jround_up  j12round_up
#define jcopy_sample_rows j12copy_sample_rows
#define jcopy_block_row  j12copy_block_row
#define jzero_far  j12zero_far

#define jpeg_natural_order  jpeg12_natural_order

#define jpeg_make_c_derived_tbl  jpeg12_make_c_derived_tbl
#define jpeg_gen_optimal_table  jpeg12_gen_optimal_table

#define jpeg_fdct_islow  jpeg12_fdct_islow
#define jpeg_fdct_ifast  jpeg12_fdct_ifast
#define jpeg_fdct_float  jpeg12_fdct_float
#define jpeg_idct_islow  jpeg12_idct_islow
#define jpeg_idct_ifast  jpeg12_idct_ifast
#define jpeg_idct_float  jpeg12_idct_float
#define jpeg_idct_7x7  jpeg12_idct_7x7
#define jpeg_idct_6x6  jpeg12_idct_6x6
#define jpeg_idct_5x5  jpeg12_idct_5x5
#define jpeg_idct_4x4  jpeg12_idct_4x4
#define jpeg_idct_3x3  jpeg12_idct_3x3
#define jpeg_idct_2x2  jpeg12_idct_2x2
#define jpeg_idct_1x1  jpeg12_idct_1x1
#define jpeg_idct_9x9  jpeg12_idct_9x9
#define jpeg_idct_10x10  jpeg12_idct_10x10
#define jpeg_idct_11x11  jpeg12_idct_11x11
#define jpeg_idct_12x12  jpeg12_idct_12x12
#define jpeg_idct_13x13  jpeg12_idct_13x13
#define jpeg_idct_14x14  jpeg12_idct_14x14
#define jpeg_idct_15x15  jpeg12_idct_15x15
#define jpeg_idct_16x16  jpeg12_idct_16x16

#define jpeg_make_d_derived_tbl  jpeg12_make_d_derived_tbl
#define jpeg_fill_bit_buffer  jpeg12_fill_bit_buffer
#define jpeg_huff_decode  jpeg12_huff_decode

#define jpeg_std_message_table  jpeg12_std_message_table

#define jpeg_get_small  jpeg12_get_small
#define jpeg_free_small  jpeg12_free_small
#define jpeg_get_large  jpeg12_get_large
#define jpeg_free_large  jpeg12_free_large
#define jpeg_mem_available  jpeg12_mem_available
#define jpeg_open_backing_store  jpeg12_open_backing_store
#define jpeg_mem_init  jpeg12_mem_init
#define jpeg_mem_term  jpeg12_mem_term

#define jsimd_can_rgb_ycc  j12simd_can_rgb_ycc
#define jsimd_can_rgb_gray  j12simd_can_rgb_gray
#define jsimd_can_ycc_rgb  j12simd_can_ycc_rgb
#define jsimd_can_ycc_rgb565  j12simd_can_ycc_rgb565
#define jsimd_c_can_null_convert  j12simd_c_can_null_convert
#define jsimd_rgb_ycc_convert  j12simd_rgb_ycc_convert
#define jsimd_rgb_gray_convert  j12simd_rgb_gray_convert
#define jsimd_ycc_rgb_convert  j12simd_ycc_rgb_convert
#define jsimd_ycc_rgb565_convert  j12simd_ycc_rgb565_convert
#define jsimd_c_null_convert  j12simd_c_null_convert
#define jsimd_can_h2v2_downsample  j12simd_can_h2v2_downsample
#define jsimd_can_h2v1_downsample  j12simd_can_h2v1_downsample
#define jsimd_h2v2_downsample  j12simd_h2v2_downsample
#define jsimd_can_h2v2_smooth_downsample  j12simd_can_h2v2_smooth_downsample
#define jsimd_h2v2_smooth_downsample  j12simd_h2v2_smooth_downsample
#define jsimd_h2v1_downsample  j12simd_h2v1_downsample
#define jsimd_can_h2v2_upsample  j12simd_can_h2v2_upsample
#define jsimd_can_h2v1_upsample  j12simd_can_h2v1_upsample
#define jsimd_can_int_upsample  j12simd_can_int_upsample
#define jsimd_h2v2_upsample  j12simd_h2v2_upsample
#define jsimd_h2v1_upsample  j12simd_h2v1_upsample
#define jsimd_int_upsample  j12simd_int_upsample
#define jsimd_can_h2v2_fancy_upsample  j12simd_can_h2v2_fancy_upsample
#define jsimd_can_h2v1_fancy_upsample  j12simd_can_h2v1_fancy_upsample
#define jsimd_can_h1v2_fancy_upsample  j12simd_can_h1v2_fancy_upsample
#define jsimd_h2v2_fancy_upsample  j12simd_h2v2_fancy_upsample
#define jsimd_h2v1_fancy_upsample  j12simd_h2v1_fancy_upsample
#define jsimd_h1v2_fancy_upsample  j12simd_h1v2_fancy_upsample
#define jsimd_can_h2v2_merged_upsample  j12simd_can_h2v2_merged_upsample
#define jsimd_can_h2v1_merged_upsample  j12simd_can_h2v1_merged_upsample
#define jsimd_h2v2_merged_upsample  j12simd_h2v2_merged_upsample
#define jsimd_h2v1_merged_upsample  j12simd_h2v1_merged_upsample
#define jsimd_can_huff_encode_one_block  j12simd_can_huff_encode_one_block
#define jsimd_huff_encode_one_block  j12simd_huff_encode_one_block
#define jsimd_can_encode_mcu_AC_first_prepare \
  j12simd_can_encode_mcu_AC_first_prepare
#define jsimd_encode_mcu_AC_first_prepare \
  j12simd_encode_mcu_AC_first_prepare
#define jsimd_can_encode_mcu_AC_refine_prepare \
  j12simd_can_encode_mcu_AC_refine_prepare
#define jsimd_encode_mcu_AC_refine_prepare \
  j12simd_encode_mcu_AC_refine_prepare

#define jsimd_can_convsamp  j12simd_can_convsamp
#define jsimd_can_convsamp_float  j12simd_can_convsamp_float
#define jsimd_convsamp  j12simd_convsamp
#define jsimd_convsamp_float  j12simd_convsamp_float
#define jsimd_can_fdct_islow  j12simd_can_fdct_islow
#define jsimd_can_fdct_ifast  j12simd_can_fdct_ifast
#define jsimd_can_fdct_float  j12simd_can_fdct_float
#define jsimd_fdct_islow  j12simd_fdct_islow
#define jsimd_fdct_ifast  j12simd_fdct_ifast
#define jsimd_fdct_float  j12simd_fdct_float
#define jsimd_can_quantize  j12simd_can_quantize
#define jsimd_can_quantize_float  j12simd_can_quantize_float
#define jsimd_quantize  j12simd_quantize
#define jsimd_quantize_float  j12simd_quantize_float
#define jsimd_can_idct_2x2  j12simd_can_idct_2x2
#define jsimd_can_idct_4x4  j12simd_can_idct_4x4
#define jsimd_can_idct_6x6  j12simd_can_idct_6x6
#define jsimd_can_idct_12x12  j12simd_can_idct_12x12
#define jsimd_idct_2x2  j12simd_idct_2x2
#define jsimd_idct_4x4  j12simd_idct_4x4
#define jsimd_idct_6x6  j12simd_idct_6x6
#define jsimd_idct_12x12  j12simd_idct_12x12
#define jsimd_can_idct_islow  j12simd_can_idct_islow
#define jsimd_can_idct_ifast  j12simd_can_idct_ifast
#define jsimd_can_idct_float  j12simd_can_idct_float
#define jsimd_idct_islow  j12simd_idct_islow
#define jsimd_idct_ifast  j12simd_idct_ifast
#define jsimd_idct_float  j12simd_idct_float

#endif /* JPEG_INTERNALS */


#else /* BITS_IN_JSAMPLE == 12 */

#include "jpeglib.h"

#endif


#endif /* JPEGLIBINT_H */

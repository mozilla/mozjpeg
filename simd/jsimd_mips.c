/*
 * jsimd_mips.c
 *
 * Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright 2009-2011 D. R. Commander
 * Copyright (C) 2013, MIPS Technologies, Inc., California
 * 
 * Based on the x86 SIMD extension for IJG JPEG library,
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 * For conditions of distribution and use, see copyright notice in jsimdext.inc
 *
 * This file contains the interface between the "normal" portions
 * of the library and the SIMD implementations when running on
 * MIPS architecture.
 *
 * Based on the stubs from 'jsimd_none.c'
 */

#define JPEG_INTERNALS
#include "../jinclude.h"
#include "../jpeglib.h"
#include "../jsimd.h"
#include "../jdct.h"
#include "../jsimddct.h"
#include "jsimd.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

static unsigned int simd_support = ~0;

#if defined(__linux__)

LOCAL(int)
parse_proc_cpuinfo(const char* search_string)
{
  const char* file_name = "/proc/cpuinfo";
  char cpuinfo_line[256];
  FILE* f = NULL;
  simd_support = 0;

  if ((f = fopen(file_name, "r")) != NULL) {
    while (fgets(cpuinfo_line, sizeof(cpuinfo_line), f) != NULL) {
      if (strstr(cpuinfo_line, search_string) != NULL) {
        fclose(f);
        simd_support |= JSIMD_MIPS_DSPR2;
        return 1;
      }
    }
    fclose(f);
  }
  /* Did not find string in the proc file, or not Linux ELF. */
  return 0;
}
#endif

/*
 * Check what SIMD accelerations are supported.
 *
 * FIXME: This code is racy under a multi-threaded environment.
 */
LOCAL(void)
init_simd (void)
{
  if (simd_support != ~0U)
    return;

  simd_support = 0;

#if defined(__mips__) && defined(__mips_dsp) && (__mips_dsp_rev >= 2)
  simd_support |= JSIMD_MIPS_DSPR2;
#elif defined(__linux__)
  /* We still have a chance to use MIPS DSPR2 regardless of globally used
   * -mdspr2 options passed to gcc by performing runtime detection via
   * /proc/cpuinfo parsing on linux */
  if (!parse_proc_cpuinfo("MIPS 74K"))
    return;
#endif
}

GLOBAL(int)
jsimd_can_rgb_ycc (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if ((RGB_PIXELSIZE != 3) && (RGB_PIXELSIZE != 4))
    return 0;
  if (simd_support & JSIMD_MIPS_DSPR2)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_rgb_gray (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_ycc_rgb (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if ((RGB_PIXELSIZE != 3) && (RGB_PIXELSIZE != 4))
    return 0;
  if (simd_support & JSIMD_MIPS_DSPR2)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_rgb_ycc_convert (j_compress_ptr cinfo,
                       JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                       JDIMENSION output_row, int num_rows)
{
  void (*mipsdspr2fct)(JDIMENSION, JSAMPARRAY, JSAMPIMAGE, JDIMENSION, int);
  switch(cinfo->in_color_space)
  {
    case JCS_EXT_RGB:
      mipsdspr2fct=jsimd_extrgb_ycc_convert_mips_dspr2;
      break;
    case JCS_EXT_RGBX:
    case JCS_EXT_RGBA:
      mipsdspr2fct=jsimd_extrgbx_ycc_convert_mips_dspr2;
      break;
    case JCS_EXT_BGR:
      mipsdspr2fct=jsimd_extbgr_ycc_convert_mips_dspr2;
      break;
    case JCS_EXT_BGRX:
    case JCS_EXT_BGRA:
      mipsdspr2fct=jsimd_extbgrx_ycc_convert_mips_dspr2;
      break;
    case JCS_EXT_XBGR:
    case JCS_EXT_ABGR:
      mipsdspr2fct=jsimd_extxbgr_ycc_convert_mips_dspr2;

      break;
    case JCS_EXT_XRGB:
    case JCS_EXT_ARGB:
      mipsdspr2fct=jsimd_extxrgb_ycc_convert_mips_dspr2;
      break;
    default:
      mipsdspr2fct=jsimd_extrgb_ycc_convert_mips_dspr2;
      break;
  }

  if (simd_support & JSIMD_MIPS_DSPR2)
    mipsdspr2fct(cinfo->image_width, input_buf,
        output_buf, output_row, num_rows);
}

GLOBAL(void)
jsimd_rgb_gray_convert (j_compress_ptr cinfo,
                        JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
                        JDIMENSION output_row, int num_rows)
{
}

GLOBAL(void)
jsimd_ycc_rgb_convert (j_decompress_ptr cinfo,
                       JSAMPIMAGE input_buf, JDIMENSION input_row,
                       JSAMPARRAY output_buf, int num_rows)
{
  void (*mipsdspr2fct)(JDIMENSION, JSAMPIMAGE, JDIMENSION, JSAMPARRAY, int);

  switch(cinfo->out_color_space)
  {
    case JCS_EXT_RGB:
      mipsdspr2fct=jsimd_ycc_extrgb_convert_mips_dspr2;
      break;
    case JCS_EXT_RGBX:
    case JCS_EXT_RGBA:
      mipsdspr2fct=jsimd_ycc_extrgbx_convert_mips_dspr2;
      break;
    case JCS_EXT_BGR:
      mipsdspr2fct=jsimd_ycc_extbgr_convert_mips_dspr2;
      break;
    case JCS_EXT_BGRX:
    case JCS_EXT_BGRA:
      mipsdspr2fct=jsimd_ycc_extbgrx_convert_mips_dspr2;
      break;
    case JCS_EXT_XBGR:
    case JCS_EXT_ABGR:
      mipsdspr2fct=jsimd_ycc_extxbgr_convert_mips_dspr2;
      break;
    case JCS_EXT_XRGB:
    case JCS_EXT_ARGB:
      mipsdspr2fct=jsimd_ycc_extxrgb_convert_mips_dspr2;
      break;
  default:
      mipsdspr2fct=jsimd_ycc_extrgb_convert_mips_dspr2;
      break;
  }

  if (simd_support & JSIMD_MIPS_DSPR2)
    mipsdspr2fct(cinfo->output_width, input_buf,
        input_row, output_buf, num_rows);
}

GLOBAL(int)
jsimd_can_h2v2_downsample (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_downsample (void)
{
  return 0;
}

GLOBAL(void)
jsimd_h2v2_downsample (j_compress_ptr cinfo, jpeg_component_info * compptr,
                       JSAMPARRAY input_data, JSAMPARRAY output_data)
{
}

GLOBAL(void)
jsimd_h2v1_downsample (j_compress_ptr cinfo, jpeg_component_info * compptr,
                       JSAMPARRAY input_data, JSAMPARRAY output_data)
{
}

GLOBAL(int)
jsimd_can_h2v2_upsample (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_upsample (void)
{
  return 0;
}

GLOBAL(void)
jsimd_h2v2_upsample (j_decompress_ptr cinfo,
                     jpeg_component_info * compptr,
                     JSAMPARRAY input_data,
                     JSAMPARRAY * output_data_ptr)
{
}

GLOBAL(void)
jsimd_h2v1_upsample (j_decompress_ptr cinfo,
                     jpeg_component_info * compptr,
                     JSAMPARRAY input_data,
                     JSAMPARRAY * output_data_ptr)
{
}

GLOBAL(int)
jsimd_can_h2v2_fancy_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (simd_support & JSIMD_MIPS_DSPR2)
    return 1;

  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_fancy_upsample (void)
{
  init_simd();

  /* The code is optimised for these values only */
  if (BITS_IN_JSAMPLE != 8)
    return 0;
  if (sizeof(JDIMENSION) != 4)
    return 0;
  if (simd_support & JSIMD_MIPS_DSPR2)
    return 1;

  return 0;
}

GLOBAL(void)
jsimd_h2v2_fancy_upsample (j_decompress_ptr cinfo,
                           jpeg_component_info * compptr,
                           JSAMPARRAY input_data,
                           JSAMPARRAY * output_data_ptr)
{
  if (simd_support & JSIMD_MIPS_DSPR2)
    jsimd_h2v2_fancy_upsample_mips_dspr2(cinfo->max_v_samp_factor,
        compptr->downsampled_width, input_data, output_data_ptr);
}

GLOBAL(void)
jsimd_h2v1_fancy_upsample (j_decompress_ptr cinfo,
                           jpeg_component_info * compptr,
                           JSAMPARRAY input_data,
                           JSAMPARRAY * output_data_ptr)
{
  if (simd_support & JSIMD_MIPS_DSPR2)
    jsimd_h2v1_fancy_upsample_mips_dspr2(cinfo->max_v_samp_factor,
        compptr->downsampled_width, input_data, output_data_ptr);
}

GLOBAL(int)
jsimd_can_h2v2_merged_upsample (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_h2v1_merged_upsample (void)
{
  return 0;
}

GLOBAL(void)
jsimd_h2v2_merged_upsample (j_decompress_ptr cinfo,
                            JSAMPIMAGE input_buf,
                            JDIMENSION in_row_group_ctr,
                            JSAMPARRAY output_buf)
{
}

GLOBAL(void)
jsimd_h2v1_merged_upsample (j_decompress_ptr cinfo,
                            JSAMPIMAGE input_buf,
                            JDIMENSION in_row_group_ctr,
                            JSAMPARRAY output_buf)
{
}

GLOBAL(int)
jsimd_can_convsamp (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_convsamp_float (void)
{
  return 0;
}

GLOBAL(void)
jsimd_convsamp (JSAMPARRAY sample_data, JDIMENSION start_col,
                DCTELEM * workspace)
{
}

GLOBAL(void)
jsimd_convsamp_float (JSAMPARRAY sample_data, JDIMENSION start_col,
                      FAST_FLOAT * workspace)
{
}

GLOBAL(int)
jsimd_can_fdct_islow (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_fdct_ifast (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_fdct_float (void)
{
  return 0;
}

GLOBAL(void)
jsimd_fdct_islow (DCTELEM * data)
{
}

GLOBAL(void)
jsimd_fdct_ifast (DCTELEM * data)
{
}

GLOBAL(void)
jsimd_fdct_float (FAST_FLOAT * data)
{
}

GLOBAL(int)
jsimd_can_quantize (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_quantize_float (void)
{
  return 0;
}

GLOBAL(void)
jsimd_quantize (JCOEFPTR coef_block, DCTELEM * divisors,
                DCTELEM * workspace)
{
}

GLOBAL(void)
jsimd_quantize_float (JCOEFPTR coef_block, FAST_FLOAT * divisors,
                      FAST_FLOAT * workspace)
{
}

GLOBAL(int)
jsimd_can_idct_2x2 (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_idct_4x4 (void)
{
  return 0;
}

GLOBAL(void)
jsimd_idct_2x2 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
}

GLOBAL(void)
jsimd_idct_4x4 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
}

GLOBAL(int)
jsimd_can_idct_islow (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_idct_ifast (void)
{
  return 0;
}

GLOBAL(int)
jsimd_can_idct_float (void)
{
  return 0;
}

GLOBAL(void)
jsimd_idct_islow (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
}

GLOBAL(void)
jsimd_idct_ifast (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
}

GLOBAL(void)
jsimd_idct_float (j_decompress_ptr cinfo, jpeg_component_info * compptr,
                JCOEFPTR coef_block, JSAMPARRAY output_buf,
                JDIMENSION output_col)
{
}

/*
 * makecfg.c
 *
 * x86 SIMD extension for IJG JPEG library
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 * For conditions of distribution and use, see copyright notice in jsimdext.inc
 * Last Modified : March 23, 2005
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

#ifndef offsetof		/* defined in <stddef.h> */
#define offsetof(type, mem) ((size_t) \
		((char *)&((type *)0)->mem - (char *)(type *)0))
#endif

void
print_structure_offset (void)
{
  printf("\n");
  printf("; ---- macros for structure access -----------------------------------------\n");
  printf("\n");

  printf("; struct jpeg_compress_struct\n\n");
  printf("%%define jcstruct_image_width(b)         ((b) + %3u) ; cinfo->image_width\n",
	(unsigned)offsetof(struct jpeg_compress_struct, image_width));
  printf("%%define jcstruct_max_v_samp_factor(b)   ((b) + %3u) ; cinfo->max_v_samp_factor\n",
	(unsigned)offsetof(struct jpeg_compress_struct, max_v_samp_factor));
  printf("\n");

  printf("; struct jpeg_decompress_struct\n\n");
  printf("%%define jdstruct_output_width(b)        ((b) + %3u) ; cinfo->output_width\n",
	(unsigned)offsetof(struct jpeg_decompress_struct, output_width));
  printf("%%define jdstruct_max_v_samp_factor(b)   ((b) + %3u) ; cinfo->max_v_samp_factor\n",
	(unsigned)offsetof(struct jpeg_decompress_struct, max_v_samp_factor));
  printf("%%define jdstruct_sample_range_limit(b)  ((b) + %3u) ; cinfo->sample_range_limit\n",
	(unsigned)offsetof(struct jpeg_decompress_struct, sample_range_limit));
  printf("\n");

  printf("; jpeg_component_info\n\n");
  printf("%%define jcompinfo_v_samp_factor(b)      ((b) + %2u) ; compptr->v_samp_factor\n",
	(unsigned)offsetof(jpeg_component_info, v_samp_factor));
  printf("%%define jcompinfo_width_in_blocks(b)    ((b) + %2u) ; compptr->width_in_blocks\n",
	(unsigned)offsetof(jpeg_component_info, width_in_blocks));
  printf("%%define jcompinfo_downsampled_width(b)  ((b) + %2u) ; compptr->downsampled_width\n",
	(unsigned)offsetof(jpeg_component_info, downsampled_width));
  printf("%%define jcompinfo_dct_table(b)          ((b) + %2u) ; compptr->dct_table\n",
	(unsigned)offsetof(jpeg_component_info, dct_table));
  printf("\n");
}


void
print_jconfig_h_macro (void)
{
  printf("\n");
  printf("; ---- macros from jconfig.h -----------------------------------------------\n");
  printf("\n");

#ifdef NEED_SHORT_EXTERNAL_NAMES
  printf("%%define NEED_SHORT_EXTERNAL_NAMES\t; Use short forms of external names\n");
#else
  printf("%%undef NEED_SHORT_EXTERNAL_NAMES\t; Use short forms of external names\n");
#endif
  printf("\n");
}


void
print_jmorecfg_h_macro (void)
{
  printf("\n");
  printf("; ---- macros from jmorecfg.h ----------------------------------------------\n");
  printf("\n");

  printf("; Capability options common to encoder and decoder:\n");
  printf("\n");
#ifdef DCT_ISLOW_SUPPORTED
  printf("%%define DCT_ISLOW_SUPPORTED\t; slow but accurate integer algorithm\n");
#else
  printf("%%undef DCT_ISLOW_SUPPORTED\t; slow but accurate integer algorithm\n");
#endif
#ifdef DCT_IFAST_SUPPORTED
  printf("%%define DCT_IFAST_SUPPORTED\t; faster, less accurate integer method\n");
#else
  printf("%%undef DCT_IFAST_SUPPORTED\t; faster, less accurate integer method\n");
#endif
#ifdef DCT_FLOAT_SUPPORTED
  printf("%%define DCT_FLOAT_SUPPORTED\t; floating-point: accurate, fast on fast HW\n");
#else
  printf("%%undef DCT_FLOAT_SUPPORTED\t; floating-point: accurate, fast on fast HW\n");
#endif
  printf("\n");

  printf("; Decoder capability options:\n");
  printf("\n");
#ifdef IDCT_SCALING_SUPPORTED
  printf("%%define IDCT_SCALING_SUPPORTED\t\t; Output rescaling via IDCT?\n");
#else
  printf("%%undef IDCT_SCALING_SUPPORTED\t\t; Output rescaling via IDCT?\n");
#endif
#ifdef UPSAMPLE_MERGING_SUPPORTED
  printf("%%define UPSAMPLE_MERGING_SUPPORTED\t; Fast path for sloppy upsampling?\n");
#else
  printf("%%undef UPSAMPLE_MERGING_SUPPORTED\t; Fast path for sloppy upsampling?\n");
#endif
#ifdef UPSAMPLE_H1V2_SUPPORTED
  printf("%%define UPSAMPLE_H1V2_SUPPORTED\t\t; Fast/fancy processing for 1h2v?\n");
#else
  printf("%%undef UPSAMPLE_H1V2_SUPPORTED\t\t; Fast/fancy processing for 1h2v?\n");
#endif
  printf("\n");

#if (RGB_PIXELSIZE == 3 || RGB_PIXELSIZE == 4) && \
    (RGB_RED < 0 || RGB_RED >= RGB_PIXELSIZE || RGB_GREEN < 0 || \
     RGB_GREEN >= RGB_PIXELSIZE || RGB_BLUE < 0 || RGB_BLUE >= RGB_PIXELSIZE || \
     RGB_RED == RGB_GREEN || RGB_GREEN == RGB_BLUE || RGB_RED == RGB_BLUE)
#error "Incorrect RGB pixel offset."
#endif
  printf("; Ordering of RGB data in scanlines passed to or from the application.\n");
  printf("\n");
  printf("%%define RGB_RED\t\t%u\t; Offset of Red in an RGB scanline element\n", RGB_RED);
  printf("%%define RGB_GREEN\t%u\t; Offset of Green\n", RGB_GREEN);
  printf("%%define RGB_BLUE\t%u\t; Offset of Blue\n", RGB_BLUE);
  printf("%%define RGB_PIXELSIZE\t%u\t; JSAMPLEs per RGB scanline element\n", RGB_PIXELSIZE);
  printf("\n");
#ifdef RGBX_FILLER_0XFF
  printf("%%define RGBX_FILLER_0XFF\t; fill dummy bytes with 0xFF in RGBX format\n");
#else
  printf("%%undef RGBX_FILLER_0XFF\t\t; fill dummy bytes with 0xFF in RGBX format\n");
#endif
  printf("\n");

  printf("; SIMD support options (encoder):\n");
  printf("\n");
#ifdef JCCOLOR_RGBYCC_MMX_SUPPORTED
  printf("%%define JCCOLOR_RGBYCC_MMX_SUPPORTED\t; RGB->YCC conversion with MMX\n");
#else
  printf("%%undef JCCOLOR_RGBYCC_MMX_SUPPORTED\t; RGB->YCC conversion with MMX\n");
#endif
#ifdef JCCOLOR_RGBYCC_SSE2_SUPPORTED
  printf("%%define JCCOLOR_RGBYCC_SSE2_SUPPORTED\t; RGB->YCC conversion with SSE2\n");
#else
  printf("%%undef JCCOLOR_RGBYCC_SSE2_SUPPORTED\t; RGB->YCC conversion with SSE2\n");
#endif
#ifdef JCSAMPLE_MMX_SUPPORTED
  printf("%%define JCSAMPLE_MMX_SUPPORTED\t\t; downsampling with MMX\n");
#else
  printf("%%undef JCSAMPLE_MMX_SUPPORTED\t\t; downsampling with MMX\n");
#endif
#ifdef JCSAMPLE_SSE2_SUPPORTED
  printf("%%define JCSAMPLE_SSE2_SUPPORTED\t\t; downsampling with SSE2\n");
#else
  printf("%%undef JCSAMPLE_SSE2_SUPPORTED\t\t; downsampling with SSE2\n");
#endif
#ifdef JFDCT_INT_MMX_SUPPORTED
  printf("%%define JFDCT_INT_MMX_SUPPORTED\t\t; forward DCT with MMX\n");
#else
  printf("%%undef JFDCT_INT_MMX_SUPPORTED\t\t; forward DCT with MMX\n");
#endif
#ifdef JFDCT_INT_SSE2_SUPPORTED
  printf("%%define JFDCT_INT_SSE2_SUPPORTED\t; forward DCT with SSE2\n");
#else
  printf("%%undef JFDCT_INT_SSE2_SUPPORTED\t\t; forward DCT with SSE2\n");
#endif
#ifdef JFDCT_FLT_3DNOW_MMX_SUPPORTED
  printf("%%define JFDCT_FLT_3DNOW_MMX_SUPPORTED\t; forward DCT with 3DNow!/MMX\n");
#else
  printf("%%undef JFDCT_FLT_3DNOW_MMX_SUPPORTED\t; forward DCT with 3DNow!/MMX\n");
#endif
#ifdef JFDCT_FLT_SSE_MMX_SUPPORTED
  printf("%%define JFDCT_FLT_SSE_MMX_SUPPORTED\t; forward DCT with SSE/MMX\n");
#else
  printf("%%undef JFDCT_FLT_SSE_MMX_SUPPORTED\t; forward DCT with SSE/MMX\n");
#endif
#ifdef JFDCT_FLT_SSE_SSE2_SUPPORTED
  printf("%%define JFDCT_FLT_SSE_SSE2_SUPPORTED\t; forward DCT with SSE/SSE2\n");
#else
  printf("%%undef JFDCT_FLT_SSE_SSE2_SUPPORTED\t; forward DCT with SSE/SSE2\n");
#endif
#ifdef JFDCT_INT_QUANTIZE_WITH_DIVISION
  printf("%%define JFDCT_INT_QUANTIZE_WITH_DIVISION ; Use general quantization method\n");
#else
  printf("%%undef JFDCT_INT_QUANTIZE_WITH_DIVISION ; Use general quantization method\n");
#endif
  printf("\n");

  printf("; SIMD support options (decoder):\n");
  printf("\n");
#ifdef JDCOLOR_YCCRGB_MMX_SUPPORTED
  printf("%%define JDCOLOR_YCCRGB_MMX_SUPPORTED\t; YCC->RGB conversion with MMX\n");
#else
  printf("%%undef JDCOLOR_YCCRGB_MMX_SUPPORTED\t; YCC->RGB conversion with MMX\n");
#endif
#ifdef JDCOLOR_YCCRGB_SSE2_SUPPORTED
  printf("%%define JDCOLOR_YCCRGB_SSE2_SUPPORTED\t; YCC->RGB conversion with SSE2\n");
#else
  printf("%%undef JDCOLOR_YCCRGB_SSE2_SUPPORTED\t; YCC->RGB conversion with SSE2\n");
#endif
#ifdef JDMERGE_MMX_SUPPORTED
  printf("%%define JDMERGE_MMX_SUPPORTED\t\t; merged upsampling with MMX\n");
#else
  printf("%%undef JDMERGE_MMX_SUPPORTED\t\t; merged upsampling with MMX\n");
#endif
#ifdef JDMERGE_SSE2_SUPPORTED
  printf("%%define JDMERGE_SSE2_SUPPORTED\t\t; merged upsampling with SSE2\n");
#else
  printf("%%undef JDMERGE_SSE2_SUPPORTED\t\t; merged upsampling with SSE2\n");
#endif
#ifdef JDSAMPLE_FANCY_MMX_SUPPORTED
  printf("%%define JDSAMPLE_FANCY_MMX_SUPPORTED\t; fancy upsampling with MMX\n");
#else
  printf("%%undef JDSAMPLE_FANCY_MMX_SUPPORTED\t; fancy upsampling with MMX\n");
#endif
#ifdef JDSAMPLE_FANCY_SSE2_SUPPORTED
  printf("%%define JDSAMPLE_FANCY_SSE2_SUPPORTED\t; fancy upsampling with SSE2\n");
#else
  printf("%%undef JDSAMPLE_FANCY_SSE2_SUPPORTED\t; fancy upsampling with SSE2\n");
#endif
#ifdef JDSAMPLE_SIMPLE_MMX_SUPPORTED
  printf("%%define JDSAMPLE_SIMPLE_MMX_SUPPORTED\t; sloppy upsampling with MMX\n");
#else
  printf("%%undef JDSAMPLE_SIMPLE_MMX_SUPPORTED\t; sloppy upsampling with MMX\n");
#endif
#ifdef JDSAMPLE_SIMPLE_SSE2_SUPPORTED
  printf("%%define JDSAMPLE_SIMPLE_SSE2_SUPPORTED\t; sloppy upsampling with SSE2\n");
#else
  printf("%%undef JDSAMPLE_SIMPLE_SSE2_SUPPORTED\t; sloppy upsampling with SSE2\n");
#endif
#ifdef JIDCT_INT_MMX_SUPPORTED
  printf("%%define JIDCT_INT_MMX_SUPPORTED\t\t; inverse DCT with MMX\n");
#else
  printf("%%undef JIDCT_INT_MMX_SUPPORTED\t\t; inverse DCT with MMX\n");
#endif
#ifdef JIDCT_INT_SSE2_SUPPORTED
  printf("%%define JIDCT_INT_SSE2_SUPPORTED\t; inverse DCT with SSE2\n");
#else
  printf("%%undef JIDCT_INT_SSE2_SUPPORTED\t\t; inverse DCT with SSE2\n");
#endif
#ifdef JIDCT_FLT_3DNOW_MMX_SUPPORTED
  printf("%%define JIDCT_FLT_3DNOW_MMX_SUPPORTED\t; inverse DCT with 3DNow!/MMX\n");
#else
  printf("%%undef JIDCT_FLT_3DNOW_MMX_SUPPORTED\t; inverse DCT with 3DNow!/MMX\n");
#endif
#ifdef JIDCT_FLT_SSE_MMX_SUPPORTED
  printf("%%define JIDCT_FLT_SSE_MMX_SUPPORTED\t; inverse DCT with SSE/MMX\n");
#else
  printf("%%undef JIDCT_FLT_SSE_MMX_SUPPORTED\t; inverse DCT with SSE/MMX\n");
#endif
#ifdef JIDCT_FLT_SSE_SSE2_SUPPORTED
  printf("%%define JIDCT_FLT_SSE_SSE2_SUPPORTED\t; inverse DCT with SSE/SSE2\n");
#else
  printf("%%undef JIDCT_FLT_SSE_SSE2_SUPPORTED\t; inverse DCT with SSE/SSE2\n");
#endif
  printf("\n");
}


void
print_jpeglib_h_macro (void)
{
  printf("\n");
  printf("; ---- macros from jpeglib.h ----------------------------------------------\n");
  printf("\n");

  printf("; Version ID for the JPEG library.\n");
  printf("; Might be useful for tests like \"#if JPEG_LIB_VERSION >= 60\".\n");
  printf("\n");
  printf("%%define JPEG_LIB_VERSION  %d\n", JPEG_LIB_VERSION);
  printf("\n");
  printf("; SIMD Ext: Version ID for the SIMD extension.\n");
  printf("\n");
  printf("%%define JPEG_SIMDEXT_VERSION  %d\n", JPEG_SIMDEXT_VERSION);
  printf("%%define JPEG_SIMDEXT_VER_STR  \"%s\"\n", JPEG_SIMDEXT_VER_STR);
  printf("\n");
}


int
main (void)
{
  printf(";\n; jsimdcfg.inc --- generated by makecfg.c");
#ifdef __DATE__
#ifdef __TIME__
  printf(" (%s, %s)", __DATE__, __TIME__);
#endif
#endif
  printf("\n;\n\n");
  printf("%%define JSIMDCFG_INCLUDED\t; so that jsimdcfg.inc doesn't do it again\n\n");

  print_structure_offset();
  print_jconfig_h_macro();
  print_jmorecfg_h_macro();
  print_jpeglib_h_macro();

  exit(0);
  return 0;			/* suppress no-return-value warnings */
}

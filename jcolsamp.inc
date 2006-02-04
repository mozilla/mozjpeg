;
; jcolsamp.inc - private declarations for color conversion & up/downsampling
;
; x86 SIMD extension for IJG JPEG library
; Copyright (C) 1999-2006, MIYASAKA Masaru.
; For conditions of distribution and use, see copyright notice in jsimdext.inc
;
; Last Modified : January 5, 2006
;
; [TAB8]

; --------------------------------------------------------------------------
;
; configuration check: BITS_IN_JSAMPLE==8 (8-bit sample values) is the only
; valid setting on this SIMD extension.
;
%if BITS_IN_JSAMPLE != 8
%error "Sorry, this SIMD code only copes with 8-bit sample values."
%endif

; Short forms of external names for systems with brain-damaged linkers.
;
%ifdef NEED_SHORT_EXTERNAL_NAMES
%define jpeg_rgb_ycc_convert_mmx	jMRgbYccCnv	; jccolmmx.asm
%define jpeg_rgb_ycc_convert_sse2	jSRgbYccCnv	; jccolss2.asm
%define jpeg_h2v1_downsample_mmx	jM21Downsample	; jcsammmx.asm
%define jpeg_h2v2_downsample_mmx	jM22Downsample	; jcsammmx.asm
%define jpeg_h2v1_downsample_sse2	jS21Downsample	; jcsamss2.asm
%define jpeg_h2v2_downsample_sse2	jS22Downsample	; jcsamss2.asm
%define jpeg_ycc_rgb_convert_mmx	jMYccRgbCnv	; jdcolmmx.asm
%define jpeg_ycc_rgb_convert_sse2	jSYccRgbCnv	; jdcolss2.asm
%define jpeg_h2v1_merged_upsample_mmx	jM21MerUpsample	; jdmermmx.asm
%define jpeg_h2v2_merged_upsample_mmx	jM22MerUpsample	; jdmermmx.asm
%define jpeg_h2v1_merged_upsample_sse2	jS21MerUpsample	; jdmerss2.asm
%define jpeg_h2v2_merged_upsample_sse2	jS22MerUpsample	; jdmerss2.asm
%define jpeg_h2v1_fancy_upsample_mmx	jM21FanUpsample	; jdsammmx.asm
%define jpeg_h2v2_fancy_upsample_mmx	jM22FanUpsample	; jdsammmx.asm
%define jpeg_h1v2_fancy_upsample_mmx	jM12FanUpsample	; jdsammmx.asm
%define jpeg_h2v1_upsample_mmx		jM21Upsample	; jdsammmx.asm
%define jpeg_h2v2_upsample_mmx		jM22Upsample	; jdsammmx.asm
%define jpeg_h2v1_fancy_upsample_sse2	jS21FanUpsample	; jdsamss2.asm
%define jpeg_h2v2_fancy_upsample_sse2	jS22FanUpsample	; jdsamss2.asm
%define jpeg_h1v2_fancy_upsample_sse2	jS12FanUpsample	; jdsamss2.asm
%define jpeg_h2v1_upsample_sse2		jS21Upsample	; jdsamss2.asm
%define jpeg_h2v2_upsample_sse2		jS22Upsample	; jdsamss2.asm
%define jconst_rgb_ycc_convert_mmx	jMCRgbYccCnv	; jccolmmx.asm
%define jconst_rgb_ycc_convert_sse2	jSCRgbYccCnv	; jccolss2.asm
%define jconst_ycc_rgb_convert_mmx	jMCYccRgbCnv	; jdcolmmx.asm
%define jconst_ycc_rgb_convert_sse2	jSCYccRgbCnv	; jdcolss2.asm
%define jconst_merged_upsample_mmx	jMCMerUpsample	; jdmermmx.asm
%define jconst_merged_upsample_sse2	jSCMerUpsample	; jdmerss2.asm
%define jconst_fancy_upsample_mmx	jMCFanUpsample	; jdsammmx.asm
%define jconst_fancy_upsample_sse2	jSCFanUpsample	; jdsamss2.asm
%endif ; NEED_SHORT_EXTERNAL_NAMES

; --------------------------------------------------------------------------

; pseudo-resisters to make ordering of RGB configurable
;
%if RGB_PIXELSIZE == 3 || RGB_PIXELSIZE == 4
%if RGB_RED < 0 || RGB_RED >= RGB_PIXELSIZE || RGB_GREEN < 0 || \
   RGB_GREEN >= RGB_PIXELSIZE || RGB_BLUE < 0 || RGB_BLUE >= RGB_PIXELSIZE || \
   RGB_RED == RGB_GREEN || RGB_GREEN == RGB_BLUE || RGB_RED == RGB_BLUE
%error "Incorrect RGB pixel offset."
%endif

%if RGB_RED == 0
%define  mmA  mm0
%define  mmB  mm1
%define xmmA xmm0
%define xmmB xmm1
%elif RGB_GREEN == 0
%define  mmA  mm2
%define  mmB  mm3
%define xmmA xmm2
%define xmmB xmm3
%elif RGB_BLUE == 0
%define  mmA  mm4
%define  mmB  mm5
%define xmmA xmm4
%define xmmB xmm5
%else
%define  mmA  mm6
%define  mmB  mm7
%define xmmA xmm6
%define xmmB xmm7
%endif

%if RGB_RED == 1
%define  mmC  mm0
%define  mmD  mm1
%define xmmC xmm0
%define xmmD xmm1
%elif RGB_GREEN == 1
%define  mmC  mm2
%define  mmD  mm3
%define xmmC xmm2
%define xmmD xmm3
%elif RGB_BLUE == 1
%define  mmC  mm4
%define  mmD  mm5
%define xmmC xmm4
%define xmmD xmm5
%else
%define  mmC  mm6
%define  mmD  mm7
%define xmmC xmm6
%define xmmD xmm7
%endif

%if RGB_RED == 2
%define  mmE  mm0
%define  mmF  mm1
%define xmmE xmm0
%define xmmF xmm1
%elif RGB_GREEN == 2
%define  mmE  mm2
%define  mmF  mm3
%define xmmE xmm2
%define xmmF xmm3
%elif RGB_BLUE == 2
%define  mmE  mm4
%define  mmF  mm5
%define xmmE xmm4
%define xmmF xmm5
%else
%define  mmE  mm6
%define  mmF  mm7
%define xmmE xmm6
%define xmmF xmm7
%endif

%if RGB_RED == 3
%define  mmG  mm0
%define  mmH  mm1
%define xmmG xmm0
%define xmmH xmm1
%elif RGB_GREEN == 3
%define  mmG  mm2
%define  mmH  mm3
%define xmmG xmm2
%define xmmH xmm3
%elif RGB_BLUE == 3
%define  mmG  mm4
%define  mmH  mm5
%define xmmG xmm4
%define xmmH xmm5
%else
%define  mmG  mm6
%define  mmH  mm7
%define xmmG xmm6
%define xmmH xmm7
%endif
%endif ; RGB_PIXELSIZE == 3 || RGB_PIXELSIZE == 4

; --------------------------------------------------------------------------

;
; jdct.inc - private declarations for forward & reverse DCT subsystems
;
; x86 SIMD extension for IJG JPEG library
; Copyright (C) 1999-2006, MIYASAKA Masaru.
; For conditions of distribution and use, see copyright notice in jsimdext.inc
;
; Last Modified : January 5, 2006
;
; [TAB8]

; ---- jdct.h --------------------------------------------------------------
;
; configuration check: BITS_IN_JSAMPLE==8 (8-bit sample values) is the only
; valid setting on this SIMD extension.
;
%if BITS_IN_JSAMPLE != 8
%error "Sorry, this SIMD code only copes with 8-bit sample values."
%endif

; A forward DCT routine is given a pointer to a work area of type DCTELEM[];
; the DCT is to be performed in-place in that buffer.
; To maximize parallelism, Type DCTELEM is changed to short (originally, int).
;
%define DCTELEM			word		; short
%define SIZEOF_DCTELEM		SIZEOF_WORD	; sizeof(DCTELEM)

; To maximize parallelism, Type MULTIPLIER is changed to short.
;
%define MULTIPLIER		word		; short
%define SIZEOF_MULTIPLIER	SIZEOF_WORD	; sizeof(MULTIPLIER)
%define FAST_FLOAT		FP32		; float
%define SIZEOF_FAST_FLOAT	SIZEOF_FP32	; sizeof(FAST_FLOAT)

; Each IDCT routine has its own ideas about the best dct_table element type.
;
%define ISLOW_MULT_TYPE 	MULTIPLIER          ; must be short
%define SIZEOF_ISLOW_MULT_TYPE	SIZEOF_MULTIPLIER   ; sizeof(ISLOW_MULT_TYPE)
%define IFAST_MULT_TYPE 	MULTIPLIER          ; must be short
%define SIZEOF_IFAST_MULT_TYPE	SIZEOF_MULTIPLIER   ; sizeof(IFAST_MULT_TYPE)
%define IFAST_SCALE_BITS	2	; fractional bits in scale factors
%define FLOAT_MULT_TYPE 	FAST_FLOAT          ; must be float
%define SIZEOF_FLOAT_MULT_TYPE	SIZEOF_FAST_FLOAT   ; sizeof(FLOAT_MULT_TYPE)

; Each IDCT routine is responsible for range-limiting its results and
; converting them to unsigned form (0..MAXJSAMPLE).  The raw outputs could
; be quite far out of range if the input data is corrupt, so a bulletproof
; range-limiting step is required.  We use a mask-and-table-lookup method
; to do the combined operations quickly.
;
%define RANGE_MASK  (MAXJSAMPLE * 4 + 3)  ; 2 bits wider than legal samples

; Short forms of external names for systems with brain-damaged linkers.
;
%ifdef NEED_SHORT_EXTERNAL_NAMES
%define jpeg_fdct_islow		jFDislow	; jfdctint.asm
%define jpeg_fdct_ifast		jFDifast	; jfdctfst.asm
%define jpeg_fdct_float		jFDfloat	; jfdctflt.asm
%define jpeg_fdct_islow_mmx	jFDMislow	; jfmmxint.asm
%define jpeg_fdct_ifast_mmx	jFDMifast	; jfmmxfst.asm
%define jpeg_fdct_float_3dnow	jFD3float	; jf3dnflt.asm
%define jpeg_fdct_islow_sse2	jFDSislow	; jfss2int.asm
%define jpeg_fdct_ifast_sse2	jFDSifast	; jfss2fst.asm
%define jpeg_fdct_float_sse	jFDSfloat	; jfsseflt.asm
%define jpeg_convsamp_int	jCnvInt		; jcqntint.asm
%define jpeg_quantize_int	jQntInt		; jcqntint.asm
%define jpeg_quantize_idiv	jQntIDiv	; jcqntint.asm
%define jpeg_convsamp_float	jCnvFloat	; jcqntflt.asm
%define jpeg_quantize_float	jQntFloat	; jcqntflt.asm
%define jpeg_convsamp_int_mmx	jCnvMmx		; jcqntmmx.asm
%define jpeg_quantize_int_mmx	jQntMmx		; jcqntmmx.asm
%define jpeg_convsamp_flt_3dnow	jCnv3dnow	; jcqnt3dn.asm
%define jpeg_quantize_flt_3dnow	jQnt3dnow	; jcqnt3dn.asm
%define jpeg_convsamp_int_sse2	jCnvISse2	; jcqnts2i.asm
%define jpeg_quantize_int_sse2	jQntISse2	; jcqnts2i.asm
%define jpeg_convsamp_flt_sse	jCnvSse		; jcqntsse.asm
%define jpeg_quantize_flt_sse	jQntSse		; jcqntsse.asm
%define jpeg_convsamp_flt_sse2	jCnvFSse2	; jcqnts2f.asm
%define jpeg_quantize_flt_sse2	jQntFSse2	; jcqnts2f.asm
%define jpeg_idct_islow		jRDislow	; jidctint.asm
%define jpeg_idct_ifast		jRDifast	; jidctfst.asm
%define jpeg_idct_float		jRDfloat	; jidctflt.asm
%define jpeg_idct_4x4		jRD4x4		; jidctred.asm
%define jpeg_idct_2x2		jRD2x2		; jidctred.asm
%define jpeg_idct_1x1		jRD1x1		; jidctred.asm
%define jpeg_idct_islow_mmx	jRDMislow	; jimmxint.asm
%define jpeg_idct_ifast_mmx	jRDMifast	; jimmxfst.asm
%define jpeg_idct_float_3dnow	jRD3float	; ji3dnflt.asm
%define jpeg_idct_4x4_mmx	jRDM4x4		; jimmxred.asm
%define jpeg_idct_2x2_mmx	jRDM2x2		; jimmxred.asm
%define jpeg_idct_islow_sse2	jRDSislow	; jiss2int.asm
%define jpeg_idct_ifast_sse2	jRDSifast	; jiss2fst.asm
%define jpeg_idct_float_sse	jRDSfloat	; jisseflt.asm
%define jpeg_idct_float_sse2	jRD2float	; jiss2flt.asm
%define jpeg_idct_4x4_sse2	jRDS4x4		; jiss2red.asm
%define jpeg_idct_2x2_sse2	jRDS2x2		; jiss2red.asm
%define jconst_fdct_float	jFCfloat	; jfdctflt.asm
%define jconst_fdct_islow_mmx	jFCMislow	; jfmmxint.asm
%define jconst_fdct_ifast_mmx	jFCMifast	; jfmmxfst.asm
%define jconst_fdct_float_3dnow	jFC3float	; jf3dnflt.asm
%define jconst_fdct_islow_sse2	jFCSislow	; jfss2int.asm
%define jconst_fdct_ifast_sse2	jFCSifast	; jfss2fst.asm
%define jconst_fdct_float_sse	jFCSfloat	; jfsseflt.asm
%define jconst_idct_float	jRCfloat	; jidctflt.asm
%define jconst_idct_islow_mmx	jRCMislow	; jimmxint.asm
%define jconst_idct_ifast_mmx	jRCMifast	; jimmxfst.asm
%define jconst_idct_float_3dnow	jRC3float	; ji3dnflt.asm
%define jconst_idct_red_mmx	jRCMred		; jimmxred.asm
%define jconst_idct_islow_sse2	jRCSislow	; jiss2int.asm
%define jconst_idct_ifast_sse2	jRCSifast	; jiss2fst.asm
%define jconst_idct_float_sse	jRCSfloat	; jisseflt.asm
%define jconst_idct_float_sse2	jRC2float	; jiss2flt.asm
%define jconst_idct_red_sse2	jRCSred		; jiss2red.asm
%endif ; NEED_SHORT_EXTERNAL_NAMES

; --------------------------------------------------------------------------

%define ROW(n,b,s)		((b)+(n)*(s))
%define COL(n,b,s)		((b)+(n)*(s)*DCTSIZE)

%define DWBLOCK(m,n,b,s)	((b)+(m)*DCTSIZE*(s)+(n)*SIZEOF_DWORD)
%define MMBLOCK(m,n,b,s)	((b)+(m)*DCTSIZE*(s)+(n)*SIZEOF_MMWORD)
%define XMMBLOCK(m,n,b,s)	((b)+(m)*DCTSIZE*(s)+(n)*SIZEOF_XMMWORD)

; --------------------------------------------------------------------------

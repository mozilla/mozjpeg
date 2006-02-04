;
; jcqntflt.asm - sample data conversion and quantization (non-SIMD, FP)
;
; x86 SIMD extension for IJG JPEG library
; Copyright (C) 1999-2006, MIYASAKA Masaru.
; For conditions of distribution and use, see copyright notice in jsimdext.inc
;
; This file should be assembled with NASM (Netwide Assembler),
; can *not* be assembled with Microsoft's MASM or any compatible
; assembler (including Borland's Turbo Assembler).
; NASM is available from http://nasm.sourceforge.net/ or
; http://sourceforge.net/project/showfiles.php?group_id=6208
;
; Last Modified : March 21, 2004
;
; [TAB8]

%include "jsimdext.inc"
%include "jdct.inc"

%ifdef DCT_FLOAT_SUPPORTED

; This module is specialized to the case DCTSIZE = 8.
;
%if DCTSIZE != 8
%error "Sorry, this code only copes with 8x8 DCTs."
%endif

; --------------------------------------------------------------------------
	SECTION	SEG_TEXT
	BITS	32
;
; Load data into workspace, applying unsigned->signed conversion
;
; GLOBAL(void)
; jpeg_convsamp_float (JSAMPARRAY sample_data, JDIMENSION start_col,
;                      FAST_FLOAT * workspace);
;

%define sample_data	ebp+8		; JSAMPARRAY sample_data
%define start_col	ebp+12		; JDIMENSION start_col
%define workspace	ebp+16		; FAST_FLOAT * workspace

	align	16
	global	EXTN(jpeg_convsamp_float)

EXTN(jpeg_convsamp_float):
	push	ebp
	mov	ebp,esp
	push	ebx
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
	push	esi
	push	edi

	mov	esi, JSAMPARRAY [sample_data]	; (JSAMPROW *)
	mov	edi, POINTER [workspace]	; (DCTELEM *)
	mov	ecx, DCTSIZE
	alignx	16,7
.convloop:
	mov	ebx, JSAMPROW [esi]		; (JSAMPLE *)
	add	ebx, JDIMENSION [start_col]

%assign i 0	; i=0
%rep 4	; -- repeat 4 times ---
	xor	eax,eax
	xor	edx,edx
	mov	al, JSAMPLE [ebx+(i+0)*SIZEOF_JSAMPLE]
	mov	dl, JSAMPLE [ebx+(i+1)*SIZEOF_JSAMPLE]
	add	eax, byte -CENTERJSAMPLE
	add	edx, byte -CENTERJSAMPLE
	push	eax
	push	edx
%assign i i+2	; i+=2
%endrep	; -- repeat end ---

	fild	INT32 [esp+0*SIZEOF_INT32]
	fild	INT32 [esp+1*SIZEOF_INT32]
	fild	INT32 [esp+2*SIZEOF_INT32]
	fild	INT32 [esp+3*SIZEOF_INT32]
	fild	INT32 [esp+4*SIZEOF_INT32]
	fild	INT32 [esp+5*SIZEOF_INT32]
	fild	INT32 [esp+6*SIZEOF_INT32]
	fild	INT32 [esp+7*SIZEOF_INT32]

	add	esp, byte DCTSIZE*SIZEOF_INT32

	fstp	FAST_FLOAT [edi+0*SIZEOF_FAST_FLOAT]
	fstp	FAST_FLOAT [edi+1*SIZEOF_FAST_FLOAT]
	fstp	FAST_FLOAT [edi+2*SIZEOF_FAST_FLOAT]
	fstp	FAST_FLOAT [edi+3*SIZEOF_FAST_FLOAT]
	fstp	FAST_FLOAT [edi+4*SIZEOF_FAST_FLOAT]
	fstp	FAST_FLOAT [edi+5*SIZEOF_FAST_FLOAT]
	fstp	FAST_FLOAT [edi+6*SIZEOF_FAST_FLOAT]
	fstp	FAST_FLOAT [edi+7*SIZEOF_FAST_FLOAT]

	add	esi, byte SIZEOF_JSAMPROW
	add	edi, byte DCTSIZE*SIZEOF_FAST_FLOAT
	dec	ecx
	jnz	near .convloop

	pop	edi
	pop	esi
;	pop	edx		; need not be preserved
;	pop	ecx		; need not be preserved
	pop	ebx
	pop	ebp
	ret


; --------------------------------------------------------------------------
;
; Quantize/descale the coefficients, and store into coef_block
;
; GLOBAL(void)
; jpeg_quantize_float (JCOEFPTR coef_block, FAST_FLOAT * divisors,
;                      FAST_FLOAT * workspace);
;

%define coef_block	ebp+8		; JCOEFPTR coef_block
%define divisors	ebp+12		; FAST_FLOAT * divisors
%define workspace	ebp+16		; FAST_FLOAT * workspace

%define FLT_ROUNDS	1		; from <float.h>

	align	16
	global	EXTN(jpeg_quantize_float)

EXTN(jpeg_quantize_float):
	push	ebp
	mov	ebp,esp
	push	ebx
;	push	ecx		; unused
;	push	edx		; unused
	push	esi
	push	edi

%if (FLT_ROUNDS != 1)
	push	eax
	fnstcw	word [esp]
	mov	eax, [esp]
	and	eax, (~0x0C00)		; round to nearest integer
	push	eax
	fldcw	word [esp]
	pop	eax
%endif
	mov	esi, POINTER [workspace]
	mov	ebx, POINTER [divisors]
	mov	edi, JCOEFPTR [coef_block]
	mov	eax, DCTSIZE2/8
	alignx	16,7
.quantloop:
	fld	FAST_FLOAT [esi+0*SIZEOF_FAST_FLOAT]
	fmul	FAST_FLOAT [ebx+0*SIZEOF_FAST_FLOAT]
	fld	FAST_FLOAT [esi+1*SIZEOF_FAST_FLOAT]
	fmul	FAST_FLOAT [ebx+1*SIZEOF_FAST_FLOAT]
	fld	FAST_FLOAT [esi+2*SIZEOF_FAST_FLOAT]
	fmul	FAST_FLOAT [ebx+2*SIZEOF_FAST_FLOAT]
	fld	FAST_FLOAT [esi+3*SIZEOF_FAST_FLOAT]
	fmul	FAST_FLOAT [ebx+3*SIZEOF_FAST_FLOAT]

	fld	FAST_FLOAT [esi+4*SIZEOF_FAST_FLOAT]
	fmul	FAST_FLOAT [ebx+4*SIZEOF_FAST_FLOAT]
	fxch	st0,st1
	fld	FAST_FLOAT [esi+5*SIZEOF_FAST_FLOAT]
	fmul	FAST_FLOAT [ebx+5*SIZEOF_FAST_FLOAT]
	fxch	st0,st3
	fld	FAST_FLOAT [esi+6*SIZEOF_FAST_FLOAT]
	fmul	FAST_FLOAT [ebx+6*SIZEOF_FAST_FLOAT]
	fxch	st0,st5
	fld	FAST_FLOAT [esi+7*SIZEOF_FAST_FLOAT]
	fmul	FAST_FLOAT [ebx+7*SIZEOF_FAST_FLOAT]
	fxch	st0,st7

	fistp	JCOEF [edi+0*SIZEOF_JCOEF]
	fistp	JCOEF [edi+1*SIZEOF_JCOEF]
	fistp	JCOEF [edi+2*SIZEOF_JCOEF]
	fistp	JCOEF [edi+3*SIZEOF_JCOEF]
	fistp	JCOEF [edi+4*SIZEOF_JCOEF]
	fistp	JCOEF [edi+5*SIZEOF_JCOEF]
	fistp	JCOEF [edi+6*SIZEOF_JCOEF]
	fistp	JCOEF [edi+7*SIZEOF_JCOEF]

	add	esi, byte 8*SIZEOF_FAST_FLOAT
	add	ebx, byte 8*SIZEOF_FAST_FLOAT
	add	edi, byte 8*SIZEOF_JCOEF
	dec	eax
	jnz	short .quantloop

%if (FLT_ROUNDS != 1)
	fldcw	word [esp]
	pop	eax		; pop old control word
%endif
	pop	edi
	pop	esi
;	pop	edx		; unused
;	pop	ecx		; unused
	pop	ebx
	pop	ebp
	ret

%endif ; DCT_FLOAT_SUPPORTED

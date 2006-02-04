;
; jcqntint.asm - sample data conversion and quantization (non-SIMD, integer)
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
; Last Modified : January 27, 2005
;
; [TAB8]

%include "jsimdext.inc"
%include "jdct.inc"

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
; jpeg_convsamp_int (JSAMPARRAY sample_data, JDIMENSION start_col,
;                    DCTELEM * workspace);
;

%define sample_data	ebp+8		; JSAMPARRAY sample_data
%define start_col	ebp+12		; JDIMENSION start_col
%define workspace	ebp+16		; DCTELEM * workspace

	align	16
	global	EXTN(jpeg_convsamp_int)

EXTN(jpeg_convsamp_int):
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
	mov	DCTELEM [edi+(i+0)*SIZEOF_DCTELEM], ax
	mov	DCTELEM [edi+(i+1)*SIZEOF_DCTELEM], dx
%assign i i+2	; i+=2
%endrep	; -- repeat end ---

	add	esi, byte SIZEOF_JSAMPROW
	add	edi, byte DCTSIZE*SIZEOF_DCTELEM
	dec	ecx
	jnz	short .convloop

	pop	edi
	pop	esi
;	pop	edx		; need not be preserved
;	pop	ecx		; need not be preserved
	pop	ebx
	pop	ebp
	ret

%ifndef JFDCT_INT_QUANTIZE_WITH_DIVISION

; --------------------------------------------------------------------------
;
; Quantize/descale the coefficients, and store into coef_block
;
; This implementation is based on an algorithm described in
;   "How to optimize for the Pentium family of microprocessors"
;   (http://www.agner.org/assem/).
;
; GLOBAL(void)
; jpeg_quantize_int (JCOEFPTR coef_block, DCTELEM * divisors,
;                    DCTELEM * workspace);
;

%define RECIPROCAL(i,b)	((b)+((i)+DCTSIZE2*0)*SIZEOF_DCTELEM)
%define CORRECTION(i,b)	((b)+((i)+DCTSIZE2*1)*SIZEOF_DCTELEM)
%define SHIFT(i,b)	((b)+((i)+DCTSIZE2*3)*SIZEOF_DCTELEM)

%define coef_block	ebp+8		; JCOEFPTR coef_block
%define divisors	ebp+12		; DCTELEM * divisors
%define workspace	ebp+16		; DCTELEM * workspace

%define UNROLL	2

	align	16
	global	EXTN(jpeg_quantize_int)

EXTN(jpeg_quantize_int):
	push	ebp
	mov	ebp,esp
	push	ebx
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
	push	esi
	push	edi

	mov	esi, POINTER [workspace]
	mov	ebx, POINTER [divisors]
	mov	edi, JCOEFPTR [coef_block]
	mov	ecx, DCTSIZE2/UNROLL
	alignx	16,7
.quantloop:
	push	ecx

%assign i 0	; i=0;
%rep UNROLL	; ---- repeat (UNROLL) times ----
	mov	cx, DCTELEM [esi+(i)*SIZEOF_DCTELEM]
	mov	ax,cx
	sar	cx,(WORD_BIT-1)
	xor	ax,cx		; if (ax < 0) ax = -ax;
	sub	ax,cx
	add	ax, DCTELEM [CORRECTION(i,ebx)]	; correction + roundfactor
	shl	ax,1
	mul	DCTELEM [RECIPROCAL(i,ebx)]	; reciprocal
	mov	ax,cx
	mov	cx, DCTELEM [SHIFT(i,ebx)]	; shift
	shr	dx,cl
	xor	dx,ax
	sub	dx,ax
	mov	JCOEF [edi+(i)*SIZEOF_JCOEF], dx
%assign i i+1	; i++;
%endrep		; ---- repeat end ----

	pop	ecx

	add	esi, byte UNROLL*SIZEOF_DCTELEM
	add	ebx, byte UNROLL*SIZEOF_DCTELEM
	add	edi, byte UNROLL*SIZEOF_JCOEF
	dec	ecx
	jnz	.quantloop

	pop	edi
	pop	esi
;	pop	edx		; need not be preserved
;	pop	ecx		; need not be preserved
	pop	ebx
	pop	ebp
	ret

%else ; JFDCT_INT_QUANTIZE_WITH_DIVISION

; --------------------------------------------------------------------------
;
; Quantize/descale the coefficients, and store into coef_block
;
; GLOBAL(void)
; jpeg_quantize_idiv (JCOEFPTR coef_block, DCTELEM * divisors,
;                     DCTELEM * workspace);
;

%define coef_block	ebp+8		; JCOEFPTR coef_block
%define divisors	ebp+12		; DCTELEM * divisors
%define workspace	ebp+16		; DCTELEM * workspace

	align	16
	global	EXTN(jpeg_quantize_idiv)

EXTN(jpeg_quantize_idiv):
	push	ebp
	mov	ebp,esp
	push	ebx
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
	push	esi
	push	edi

	mov	esi, POINTER [workspace]
	mov	ebx, POINTER [divisors]
	mov	edi, JCOEFPTR [coef_block]
	mov	ecx, DCTSIZE2
	alignx	16,7
.quantloop:
	push	ecx

	movsx	ecx, DCTELEM [esi]	; temp
	mov	eax,ecx
	sar	ecx,(DWORD_BIT-1)
	xor	edx,edx
	mov	dx, DCTELEM [ebx]	; qval
	xor	eax,ecx			; if (eax < 0) eax = -eax;
	shr	edx,1
	sub	eax,ecx
	cmp	eax,edx			; if (temp + qval/2 >= qval)
	jge	short .quant
	; ---- if the quantized coefficient is zero
	xor	eax,eax
	jmp	short .output
	alignx	16,7
.quant:	; ---- do quantization
	add	eax,edx
	xor	edx,edx
	div	DCTELEM [ebx]		; Q:ax,R:dx
	xor	ax,cx
	sub	ax,cx
	alignx	16,7
.output:
	mov	JCOEF [edi], ax

	pop	ecx

	add	esi, byte SIZEOF_DCTELEM
	add	ebx, byte SIZEOF_DCTELEM
	add	edi, byte SIZEOF_JCOEF
	dec	ecx
	jnz	short .quantloop

	pop	edi
	pop	esi
;	pop	edx		; need not be preserved
;	pop	ecx		; need not be preserved
	pop	ebx
	pop	ebp
	ret

%endif ; !JFDCT_INT_QUANTIZE_WITH_DIVISION

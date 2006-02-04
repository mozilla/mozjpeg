;
; jcqntmmx.asm - sample data conversion and quantization (MMX)
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

%ifdef JFDCT_INT_MMX_SUPPORTED

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
; jpeg_convsamp_int_mmx (JSAMPARRAY sample_data, JDIMENSION start_col,
;                        DCTELEM * workspace);
;

%define sample_data	ebp+8		; JSAMPARRAY sample_data
%define start_col	ebp+12		; JDIMENSION start_col
%define workspace	ebp+16		; DCTELEM * workspace

	align	16
	global	EXTN(jpeg_convsamp_int_mmx)

EXTN(jpeg_convsamp_int_mmx):
	push	ebp
	mov	ebp,esp
	push	ebx
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
	push	esi
	push	edi

	pxor	mm6,mm6			; mm6=(all 0's)
	pcmpeqw	mm7,mm7
	psllw	mm7,7			; mm7={0xFF80 0xFF80 0xFF80 0xFF80}

	mov	esi, JSAMPARRAY [sample_data]	; (JSAMPROW *)
	mov	eax, JDIMENSION [start_col]
	mov	edi, POINTER [workspace]	; (DCTELEM *)
	mov	ecx, DCTSIZE/4
	alignx	16,7
.convloop:
	mov	ebx, JSAMPROW [esi+0*SIZEOF_JSAMPROW]	; (JSAMPLE *)
	mov	edx, JSAMPROW [esi+1*SIZEOF_JSAMPROW]	; (JSAMPLE *)

	movq	mm0, MMWORD [ebx+eax*SIZEOF_JSAMPLE]	; mm0=(01234567)
	movq	mm1, MMWORD [edx+eax*SIZEOF_JSAMPLE]	; mm1=(89ABCDEF)

	mov	ebx, JSAMPROW [esi+2*SIZEOF_JSAMPROW]	; (JSAMPLE *)
	mov	edx, JSAMPROW [esi+3*SIZEOF_JSAMPROW]	; (JSAMPLE *)

	movq	mm2, MMWORD [ebx+eax*SIZEOF_JSAMPLE]	; mm2=(GHIJKLMN)
	movq	mm3, MMWORD [edx+eax*SIZEOF_JSAMPLE]	; mm3=(OPQRSTUV)

	movq      mm4,mm0
	punpcklbw mm0,mm6		; mm0=(0123)
	punpckhbw mm4,mm6		; mm4=(4567)
	movq      mm5,mm1
	punpcklbw mm1,mm6		; mm1=(89AB)
	punpckhbw mm5,mm6		; mm5=(CDEF)

	paddw	mm0,mm7
	paddw	mm4,mm7
	paddw	mm1,mm7
	paddw	mm5,mm7

	movq	MMWORD [MMBLOCK(0,0,edi,SIZEOF_DCTELEM)], mm0
	movq	MMWORD [MMBLOCK(0,1,edi,SIZEOF_DCTELEM)], mm4
	movq	MMWORD [MMBLOCK(1,0,edi,SIZEOF_DCTELEM)], mm1
	movq	MMWORD [MMBLOCK(1,1,edi,SIZEOF_DCTELEM)], mm5

	movq      mm0,mm2
	punpcklbw mm2,mm6		; mm2=(GHIJ)
	punpckhbw mm0,mm6		; mm0=(KLMN)
	movq      mm4,mm3
	punpcklbw mm3,mm6		; mm3=(OPQR)
	punpckhbw mm4,mm6		; mm4=(STUV)

	paddw	mm2,mm7
	paddw	mm0,mm7
	paddw	mm3,mm7
	paddw	mm4,mm7

	movq	MMWORD [MMBLOCK(2,0,edi,SIZEOF_DCTELEM)], mm2
	movq	MMWORD [MMBLOCK(2,1,edi,SIZEOF_DCTELEM)], mm0
	movq	MMWORD [MMBLOCK(3,0,edi,SIZEOF_DCTELEM)], mm3
	movq	MMWORD [MMBLOCK(3,1,edi,SIZEOF_DCTELEM)], mm4

	add	esi, byte 4*SIZEOF_JSAMPROW
	add	edi, byte 4*DCTSIZE*SIZEOF_DCTELEM
	dec	ecx
	jnz	short .convloop

	emms		; empty MMX state

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
; jpeg_quantize_int_mmx (JCOEFPTR coef_block, DCTELEM * divisors,
;                        DCTELEM * workspace);
;

%define RECIPROCAL(m,n,b) MMBLOCK(DCTSIZE*0+(m),(n),(b),SIZEOF_DCTELEM)
%define CORRECTION(m,n,b) MMBLOCK(DCTSIZE*1+(m),(n),(b),SIZEOF_DCTELEM)
%define SCALE(m,n,b)      MMBLOCK(DCTSIZE*2+(m),(n),(b),SIZEOF_DCTELEM)

%define coef_block	ebp+8		; JCOEFPTR coef_block
%define divisors	ebp+12		; DCTELEM * divisors
%define workspace	ebp+16		; DCTELEM * workspace

	align	16
	global	EXTN(jpeg_quantize_int_mmx)

EXTN(jpeg_quantize_int_mmx):
	push	ebp
	mov	ebp,esp
;	push	ebx		; unused
;	push	ecx		; unused
;	push	edx		; need not be preserved
	push	esi
	push	edi

	mov	esi, POINTER [workspace]
	mov	edx, POINTER [divisors]
	mov	edi, JCOEFPTR [coef_block]
	mov	ah, 2
	alignx	16,7
.quantloop1:
	mov	al, DCTSIZE2/8/2
	alignx	16,7
.quantloop2:
	movq	mm2, MMWORD [MMBLOCK(0,0,esi,SIZEOF_DCTELEM)]
	movq	mm3, MMWORD [MMBLOCK(0,1,esi,SIZEOF_DCTELEM)]
	movq	mm0,mm2
	movq	mm1,mm3
	psraw	mm2,(WORD_BIT-1)
	psraw	mm3,(WORD_BIT-1)
	pxor	mm0,mm2
	pxor	mm1,mm3
	psubw	mm0,mm2		; if (mm0 < 0) mm0 = -mm0;
	psubw	mm1,mm3		; if (mm1 < 0) mm1 = -mm1;

	; unsigned long unsigned_multiply(unsigned short x, unsigned short y)
	; {
	;   enum { SHORT_BIT = 16 };
	;   signed short sx = (signed short) x;
	;   signed short sy = (signed short) y;
	;   signed long sz;
	; 
	;   sz = (long) sx * (long) sy;     /* signed multiply */
	; 
	;   if (sx < 0) sz += (long) sy << SHORT_BIT;
	;   if (sy < 0) sz += (long) sx << SHORT_BIT;
	; 
	;   return (unsigned long) sz;
	; }

	paddw	mm0, MMWORD [CORRECTION(0,0,edx)]   ; correction + roundfactor
	paddw	mm1, MMWORD [CORRECTION(0,1,edx)]
	psllw	mm0,1
	psllw	mm1,1
	movq	mm4,mm0
	movq	mm5,mm1
	pmulhw	mm0, MMWORD [RECIPROCAL(0,0,edx)]   ; reciprocal
	pmulhw	mm1, MMWORD [RECIPROCAL(0,1,edx)]
	movq	mm6, MMWORD [SCALE(0,0,edx)]	; scale
	movq	mm7, MMWORD [SCALE(0,1,edx)]
	paddw	mm0,mm4		; reciprocal is always negative (MSB=1)
	paddw	mm1,mm5
	psllw	mm0,1
	psllw	mm1,1
	movq	mm4,mm0
	movq	mm5,mm1
	pmulhw	mm0,mm6
	pmulhw	mm1,mm7
	psraw	mm6,(WORD_BIT-1)
	psraw	mm7,(WORD_BIT-1)
	pand	mm6,mm4
	pand	mm7,mm5
	paddw	mm0,mm6
	paddw	mm1,mm7
	psraw	mm4,(WORD_BIT-1)
	psraw	mm5,(WORD_BIT-1)
	pand	mm4, MMWORD [SCALE(0,0,edx)]	; scale
	pand	mm5, MMWORD [SCALE(0,1,edx)]
	paddw	mm0,mm4
	paddw	mm1,mm5

	pxor	mm0,mm2
	pxor	mm1,mm3
	psubw	mm0,mm2
	psubw	mm1,mm3
	movq	MMWORD [MMBLOCK(0,0,edi,SIZEOF_DCTELEM)], mm0
	movq	MMWORD [MMBLOCK(0,1,edi,SIZEOF_DCTELEM)], mm1

	add	esi, byte 8*SIZEOF_DCTELEM
	add	edx, byte 8*SIZEOF_DCTELEM
	add	edi, byte 8*SIZEOF_JCOEF
	dec	al
	jnz	near .quantloop2
	dec	ah
	jnz	near .quantloop1	; to avoid branch misprediction

	emms		; empty MMX state

	pop	edi
	pop	esi
;	pop	edx		; need not be preserved
;	pop	ecx		; unused
;	pop	ebx		; unused
	pop	ebp
	ret

%endif ; !JFDCT_INT_QUANTIZE_WITH_DIVISION
%endif ; JFDCT_INT_MMX_SUPPORTED

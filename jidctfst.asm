;
; jidctfst.asm - fast integer IDCT (non-SIMD)
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
; This file contains a fast, not so accurate integer implementation of
; the inverse DCT (Discrete Cosine Transform). The following code is
; based directly on the IJG's original jidctfst.c; see the jidctfst.c
; for more details.
;
; Last Modified : October 17, 2004
;
; [TAB8]

%include "jsimdext.inc"
%include "jdct.inc"

%ifdef DCT_IFAST_SUPPORTED

; This module is specialized to the case DCTSIZE = 8.
;
%if DCTSIZE != 8
%error "Sorry, this code only copes with 8x8 DCTs."
%endif

; --------------------------------------------------------------------------

; We can gain a little more speed, with a further compromise in accuracy,
; by omitting the addition in a descaling shift.  This yields an
; incorrectly rounded result half the time...
;
%macro	descale 2
%ifdef USE_ACCURATE_ROUNDING
%if (%2)<=7
	add	%1, byte (1<<((%2)-1))	; add reg32,imm8
%else
	add	%1, (1<<((%2)-1))	; add reg32,imm32
%endif
%endif
	sar	%1,%2
%endmacro

; --------------------------------------------------------------------------

%define CONST_BITS	8
%define PASS1_BITS	2

%if IFAST_SCALE_BITS != PASS1_BITS
%error "'IFAST_SCALE_BITS' must be equal to 'PASS1_BITS'."
%endif

%if CONST_BITS == 8
F_1_082	equ	277		; FIX(1.082392200)
F_1_414	equ	362		; FIX(1.414213562)
F_1_847	equ	473		; FIX(1.847759065)
F_2_613	equ	669		; FIX(2.613125930)
%else
; NASM cannot do compile-time arithmetic on floating-point constants.
%define DESCALE(x,n)  (((x)+(1<<((n)-1)))>>(n))
F_1_082	equ	DESCALE(1162209775,30-CONST_BITS)	; FIX(1.082392200)
F_1_414	equ	DESCALE(1518500249,30-CONST_BITS)	; FIX(1.414213562)
F_1_847	equ	DESCALE(1984016188,30-CONST_BITS)	; FIX(1.847759065)
F_2_613	equ	DESCALE(2805822602,30-CONST_BITS)	; FIX(2.613125930)
%endif

; --------------------------------------------------------------------------
	SECTION	SEG_TEXT
	BITS	32
;
; Perform dequantization and inverse DCT on one block of coefficients.
;
; GLOBAL(void)
; jpeg_idct_ifast (j_decompress_ptr cinfo, jpeg_component_info * compptr,
;                  JCOEFPTR coef_block,
;                  JSAMPARRAY output_buf, JDIMENSION output_col)
;

%define cinfo(b)	(b)+8		; j_decompress_ptr cinfo
%define compptr(b)	(b)+12		; jpeg_component_info * compptr
%define coef_block(b)	(b)+16		; JCOEFPTR coef_block
%define output_buf(b)	(b)+20		; JSAMPARRAY output_buf
%define output_col(b)	(b)+24		; JDIMENSION output_col

%define range_limit	ebp-SIZEOF_POINTER		; JSAMPLE * range_limit
%define ptr		range_limit-SIZEOF_POINTER	; void * ptr
%define workspace	ptr-DCTSIZE2*SIZEOF_INT
					; int workspace[DCTSIZE2]

	align	16
	global	EXTN(jpeg_idct_ifast)

EXTN(jpeg_idct_ifast):
	push	ebp
	mov	ebp,esp
	lea	esp, [workspace]
	push	ebx
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
	push	esi
	push	edi

	; ---- Pass 1: process columns from input, store into work array.

	mov	edx, POINTER [compptr(ebp)]
	mov	edx, POINTER [jcompinfo_dct_table(edx)]	; quantptr
	mov	esi, JCOEFPTR [coef_block(ebp)]		; inptr
	lea	edi, [workspace]			; int * wsptr
	mov	ecx, DCTSIZE				; ctr
	alignx	16,7
.columnloop:
	mov	ax, JCOEF [COL(1,esi,SIZEOF_JCOEF)]
	or	ax, JCOEF [COL(2,esi,SIZEOF_JCOEF)]
	jnz	short .columnDCT

	mov	bx, JCOEF [COL(3,esi,SIZEOF_JCOEF)]
	mov	ax, JCOEF [COL(4,esi,SIZEOF_JCOEF)]
	or	bx, JCOEF [COL(5,esi,SIZEOF_JCOEF)]
	or	ax, JCOEF [COL(6,esi,SIZEOF_JCOEF)]
	or	bx, JCOEF [COL(7,esi,SIZEOF_JCOEF)]
	or	ax,bx
	jnz	short .columnDCT

	; -- AC terms all zero

	mov	ax, JCOEF [COL(0,esi,SIZEOF_JCOEF)]
	imul	ax, IFAST_MULT_TYPE [COL(0,edx,SIZEOF_IFAST_MULT_TYPE)]
	cwde

	mov	INT [COL(0,edi,SIZEOF_INT)], eax
	mov	INT [COL(1,edi,SIZEOF_INT)], eax
	mov	INT [COL(2,edi,SIZEOF_INT)], eax
	mov	INT [COL(3,edi,SIZEOF_INT)], eax
	mov	INT [COL(4,edi,SIZEOF_INT)], eax
	mov	INT [COL(5,edi,SIZEOF_INT)], eax
	mov	INT [COL(6,edi,SIZEOF_INT)], eax
	mov	INT [COL(7,edi,SIZEOF_INT)], eax
	jmp	near .nextcolumn
	alignx	16,7

.columnDCT:
	push	ecx	; ctr
	push	esi	; coef_block
	push	edx	; quantptr

	mov	POINTER [ptr], edi	; wsptr

	; -- Even part

	movsx	eax, JCOEF [COL(0,esi,SIZEOF_JCOEF)]
	movsx	ecx, JCOEF [COL(4,esi,SIZEOF_JCOEF)]
	imul	ax, IFAST_MULT_TYPE [COL(0,edx,SIZEOF_IFAST_MULT_TYPE)]
	imul	cx, IFAST_MULT_TYPE [COL(4,edx,SIZEOF_IFAST_MULT_TYPE)]
	movsx	ebx, JCOEF [COL(2,esi,SIZEOF_JCOEF)]
	movsx	edi, JCOEF [COL(6,esi,SIZEOF_JCOEF)]
	imul	bx, IFAST_MULT_TYPE [COL(2,edx,SIZEOF_IFAST_MULT_TYPE)]
	imul	di, IFAST_MULT_TYPE [COL(6,edx,SIZEOF_IFAST_MULT_TYPE)]

	lea	edx,[eax+ecx]		; edx=tmp10
	sub	eax,ecx			; eax=tmp11

	lea	ecx,[ebx+edi]		; ecx=tmp13
	sub	ebx,edi
	imul	ebx,(F_1_414)
	descale	ebx,CONST_BITS
	sub	ebx,ecx			; ebx=tmp12

	lea	edi,[edx+ecx]		; edi=tmp0
	sub	edx,ecx			; edx=tmp3
	lea	ecx,[eax+ebx]		; ecx=tmp1
	sub	eax,ebx			; eax=tmp2

	push	edx		; tmp3
	push	eax		; tmp2
	push	ecx		; tmp1
	push	edi		; tmp0

	; -- Odd part

	mov	edx, POINTER [esp+16]	; quantptr

	movsx	eax, JCOEF [COL(1,esi,SIZEOF_JCOEF)]
	movsx	ebx, JCOEF [COL(7,esi,SIZEOF_JCOEF)]
	imul	ax, IFAST_MULT_TYPE [COL(1,edx,SIZEOF_IFAST_MULT_TYPE)]
	imul	bx, IFAST_MULT_TYPE [COL(7,edx,SIZEOF_IFAST_MULT_TYPE)]
	movsx	edi, JCOEF [COL(5,esi,SIZEOF_JCOEF)]
	movsx	ecx, JCOEF [COL(3,esi,SIZEOF_JCOEF)]
	imul	di, IFAST_MULT_TYPE [COL(5,edx,SIZEOF_IFAST_MULT_TYPE)]
	imul	cx, IFAST_MULT_TYPE [COL(3,edx,SIZEOF_IFAST_MULT_TYPE)]

	lea	esi,[eax+ebx]		; esi=z11
	sub	eax,ebx			; eax=z12
	lea	edx,[edi+ecx]		; edx=z13
	sub	edi,ecx			; edi=z10

	lea	ebx,[esi+edx]		; ebx=tmp7
	sub	esi,edx
	imul	esi,(F_1_414)		; esi=tmp11
	descale	esi,CONST_BITS

	lea	ecx,[edi+eax]
	imul	ecx,(F_1_847)		; ecx=z5
	imul	edi,(-F_2_613)		; edi=MULTIPLY(z10,-FIX_2_613125930)
	imul	eax,(F_1_082)		; eax=MULTIPLY(z12,FIX_1_082392200)
	descale	ecx,CONST_BITS
	descale	edi,CONST_BITS
	descale	eax,CONST_BITS
	add	edi,ecx			; edi=tmp12
	sub	eax,ecx			; eax=tmp10

	; -- Final output stage

	sub	edi,ebx		; edi=tmp6
	pop	edx		; edx=tmp0
	sub	esi,edi		; esi=tmp5
	pop	ecx		; ecx=tmp1
	add	eax,esi		; eax=tmp4
	push	esi		; tmp5
	push	eax		; tmp4

	lea	eax,[edx+ebx]	; eax=data0(=tmp0+tmp7)
	sub	edx,ebx		; edx=data7(=tmp0-tmp7)
	lea	ebx,[ecx+edi]	; ebx=data1(=tmp1+tmp6)
	sub	ecx,edi		; ecx=data6(=tmp1-tmp6)

	mov	edi, POINTER [ptr]	; edi=wsptr

	mov	INT [COL(0,edi,SIZEOF_INT)], eax
	mov	INT [COL(7,edi,SIZEOF_INT)], edx
	mov	INT [COL(1,edi,SIZEOF_INT)], ebx
	mov	INT [COL(6,edi,SIZEOF_INT)], ecx

	pop	esi		; esi=tmp4
	pop	eax		; eax=tmp5
	pop	edx		; edx=tmp2
	pop	ecx		; ecx=tmp3

	lea	ebx,[edx+eax]	; ebx=data2(=tmp2+tmp5)
	sub	edx,eax		; edx=data5(=tmp2-tmp5)
	lea	eax,[ecx+esi]	; eax=data4(=tmp3+tmp4)
	sub	ecx,esi		; ecx=data3(=tmp3-tmp4)

	mov	INT [COL(2,edi,SIZEOF_INT)], ebx
	mov	INT [COL(5,edi,SIZEOF_INT)], edx
	mov	INT [COL(4,edi,SIZEOF_INT)], eax
	mov	INT [COL(3,edi,SIZEOF_INT)], ecx

	pop	edx	; quantptr
	pop	esi	; coef_block
	pop	ecx	; ctr

.nextcolumn:
	add	esi, byte SIZEOF_JCOEF	; advance pointers to next column
	add	edx, byte SIZEOF_IFAST_MULT_TYPE
	add	edi, byte SIZEOF_INT
	dec	ecx
	jnz	near .columnloop

	; ---- Pass 2: process rows from work array, store into output array.

	mov	eax, POINTER [cinfo(ebp)]
	mov	eax, POINTER [jdstruct_sample_range_limit(eax)]
	sub	eax, byte -CENTERJSAMPLE*SIZEOF_JSAMPLE	; JSAMPLE * range_limit
	mov	POINTER [range_limit], eax

	lea	esi, [workspace]			; int * wsptr
	mov	edi, JSAMPARRAY [output_buf(ebp)]	; (JSAMPROW *)
	mov	ecx, DCTSIZE				; ctr
	alignx	16,7
.rowloop:
	push	edi
	mov	edi, JSAMPROW [edi]			; (JSAMPLE *)
	add	edi, JDIMENSION [output_col(ebp)]	; edi=outptr

%ifndef NO_ZERO_ROW_TEST
	mov	eax, INT [ROW(1,esi,SIZEOF_INT)]
	or	eax, INT [ROW(2,esi,SIZEOF_INT)]
	jnz	short .rowDCT

	mov	ebx, INT [ROW(3,esi,SIZEOF_INT)]
	mov	eax, INT [ROW(4,esi,SIZEOF_INT)]
	or	ebx, INT [ROW(5,esi,SIZEOF_INT)]
	or	eax, INT [ROW(6,esi,SIZEOF_INT)]
	or	ebx, INT [ROW(7,esi,SIZEOF_INT)]
	or	eax,ebx
	jnz	short .rowDCT

	; -- AC terms all zero

	mov	eax, INT [ROW(0,esi,SIZEOF_INT)]

	mov	edx, POINTER [range_limit]	; (JSAMPLE *)

	descale	eax,(PASS1_BITS+3)
	and	eax,RANGE_MASK
	mov	al, JSAMPLE [edx+eax*SIZEOF_JSAMPLE]
	mov	JSAMPLE [edi+0*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+1*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+2*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+3*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+4*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+5*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+6*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+7*SIZEOF_JSAMPLE], al
	jmp	near .nextrow
	alignx	16,7
%endif
.rowDCT:
	push	esi	; wsptr
	push	ecx	; ctr

	mov	POINTER [ptr], edi	; outptr

	; -- Even part

	mov	eax, INT [ROW(0,esi,SIZEOF_INT)]
	mov	ebx, INT [ROW(2,esi,SIZEOF_INT)]
	mov	ecx, INT [ROW(4,esi,SIZEOF_INT)]
	mov	edi, INT [ROW(6,esi,SIZEOF_INT)]

	lea	edx,[eax+ecx]		; edx=tmp10
	sub	eax,ecx			; eax=tmp11

	lea	ecx,[ebx+edi]		; ecx=tmp13
	sub	ebx,edi
	imul	ebx,(F_1_414)
	descale	ebx,CONST_BITS
	sub	ebx,ecx			; ebx=tmp12

	lea	edi,[edx+ecx]		; edi=tmp0
	sub	edx,ecx			; edx=tmp3
	lea	ecx,[eax+ebx]		; ecx=tmp1
	sub	eax,ebx			; eax=tmp2

	push	edx		; tmp3
	push	eax		; tmp2
	push	ecx		; tmp1
	push	edi		; tmp0

	; -- Odd part

	mov	eax, INT [ROW(1,esi,SIZEOF_INT)]
	mov	ecx, INT [ROW(3,esi,SIZEOF_INT)]
	mov	edi, INT [ROW(5,esi,SIZEOF_INT)]
	mov	ebx, INT [ROW(7,esi,SIZEOF_INT)]

	lea	esi,[eax+ebx]		; esi=z11
	sub	eax,ebx			; eax=z12
	lea	edx,[edi+ecx]		; edx=z13
	sub	edi,ecx			; edi=z10

	lea	ebx,[esi+edx]		; ebx=tmp7
	sub	esi,edx
	imul	esi,(F_1_414)		; esi=tmp11
	descale	esi,CONST_BITS

	lea	ecx,[edi+eax]
	imul	ecx,(F_1_847)		; ecx=z5
	imul	edi,(-F_2_613)		; edi=MULTIPLY(z10,-FIX_2_613125930)
	imul	eax,(F_1_082)		; eax=MULTIPLY(z12,FIX_1_082392200)
	descale	ecx,CONST_BITS
	descale	edi,CONST_BITS
	descale	eax,CONST_BITS
	add	edi,ecx			; edi=tmp12
	sub	eax,ecx			; eax=tmp10

	; -- Final output stage

	sub	edi,ebx		; edi=tmp6
	pop	edx		; edx=tmp0
	sub	esi,edi		; esi=tmp5
	pop	ecx		; ecx=tmp1
	add	eax,esi		; eax=tmp4
	push	esi		; tmp5
	push	eax		; tmp4

	lea	eax,[edx+ebx]	; eax=data0(=tmp0+tmp7)
	sub	edx,ebx		; edx=data7(=tmp0-tmp7)
	lea	ebx,[ecx+edi]	; ebx=data1(=tmp1+tmp6)
	sub	ecx,edi		; ecx=data6(=tmp1-tmp6)

	mov	esi, POINTER [range_limit]	; (JSAMPLE *)

	descale	eax,(PASS1_BITS+3)
	descale	edx,(PASS1_BITS+3)
	descale	ebx,(PASS1_BITS+3)
	descale	ecx,(PASS1_BITS+3)

	mov	edi, POINTER [ptr]		; edi=outptr

	and	eax,RANGE_MASK
	and	edx,RANGE_MASK
	and	ebx,RANGE_MASK
	and	ecx,RANGE_MASK

	mov	al, JSAMPLE [esi+eax*SIZEOF_JSAMPLE]
	mov	dl, JSAMPLE [esi+edx*SIZEOF_JSAMPLE]
	mov	bl, JSAMPLE [esi+ebx*SIZEOF_JSAMPLE]
	mov	cl, JSAMPLE [esi+ecx*SIZEOF_JSAMPLE]

	mov	JSAMPLE [edi+0*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+7*SIZEOF_JSAMPLE], dl
	mov	JSAMPLE [edi+1*SIZEOF_JSAMPLE], bl
	mov	JSAMPLE [edi+6*SIZEOF_JSAMPLE], cl

	pop	esi		; esi=tmp4
	pop	eax		; eax=tmp5
	pop	edx		; edx=tmp2
	pop	ecx		; ecx=tmp3

	lea	ebx,[edx+eax]	; ebx=data2(=tmp2+tmp5)
	sub	edx,eax		; edx=data5(=tmp2-tmp5)
	lea	eax,[ecx+esi]	; eax=data4(=tmp3+tmp4)
	sub	ecx,esi		; ecx=data3(=tmp3-tmp4)

	mov	esi, POINTER [range_limit]	; (JSAMPLE *)

	descale	ebx,(PASS1_BITS+3)
	descale	edx,(PASS1_BITS+3)
	descale	eax,(PASS1_BITS+3)
	descale	ecx,(PASS1_BITS+3)

	and	ebx,RANGE_MASK
	and	edx,RANGE_MASK
	and	eax,RANGE_MASK
	and	ecx,RANGE_MASK

	mov	bl, JSAMPLE [esi+ebx*SIZEOF_JSAMPLE]
	mov	dl, JSAMPLE [esi+edx*SIZEOF_JSAMPLE]
	mov	al, JSAMPLE [esi+eax*SIZEOF_JSAMPLE]
	mov	cl, JSAMPLE [esi+ecx*SIZEOF_JSAMPLE]

	mov	JSAMPLE [edi+2*SIZEOF_JSAMPLE], bl
	mov	JSAMPLE [edi+5*SIZEOF_JSAMPLE], dl
	mov	JSAMPLE [edi+4*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+3*SIZEOF_JSAMPLE], cl

	pop	ecx	; ctr
	pop	esi	; wsptr

.nextrow:
	pop	edi
	add	esi, byte DCTSIZE*SIZEOF_INT	; advance pointer to next row
	add	edi, byte SIZEOF_JSAMPROW
	dec	ecx
	jnz	near .rowloop

	pop	edi
	pop	esi
;	pop	edx		; need not be preserved
;	pop	ecx		; need not be preserved
	pop	ebx
	mov	esp,ebp
	pop	ebp
	ret

%endif ; DCT_IFAST_SUPPORTED

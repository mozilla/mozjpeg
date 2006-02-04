;
; jfdctfst.asm - fast integer FDCT (non-SIMD)
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
; the forward DCT (Discrete Cosine Transform). The following code is based
; directly on the IJG's original jfdctfst.c; see the jfdctfst.c for
; more details.
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

%if CONST_BITS == 8
F_0_382	equ	 98		; FIX(0.382683433)
F_0_541	equ	139		; FIX(0.541196100)
F_0_707	equ	181		; FIX(0.707106781)
F_1_306	equ	334		; FIX(1.306562965)
%else
; NASM cannot do compile-time arithmetic on floating-point constants.
%define DESCALE(x,n)  (((x)+(1<<((n)-1)))>>(n))
F_0_382	equ	DESCALE( 410903207,30-CONST_BITS)	; FIX(0.382683433)
F_0_541	equ	DESCALE( 581104887,30-CONST_BITS)	; FIX(0.541196100)
F_0_707	equ	DESCALE( 759250124,30-CONST_BITS)	; FIX(0.707106781)
F_1_306	equ	DESCALE(1402911301,30-CONST_BITS)	; FIX(1.306562965)
%endif

; --------------------------------------------------------------------------
	SECTION	SEG_TEXT
	BITS	32
;
; Perform the forward DCT on one block of samples.
;
; GLOBAL(void)
; jpeg_fdct_ifast (DCTELEM * data)
;

%define data(b)	(b)+8		; DCTELEM * data

	align	16
	global	EXTN(jpeg_fdct_ifast)

EXTN(jpeg_fdct_ifast):
	push	ebp
	mov	ebp,esp
	push	ebx
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
	push	esi
	push	edi

	; ---- Pass 1: process rows.

	mov	ecx, DCTSIZE
	mov	edx, POINTER [data(ebp)]	; (DCTELEM *)
	alignx	16,7
.rowloop:
	push	ecx		; ctr
	push	edx		; dataptr

	movsx	eax, DCTELEM [ROW(0,edx,SIZEOF_DCTELEM)]
	movsx	edi, DCTELEM [ROW(7,edx,SIZEOF_DCTELEM)]
	lea	esi,[eax+edi]	; esi=tmp0
	sub	eax,edi		; eax=tmp7
	push	eax

	movsx	ebx, DCTELEM [ROW(1,edx,SIZEOF_DCTELEM)]
	movsx	ecx, DCTELEM [ROW(6,edx,SIZEOF_DCTELEM)]
	lea	edi,[ebx+ecx]	; edi=tmp1
	sub	ebx,ecx		; ebx=tmp6
	push	ebx

	movsx	eax, DCTELEM [ROW(2,edx,SIZEOF_DCTELEM)]
	movsx	ecx, DCTELEM [ROW(5,edx,SIZEOF_DCTELEM)]
	lea	ebx,[eax+ecx]	; ebx=tmp2
	sub	eax,ecx		; eax=tmp5
	push	eax

	movsx	ecx, DCTELEM [ROW(3,edx,SIZEOF_DCTELEM)]
	movsx	eax, DCTELEM [ROW(4,edx,SIZEOF_DCTELEM)]
	lea	edx,[ecx+eax]	; edx=tmp3
	sub	ecx,eax		; ecx=tmp4
	push	ecx

	; -- Even part

	lea	eax,[esi+edx]	; eax=tmp10
	lea	ecx,[edi+ebx]	; ecx=tmp11
	sub	esi,edx		; esi=tmp13
	sub	edi,ebx		; edi=tmp12

	mov	edx, POINTER [esp+16]	; dataptr

	add	edi,esi
	imul	edi,(F_0_707)	; edi=z1
	descale	edi,CONST_BITS

	lea	ebx,[eax+ecx]	; ebx=data0
	sub	eax,ecx		; eax=data4
	mov	DCTELEM [ROW(0,edx,SIZEOF_DCTELEM)], bx
	mov	DCTELEM [ROW(4,edx,SIZEOF_DCTELEM)], ax

	lea	ecx,[esi+edi]	; ecx=data2
	sub	esi,edi		; esi=data6
	mov	DCTELEM [ROW(2,edx,SIZEOF_DCTELEM)], cx
	mov	DCTELEM [ROW(6,edx,SIZEOF_DCTELEM)], si

	; -- Odd part

	pop	eax	; eax=tmp4
	pop	edx	; edx=tmp5
	pop	ebx	; ebx=tmp6
	pop	edi	; edi=tmp7

	add	eax,edx		; eax=tmp10
	add	edx,ebx		; edx=tmp11
	add	ebx,edi		; ebx=tmp12, edi=tmp7

	imul	edx,(F_0_707)	; edx=z3
	descale	edx,CONST_BITS
	lea	esi,[edi+edx]	; esi=z11
	sub	edi,edx		; edi=z13

	mov	ecx,eax		; ecx=tmp10
	sub	eax,ebx
	imul	eax,(F_0_382)	; eax=z5
	imul	ecx,(F_0_541)	; ecx=MULTIPLY(tmp10,FIX_0_541196100)
	imul	ebx,(F_1_306)	; ebx=MULTIPLY(tmp12,FIX_1_306562965)
	descale	eax,CONST_BITS
	descale	ecx,CONST_BITS
	descale	ebx,CONST_BITS
	add	ecx,eax		; ecx=z2
	add	ebx,eax		; ebx=z4

	pop	edx		; dataptr

	lea	eax,[edi+ecx]	; eax=data5
	sub	edi,ecx		; edi=data3
	mov	DCTELEM [ROW(5,edx,SIZEOF_DCTELEM)], ax
	mov	DCTELEM [ROW(3,edx,SIZEOF_DCTELEM)], di

	lea	ecx,[esi+ebx]	; ecx=data1
	sub	esi,ebx		; esi=data7
	mov	DCTELEM [ROW(1,edx,SIZEOF_DCTELEM)], cx
	mov	DCTELEM [ROW(7,edx,SIZEOF_DCTELEM)], si

	pop	ecx		; ctr

	add	edx, byte DCTSIZE*SIZEOF_DCTELEM
	dec	ecx			; advance pointer to next row
	jnz	near .rowloop

	; ---- Pass 2: process columns.

	mov	ecx, DCTSIZE
	mov	edx, POINTER [data(ebp)]	; (DCTELEM *)
	alignx	16,7
.columnloop:
	push	ecx		; ctr
	push	edx		; dataptr

	movsx	eax, DCTELEM [COL(0,edx,SIZEOF_DCTELEM)]
	movsx	edi, DCTELEM [COL(7,edx,SIZEOF_DCTELEM)]
	lea	esi,[eax+edi]	; esi=tmp0
	sub	eax,edi		; eax=tmp7
	push	eax

	movsx	ebx, DCTELEM [COL(1,edx,SIZEOF_DCTELEM)]
	movsx	ecx, DCTELEM [COL(6,edx,SIZEOF_DCTELEM)]
	lea	edi,[ebx+ecx]	; edi=tmp1
	sub	ebx,ecx		; ebx=tmp6
	push	ebx

	movsx	eax, DCTELEM [COL(2,edx,SIZEOF_DCTELEM)]
	movsx	ecx, DCTELEM [COL(5,edx,SIZEOF_DCTELEM)]
	lea	ebx,[eax+ecx]	; ebx=tmp2
	sub	eax,ecx		; eax=tmp5
	push	eax

	movsx	ecx, DCTELEM [COL(3,edx,SIZEOF_DCTELEM)]
	movsx	eax, DCTELEM [COL(4,edx,SIZEOF_DCTELEM)]
	lea	edx,[ecx+eax]	; edx=tmp3
	sub	ecx,eax		; ecx=tmp4
	push	ecx

	; -- Even part

	lea	eax,[esi+edx]	; eax=tmp10
	lea	ecx,[edi+ebx]	; ecx=tmp11
	sub	esi,edx		; esi=tmp13
	sub	edi,ebx		; edi=tmp12

	mov	edx, POINTER [esp+16]	; dataptr

	add	edi,esi
	imul	edi,(F_0_707)	; edi=z1
	descale	edi,CONST_BITS

	lea	ebx,[eax+ecx]	; ebx=data0
	sub	eax,ecx		; eax=data4
	mov	DCTELEM [COL(0,edx,SIZEOF_DCTELEM)], bx
	mov	DCTELEM [COL(4,edx,SIZEOF_DCTELEM)], ax

	lea	ecx,[esi+edi]	; ecx=data2
	sub	esi,edi		; esi=data6
	mov	DCTELEM [COL(2,edx,SIZEOF_DCTELEM)], cx
	mov	DCTELEM [COL(6,edx,SIZEOF_DCTELEM)], si

	; -- Odd part

	pop	eax	; eax=tmp4
	pop	edx	; edx=tmp5
	pop	ebx	; ebx=tmp6
	pop	edi	; edi=tmp7

	add	eax,edx		; eax=tmp10
	add	edx,ebx		; edx=tmp11
	add	ebx,edi		; ebx=tmp12, edi=tmp7

	imul	edx,(F_0_707)	; edx=z3
	descale	edx,CONST_BITS
	lea	esi,[edi+edx]	; esi=z11
	sub	edi,edx		; edi=z13

	mov	ecx,eax		; ecx=tmp10
	sub	eax,ebx
	imul	eax,(F_0_382)	; eax=z5
	imul	ecx,(F_0_541)	; ecx=MULTIPLY(tmp10,FIX_0_541196100)
	imul	ebx,(F_1_306)	; ebx=MULTIPLY(tmp12,FIX_1_306562965)
	descale	eax,CONST_BITS
	descale	ecx,CONST_BITS
	descale	ebx,CONST_BITS
	add	ecx,eax		; ecx=z2
	add	ebx,eax		; ebx=z4

	pop	edx		; dataptr

	lea	eax,[edi+ecx]	; eax=data5
	sub	edi,ecx		; edi=data3
	mov	DCTELEM [COL(5,edx,SIZEOF_DCTELEM)], ax
	mov	DCTELEM [COL(3,edx,SIZEOF_DCTELEM)], di

	lea	ecx,[esi+ebx]	; ecx=data1
	sub	esi,ebx		; esi=data7
	mov	DCTELEM [COL(1,edx,SIZEOF_DCTELEM)], cx
	mov	DCTELEM [COL(7,edx,SIZEOF_DCTELEM)], si

	pop	ecx		; ctr

	add	edx, byte SIZEOF_DCTELEM    ; advance pointer to next column
	dec	ecx
	jnz	near .columnloop

	pop	edi
	pop	esi
;	pop	edx		; need not be preserved
;	pop	ecx		; need not be preserved
	pop	ebx
	pop	ebp
	ret

%endif ; DCT_IFAST_SUPPORTED

;
; jfdctint.asm - accurate integer FDCT (non-SIMD)
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
; This file contains a slow-but-accurate integer implementation of the
; forward DCT (Discrete Cosine Transform). The following code is based
; directly on the IJG's original jfdctint.c; see the jfdctint.c for
; more details.
;
; Last Modified : October 17, 2004
;
; [TAB8]

%include "jsimdext.inc"
%include "jdct.inc"

%ifdef DCT_ISLOW_SUPPORTED

; This module is specialized to the case DCTSIZE = 8.
;
%if DCTSIZE != 8
%error "Sorry, this code only copes with 8x8 DCTs."
%endif

; --------------------------------------------------------------------------

; Descale and correctly round a DWORD value that's scaled by N bits.
;
%macro	descale 2
%if (%2)<=7
	add	%1, byte (1<<((%2)-1))	; add reg32,imm8
%else
	add	%1, (1<<((%2)-1))	; add reg32,imm32
%endif
	sar	%1,%2
%endmacro

; --------------------------------------------------------------------------

%define CONST_BITS	13
%define PASS1_BITS	2

%if CONST_BITS == 13
F_0_298	equ	 2446		; FIX(0.298631336)
F_0_390	equ	 3196		; FIX(0.390180644)
F_0_541	equ	 4433		; FIX(0.541196100)
F_0_765	equ	 6270		; FIX(0.765366865)
F_0_899	equ	 7373		; FIX(0.899976223)
F_1_175	equ	 9633		; FIX(1.175875602)
F_1_501	equ	12299		; FIX(1.501321110)
F_1_847	equ	15137		; FIX(1.847759065)
F_1_961	equ	16069		; FIX(1.961570560)
F_2_053	equ	16819		; FIX(2.053119869)
F_2_562	equ	20995		; FIX(2.562915447)
F_3_072	equ	25172		; FIX(3.072711026)
%else
; NASM cannot do compile-time arithmetic on floating-point constants.
%define DESCALE(x,n)  (((x)+(1<<((n)-1)))>>(n))
F_0_298	equ	DESCALE( 320652955,30-CONST_BITS)	; FIX(0.298631336)
F_0_390	equ	DESCALE( 418953276,30-CONST_BITS)	; FIX(0.390180644)
F_0_541	equ	DESCALE( 581104887,30-CONST_BITS)	; FIX(0.541196100)
F_0_765	equ	DESCALE( 821806413,30-CONST_BITS)	; FIX(0.765366865)
F_0_899	equ	DESCALE( 966342111,30-CONST_BITS)	; FIX(0.899976223)
F_1_175	equ	DESCALE(1262586813,30-CONST_BITS)	; FIX(1.175875602)
F_1_501	equ	DESCALE(1612031267,30-CONST_BITS)	; FIX(1.501321110)
F_1_847	equ	DESCALE(1984016188,30-CONST_BITS)	; FIX(1.847759065)
F_1_961	equ	DESCALE(2106220350,30-CONST_BITS)	; FIX(1.961570560)
F_2_053	equ	DESCALE(2204520673,30-CONST_BITS)	; FIX(2.053119869)
F_2_562	equ	DESCALE(2751909506,30-CONST_BITS)	; FIX(2.562915447)
F_3_072	equ	DESCALE(3299298341,30-CONST_BITS)	; FIX(3.072711026)
%endif

; --------------------------------------------------------------------------
	SECTION	SEG_TEXT
	BITS	32
;
; Perform the forward DCT on one block of samples.
;
; GLOBAL(void)
; jpeg_fdct_islow (DCTELEM * data)
;

%define data(b)	(b)+8		; DCTELEM * data

	align	16
	global	EXTN(jpeg_fdct_islow)

EXTN(jpeg_fdct_islow):
	push	ebp
	mov	ebp,esp
	push	ebx
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
	push	esi
	push	edi

	; ---- Pass 1: process rows.

	mov	edx, POINTER [data(ebp)]	; (DCTELEM *)
	mov	ecx, DCTSIZE
	alignx	16,7
.rowloop:
	movsx	eax, DCTELEM [ROW(0,edx,SIZEOF_DCTELEM)]
	movsx	edi, DCTELEM [ROW(7,edx,SIZEOF_DCTELEM)]
	lea	esi,[eax+edi]	; esi=tmp0
	sub	eax,edi		; eax=tmp7
	push	ecx		; ctr
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
	push	edx		; dataptr
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

	lea	ebx,[eax+ecx]	; ebx=data0
	sub	eax,ecx		; eax=data4
	mov	edx, POINTER [esp+8]	; dataptr
	sal	ebx, PASS1_BITS
	sal	eax, PASS1_BITS
	mov	DCTELEM [ROW(0,edx,SIZEOF_DCTELEM)], bx
	mov	DCTELEM [ROW(4,edx,SIZEOF_DCTELEM)], ax

	lea	ecx,[edi+esi]
	imul	ecx,(F_0_541)	; ecx=z1
	imul	esi,(F_0_765)	; esi=MULTIPLY(tmp13,FIX_0_765366865)
	imul	edi,(-F_1_847)	; edi=MULTIPLY(tmp12,-FIX_1_847759065)
	add	esi,ecx		; esi=data2
	add	edi,ecx		; edi=data6
	descale	esi,(CONST_BITS-PASS1_BITS)
	descale	edi,(CONST_BITS-PASS1_BITS)
	mov	DCTELEM [ROW(2,edx,SIZEOF_DCTELEM)], si
	mov	DCTELEM [ROW(6,edx,SIZEOF_DCTELEM)], di

	; -- Odd part

	mov	eax, INT32 [esp]	; eax=tmp4
	mov	ebx, INT32 [esp+4]	; ebx=tmp5
	mov	ecx, INT32 [esp+12]	; ecx=tmp6
	mov	esi, INT32 [esp+16]	; esi=tmp7

	lea	edx,[eax+ecx]	; edx=z3
	lea	edi,[ebx+esi]	; edi=z4
	add	eax,esi		; eax=z1
	add	ebx,ecx		; ebx=z2

	lea	esi,[edx+edi]
	imul	esi,(F_1_175)	; esi=z5

	imul	edx,(-F_1_961)	; edx=z3(=MULTIPLY(z3,-FIX_1_961570560))
	imul	edi,(-F_0_390)	; edi=z4(=MULTIPLY(z4,-FIX_0_390180644))
	imul	eax,(-F_0_899)	; eax=z1(=MULTIPLY(z1,-FIX_0_899976223))
	imul	ebx,(-F_2_562)	; ebx=z2(=MULTIPLY(z2,-FIX_2_562915447))

	add	edx,esi		; edx=z3(=z3+z5)
	add	edi,esi		; edi=z4(=z4+z5)

	lea	ecx,[eax+edx]	; ecx=z1+z3
	lea	esi,[ebx+edi]	; esi=z2+z4
	add	eax,edi		; eax=z1+z4
	add	ebx,edx		; ebx=z2+z3

	pop	edx		; edx=tmp4
	pop	edi		; edi=tmp5
	imul	edx,(F_0_298)	; edx=tmp4(=MULTIPLY(tmp4,FIX_0_298631336))
	imul	edi,(F_2_053)	; edi=tmp5(=MULTIPLY(tmp5,FIX_2_053119869))
	add	ecx,edx		; ecx=data7(=tmp4+z1+z3)
	add	esi,edi		; esi=data5(=tmp5+z2+z4)
	pop	edx		; dataptr
	descale	ecx,(CONST_BITS-PASS1_BITS)
	descale	esi,(CONST_BITS-PASS1_BITS)
	mov	DCTELEM [ROW(7,edx,SIZEOF_DCTELEM)], cx
	mov	DCTELEM [ROW(5,edx,SIZEOF_DCTELEM)], si

	pop	edi		; edi=tmp6
	pop	ecx		; ecx=tmp7
	imul	edi,(F_3_072)	; edi=tmp6(=MULTIPLY(tmp6,FIX_3_072711026))
	imul	ecx,(F_1_501)	; ecx=tmp7(=MULTIPLY(tmp7,FIX_1_501321110))
	add	ebx,edi		; ebx=data3(=tmp6+z2+z3)
	add	eax,ecx		; eax=data1(=tmp7+z1+z4)
	pop	ecx		; ctr
	descale	ebx,(CONST_BITS-PASS1_BITS)
	descale	eax,(CONST_BITS-PASS1_BITS)
	mov	DCTELEM [ROW(3,edx,SIZEOF_DCTELEM)], bx
	mov	DCTELEM [ROW(1,edx,SIZEOF_DCTELEM)], ax

	add	edx, byte DCTSIZE*SIZEOF_DCTELEM
	dec	ecx			; advance pointer to next row
	jnz	near .rowloop

	; ---- Pass 2: process columns.

	mov	edx, POINTER [data(ebp)]	; (DCTELEM *)
	mov	ecx, DCTSIZE
	alignx	16,7
.columnloop:
	movsx	eax, DCTELEM [COL(0,edx,SIZEOF_DCTELEM)]
	movsx	edi, DCTELEM [COL(7,edx,SIZEOF_DCTELEM)]
	lea	esi,[eax+edi]	; esi=tmp0
	sub	eax,edi		; eax=tmp7
	push	ecx		; ctr
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
	push	edx		; dataptr
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

	lea	ebx,[eax+ecx]	; ebx=data0
	sub	eax,ecx		; eax=data4
	mov	edx, POINTER [esp+8]	; dataptr
	descale	ebx, PASS1_BITS
	descale	eax, PASS1_BITS
	mov	DCTELEM [COL(0,edx,SIZEOF_DCTELEM)], bx
	mov	DCTELEM [COL(4,edx,SIZEOF_DCTELEM)], ax

	lea	ecx,[edi+esi]
	imul	ecx,(F_0_541)	; ecx=z1
	imul	esi,(F_0_765)	; esi=MULTIPLY(tmp13,FIX_0_765366865)
	imul	edi,(-F_1_847)	; edi=MULTIPLY(tmp12,-FIX_1_847759065)
	add	esi,ecx		; esi=data2
	add	edi,ecx		; edi=data6
	descale	esi,(CONST_BITS+PASS1_BITS)
	descale	edi,(CONST_BITS+PASS1_BITS)
	mov	DCTELEM [COL(2,edx,SIZEOF_DCTELEM)], si
	mov	DCTELEM [COL(6,edx,SIZEOF_DCTELEM)], di

	; -- Odd part

	mov	eax, INT32 [esp]	; eax=tmp4
	mov	ebx, INT32 [esp+4]	; ebx=tmp5
	mov	ecx, INT32 [esp+12]	; ecx=tmp6
	mov	esi, INT32 [esp+16]	; esi=tmp7

	lea	edx,[eax+ecx]	; edx=z3
	lea	edi,[ebx+esi]	; edi=z4
	add	eax,esi		; eax=z1
	add	ebx,ecx		; ebx=z2

	lea	esi,[edx+edi]
	imul	esi,(F_1_175)	; esi=z5

	imul	edx,(-F_1_961)	; edx=z3(=MULTIPLY(z3,-FIX_1_961570560))
	imul	edi,(-F_0_390)	; edi=z4(=MULTIPLY(z4,-FIX_0_390180644))
	imul	eax,(-F_0_899)	; eax=z1(=MULTIPLY(z1,-FIX_0_899976223))
	imul	ebx,(-F_2_562)	; ebx=z2(=MULTIPLY(z2,-FIX_2_562915447))

	add	edx,esi		; edx=z3(=z3+z5)
	add	edi,esi		; edi=z4(=z4+z5)

	lea	ecx,[eax+edx]	; ecx=z1+z3
	lea	esi,[ebx+edi]	; esi=z2+z4
	add	eax,edi		; eax=z1+z4
	add	ebx,edx		; ebx=z2+z3

	pop	edx		; edx=tmp4
	pop	edi		; edi=tmp5
	imul	edx,(F_0_298)	; edx=tmp4(=MULTIPLY(tmp4,FIX_0_298631336))
	imul	edi,(F_2_053)	; edi=tmp5(=MULTIPLY(tmp5,FIX_2_053119869))
	add	ecx,edx		; ecx=data7(=tmp4+z1+z3)
	add	esi,edi		; esi=data5(=tmp5+z2+z4)
	pop	edx		; dataptr
	descale	ecx,(CONST_BITS+PASS1_BITS)
	descale	esi,(CONST_BITS+PASS1_BITS)
	mov	DCTELEM [COL(7,edx,SIZEOF_DCTELEM)], cx
	mov	DCTELEM [COL(5,edx,SIZEOF_DCTELEM)], si

	pop	edi		; edi=tmp6
	pop	ecx		; ecx=tmp7
	imul	edi,(F_3_072)	; edi=tmp6(=MULTIPLY(tmp6,FIX_3_072711026))
	imul	ecx,(F_1_501)	; ecx=tmp7(=MULTIPLY(tmp7,FIX_1_501321110))
	add	ebx,edi		; ebx=data3(=tmp6+z2+z3)
	add	eax,ecx		; eax=data1(=tmp7+z1+z4)
	pop	ecx		; ctr
	descale	ebx,(CONST_BITS+PASS1_BITS)
	descale	eax,(CONST_BITS+PASS1_BITS)
	mov	DCTELEM [COL(3,edx,SIZEOF_DCTELEM)], bx
	mov	DCTELEM [COL(1,edx,SIZEOF_DCTELEM)], ax

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

%endif ; DCT_ISLOW_SUPPORTED

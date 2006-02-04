;
; jidctint.asm - accurate integer IDCT (non-SIMD)
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
; inverse DCT (Discrete Cosine Transform). The following code is based
; directly on the IJG's original jidctint.c; see the jidctint.c for
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
; Perform dequantization and inverse DCT on one block of coefficients.
;
; GLOBAL(void)
; jpeg_idct_islow (j_decompress_ptr cinfo, jpeg_component_info * compptr,
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
	global	EXTN(jpeg_idct_islow)

EXTN(jpeg_idct_islow):
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
	imul	ax, ISLOW_MULT_TYPE [COL(0,edx,SIZEOF_ISLOW_MULT_TYPE)]
	cwde

	sal	eax,PASS1_BITS

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
	imul	ax, ISLOW_MULT_TYPE [COL(0,edx,SIZEOF_ISLOW_MULT_TYPE)]
	imul	cx, ISLOW_MULT_TYPE [COL(4,edx,SIZEOF_ISLOW_MULT_TYPE)]
	movsx	ebx, JCOEF [COL(2,esi,SIZEOF_JCOEF)]
	movsx	edi, JCOEF [COL(6,esi,SIZEOF_JCOEF)]
	imul	bx, ISLOW_MULT_TYPE [COL(2,edx,SIZEOF_ISLOW_MULT_TYPE)]
	imul	di, ISLOW_MULT_TYPE [COL(6,edx,SIZEOF_ISLOW_MULT_TYPE)]

	lea	edx,[eax+ecx]
	sub	eax,ecx
	sal	edx,CONST_BITS	; edx=tmp0
	sal	eax,CONST_BITS	; eax=tmp1

	lea	ecx,[ebx+edi]
	imul	ecx,(F_0_541)	; ecx=z1
	imul	ebx,(F_0_765)	; ebx=MULTIPLY(z2,FIX_0_765366865)
	imul	edi,(-F_1_847)	; edi=MULTIPLY(z3,-FIX_1_847759065)
	add	ebx,ecx		; ebx=tmp3
	add	edi,ecx		; edi=tmp2

	lea	ecx,[edx+ebx]	; ecx=tmp10
	sub	edx,ebx		; edx=tmp13
	lea	ebx,[eax+edi]	; ebx=tmp11
	sub	eax,edi		; eax=tmp12

	push	edx		; tmp13
	push	eax		; tmp12
	push	ebx		; tmp11
	push	ecx		; tmp10

	; -- Odd part

	mov	edx, POINTER [esp+16]	; quantptr

	movsx	eax, JCOEF [COL(1,esi,SIZEOF_JCOEF)]
	movsx	edi, JCOEF [COL(3,esi,SIZEOF_JCOEF)]
	imul	ax, ISLOW_MULT_TYPE [COL(1,edx,SIZEOF_ISLOW_MULT_TYPE)]
	imul	di, ISLOW_MULT_TYPE [COL(3,edx,SIZEOF_ISLOW_MULT_TYPE)]
	movsx	ecx, JCOEF [COL(5,esi,SIZEOF_JCOEF)]
	movsx	ebx, JCOEF [COL(7,esi,SIZEOF_JCOEF)]
	imul	cx, ISLOW_MULT_TYPE [COL(5,edx,SIZEOF_ISLOW_MULT_TYPE)]
	imul	bx, ISLOW_MULT_TYPE [COL(7,edx,SIZEOF_ISLOW_MULT_TYPE)]

	push	eax		; eax=tmp3
	push	edi		; edi=tmp2
	push	ecx		; ecx=tmp1
	push	ebx		; ebx=tmp0

	lea	esi,[ebx+edi]	; esi=z3
	lea	edx,[ecx+eax]	; edx=z4
	add	ebx,eax		; ebx=z1
	add	ecx,edi		; ecx=z2

	lea	eax,[esi+edx]
	imul	eax,(F_1_175)	; eax=z5

	imul	esi,(-F_1_961)	; esi=z3(=MULTIPLY(z3,-FIX_1_961570560))
	imul	edx,(-F_0_390)	; edx=z4(=MULTIPLY(z4,-FIX_0_390180644))
	imul	ebx,(-F_0_899)	; ebx=z1(=MULTIPLY(z1,-FIX_0_899976223))
	imul	ecx,(-F_2_562)	; ecx=z2(=MULTIPLY(z2,-FIX_2_562915447))

	add	esi,eax		; esi=z3(=z3+z5)
	add	edx,eax		; edx=z4(=z4+z5)

	lea	edi,[esi+ebx]	; edi=z1+z3
	lea	eax,[edx+ecx]	; eax=z2+z4
	add	esi,ecx		; esi=z2+z3
	add	edx,ebx		; edx=z1+z4

	pop	ecx		; ecx=tmp0
	pop	ebx		; ebx=tmp1
	imul	ecx,(F_0_298)	; ecx=tmp0(=MULTIPLY(tmp0,FIX_0_298631336))
	imul	ebx,(F_2_053)	; ebx=tmp1(=MULTIPLY(tmp1,FIX_2_053119869))
	add	edi,ecx		; edi=tmp0(=tmp0+z1+z3)
	add	eax,ebx		; eax=tmp1(=tmp1+z2+z4)

	pop	ecx		; ecx=tmp2
	pop	ebx		; ebx=tmp3
	imul	ecx,(F_3_072)	; ecx=tmp2(=MULTIPLY(tmp2,FIX_3_072711026))
	imul	ebx,(F_1_501)	; ebx=tmp3(=MULTIPLY(tmp3,FIX_1_501321110))
	add	esi,ecx		; esi=tmp2(=tmp2+z2+z3)
	add	edx,ebx		; edx=tmp3(=tmp3+z1+z4)

	; -- Final output stage

	pop	ecx		; ecx=tmp10
	pop	ebx		; ebx=tmp11
	push	eax		; tmp1
	push	edi		; tmp0

	lea	eax,[ecx+edx]	; eax=data0(=tmp10+tmp3)
	sub	ecx,edx		; ecx=data7(=tmp10-tmp3)
	lea	edx,[ebx+esi]	; edx=data1(=tmp11+tmp2)
	sub	ebx,esi		; ebx=data6(=tmp11-tmp2)

	mov	edi, POINTER [ptr]	; edi=wsptr

	descale	eax,(CONST_BITS-PASS1_BITS)
	descale	ecx,(CONST_BITS-PASS1_BITS)
	descale	edx,(CONST_BITS-PASS1_BITS)
	descale	ebx,(CONST_BITS-PASS1_BITS)

	mov	INT [COL(0,edi,SIZEOF_INT)], eax
	mov	INT [COL(7,edi,SIZEOF_INT)], ecx
	mov	INT [COL(1,edi,SIZEOF_INT)], edx
	mov	INT [COL(6,edi,SIZEOF_INT)], ebx

	pop	esi		; esi=tmp0
	pop	eax		; eax=tmp1
	pop	ecx		; ecx=tmp12
	pop	edx		; edx=tmp13

	lea	ebx,[ecx+eax]	; ebx=data2(=tmp12+tmp1)
	sub	ecx,eax		; ecx=data5(=tmp12-tmp1)
	lea	eax,[edx+esi]	; eax=data3(=tmp13+tmp0)
	sub	edx,esi		; edx=data4(=tmp13-tmp0)

	descale	ebx,(CONST_BITS-PASS1_BITS)
	descale	ecx,(CONST_BITS-PASS1_BITS)
	descale	eax,(CONST_BITS-PASS1_BITS)
	descale	edx,(CONST_BITS-PASS1_BITS)

	mov	INT [COL(2,edi,SIZEOF_INT)], ebx
	mov	INT [COL(5,edi,SIZEOF_INT)], ecx
	mov	INT [COL(3,edi,SIZEOF_INT)], eax
	mov	INT [COL(4,edi,SIZEOF_INT)], edx

	pop	edx	; quantptr
	pop	esi	; coef_block
	pop	ecx	; ctr

.nextcolumn:
	add	esi, byte SIZEOF_JCOEF	; advance pointers to next column
	add	edx, byte SIZEOF_ISLOW_MULT_TYPE
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

	lea	edx,[eax+ecx]
	sub	eax,ecx
	sal	edx,CONST_BITS	; edx=tmp0
	sal	eax,CONST_BITS	; eax=tmp1

	lea	ecx,[ebx+edi]
	imul	ecx,(F_0_541)	; ecx=z1
	imul	ebx,(F_0_765)	; ebx=MULTIPLY(z2,FIX_0_765366865)
	imul	edi,(-F_1_847)	; edi=MULTIPLY(z3,-FIX_1_847759065)
	add	ebx,ecx		; ebx=tmp3
	add	edi,ecx		; edi=tmp2

	lea	ecx,[edx+ebx]	; ecx=tmp10
	sub	edx,ebx		; edx=tmp13
	lea	ebx,[eax+edi]	; ebx=tmp11
	sub	eax,edi		; eax=tmp12

	push	edx		; tmp13
	push	eax		; tmp12
	push	ebx		; tmp11
	push	ecx		; tmp10

	; -- Odd part

	mov	eax, INT [ROW(1,esi,SIZEOF_INT)]
	mov	edi, INT [ROW(3,esi,SIZEOF_INT)]
	mov	ecx, INT [ROW(5,esi,SIZEOF_INT)]
	mov	ebx, INT [ROW(7,esi,SIZEOF_INT)]

	push	eax		; eax=tmp3
	push	edi		; edi=tmp2
	push	ecx		; ecx=tmp1
	push	ebx		; ebx=tmp0

	lea	esi,[ebx+edi]	; esi=z3
	lea	edx,[ecx+eax]	; edx=z4
	add	ebx,eax		; ebx=z1
	add	ecx,edi		; ecx=z2

	lea	eax,[esi+edx]
	imul	eax,(F_1_175)	; eax=z5

	imul	esi,(-F_1_961)	; esi=z3(=MULTIPLY(z3,-FIX_1_961570560))
	imul	edx,(-F_0_390)	; edx=z4(=MULTIPLY(z4,-FIX_0_390180644))
	imul	ebx,(-F_0_899)	; ebx=z1(=MULTIPLY(z1,-FIX_0_899976223))
	imul	ecx,(-F_2_562)	; ecx=z2(=MULTIPLY(z2,-FIX_2_562915447))

	add	esi,eax		; esi=z3(=z3+z5)
	add	edx,eax		; edx=z4(=z4+z5)

	lea	edi,[esi+ebx]	; edi=z1+z3
	lea	eax,[edx+ecx]	; eax=z2+z4
	add	esi,ecx		; esi=z2+z3
	add	edx,ebx		; edx=z1+z4

	pop	ecx		; ecx=tmp0
	pop	ebx		; ebx=tmp1
	imul	ecx,(F_0_298)	; ecx=tmp0(=MULTIPLY(tmp0,FIX_0_298631336))
	imul	ebx,(F_2_053)	; ebx=tmp1(=MULTIPLY(tmp1,FIX_2_053119869))
	add	edi,ecx		; edi=tmp0(=tmp0+z1+z3)
	add	eax,ebx		; eax=tmp1(=tmp1+z2+z4)

	pop	ecx		; ecx=tmp2
	pop	ebx		; ebx=tmp3
	imul	ecx,(F_3_072)	; ecx=tmp2(=MULTIPLY(tmp2,FIX_3_072711026))
	imul	ebx,(F_1_501)	; ebx=tmp3(=MULTIPLY(tmp3,FIX_1_501321110))
	add	esi,ecx		; esi=tmp2(=tmp2+z2+z3)
	add	edx,ebx		; edx=tmp3(=tmp3+z1+z4)

	; -- Final output stage

	pop	ecx		; ecx=tmp10
	pop	ebx		; ebx=tmp11
	push	eax		; tmp1
	push	edi		; tmp0

	lea	eax,[ecx+edx]	; eax=data0(=tmp10+tmp3)
	sub	ecx,edx		; ecx=data7(=tmp10-tmp3)
	lea	edx,[ebx+esi]	; edx=data1(=tmp11+tmp2)
	sub	ebx,esi		; ebx=data6(=tmp11-tmp2)

	mov	esi, POINTER [range_limit]	; (JSAMPLE *)

	descale	eax,(CONST_BITS+PASS1_BITS+3)
	descale	ecx,(CONST_BITS+PASS1_BITS+3)
	descale	edx,(CONST_BITS+PASS1_BITS+3)
	descale	ebx,(CONST_BITS+PASS1_BITS+3)

	mov	edi, POINTER [ptr]		; edi=outptr

	and	eax,RANGE_MASK
	and	ecx,RANGE_MASK
	and	edx,RANGE_MASK
	and	ebx,RANGE_MASK

	mov	al, JSAMPLE [esi+eax*SIZEOF_JSAMPLE]
	mov	cl, JSAMPLE [esi+ecx*SIZEOF_JSAMPLE]
	mov	dl, JSAMPLE [esi+edx*SIZEOF_JSAMPLE]
	mov	bl, JSAMPLE [esi+ebx*SIZEOF_JSAMPLE]

	mov	JSAMPLE [edi+0*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+7*SIZEOF_JSAMPLE], cl
	mov	JSAMPLE [edi+1*SIZEOF_JSAMPLE], dl
	mov	JSAMPLE [edi+6*SIZEOF_JSAMPLE], bl

	pop	esi		; esi=tmp0
	pop	eax		; eax=tmp1
	pop	ecx		; ecx=tmp12
	pop	edx		; edx=tmp13

	lea	ebx,[ecx+eax]	; ebx=data2(=tmp12+tmp1)
	sub	ecx,eax		; ecx=data5(=tmp12-tmp1)
	lea	eax,[edx+esi]	; eax=data3(=tmp13+tmp0)
	sub	edx,esi		; edx=data4(=tmp13-tmp0)

	mov	esi, POINTER [range_limit]	; (JSAMPLE *)

	descale	ebx,(CONST_BITS+PASS1_BITS+3)
	descale	ecx,(CONST_BITS+PASS1_BITS+3)
	descale	eax,(CONST_BITS+PASS1_BITS+3)
	descale	edx,(CONST_BITS+PASS1_BITS+3)

	and	ebx,RANGE_MASK
	and	ecx,RANGE_MASK
	and	eax,RANGE_MASK
	and	edx,RANGE_MASK

	mov	bl, JSAMPLE [esi+ebx*SIZEOF_JSAMPLE]
	mov	cl, JSAMPLE [esi+ecx*SIZEOF_JSAMPLE]
	mov	al, JSAMPLE [esi+eax*SIZEOF_JSAMPLE]
	mov	dl, JSAMPLE [esi+edx*SIZEOF_JSAMPLE]

	mov	JSAMPLE [edi+2*SIZEOF_JSAMPLE], bl
	mov	JSAMPLE [edi+5*SIZEOF_JSAMPLE], cl
	mov	JSAMPLE [edi+3*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+4*SIZEOF_JSAMPLE], dl

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

%endif ; DCT_ISLOW_SUPPORTED

;
; jidctred.asm - reduced-size IDCT (non-SIMD)
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
; This file contains inverse-DCT routines that produce reduced-size output:
; either 4x4, 2x2, or 1x1 pixels from an 8x8 DCT block.
; The following code is based directly on the IJG's original jidctred.c;
; see the jidctred.c for more details.
;
; Last Modified : October 17, 2004
;
; [TAB8]

%include "jsimdext.inc"
%include "jdct.inc"

%ifdef IDCT_SCALING_SUPPORTED

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
F_0_211	equ	 1730		; FIX(0.211164243)
F_0_509	equ	 4176		; FIX(0.509795579)
F_0_601	equ	 4926		; FIX(0.601344887)
F_0_720	equ	 5906		; FIX(0.720959822)
F_0_765	equ	 6270		; FIX(0.765366865)
F_0_850	equ	 6967		; FIX(0.850430095)
F_0_899	equ	 7373		; FIX(0.899976223)
F_1_061	equ	 8697		; FIX(1.061594337)
F_1_272	equ	10426		; FIX(1.272758580)
F_1_451	equ	11893		; FIX(1.451774981)
F_1_847	equ	15137		; FIX(1.847759065)
F_2_172	equ	17799		; FIX(2.172734803)
F_2_562	equ	20995		; FIX(2.562915447)
F_3_624	equ	29692		; FIX(3.624509785)
%else
; NASM cannot do compile-time arithmetic on floating-point constants.
%define DESCALE(x,n)  (((x)+(1<<((n)-1)))>>(n))
F_0_211	equ	DESCALE( 226735879,30-CONST_BITS)	; FIX(0.211164243)
F_0_509	equ	DESCALE( 547388834,30-CONST_BITS)	; FIX(0.509795579)
F_0_601	equ	DESCALE( 645689155,30-CONST_BITS)	; FIX(0.601344887)
F_0_720	equ	DESCALE( 774124714,30-CONST_BITS)	; FIX(0.720959822)
F_0_765	equ	DESCALE( 821806413,30-CONST_BITS)	; FIX(0.765366865)
F_0_850	equ	DESCALE( 913142361,30-CONST_BITS)	; FIX(0.850430095)
F_0_899	equ	DESCALE( 966342111,30-CONST_BITS)	; FIX(0.899976223)
F_1_061	equ	DESCALE(1139878239,30-CONST_BITS)	; FIX(1.061594337)
F_1_272	equ	DESCALE(1366614119,30-CONST_BITS)	; FIX(1.272758580)
F_1_451	equ	DESCALE(1558831516,30-CONST_BITS)	; FIX(1.451774981)
F_1_847	equ	DESCALE(1984016188,30-CONST_BITS)	; FIX(1.847759065)
F_2_172	equ	DESCALE(2332956230,30-CONST_BITS)	; FIX(2.172734803)
F_2_562	equ	DESCALE(2751909506,30-CONST_BITS)	; FIX(2.562915447)
F_3_624	equ	DESCALE(3891787747,30-CONST_BITS)	; FIX(3.624509785)
%endif

; --------------------------------------------------------------------------
	SECTION	SEG_TEXT
	BITS	32
;
; Perform dequantization and inverse DCT on one block of coefficients,
; producing a reduced-size 4x4 output block.
;
; GLOBAL(void)
; jpeg_idct_4x4 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
;                JCOEFPTR coef_block,
;                JSAMPARRAY output_buf, JDIMENSION output_col)
;

%define cinfo(b)	(b)+8		; j_decompress_ptr cinfo
%define compptr(b)	(b)+12		; jpeg_component_info * compptr
%define coef_block(b)	(b)+16		; JCOEFPTR coef_block
%define output_buf(b)	(b)+20		; JSAMPARRAY output_buf
%define output_col(b)	(b)+24		; JDIMENSION output_col

%define range_limit	ebp-SIZEOF_POINTER	; JSAMPLE * range_limit
%define workspace	range_limit-(DCTSIZE*4)*SIZEOF_INT
					; int workspace[DCTSIZE*4]

	align	16
	global	EXTN(jpeg_idct_4x4)

EXTN(jpeg_idct_4x4):
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
	; Don't bother to process column 4, because second pass won't use it
	cmp	ecx, byte DCTSIZE-4
	je	near .nextcolumn

	mov	ax, JCOEF [COL(1,esi,SIZEOF_JCOEF)]
	or	ax, JCOEF [COL(2,esi,SIZEOF_JCOEF)]
	jnz	short .columnDCT

	mov	ax, JCOEF [COL(3,esi,SIZEOF_JCOEF)]
	mov	bx, JCOEF [COL(5,esi,SIZEOF_JCOEF)]
	or	ax, JCOEF [COL(6,esi,SIZEOF_JCOEF)]
	or	bx, JCOEF [COL(7,esi,SIZEOF_JCOEF)]
	or	ax,bx
	jnz	short .columnDCT

	; -- AC terms all zero; we need not examine term 4 for 4x4 output

	mov	ax, JCOEF [COL(0,esi,SIZEOF_JCOEF)]
	imul	ax, ISLOW_MULT_TYPE [COL(0,edx,SIZEOF_ISLOW_MULT_TYPE)]
	cwde

	sal	eax, PASS1_BITS

	mov	INT [COL(0,edi,SIZEOF_INT)], eax
	mov	INT [COL(1,edi,SIZEOF_INT)], eax
	mov	INT [COL(2,edi,SIZEOF_INT)], eax
	mov	INT [COL(3,edi,SIZEOF_INT)], eax
	jmp	near .nextcolumn
	alignx	16,7

.columnDCT:
	push	ecx	; ctr
	push	esi	; coef_block
	push	edx	; quantptr
	push	edi	; wsptr

	; -- Even part

	movsx	ebx, JCOEF [COL(2,esi,SIZEOF_JCOEF)]
	movsx	ecx, JCOEF [COL(6,esi,SIZEOF_JCOEF)]
	movsx	eax, JCOEF [COL(0,esi,SIZEOF_JCOEF)]
	imul	bx, ISLOW_MULT_TYPE [COL(2,edx,SIZEOF_ISLOW_MULT_TYPE)]
	imul	cx, ISLOW_MULT_TYPE [COL(6,edx,SIZEOF_ISLOW_MULT_TYPE)]
	imul	ax, ISLOW_MULT_TYPE [COL(0,edx,SIZEOF_ISLOW_MULT_TYPE)]

	imul	ebx,(F_1_847)		; ebx=MULTIPLY(z2,FIX_1_847759065)
	imul	ecx,(-F_0_765)		; ecx=MULTIPLY(z3,-FIX_0_765366865)
	sal	eax,(CONST_BITS+1)	; eax=tmp0
	add	ecx,ebx			; ecx=tmp2

	lea	edi,[eax+ecx]		; edi=tmp10
	sub	eax,ecx			; eax=tmp12

	push	eax		; tmp12
	push	edi		; tmp10

	; -- Odd part

	movsx	edi, JCOEF [COL(7,esi,SIZEOF_JCOEF)]
	movsx	ecx, JCOEF [COL(5,esi,SIZEOF_JCOEF)]
	imul	di, ISLOW_MULT_TYPE [COL(7,edx,SIZEOF_ISLOW_MULT_TYPE)]
	imul	cx, ISLOW_MULT_TYPE [COL(5,edx,SIZEOF_ISLOW_MULT_TYPE)]
	movsx	ebx, JCOEF [COL(3,esi,SIZEOF_JCOEF)]
	movsx	eax, JCOEF [COL(1,esi,SIZEOF_JCOEF)]
	imul	bx, ISLOW_MULT_TYPE [COL(3,edx,SIZEOF_ISLOW_MULT_TYPE)]
	imul	ax, ISLOW_MULT_TYPE [COL(1,edx,SIZEOF_ISLOW_MULT_TYPE)]

	mov	esi,edi		; esi=edi=z1
	mov	edx,ecx		; edx=ecx=z2
	imul	edi,(-F_0_211)	; edi=MULTIPLY(z1,-FIX_0_211164243)
	imul	ecx,(F_1_451)	; ecx=MULTIPLY(z2,FIX_1_451774981)
	imul	esi,(-F_0_509)	; esi=MULTIPLY(z1,-FIX_0_509795579)
	imul	edx,(-F_0_601)	; edx=MULTIPLY(z2,-FIX_0_601344887)

	add	edi,ecx		; edi=(tmp0)
	add	esi,edx		; esi=(tmp2)

	mov	ecx,ebx		; ecx=ebx=z3
	mov	edx,eax		; edx=eax=z4
	imul	ebx,(-F_2_172)	; ebx=MULTIPLY(z3,-FIX_2_172734803)
	imul	eax,(F_1_061)	; eax=MULTIPLY(z4,FIX_1_061594337)
	imul	ecx,(F_0_899)	; ecx=MULTIPLY(z3,FIX_0_899976223)
	imul	edx,(F_2_562)	; edx=MULTIPLY(z4,FIX_2_562915447)

	add	edi,ebx
	add	esi,ecx
	add	edi,eax		; edi=tmp0
	add	esi,edx		; esi=tmp2

	; -- Final output stage

	pop	ebx		; ebx=tmp10
	pop	ecx		; ecx=tmp12

	lea	eax,[ebx+esi]	; eax=data0(=tmp10+tmp2)
	sub	ebx,esi		; ebx=data3(=tmp10-tmp2)
	lea	edx,[ecx+edi]	; edx=data1(=tmp12+tmp0)
	sub	ecx,edi		; ecx=data2(=tmp12-tmp0)

	pop	edi	; wsptr

	descale	eax,(CONST_BITS-PASS1_BITS+1)
	descale	ebx,(CONST_BITS-PASS1_BITS+1)
	descale	edx,(CONST_BITS-PASS1_BITS+1)
	descale	ecx,(CONST_BITS-PASS1_BITS+1)

	mov	INT [COL(0,edi,SIZEOF_INT)], eax
	mov	INT [COL(3,edi,SIZEOF_INT)], ebx
	mov	INT [COL(1,edi,SIZEOF_INT)], edx
	mov	INT [COL(2,edi,SIZEOF_INT)], ecx

	pop	edx	; quantptr
	pop	esi	; coef_block
	pop	ecx	; ctr

.nextcolumn:
	add	esi, byte SIZEOF_JCOEF	; advance pointers to next column
	add	edx, byte SIZEOF_ISLOW_MULT_TYPE
	add	edi, byte SIZEOF_INT
	dec	ecx
	jnz	near .columnloop

	; ---- Pass 2: process 4 rows from work array, store into output array.

	mov	eax, POINTER [cinfo(ebp)]
	mov	eax, POINTER [jdstruct_sample_range_limit(eax)]
	sub	eax, byte -CENTERJSAMPLE*SIZEOF_JSAMPLE	; JSAMPLE * range_limit
	mov	POINTER [range_limit], eax

	lea	esi, [workspace]			; int * wsptr
	mov	edi, JSAMPARRAY [output_buf(ebp)]	; (JSAMPROW *)
	mov	ecx, DCTSIZE/2				; ctr
	alignx	16,7
.rowloop:
	push	edi
	mov	edi, JSAMPROW [edi]			; (JSAMPLE *)
	add	edi, JDIMENSION [output_col(ebp)]	; edi=outptr

%ifndef NO_ZERO_ROW_TEST
	mov	eax, INT [ROW(1,esi,SIZEOF_INT)]
	or	eax, INT [ROW(2,esi,SIZEOF_INT)]
	jnz	short .rowDCT

	mov	eax, INT [ROW(3,esi,SIZEOF_INT)]
	mov	ebx, INT [ROW(5,esi,SIZEOF_INT)]
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
	jmp	near .nextrow
	alignx	16,7
%endif
.rowDCT:
	push	esi	; wsptr
	push	ecx	; ctr
	push	edi	; outptr

	; -- Even part

	mov	eax, INT [ROW(0,esi,SIZEOF_INT)]
	mov	ebx, INT [ROW(2,esi,SIZEOF_INT)]
	mov	ecx, INT [ROW(6,esi,SIZEOF_INT)]

	imul	ebx,(F_1_847)		; ebx=MULTIPLY(z2,FIX_1_847759065)
	imul	ecx,(-F_0_765)		; ecx=MULTIPLY(z3,-FIX_0_765366865)
	sal	eax,(CONST_BITS+1)	; eax=tmp0
	add	ecx,ebx			; ecx=tmp2

	lea	edi,[eax+ecx]		; edi=tmp10
	sub	eax,ecx			; eax=tmp12

	push	eax		; tmp12
	push	edi		; tmp10

	; -- Odd part

	mov	eax, INT [ROW(1,esi,SIZEOF_INT)]
	mov	ebx, INT [ROW(3,esi,SIZEOF_INT)]
	mov	ecx, INT [ROW(5,esi,SIZEOF_INT)]
	mov	edi, INT [ROW(7,esi,SIZEOF_INT)]

	mov	esi,edi		; esi=edi=z1
	mov	edx,ecx		; edx=ecx=z2
	imul	edi,(-F_0_211)	; edi=MULTIPLY(z1,-FIX_0_211164243)
	imul	ecx,(F_1_451)	; ecx=MULTIPLY(z2,FIX_1_451774981)
	imul	esi,(-F_0_509)	; esi=MULTIPLY(z1,-FIX_0_509795579)
	imul	edx,(-F_0_601)	; edx=MULTIPLY(z2,-FIX_0_601344887)

	add	edi,ecx		; edi=(tmp0)
	add	esi,edx		; esi=(tmp2)

	mov	ecx,ebx		; ecx=ebx=z3
	mov	edx,eax		; edx=eax=z4
	imul	ebx,(-F_2_172)	; ebx=MULTIPLY(z3,-FIX_2_172734803)
	imul	eax,(F_1_061)	; eax=MULTIPLY(z4,FIX_1_061594337)
	imul	ecx,(F_0_899)	; ecx=MULTIPLY(z3,FIX_0_899976223)
	imul	edx,(F_2_562)	; edx=MULTIPLY(z4,FIX_2_562915447)

	add	edi,ebx
	add	esi,ecx
	add	edi,eax		; edi=tmp0
	add	esi,edx		; esi=tmp2

	; -- Final output stage

	pop	ebx		; ebx=tmp10
	pop	ecx		; ecx=tmp12

	lea	eax,[ebx+esi]	; eax=data0(=tmp10+tmp2)
	sub	ebx,esi		; ebx=data3(=tmp10-tmp2)
	lea	edx,[ecx+edi]	; edx=data1(=tmp12+tmp0)
	sub	ecx,edi		; ecx=data2(=tmp12-tmp0)

	mov	esi, POINTER [range_limit]	; (JSAMPLE *)

	descale	eax,(CONST_BITS+PASS1_BITS+3+1)
	descale	ebx,(CONST_BITS+PASS1_BITS+3+1)
	descale	edx,(CONST_BITS+PASS1_BITS+3+1)
	descale	ecx,(CONST_BITS+PASS1_BITS+3+1)

	pop	edi	; outptr

	and	eax,RANGE_MASK
	and	ebx,RANGE_MASK
	and	edx,RANGE_MASK
	and	ecx,RANGE_MASK

	mov	al, JSAMPLE [esi+eax*SIZEOF_JSAMPLE]
	mov	bl, JSAMPLE [esi+ebx*SIZEOF_JSAMPLE]
	mov	dl, JSAMPLE [esi+edx*SIZEOF_JSAMPLE]
	mov	cl, JSAMPLE [esi+ecx*SIZEOF_JSAMPLE]

	mov	JSAMPLE [edi+0*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+3*SIZEOF_JSAMPLE], bl
	mov	JSAMPLE [edi+1*SIZEOF_JSAMPLE], dl
	mov	JSAMPLE [edi+2*SIZEOF_JSAMPLE], cl

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


; --------------------------------------------------------------------------
;
; Perform dequantization and inverse DCT on one block of coefficients,
; producing a reduced-size 2x2 output block.
;
; GLOBAL(void)
; jpeg_idct_2x2 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
;                JCOEFPTR coef_block,
;                JSAMPARRAY output_buf, JDIMENSION output_col)
;

%define cinfo(b)	(b)+8		; j_decompress_ptr cinfo
%define compptr(b)	(b)+12		; jpeg_component_info * compptr
%define coef_block(b)	(b)+16		; JCOEFPTR coef_block
%define output_buf(b)	(b)+20		; JSAMPARRAY output_buf
%define output_col(b)	(b)+24		; JDIMENSION output_col

%define range_limit	ebp-SIZEOF_POINTER	; JSAMPLE * range_limit
%define workspace	range_limit-(DCTSIZE*2)*SIZEOF_INT
					; int workspace[DCTSIZE*2]

	align	16
	global	EXTN(jpeg_idct_2x2)

EXTN(jpeg_idct_2x2):
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
	; Don't bother to process columns 2,4,6
	test	ecx, 0x09
	jz	near .nextcolumn

	mov	ax, JCOEF [COL(1,esi,SIZEOF_JCOEF)]
	or	ax, JCOEF [COL(3,esi,SIZEOF_JCOEF)]
	jnz	short .columnDCT

	mov	ax, JCOEF [COL(5,esi,SIZEOF_JCOEF)]
	or	ax, JCOEF [COL(7,esi,SIZEOF_JCOEF)]
	jnz	short .columnDCT

	; -- AC terms all zero; we need not examine terms 2,4,6 for 2x2 output

	mov	ax, JCOEF [COL(0,esi,SIZEOF_JCOEF)]
	imul	ax, ISLOW_MULT_TYPE [COL(0,edx,SIZEOF_ISLOW_MULT_TYPE)]
	cwde

	sal	eax, PASS1_BITS

	mov	INT [COL(0,edi,SIZEOF_INT)], eax
	mov	INT [COL(1,edi,SIZEOF_INT)], eax
	jmp	short .nextcolumn
	alignx	16,7

.columnDCT:
	push	ecx	; ctr
	push	edi	; wsptr

	; -- Odd part

	movsx	eax, JCOEF [COL(1,esi,SIZEOF_JCOEF)]
	movsx	ebx, JCOEF [COL(3,esi,SIZEOF_JCOEF)]
	imul	ax, ISLOW_MULT_TYPE [COL(1,edx,SIZEOF_ISLOW_MULT_TYPE)]
	imul	bx, ISLOW_MULT_TYPE [COL(3,edx,SIZEOF_ISLOW_MULT_TYPE)]
	movsx	ecx, JCOEF [COL(5,esi,SIZEOF_JCOEF)]
	movsx	edi, JCOEF [COL(7,esi,SIZEOF_JCOEF)]
	imul	cx, ISLOW_MULT_TYPE [COL(5,edx,SIZEOF_ISLOW_MULT_TYPE)]
	imul	di, ISLOW_MULT_TYPE [COL(7,edx,SIZEOF_ISLOW_MULT_TYPE)]

	imul	eax,(F_3_624)	; eax=MULTIPLY(data1,FIX_3_624509785)
	imul	ebx,(-F_1_272)	; ebx=MULTIPLY(data3,-FIX_1_272758580)
	imul	ecx,(F_0_850)	; ecx=MULTIPLY(data5,FIX_0_850430095)
	imul	edi,(-F_0_720)	; edi=MULTIPLY(data7,-FIX_0_720959822)

	add	eax,ebx
	add	ecx,edi
	add	ecx,eax		; ecx=tmp0

	; -- Even part

	mov	ax, JCOEF [COL(0,esi,SIZEOF_JCOEF)]
	imul	ax, ISLOW_MULT_TYPE [COL(0,edx,SIZEOF_ISLOW_MULT_TYPE)]
	cwde

	sal	eax,(CONST_BITS+2)	; eax=tmp10

	; -- Final output stage

	pop	edi	; wsptr

	lea	ebx,[eax+ecx]	; ebx=data0(=tmp10+tmp0)
	sub	eax,ecx		; eax=data1(=tmp10-tmp0)

	pop	ecx	; ctr

	descale	ebx,(CONST_BITS-PASS1_BITS+2)
	descale	eax,(CONST_BITS-PASS1_BITS+2)

	mov	INT [COL(0,edi,SIZEOF_INT)], ebx
	mov	INT [COL(1,edi,SIZEOF_INT)], eax

.nextcolumn:
	add	esi, byte SIZEOF_JCOEF	; advance pointers to next column
	add	edx, byte SIZEOF_ISLOW_MULT_TYPE
	add	edi, byte SIZEOF_INT
	dec	ecx
	jnz	near .columnloop

	; ---- Pass 2: process 2 rows from work array, store into output array.

	mov	eax, POINTER [cinfo(ebp)]
	mov	eax, POINTER [jdstruct_sample_range_limit(eax)]
	sub	eax, byte -CENTERJSAMPLE*SIZEOF_JSAMPLE	; JSAMPLE * range_limit
	mov	POINTER [range_limit], eax

	lea	esi, [workspace]			; int * wsptr
	mov	edi, JSAMPARRAY [output_buf(ebp)]	; (JSAMPROW *)
	mov	ecx, DCTSIZE/4				; ctr
	alignx	16,7
.rowloop:
	push	edi
	mov	edi, JSAMPROW [edi]			; (JSAMPLE *)
	add	edi, JDIMENSION [output_col(ebp)]	; edi=outptr

%ifndef NO_ZERO_ROW_TEST
	mov	eax, INT [ROW(1,esi,SIZEOF_INT)]
	or	eax, INT [ROW(3,esi,SIZEOF_INT)]
	jnz	short .rowDCT

	mov	eax, INT [ROW(5,esi,SIZEOF_INT)]
	or	eax, INT [ROW(7,esi,SIZEOF_INT)]
	jnz	short .rowDCT

	; -- AC terms all zero

	mov	eax, INT [ROW(0,esi,SIZEOF_INT)]

	mov	edx, POINTER [range_limit]	; (JSAMPLE *)

	descale	eax,(PASS1_BITS+3)
	and	eax,RANGE_MASK
	mov	al, JSAMPLE [edx+eax*SIZEOF_JSAMPLE]
	mov	JSAMPLE [edi+0*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+1*SIZEOF_JSAMPLE], al
	jmp	short .nextrow
	alignx	16,7
%endif
.rowDCT:
	push	ecx	; ctr

	; -- Odd part

	mov	eax, INT [ROW(1,esi,SIZEOF_INT)]
	mov	ebx, INT [ROW(3,esi,SIZEOF_INT)]
	mov	ecx, INT [ROW(5,esi,SIZEOF_INT)]
	mov	edx, INT [ROW(7,esi,SIZEOF_INT)]

	imul	eax,(F_3_624)	; eax=MULTIPLY(data1,FIX_3_624509785)
	imul	ebx,(-F_1_272)	; ebx=MULTIPLY(data3,-FIX_1_272758580)
	imul	ecx,(F_0_850)	; ecx=MULTIPLY(data5,FIX_0_850430095)
	imul	edx,(-F_0_720)	; edx=MULTIPLY(data7,-FIX_0_720959822)

	add	eax,ebx
	add	ecx,edx
	add	ecx,eax		; ecx=tmp0

	; -- Even part

	mov	eax, INT [ROW(0,esi,SIZEOF_INT)]

	sal	eax,(CONST_BITS+2)	; eax=tmp10

	; -- Final output stage

	mov	edx, POINTER [range_limit]	; (JSAMPLE *)

	lea	ebx,[eax+ecx]	; ebx=data0(=tmp10+tmp0)
	sub	eax,ecx		; eax=data1(=tmp10-tmp0)

	pop	ecx	; ctr

	descale	ebx,(CONST_BITS+PASS1_BITS+3+2)
	descale	eax,(CONST_BITS+PASS1_BITS+3+2)

	and	ebx,RANGE_MASK
	and	eax,RANGE_MASK
	mov	bl, JSAMPLE [edx+ebx*SIZEOF_JSAMPLE]
	mov	al, JSAMPLE [edx+eax*SIZEOF_JSAMPLE]
	mov	JSAMPLE [edi+0*SIZEOF_JSAMPLE], bl
	mov	JSAMPLE [edi+1*SIZEOF_JSAMPLE], al

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


; --------------------------------------------------------------------------
;
; Perform dequantization and inverse DCT on one block of coefficients,
; producing a reduced-size 1x1 output block.
;
; GLOBAL(void)
; jpeg_idct_1x1 (j_decompress_ptr cinfo, jpeg_component_info * compptr,
;                JCOEFPTR coef_block,
;                JSAMPARRAY output_buf, JDIMENSION output_col)
;

%define cinfo(b)	(b)+8		; j_decompress_ptr cinfo
%define compptr(b)	(b)+12		; jpeg_component_info * compptr
%define coef_block(b)	(b)+16		; JCOEFPTR coef_block
%define output_buf(b)	(b)+20		; JSAMPARRAY output_buf
%define output_col(b)	(b)+24		; JDIMENSION output_col

%define ebp		esp-4		; use esp instead of ebp

	align	16
	global	EXTN(jpeg_idct_1x1)

EXTN(jpeg_idct_1x1):
;	push	ebp
;	mov	ebp,esp
;	push	ebx		; unused
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
;	push	esi		; unused
;	push	edi		; unused

	; We hardly need an inverse DCT routine for this: just take the
	; average pixel value, which is one-eighth of the DC coefficient.

	mov	edx, POINTER [compptr(ebp)]
	mov	ecx, JCOEFPTR [coef_block(ebp)]		; inptr
	mov	edx, POINTER [jcompinfo_dct_table(edx)]	; quantptr

	mov	ax, JCOEF [COL(0,ecx,SIZEOF_JCOEF)]
	imul	ax, ISLOW_MULT_TYPE [COL(0,edx,SIZEOF_ISLOW_MULT_TYPE)]

	mov	ecx, JSAMPARRAY [output_buf(ebp)]	; (JSAMPROW *)
	mov	edx, JDIMENSION [output_col(ebp)]
	mov	ecx, JSAMPROW [ecx]			; (JSAMPLE *)

	add	ax, (1 << (3-1)) + (CENTERJSAMPLE << 3)
	sar	ax,3		; descale

	test	ah,ah		; unsigned saturation
	jz	short .output
	not	ax
	sar	ax,15
	alignx	16,3
.output:
	mov	JSAMPLE [ecx+edx*SIZEOF_JSAMPLE], al

;	pop	edi		; unused
;	pop	esi		; unused
;	pop	edx		; need not be preserved
;	pop	ecx		; need not be preserved
;	pop	ebx		; unused
;	pop	ebp
	ret

%endif ; IDCT_SCALING_SUPPORTED

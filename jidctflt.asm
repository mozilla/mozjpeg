;
; jidctflt.asm - floating-point IDCT (non-SIMD)
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
; This file contains a floating-point implementation of the inverse DCT
; (Discrete Cosine Transform). The following code is based directly on
; the IJG's original jidctflt.c; see the jidctflt.c for more details.
;
; Last Modified : October 17, 2004
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
	SECTION	SEG_CONST

%define ROTATOR_TYPE	FP32	; float

	alignz	16
	global	EXTN(jconst_idct_float)

EXTN(jconst_idct_float):

F_1_414	dd	1.414213562373095048801689	; 2*cos(PI*1/4)
F_1_847	dd	1.847759065022573512256366	; 2*cos(PI*1/8)
F_1_082	dd	1.082392200292393968799446	; 2*(cos(PI*1/8)-cos(PI*3/8))
F_2_613	dd	2.613125929752753055713286	; 2*(cos(PI*1/8)+cos(PI*3/8))

	alignz	16

; --------------------------------------------------------------------------
	SECTION	SEG_TEXT
	BITS	32
;
; Perform dequantization and inverse DCT on one block of coefficients.
;
; GLOBAL(void)
; jpeg_idct_float (j_decompress_ptr cinfo, jpeg_component_info * compptr,
;                  JCOEFPTR coef_block,
;                  JSAMPARRAY output_buf, JDIMENSION output_col)
;

%define cinfo(b)	(b)+8		; j_decompress_ptr cinfo
%define compptr(b)	(b)+12		; jpeg_component_info * compptr
%define coef_block(b)	(b)+16		; JCOEFPTR coef_block
%define output_buf(b)	(b)+20		; JSAMPARRAY output_buf
%define output_col(b)	(b)+24		; JDIMENSION output_col

%define tmp		ebp-SIZEOF_FP64	; double tmp
%define workspace	tmp-DCTSIZE2*SIZEOF_FAST_FLOAT
					; FAST_FLOAT workspace[DCTSIZE2]
%define rndint_magic	workspace-SIZEOF_FP32
					; float rndint_magic = 100663296.0F
%define gotptr		rndint_magic-SIZEOF_POINTER	; void * gotptr

	align	16
	global	EXTN(jpeg_idct_float)

EXTN(jpeg_idct_float):
	push	ebp
	mov	ebp,esp
	lea	esp, [workspace]
	push	FP32 0x4CC00000		; (float)(0x00C00000 << 3)
	pushpic	eax			; make a room for GOT address
	push	ebx
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
	push	esi
	push	edi

	get_GOT	ebx			; get GOT address
	movpic	POINTER [gotptr], ebx	; save GOT address

	; ---- Pass 1: process columns from input, store into work array.

	mov	edx, POINTER [compptr(ebp)]
	mov	edx, POINTER [jcompinfo_dct_table(edx)]	; quantptr
	mov	esi, JCOEFPTR [coef_block(ebp)]		; inptr
	lea	edi, [workspace]			; FAST_FLOAT * wsptr
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

	fild	JCOEF [COL(0,esi,SIZEOF_JCOEF)]
	fmul	FLOAT_MULT_TYPE [COL(0,edx,SIZEOF_FLOAT_MULT_TYPE)]

	fst	FAST_FLOAT [COL(0,edi,SIZEOF_FAST_FLOAT)]
	fst	FAST_FLOAT [COL(1,edi,SIZEOF_FAST_FLOAT)]
	fst	FAST_FLOAT [COL(2,edi,SIZEOF_FAST_FLOAT)]
	fst	FAST_FLOAT [COL(3,edi,SIZEOF_FAST_FLOAT)]
	fst	FAST_FLOAT [COL(4,edi,SIZEOF_FAST_FLOAT)]
	fst	FAST_FLOAT [COL(5,edi,SIZEOF_FAST_FLOAT)]
	fst	FAST_FLOAT [COL(6,edi,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(7,edi,SIZEOF_FAST_FLOAT)]
	jmp	near .nextcolumn
	alignx	16,7

.columnDCT:
	movpic	ebx, POINTER [gotptr]	; load GOT address

	; -- Even part

	fild	JCOEF [COL(2,esi,SIZEOF_JCOEF)]
	fild	JCOEF [COL(6,esi,SIZEOF_JCOEF)]
	fild	JCOEF [COL(4,esi,SIZEOF_JCOEF)]
	fild	JCOEF [COL(0,esi,SIZEOF_JCOEF)]

	fxch	st0,st3

	fmul	FLOAT_MULT_TYPE [COL(2,edx,SIZEOF_FLOAT_MULT_TYPE)]
	fxch	st0,st2
	fmul	FLOAT_MULT_TYPE [COL(6,edx,SIZEOF_FLOAT_MULT_TYPE)]
	fxch	st0,st1
	fmul	FLOAT_MULT_TYPE [COL(4,edx,SIZEOF_FLOAT_MULT_TYPE)]
	fxch	st0,st3
	fmul	FLOAT_MULT_TYPE [COL(0,edx,SIZEOF_FLOAT_MULT_TYPE)]
	fxch	st0,st1

	fld	st2	; st2 = st2 + st0, st0 = st2 - st0
	fsub	st0,st1
	fxch	st0,st1
	faddp	st3,st0

	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_1_414)]

	fld	st3	; st1 = st1 + st3, st3 = st1 - st3
	fsubr	st0,st2
	fxch	st0,st4
	faddp	st2,st0

	fsub	st0,st2

	fld	st1	; st2 = st1 + st2, st1 = st1 - st2
	fsub	st0,st3
	fxch	st0,st2
	faddp	st3,st0
	fld	st3	; st0 = st3 + st0, st3 = st3 - st0
	fsub	st0,st1
	fxch	st0,st4
	faddp	st1,st0

	; -- Odd part

	fild	JCOEF [COL(1,esi,SIZEOF_JCOEF)]
	fild	JCOEF [COL(7,esi,SIZEOF_JCOEF)]
	fild	JCOEF [COL(3,esi,SIZEOF_JCOEF)]
	fild	JCOEF [COL(5,esi,SIZEOF_JCOEF)]

	fxch	st0,st3

	fmul	FLOAT_MULT_TYPE [COL(1,edx,SIZEOF_FLOAT_MULT_TYPE)]
	fxch	st0,st2
	fmul	FLOAT_MULT_TYPE [COL(7,edx,SIZEOF_FLOAT_MULT_TYPE)]
	fxch	st0,st1
	fmul	FLOAT_MULT_TYPE [COL(3,edx,SIZEOF_FLOAT_MULT_TYPE)]
	fxch	st0,st6
	fxch	st3,st0
	fmul	FLOAT_MULT_TYPE [COL(5,edx,SIZEOF_FLOAT_MULT_TYPE)]
	fxch	st0,st5
	fstp	FP64 [tmp]

	fld	st1	; st1 = st1 + st0, st0 = st1 - st0
	fsub	st0,st1
	fxch	st0,st1
	faddp	st2,st0
	fld	st5	; st4 = st4 + st5, st5 = st4 - st5
	fsubr	st0,st5
	fxch	st0,st6
	faddp	st5,st0

	fld	st1	; st1 = st1 + st4, st4 = st1 - st4
	fsub	st0,st5
	fxch	st0,st5
	faddp	st2,st0

	fld	st5
	fadd	st0,st1
	fxch	st0,st5
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_1_414)]
	fxch	st0,st5
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_1_847)]
	fxch	st0,st6
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_2_613)]
	fxch	st0,st1
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_1_082)]
	fxch	st0,st6
	fsubr	st1,st0
	fsubp	st6,st0

	; -- Final output stage

	fsub	st0,st1
	fld	st2	; st1 = st2 + st1, st2 = st2 - st1
	fsub	st0,st2
	fxch	st0,st3
	faddp	st2,st0
	fsub	st4,st0
	fld	st3	; st0 = st3 + st0, st3 = st3 - st0
	fsub	st0,st1
	fxch	st0,st4
	faddp	st1,st0

	fxch	st0,st2

	fstp	FAST_FLOAT [COL(7,edi,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(0,edi,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(1,edi,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(6,edi,SIZEOF_FAST_FLOAT)]

	fadd	st1,st0
	fld	FP64 [tmp]
	fld	st1	; st3 = st3 + st1, st1 = st3 - st1
	fsubr	st0,st4
	fxch	st0,st2
	faddp	st4,st0
	fld	st0	; st0 = st0 + st2, st2 = st0 - st2
	fsub	st0,st3
	fxch	st0,st3
	faddp	st1,st0

	fxch	st0,st3

	fstp	FAST_FLOAT [COL(2,edi,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(5,edi,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(3,edi,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(4,edi,SIZEOF_FAST_FLOAT)]

.nextcolumn:
	add	esi, byte SIZEOF_JCOEF	; advance pointers to next column
	add	edx, byte SIZEOF_FLOAT_MULT_TYPE
	add	edi, byte SIZEOF_FAST_FLOAT
	dec	ecx
	jnz	near .columnloop

	; ---- Pass 2: process rows from work array, store into output array.

	mov	edx, POINTER [cinfo(ebp)]
	mov	edx, POINTER [jdstruct_sample_range_limit(edx)]
	sub	edx, byte -CENTERJSAMPLE*SIZEOF_JSAMPLE	; JSAMPLE * range_limit

	lea	esi, [workspace]			; FAST_FLOAT * wsptr
	mov	edi, JSAMPARRAY [output_buf(ebp)]	; (JSAMPROW *)
	mov	ecx, DCTSIZE				; ctr
	alignx	16,7
.rowloop:
	push	edi
	mov	edi, JSAMPROW [edi]			; (JSAMPLE *)
	add	edi, JDIMENSION [output_col(ebp)]	; edi=outptr

%ifndef NO_ZERO_ROW_TEST_FLOAT
	mov	eax, FAST_FLOAT [ROW(1,esi,SIZEOF_FAST_FLOAT)]
	add	eax,eax			; shl eax,1 (shift out the sign bit)
	jnz	short .rowDCT

	mov	eax, FAST_FLOAT [ROW(2,esi,SIZEOF_FAST_FLOAT)]
	mov	ebx, FAST_FLOAT [ROW(3,esi,SIZEOF_FAST_FLOAT)]
	or	eax, FAST_FLOAT [ROW(4,esi,SIZEOF_FAST_FLOAT)]
	or	ebx, FAST_FLOAT [ROW(5,esi,SIZEOF_FAST_FLOAT)]
	or	eax, FAST_FLOAT [ROW(6,esi,SIZEOF_FAST_FLOAT)]
	or	ebx, FAST_FLOAT [ROW(7,esi,SIZEOF_FAST_FLOAT)]
	or	eax,ebx
	add	eax,eax			; shl eax,1 (shift out the sign bit)
	jnz	short .rowDCT

	; -- AC terms all zero

	push	eax

	fld	FAST_FLOAT [ROW(0,esi,SIZEOF_FAST_FLOAT)]
	fadd	FP32 [rndint_magic]
	fstp	FP32 [esp]

	pop	eax
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
	movpic	ebx, POINTER [gotptr]	; load GOT address

	; -- Even part

	fld	FAST_FLOAT [ROW(4,esi,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [ROW(2,esi,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [ROW(0,esi,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [ROW(6,esi,SIZEOF_FAST_FLOAT)]

	fld	st2	; st2 = st2 + st0, st0 = st2 - st0
	fsub	st0,st1
	fxch	st0,st1
	faddp	st3,st0

	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_1_414)]

	fld	st3	; st1 = st1 + st3, st3 = st1 - st3
	fsubr	st0,st2
	fxch	st0,st4
	faddp	st2,st0

	fsub	st0,st2

	fld	st1	; st2 = st1 + st2, st1 = st1 - st2
	fsub	st0,st3
	fxch	st0,st2
	faddp	st3,st0
	fld	st3	; st0 = st3 + st0, st3 = st3 - st0
	fsub	st0,st1
	fxch	st0,st4
	faddp	st1,st0

	; -- Odd part

	fld	FAST_FLOAT [ROW(3,esi,SIZEOF_FAST_FLOAT)]
	fxch	st0,st3
	fld	FAST_FLOAT [ROW(1,esi,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [ROW(7,esi,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [ROW(5,esi,SIZEOF_FAST_FLOAT)]
	fxch	st0,st5
	fstp	FP64 [tmp]

	fld	st1	; st1 = st1 + st0, st0 = st1 - st0
	fsub	st0,st1
	fxch	st0,st1
	faddp	st2,st0
	fld	st5	; st4 = st4 + st5, st5 = st4 - st5
	fsubr	st0,st5
	fxch	st0,st6
	faddp	st5,st0

	fld	st1	; st1 = st1 + st4, st4 = st1 - st4
	fsub	st0,st5
	fxch	st0,st5
	faddp	st2,st0

	fld	st5
	fadd	st0,st1
	fxch	st0,st5
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_1_414)]
	fxch	st0,st5
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_1_847)]
	fxch	st0,st6
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_2_613)]
	fxch	st0,st1
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_1_082)]
	fxch	st0,st6
	fsubr	st1,st0
	fsubp	st6,st0

	; -- Final output stage

	sub	esp, byte DCTSIZE*SIZEOF_FP32

	fsub	st0,st1
	fld	st2	; st1 = st2 + st1, st2 = st2 - st1
	fsub	st0,st2
	fxch	st0,st3
	faddp	st2,st0
	fsub	st4,st0
	fld	st3	; st0 = st3 + st0, st3 = st3 - st0
	fsub	st0,st1
	fxch	st0,st4
	faddp	st1,st0

	fld	FP32 [rndint_magic]

	fadd	st4,st0
	fadd	st1,st0
	fadd	st2,st0
	fadd	st3,st0

	fxch	st0,st4

	fstp	FP32 [esp+6*SIZEOF_FP32]
	fstp	FP32 [esp+1*SIZEOF_FP32]
	fstp	FP32 [esp+0*SIZEOF_FP32]
	fstp	FP32 [esp+7*SIZEOF_FP32]

	fxch	st0,st1

	fadd	st2,st0
	fld	FP64 [tmp]
	fld	st1	; st4 = st4 + st1, st1 = st4 - st1
	fsubr	st0,st5
	fxch	st0,st2
	faddp	st5,st0
	fld	st0	; st0 = st0 + st3, st3 = st0 - st3
	fsub	st0,st4
	fxch	st0,st4
	faddp	st1,st0

	fxch	st0,st2

	fadd	st1,st0
	fadd	st2,st0
	fadd	st3,st0
	faddp	st4,st0

	fstp	FP32 [esp+5*SIZEOF_FP32]
	fstp	FP32 [esp+4*SIZEOF_FP32]
	fstp	FP32 [esp+3*SIZEOF_FP32]
	fstp	FP32 [esp+2*SIZEOF_FP32]

%assign i 0	; i=0;
%rep 4	; -- repeat 4 times ---
	pop	eax
	pop	ebx
	and	eax,RANGE_MASK
	and	ebx,RANGE_MASK
	mov	al, JSAMPLE [edx+eax*SIZEOF_JSAMPLE]
	mov	bl, JSAMPLE [edx+ebx*SIZEOF_JSAMPLE]
	mov	JSAMPLE [edi+(i+0)*SIZEOF_JSAMPLE], al
	mov	JSAMPLE [edi+(i+1)*SIZEOF_JSAMPLE], bl
%assign i i+2	; i+=2;
%endrep	; -- repeat end ---

.nextrow:
	pop	edi
	add	esi, byte DCTSIZE*SIZEOF_FAST_FLOAT
	add	edi, byte SIZEOF_JSAMPROW	; advance pointer to next row
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

%endif ; DCT_FLOAT_SUPPORTED

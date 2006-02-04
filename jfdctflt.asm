;
; jfdctflt.asm - floating-point FDCT (non-SIMD)
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
; This file contains a floating-point implementation of the forward DCT
; (Discrete Cosine Transform). The following code is based directly on
; the IJG's original jfdctflt.c; see the jfdctflt.c for more details.
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
	global	EXTN(jconst_fdct_float)

EXTN(jconst_fdct_float):

F_0_382	dd	0.382683432365089771728460	; cos(PI*3/8)
F_0_707	dd	0.707106781186547524400844	; cos(PI*1/4)
F_0_541	dd	0.541196100146196984399723	; cos(PI*1/8)-cos(PI*3/8)
F_1_306	dd	1.306562964876376527856643	; cos(PI*1/8)+cos(PI*3/8)

	alignz	16

; --------------------------------------------------------------------------
	SECTION	SEG_TEXT
	BITS	32
;
; Perform the forward DCT on one block of samples.
;
; GLOBAL(void)
; jpeg_fdct_float (FAST_FLOAT * data)
;

%define data(b)	(b)+8		; FAST_FLOAT * data

	align	16
	global	EXTN(jpeg_fdct_float)

EXTN(jpeg_fdct_float):
	push	ebp
	mov	ebp,esp
	pushpic	ebx
;	push	ecx		; need not be preserved
;	push	edx		; need not be preserved
;	push	esi		; unused
;	push	edi		; unused

	get_GOT	ebx		; get GOT address

	; ---- Pass 1: process rows.

	mov	edx, POINTER [data(ebp)]	; (FAST_FLOAT *)
	mov	ecx, DCTSIZE
	alignx	16,7
.rowloop:
	fld	FAST_FLOAT [ROW(1,edx,SIZEOF_FAST_FLOAT)]
	fadd	FAST_FLOAT [ROW(6,edx,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [ROW(0,edx,SIZEOF_FAST_FLOAT)]
	fadd	FAST_FLOAT [ROW(7,edx,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [ROW(3,edx,SIZEOF_FAST_FLOAT)]
	fadd	FAST_FLOAT [ROW(4,edx,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [ROW(2,edx,SIZEOF_FAST_FLOAT)]
	fadd	FAST_FLOAT [ROW(5,edx,SIZEOF_FAST_FLOAT)]

	; -- Even part

	fld	st2	; st2 = st2 + st1, st1 = st2 - st1
	fsub	st0,st2
	fxch	st0,st2
	faddp	st3,st0
	fld	st3	; st3 = st3 + st0, st0 = st3 - st0
	fsub	st0,st1
	fxch	st0,st1
	faddp	st4,st0

	fadd	st0,st1
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_0_707)]

	fld	st2	; st3 = st2 + st3, st2 = st2 - st3
	fsub	st0,st4
	fxch	st0,st3
	faddp	st4,st0
	fld	st1	; st0 = st1 + st0, st1 = st1 - st0
	fsub	st0,st1
	fxch	st0,st2
	faddp	st1,st0

	fld	FAST_FLOAT [ROW(0,edx,SIZEOF_FAST_FLOAT)]
	fsub	FAST_FLOAT [ROW(7,edx,SIZEOF_FAST_FLOAT)]
	fxch	st0,st4
	fld	FAST_FLOAT [ROW(3,edx,SIZEOF_FAST_FLOAT)]
	fsub	FAST_FLOAT [ROW(4,edx,SIZEOF_FAST_FLOAT)]
	fxch	st0,st4
	fld	FAST_FLOAT [ROW(1,edx,SIZEOF_FAST_FLOAT)]
	fsub	FAST_FLOAT [ROW(6,edx,SIZEOF_FAST_FLOAT)]
	fxch	st0,st4
	fld	FAST_FLOAT [ROW(2,edx,SIZEOF_FAST_FLOAT)]
	fsub	FAST_FLOAT [ROW(5,edx,SIZEOF_FAST_FLOAT)]
	fxch	st0,st4

	fstp	FAST_FLOAT [ROW(2,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [ROW(6,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [ROW(4,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [ROW(0,edx,SIZEOF_FAST_FLOAT)]

	; -- Odd part

	fadd	st2,st0
	fadd	st0,st1
	fxch	st0,st3
	fadd	st1,st0
	fxch	st0,st3

	fld	st2
	fxch	st0,st1
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_0_707)]
	fxch	st0,st1
	fsub	st0,st2
	fxch	st0,st3
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_0_541)]
	fxch	st0,st3
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_0_382)]
	fxch	st0,st2
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_1_306)]
	fxch	st0,st2
	fadd	st3,st0
	faddp	st2,st0

	fld	st3	; st3 = st3 + st0, st0 = st3 - st0
	fsub	st0,st1
	fxch	st0,st1
	faddp	st4,st0

	fld	st2	; st0 = st0 + st2, st2 = st0 - st2
	fsubr	st0,st1
	fxch	st0,st3
	faddp	st1,st0
	fld	st1	; st3 = st3 + st1, st1 = st3 - st1
	fsubr	st0,st4
	fxch	st0,st2
	faddp	st4,st0

	fstp	FAST_FLOAT [ROW(5,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [ROW(7,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [ROW(3,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [ROW(1,edx,SIZEOF_FAST_FLOAT)]

	add	edx, byte DCTSIZE*SIZEOF_FAST_FLOAT
	dec	ecx				; advance pointer to next row
	jnz	near .rowloop

	; ---- Pass 2: process columns.

	mov	edx, POINTER [data(ebp)]	; (FAST_FLOAT *)
	mov	ecx, DCTSIZE
	alignx	16,7
.columnloop:
	fld	FAST_FLOAT [COL(1,edx,SIZEOF_FAST_FLOAT)]
	fadd	FAST_FLOAT [COL(6,edx,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [COL(0,edx,SIZEOF_FAST_FLOAT)]
	fadd	FAST_FLOAT [COL(7,edx,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [COL(3,edx,SIZEOF_FAST_FLOAT)]
	fadd	FAST_FLOAT [COL(4,edx,SIZEOF_FAST_FLOAT)]
	fld	FAST_FLOAT [COL(2,edx,SIZEOF_FAST_FLOAT)]
	fadd	FAST_FLOAT [COL(5,edx,SIZEOF_FAST_FLOAT)]

	; -- Even part

	fld	st2	; st2 = st2 + st1, st1 = st2 - st1
	fsub	st0,st2
	fxch	st0,st2
	faddp	st3,st0
	fld	st3	; st3 = st3 + st0, st0 = st3 - st0
	fsub	st0,st1
	fxch	st0,st1
	faddp	st4,st0

	fadd	st0,st1
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_0_707)]

	fld	st2	; st3 = st2 + st3, st2 = st2 - st3
	fsub	st0,st4
	fxch	st0,st3
	faddp	st4,st0
	fld	st1	; st0 = st1 + st0, st1 = st1 - st0
	fsub	st0,st1
	fxch	st0,st2
	faddp	st1,st0

	fld	FAST_FLOAT [COL(0,edx,SIZEOF_FAST_FLOAT)]
	fsub	FAST_FLOAT [COL(7,edx,SIZEOF_FAST_FLOAT)]
	fxch	st0,st4
	fld	FAST_FLOAT [COL(3,edx,SIZEOF_FAST_FLOAT)]
	fsub	FAST_FLOAT [COL(4,edx,SIZEOF_FAST_FLOAT)]
	fxch	st0,st4
	fld	FAST_FLOAT [COL(1,edx,SIZEOF_FAST_FLOAT)]
	fsub	FAST_FLOAT [COL(6,edx,SIZEOF_FAST_FLOAT)]
	fxch	st0,st4
	fld	FAST_FLOAT [COL(2,edx,SIZEOF_FAST_FLOAT)]
	fsub	FAST_FLOAT [COL(5,edx,SIZEOF_FAST_FLOAT)]
	fxch	st0,st4

	fstp	FAST_FLOAT [COL(2,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(6,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(4,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(0,edx,SIZEOF_FAST_FLOAT)]

	; -- Odd part

	fadd	st2,st0
	fadd	st0,st1
	fxch	st0,st3
	fadd	st1,st0
	fxch	st0,st3

	fld	st2
	fxch	st0,st1
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_0_707)]
	fxch	st0,st1
	fsub	st0,st2
	fxch	st0,st3
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_0_541)]
	fxch	st0,st3
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_0_382)]
	fxch	st0,st2
	fmul	ROTATOR_TYPE [GOTOFF(ebx,F_1_306)]
	fxch	st0,st2
	fadd	st3,st0
	faddp	st2,st0

	fld	st3	; st3 = st3 + st0, st0 = st3 - st0
	fsub	st0,st1
	fxch	st0,st1
	faddp	st4,st0

	fld	st2	; st0 = st0 + st2, st2 = st0 - st2
	fsubr	st0,st1
	fxch	st0,st3
	faddp	st1,st0
	fld	st1	; st3 = st3 + st1, st1 = st3 - st1
	fsubr	st0,st4
	fxch	st0,st2
	faddp	st4,st0

	fstp	FAST_FLOAT [COL(5,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(7,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(3,edx,SIZEOF_FAST_FLOAT)]
	fstp	FAST_FLOAT [COL(1,edx,SIZEOF_FAST_FLOAT)]

	add	edx, byte SIZEOF_FAST_FLOAT ; advance pointer to next column
	dec	ecx
	jnz	near .columnloop

;	pop	edi		; unused
;	pop	esi		; unused
;	pop	edx		; need not be preserved
;	pop	ecx		; need not be preserved
	poppic	ebx
	pop	ebp
	ret

%endif ; DCT_FLOAT_SUPPORTED

;
; jfdctfst.asm - fast integer FDCT (64-bit SSE2)
;
; Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
; Copyright 2009, 2014, D. R. Commander
;
; Based on
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
; the forward DCT (Discrete Cosine Transform). The following code is
; based directly on the IJG's original jfdctfst.c; see the jfdctfst.c
; for more details.
;
; [TAB8]

%include "jsimdext.inc"
%include "jdct.inc"

; --------------------------------------------------------------------------

%define CONST_BITS      8       ; 14 is also OK.

%if CONST_BITS == 8
F_0_382 equ      98             ; FIX(0.382683433)
F_0_541 equ     139             ; FIX(0.541196100)
F_0_707 equ     181             ; FIX(0.707106781)
F_1_306 equ     334             ; FIX(1.306562965)
%else
; NASM cannot do compile-time arithmetic on floating-point constants.
%define DESCALE(x,n)  (((x)+(1<<((n)-1)))>>(n))
F_0_382 equ     DESCALE( 410903207,30-CONST_BITS)       ; FIX(0.382683433)
F_0_541 equ     DESCALE( 581104887,30-CONST_BITS)       ; FIX(0.541196100)
F_0_707 equ     DESCALE( 759250124,30-CONST_BITS)       ; FIX(0.707106781)
F_1_306 equ     DESCALE(1402911301,30-CONST_BITS)       ; FIX(1.306562965)
%endif

; --------------------------------------------------------------------------
        SECTION SEG_CONST

; PRE_MULTIPLY_SCALE_BITS <= 2 (to avoid overflow)
; CONST_BITS + CONST_SHIFT + PRE_MULTIPLY_SCALE_BITS == 16 (for pmulhw)

%define PRE_MULTIPLY_SCALE_BITS   2
%define CONST_SHIFT     (16 - PRE_MULTIPLY_SCALE_BITS - CONST_BITS)

        alignz  16
        global  EXTN(jconst_fdct_ifast_sse2)

EXTN(jconst_fdct_ifast_sse2):

PW_F0707        times 8 dw  F_0_707 << CONST_SHIFT
PW_F0382        times 8 dw  F_0_382 << CONST_SHIFT
PW_F0541        times 8 dw  F_0_541 << CONST_SHIFT
PW_F1306        times 8 dw  F_1_306 << CONST_SHIFT

        alignz  16

; --------------------------------------------------------------------------
        SECTION SEG_TEXT
        BITS    64
;
; Perform the forward DCT on one block of samples.
;
; GLOBAL(void)
; jsimd_fdct_ifast_sse2 (DCTELEM * data)
;

; r10 = DCTELEM * data

        align   16
        global  EXTN(jsimd_fdct_ifast_sse2)

EXTN(jsimd_fdct_ifast_sse2):
        collect_args

        ; ---- Pass 1: process rows.

        mov     rdx, r10        ; (DCTELEM *)

        movdqa  xmm8, XMMWORD [XMMBLOCK(0,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm9, XMMWORD [XMMBLOCK(1,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm2, XMMWORD [XMMBLOCK(2,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm11, XMMWORD [XMMBLOCK(3,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm4, XMMWORD [XMMBLOCK(4,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm13, XMMWORD [XMMBLOCK(5,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm5, XMMWORD [XMMBLOCK(6,0,rdx,SIZEOF_DCTELEM)]
        movdqa  xmm15, XMMWORD [XMMBLOCK(7,0,rdx,SIZEOF_DCTELEM)]

        ; xmm8=(00 01 02 03 04 05 06 07), xmm9=(10 11 12 13 14 15 16 17)
        ; xmm2=(20 21 22 23 24 25 26 27), xmm11=(30 31 32 33 34 35 36 37)
        ; xmm4=(40 41 42 43 44 45 46 47), xmm13=(50 51 52 53 54 55 56 57)
        ; xmm5=(60 61 62 63 64 65 66 67), xmm15=(70 71 72 73 74 75 76 77)

        movdqa    xmm12,xmm8            ; transpose coefficients(phase 1)
        punpcklwd xmm8,xmm9             ; xmm8=(00 10 01 11 02 12 03 13)
        punpckhwd xmm12,xmm9            ; xmm12=(04 14 05 15 06 16 07 17)
        movdqa    xmm1,xmm2             ; transpose coefficients(phase 1)
        punpcklwd xmm2,xmm11            ; xmm2=(20 30 21 31 22 32 23 33)
        punpckhwd xmm1,xmm11            ; xmm1=(24 34 25 35 26 36 27 37)

        movdqa    xmm0,xmm4             ; transpose coefficients(phase 1)
        punpcklwd xmm4,xmm13            ; xmm4=(40 50 41 51 42 52 43 53)
        punpckhwd xmm0,xmm13            ; xmm0=(44 54 45 55 46 56 47 57)
        movdqa    xmm3,xmm5             ; transpose coefficients(phase 1)
        punpcklwd xmm5,xmm15            ; xmm5=(60 70 61 71 62 72 63 73)
        punpckhwd xmm3,xmm15            ; xmm3=(64 74 65 75 66 76 67 77)

        movdqa    xmm10,xmm8            ; transpose coefficients(phase 2)
        punpckldq xmm8,xmm2             ; xmm8=(00 10 20 30 01 11 21 31)
        punpckhdq xmm10,xmm2            ; xmm10=(02 12 22 32 03 13 23 33)
        movdqa    xmm14,xmm12           ; transpose coefficients(phase 2)
        punpckldq xmm12,xmm1            ; xmm12=(04 14 24 34 05 15 25 35)
        punpckhdq xmm14,xmm1            ; xmm14=(06 16 26 36 07 17 27 37)

        movdqa    xmm6,xmm4             ; transpose coefficients(phase 2)
        punpckldq xmm4,xmm5             ; xmm4=(40 50 60 70 41 51 61 71)
        punpckhdq xmm6,xmm5             ; xmm6=(42 52 62 72 43 53 63 73)
        movdqa    xmm7,xmm0             ; transpose coefficients(phase 2)
        punpckldq xmm0,xmm3             ; xmm0=(44 54 64 74 45 55 65 75)
        punpckhdq xmm7,xmm3             ; xmm7=(46 56 66 76 47 57 67 77)

        movdqa     xmm9,xmm8            ; transpose coefficients(phase 3)
        punpcklqdq xmm8,xmm4            ; xmm8=(00 10 20 30 40 50 60 70)=data0
        punpckhqdq xmm9,xmm4            ; xmm9=(01 11 21 31 41 51 61 71)=data1
        movdqa     xmm11,xmm10          ; transpose coefficients(phase 3)
        punpcklqdq xmm10,xmm6           ; xmm10=(02 12 22 32 42 52 62 72)=data2
        punpckhqdq xmm11,xmm6           ; xmm11=(03 13 23 33 43 53 63 73)=data3

        movdqa     xmm13,xmm12          ; transpose coefficients(phase 3)
        punpcklqdq xmm12,xmm0           ; xmm12=(04 14 24 34 44 54 64 74)=data4
        punpckhqdq xmm13,xmm0           ; xmm13=(05 15 25 35 45 55 65 75)=data5
        movdqa     xmm15,xmm14          ; transpose coefficients(phase 3)
        punpcklqdq xmm14,xmm7           ; xmm14=(06 16 26 36 46 56 66 76)=data6
        punpckhqdq xmm15,xmm7           ; xmm15=(07 17 27 37 47 57 67 77)=data7

        movdqa  xmm0,xmm8
        paddw   xmm0,xmm15              ; xmm0=data0+data7=tmp0
        movdqa  xmm1,xmm9
        paddw   xmm1,xmm14              ; xmm1=data1+data6=tmp1
        movdqa  xmm2,xmm10
        paddw   xmm2,xmm13              ; xmm2=data2+data5=tmp2
        movdqa  xmm3,xmm11
        paddw   xmm3,xmm12              ; xmm3=data3+data4=tmp3

        psubw   xmm11,xmm12             ; xmm11=data3-data4=tmp4
        psubw   xmm10,xmm13             ; xmm10=data2-data5=tmp5
        psubw   xmm9,xmm14              ; xmm9=data1-data6=tmp6
        psubw   xmm8,xmm15              ; xmm8=data0-data7=tmp7

        ; -- Even part

        movdqa  xmm4,xmm0
        paddw   xmm4,xmm3               ; xmm4=tmp0+tmp3=tmp10
        movdqa  xmm5,xmm1
        paddw   xmm5,xmm2               ; xmm5=tmp1+tmp2=tmp11
        psubw   xmm1,xmm2               ; xmm1=tmp1-tmp2=tmp12
        psubw   xmm0,xmm3               ; xmm0=tmp0-tmp3=tmp13

        paddw   xmm1,xmm0               ; xmm1=tmp12+tmp13
        psllw   xmm1,PRE_MULTIPLY_SCALE_BITS
        pmulhw  xmm1,[rel PW_F0707]     ; xmm1=z1

        movdqa  xmm12,xmm4
        paddw   xmm12,xmm5              ; xmm12=tmp10+tmp11=out0
        movdqa  xmm14,xmm0
        paddw   xmm14,xmm1              ; xmm14=tmp13+z1=out2
        psubw   xmm4,xmm5               ; xmm4=tmp10-tmp11=out4
        psubw   xmm0,xmm1               ; xmm0=tmp13-z1=out6

        ; -- Odd part

        paddw   xmm11,xmm10             ; xmm11=tmp4+tmp5=tmp10
        paddw   xmm10,xmm9              ; xmm10=tmp5+tmp6=tmp11
        paddw   xmm9,xmm8               ; xmm9=tmp6+tmp7=tmp12

        psllw   xmm11,PRE_MULTIPLY_SCALE_BITS
        psllw   xmm10,PRE_MULTIPLY_SCALE_BITS
        psllw   xmm9,PRE_MULTIPLY_SCALE_BITS

        movdqa  xmm5,xmm11
        psubw   xmm5,xmm9               ; xmm5=tmp10-tmp12
        pmulhw  xmm5,[rel PW_F0382]     ; xmm5=z5

        pmulhw  xmm11,[rel PW_F0541]    ; xmm11=MULTIPLY(tmp10,FIX_0_541196)
        paddw   xmm11,xmm5              ;       +z5=z2

        pmulhw  xmm9,[rel PW_F1306]     ; xmm9=MULTIPLY(tmp12,FIX_1_306562)
        paddw   xmm9,xmm5               ;      +z5=z4

        pmulhw  xmm10,[rel PW_F0707]    ; xmm10=MULTIPLY(tmp11,FIX_1_306562)=z3

        movdqa  xmm1,xmm8
        paddw   xmm1,xmm10              ; xmm1=tmp7+z3=z11
        psubw   xmm8,xmm10              ; xmm8=tmp7-z3=z13

        movdqa  xmm13,xmm8
        paddw   xmm13,xmm11             ; xmm13=z13+z2=out5
        movdqa  xmm15,xmm1
        psubw   xmm15,xmm9              ; xmm15=z11-z4=out7
        paddw   xmm9,xmm1               ; xmm9=z11+z4=out1
        psubw   xmm8,xmm11              ; xmm8=z13-z2=out3

        ; ---- Pass 2: process columns.

        ; Re-order registers so we can reuse the same transpose code
        movdqa    xmm11,xmm8
        movdqa    xmm8,xmm12
        movdqa    xmm2,xmm14
        movdqa    xmm5,xmm0

        ; xmm8=(00 10 20 30 40 50 60 70), xmm9=(01 11 21 31 41 51 61 71)
        ; xmm2=(02 12 22 32 42 52 62 72), xmm11=(03 13 23 33 43 53 63 73)
        ; xmm4=(04 14 24 34 44 54 64 74), xmm13=(05 15 25 35 45 55 65 75)
        ; xmm5=(06 16 26 36 46 56 66 76), xmm15=(07 17 27 37 47 57 67 77)

        movdqa    xmm12,xmm8            ; transpose coefficients(phase 1)
        punpcklwd xmm8,xmm9             ; xmm8=(00 01 10 11 20 21 30 31)
        punpckhwd xmm12,xmm9            ; xmm12=(40 41 50 51 60 61 70 71)
        movdqa    xmm1,xmm2             ; transpose coefficients(phase 1)
        punpcklwd xmm2,xmm11            ; xmm2=(02 03 12 13 22 23 32 33)
        punpckhwd xmm1,xmm11            ; xmm1=(42 43 52 53 62 63 72 73)

        movdqa    xmm0,xmm4             ; transpose coefficients(phase 1)
        punpcklwd xmm4,xmm13            ; xmm4=(04 05 14 15 24 25 34 35)
        punpckhwd xmm0,xmm13            ; xmm0=(44 45 54 55 64 65 74 75)
        movdqa    xmm3,xmm5             ; transpose coefficients(phase 1)
        punpcklwd xmm5,xmm15            ; xmm5=(06 07 16 17 26 27 36 37)
        punpckhwd xmm3,xmm15            ; xmm3=(46 47 56 57 66 67 76 77)

        movdqa    xmm10,xmm8            ; transpose coefficients(phase 2)
        punpckldq xmm8,xmm2             ; xmm8=(00 01 02 03 10 11 12 13)
        punpckhdq xmm10,xmm2            ; xmm10=(20 21 22 23 30 31 32 33)
        movdqa    xmm14,xmm12           ; transpose coefficients(phase 2)
        punpckldq xmm12,xmm1            ; xmm12=(40 41 42 43 50 51 52 53)
        punpckhdq xmm14,xmm1            ; xmm14=(60 61 62 63 70 71 72 73)

        movdqa    xmm6,xmm4             ; transpose coefficients(phase 2)
        punpckldq xmm4,xmm5             ; xmm4=(04 05 06 07 14 15 16 17)
        punpckhdq xmm6,xmm5             ; xmm6=(24 25 26 27 34 35 36 37)
        movdqa    xmm7,xmm0             ; transpose coefficients(phase 2)
        punpckldq xmm0,xmm3             ; xmm0=(44 45 46 47 54 55 56 57)
        punpckhdq xmm7,xmm3             ; xmm7=(64 65 66 67 74 75 76 77)

        movdqa     xmm9,xmm8            ; transpose coefficients(phase 3)
        punpcklqdq xmm8,xmm4            ; xmm8=(00 01 02 03 04 05 06 07)=data0
        punpckhqdq xmm9,xmm4            ; xmm9=(10 11 12 13 14 15 16 17)=data1
        movdqa     xmm11,xmm10          ; transpose coefficients(phase 3)
        punpcklqdq xmm10,xmm6           ; xmm10=(20 21 22 23 24 25 26 27)=data2
        punpckhqdq xmm11,xmm6           ; xmm11=(30 31 32 33 34 35 36 37)=data3

        movdqa     xmm13,xmm12          ; transpose coefficients(phase 3)
        punpcklqdq xmm12,xmm0           ; xmm12=(40 41 42 43 44 45 46 47)=data4
        punpckhqdq xmm13,xmm0           ; xmm13=(50 51 52 53 54 55 56 57)=data5
        movdqa     xmm15,xmm14          ; transpose coefficients(phase 3)
        punpcklqdq xmm14,xmm7           ; xmm14=(60 61 62 63 64 65 66 67)=data6
        punpckhqdq xmm15,xmm7           ; xmm15=(70 71 72 73 74 75 76 77)=data7

        movdqa  xmm0,xmm8
        paddw   xmm0,xmm15              ; xmm0=data0+data7=tmp0
        movdqa  xmm1,xmm9
        paddw   xmm1,xmm14              ; xmm1=data1+data6=tmp1
        movdqa  xmm2,xmm10
        paddw   xmm2,xmm13              ; xmm2=data2+data5=tmp2
        movdqa  xmm3,xmm11
        paddw   xmm3,xmm12              ; xmm3=data3+data4=tmp3

        psubw   xmm11,xmm12             ; xmm11=data3-data4=tmp4
        psubw   xmm10,xmm13             ; xmm10=data2-data5=tmp5
        psubw   xmm9,xmm14              ; xmm9=data1-data6=tmp6
        psubw   xmm8,xmm15              ; xmm8=data0-data7=tmp7

        ; -- Even part

        movdqa  xmm4,xmm0
        paddw   xmm4,xmm3               ; xmm4=tmp0+tmp3=tmp10
        movdqa  xmm5,xmm1
        paddw   xmm5,xmm2               ; xmm5=tmp1+tmp2=tmp11
        psubw   xmm1,xmm2               ; xmm1=tmp1-tmp2=tmp12
        psubw   xmm0,xmm3               ; xmm0=tmp0-tmp3=tmp13

        paddw   xmm1,xmm0               ; xmm1=tmp12+tmp13
        psllw   xmm1,PRE_MULTIPLY_SCALE_BITS
        pmulhw  xmm1,[rel PW_F0707]     ; xmm1=z1

        movdqa  xmm12,xmm4
        paddw   xmm12,xmm5              ; xmm12=tmp10+tmp11=out0
        movdqa  xmm14,xmm0
        paddw   xmm14,xmm1              ; xmm14=tmp13+z1=out2
        psubw   xmm4,xmm5               ; xmm4=tmp10-tmp11=out4
        psubw   xmm0,xmm1               ; xmm0=tmp13-z1=out6

        ; -- Odd part

        paddw   xmm11,xmm10             ; xmm11=tmp4+tmp5=tmp10
        paddw   xmm10,xmm9              ; xmm10=tmp5+tmp6=tmp11
        paddw   xmm9,xmm8               ; xmm9=tmp6+tmp7=tmp12

        psllw   xmm11,PRE_MULTIPLY_SCALE_BITS
        psllw   xmm10,PRE_MULTIPLY_SCALE_BITS
        psllw   xmm9,PRE_MULTIPLY_SCALE_BITS

        movdqa  xmm5,xmm11
        psubw   xmm5,xmm9               ; xmm5=tmp10-tmp12
        pmulhw  xmm5,[rel PW_F0382]     ; xmm5=z5

        pmulhw  xmm11,[rel PW_F0541]    ; xmm11=MULTIPLY(tmp10,FIX_0_541196)
        paddw   xmm11,xmm5              ;       +z5=z2

        pmulhw  xmm9,[rel PW_F1306]     ; xmm9=MULTIPLY(tmp12,FIX_1_306562)
        paddw   xmm9,xmm5               ;      +z5=z4

        pmulhw  xmm10,[rel PW_F0707]    ; xmm10=MULTIPLY(tmp11,FIX_1_306562)=z3

        movdqa  xmm1,xmm8
        paddw   xmm1,xmm10              ; xmm1=tmp7+z3=z11
        psubw   xmm8,xmm10              ; xmm8=tmp7-z3=z13

        movdqa  xmm13,xmm8
        paddw   xmm13,xmm11             ; xmm13=z13+z2=out5
        movdqa  xmm15,xmm1
        psubw   xmm15,xmm9              ; xmm15=z11-z4=out7
        paddw   xmm9,xmm1               ; xmm9=z11+z4=out1
        psubw   xmm8,xmm11              ; xmm8=z13-z2=out3

        ; -- Write result

        movdqa  XMMWORD [XMMBLOCK(0,0,rdx,SIZEOF_DCTELEM)], xmm12
        movdqa  XMMWORD [XMMBLOCK(1,0,rdx,SIZEOF_DCTELEM)], xmm9
        movdqa  XMMWORD [XMMBLOCK(2,0,rdx,SIZEOF_DCTELEM)], xmm14
        movdqa  XMMWORD [XMMBLOCK(3,0,rdx,SIZEOF_DCTELEM)], xmm8
        movdqa  XMMWORD [XMMBLOCK(4,0,rdx,SIZEOF_DCTELEM)], xmm4
        movdqa  XMMWORD [XMMBLOCK(5,0,rdx,SIZEOF_DCTELEM)], xmm13
        movdqa  XMMWORD [XMMBLOCK(6,0,rdx,SIZEOF_DCTELEM)], xmm0
        movdqa  XMMWORD [XMMBLOCK(7,0,rdx,SIZEOF_DCTELEM)], xmm15

        uncollect_args
        ret

; For some reason, the OS X linker does not honor the request to align the
; segment unless we do this.
        align   16

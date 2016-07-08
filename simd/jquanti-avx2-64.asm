;
; jquanti.asm - sample data conversion and quantization (64-bit AVX2)
;
; Copyright 2009 Pierre Ossman <ossman@cendio.se> for Cendio AB
; Copyright (C) 2009, 2016, D. R. Commander.
; Copyright (C) 2016, Matthieu Darbois.
;
; Based on the x86 SIMD extension for IJG JPEG library
; Copyright (C) 1999-2006, MIYASAKA Masaru.
; For conditions of distribution and use, see copyright notice in jsimdext.inc
;
; This file should be assembled with NASM (Netwide Assembler),
; can *not* be assembled with Microsoft's MASM or any compatible
; assembler (including Borland's Turbo Assembler).
; NASM is available from http://nasm.sourceforge.net/ or
; http://sourceforge.net/project/showfiles.php?group_id=6208
;
; [TAB8]

%include "jsimdext.inc"
%include "jdct.inc"

; --------------------------------------------------------------------------
    SECTION     SEG_TEXT
    BITS        64

; --------------------------------------------------------------------------
;
; Quantize/descale the coefficients, and store into coef_block
;
; This implementation is based on an algorithm described in
;   "How to optimize for the Pentium family of microprocessors"
;   (http://www.agner.org/assem/).
;
; GLOBAL(void)
; jsimd_quantize_avx2 (JCOEFPTR coef_block, DCTELEM *divisors,
;                      DCTELEM *workspace);
;

%define RECIPROCAL(m,n,b)  XMMBLOCK(DCTSIZE*0+(m),(n),(b),SIZEOF_DCTELEM)
%define CORRECTION(m,n,b)  XMMBLOCK(DCTSIZE*1+(m),(n),(b),SIZEOF_DCTELEM)
%define SCALE(m,n,b)       XMMBLOCK(DCTSIZE*2+(m),(n),(b),SIZEOF_DCTELEM)

; r10 = JCOEFPTR coef_block
; r11 = DCTELEM *divisors
; r12 = DCTELEM *workspace

    align       32
    global      EXTN(jsimd_quantize_avx2)

EXTN(jsimd_quantize_avx2):
    push        rbp
    mov         rax, rsp
    mov         rbp, rsp
    collect_args 3

    vmovdqu     ymm4, [XMMBLOCK(0,0,r12,SIZEOF_DCTELEM)]
    vmovdqu     ymm5, [XMMBLOCK(2,0,r12,SIZEOF_DCTELEM)]
    vmovdqu     ymm6, [XMMBLOCK(4,0,r12,SIZEOF_DCTELEM)]
    vmovdqu     ymm7, [XMMBLOCK(6,0,r12,SIZEOF_DCTELEM)]
    vpabsw      ymm0, ymm4
    vpabsw      ymm1, ymm5
    vpabsw      ymm2, ymm6
    vpabsw      ymm3, ymm7

    vpaddw      ymm0, YMMWORD [CORRECTION(0,0,r11)]  ; correction + roundfactor
    vpaddw      ymm1, YMMWORD [CORRECTION(2,0,r11)]
    vpaddw      ymm2, YMMWORD [CORRECTION(4,0,r11)]
    vpaddw      ymm3, YMMWORD [CORRECTION(6,0,r11)]
    vpmulhuw    ymm0, YMMWORD [RECIPROCAL(0,0,r11)]  ; reciprocal
    vpmulhuw    ymm1, YMMWORD [RECIPROCAL(2,0,r11)]
    vpmulhuw    ymm2, YMMWORD [RECIPROCAL(4,0,r11)]
    vpmulhuw    ymm3, YMMWORD [RECIPROCAL(6,0,r11)]
    vpmulhuw    ymm0, YMMWORD [SCALE(0,0,r11)]       ; scale
    vpmulhuw    ymm1, YMMWORD [SCALE(2,0,r11)]
    vpmulhuw    ymm2, YMMWORD [SCALE(4,0,r11)]
    vpmulhuw    ymm3, YMMWORD [SCALE(6,0,r11)]

    vpsignw     ymm0, ymm0, ymm4
    vpsignw     ymm1, ymm1, ymm5
    vpsignw     ymm2, ymm2, ymm6
    vpsignw     ymm3, ymm3, ymm7

    vmovdqu     [XMMBLOCK(0,0,r10,SIZEOF_DCTELEM)], ymm0
    vmovdqu     [XMMBLOCK(2,0,r10,SIZEOF_DCTELEM)], ymm1
    vmovdqu     [XMMBLOCK(4,0,r10,SIZEOF_DCTELEM)], ymm2
    vmovdqu     [XMMBLOCK(6,0,r10,SIZEOF_DCTELEM)], ymm3

    vzeroupper
    uncollect_args 3
    pop         rbp
    ret

; For some reason, the OS X linker does not honor the request to align the
; segment unless we do this.
    align       32

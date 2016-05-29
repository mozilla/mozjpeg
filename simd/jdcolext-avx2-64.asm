;
; jdcolext.asm - colorspace conversion (64-bit AVX2)
;
; Copyright 2009, 2012 Pierre Ossman <ossman@cendio.se> for Cendio AB
; Copyright (C) 2009, 2012, 2016, D. R. Commander.
; Copyright (C) 2015, Intel Corporation.
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

%include "jcolsamp.inc"

; --------------------------------------------------------------------------
;
; Convert some rows of samples to the output colorspace.
;
; GLOBAL(void)
; jsimd_ycc_rgb_convert_avx2 (JDIMENSION out_width,
;                             JSAMPIMAGE input_buf, JDIMENSION input_row,
;                             JSAMPARRAY output_buf, int num_rows)
;

; r10d = JDIMENSION out_width
; r11 = JSAMPIMAGE input_buf
; r12d = JDIMENSION input_row
; r13 = JSAMPARRAY output_buf
; r14d = int num_rows

%define wk(i)   rbp-(WK_NUM-(i))*SIZEOF_YMMWORD  ; ymmword wk[WK_NUM]
%define WK_NUM  2

    align       32
    global      EXTN(jsimd_ycc_rgb_convert_avx2)

EXTN(jsimd_ycc_rgb_convert_avx2):
    push        rbp
    mov         rax, rsp                     ; rax = original rbp
    sub         rsp, byte 4
    and         rsp, byte (-SIZEOF_YMMWORD)  ; align to 256 bits
    mov         [rsp], rax
    mov         rbp, rsp                     ; rbp = aligned rbp
    lea         rsp, [wk(0)]
    collect_args 5
    push        rbx

    mov         ecx, r10d               ; num_cols
    test        rcx, rcx
    jz          near .return

    push        rcx

    mov         rdi, r11
    mov         ecx, r12d
    mov         rsi, JSAMPARRAY [rdi+0*SIZEOF_JSAMPARRAY]
    mov         rbx, JSAMPARRAY [rdi+1*SIZEOF_JSAMPARRAY]
    mov         rdx, JSAMPARRAY [rdi+2*SIZEOF_JSAMPARRAY]
    lea         rsi, [rsi+rcx*SIZEOF_JSAMPROW]
    lea         rbx, [rbx+rcx*SIZEOF_JSAMPROW]
    lea         rdx, [rdx+rcx*SIZEOF_JSAMPROW]

    pop         rcx

    mov         rdi, r13
    mov         eax, r14d
    test        rax, rax
    jle         near .return
.rowloop:
    push        rax
    push        rdi
    push        rdx
    push        rbx
    push        rsi
    push        rcx                     ; col

    mov         rsi, JSAMPROW [rsi]     ; inptr0
    mov         rbx, JSAMPROW [rbx]     ; inptr1
    mov         rdx, JSAMPROW [rdx]     ; inptr2
    mov         rdi, JSAMPROW [rdi]     ; outptr
.columnloop:

    vmovdqu     ymm5, YMMWORD [rbx]
    vmovdqu     ymm1, YMMWORD [rdx]

    vpcmpeqw    ymm0, ymm0, ymm0
    vpcmpeqw    ymm7, ymm7, ymm7
    vpsrlw      ymm0, ymm0, BYTE_BIT
    vpsllw      ymm7, ymm7, 7

    vpand       ymm4, ymm0, ymm5
    vpsrlw      ymm5, ymm5, BYTE_BIT
    vpand       ymm0, ymm0, ymm1
    vpsrlw      ymm1, ymm1, BYTE_BIT

    vpaddw      ymm2, ymm4, ymm7
    vpaddw      ymm3, ymm5, ymm7
    vpaddw      ymm6, ymm0, ymm7
    vpaddw      ymm7, ymm1, ymm7

    vpaddw      ymm4, ymm2, ymm2
    vpaddw      ymm5, ymm3, ymm3
    vpaddw      ymm0, ymm6, ymm6
    vpaddw      ymm1, ymm7, ymm7

    vpmulhw     ymm4, ymm4, [rel PW_MF0228]
    vpmulhw     ymm5, ymm5, [rel PW_MF0228]
    vpmulhw     ymm0, ymm0, [rel PW_F0402]
    vpmulhw     ymm1, ymm1, [rel PW_F0402]

    vpaddw      ymm4, ymm4, [rel PW_ONE]
    vpaddw      ymm5, ymm5, [rel PW_ONE]
    vpsraw      ymm4, ymm4, 1
    vpsraw      ymm5, ymm5, 1
    vpaddw      ymm0, ymm0, [rel PW_ONE]
    vpaddw      ymm1, ymm1, [rel PW_ONE]
    vpsraw      ymm0, ymm0, 1
    vpsraw      ymm1, ymm1, 1

    vpaddw      ymm4, ymm4, ymm2
    vpaddw      ymm5, ymm5, ymm3
    vpaddw      ymm4, ymm4, ymm2
    vpaddw      ymm5, ymm5, ymm3
    vpaddw      ymm0, ymm0, ymm6
    vpaddw      ymm1, ymm1, ymm7

    vmovdqa     YMMWORD [wk(0)], ymm4
    vmovdqa     YMMWORD [wk(1)], ymm5

    vpunpckhwd  ymm4, ymm2, ymm6
    vpunpcklwd  ymm2, ymm2, ymm6
    vpmaddwd    ymm2, ymm2, [rel PW_MF0344_F0285]
    vpmaddwd    ymm4, ymm4, [rel PW_MF0344_F0285]
    vpunpckhwd  ymm5, ymm3, ymm7
    vpunpcklwd  ymm3, ymm3, ymm7
    vpmaddwd    ymm3, ymm3, [rel PW_MF0344_F0285]
    vpmaddwd    ymm5, ymm5, [rel PW_MF0344_F0285]

    vpaddd      ymm2, ymm2, [rel PD_ONEHALF]
    vpaddd      ymm4, ymm4, [rel PD_ONEHALF]
    vpsrad      ymm2, ymm2, SCALEBITS
    vpsrad      ymm4, ymm4, SCALEBITS
    vpaddd      ymm3, ymm3, [rel PD_ONEHALF]
    vpaddd      ymm5, ymm5, [rel PD_ONEHALF]
    vpsrad      ymm3, ymm3, SCALEBITS
    vpsrad      ymm5, ymm5, SCALEBITS

    vpackssdw   ymm2, ymm2, ymm4
    vpackssdw   ymm3, ymm3, ymm5
    vpsubw      ymm2, ymm2, ymm6
    vpsubw      ymm3, ymm3, ymm7

    vmovdqu     ymm5, YMMWORD [rsi]

    vpcmpeqw    ymm4, ymm4, ymm4
    vpsrlw      ymm4, ymm4, BYTE_BIT
    vpand       ymm4, ymm4, ymm5
    vpsrlw      ymm5, ymm5, BYTE_BIT

    vpaddw      ymm0, ymm0, ymm4
    vpaddw      ymm1, ymm1, ymm5
    vpackuswb   ymm0, ymm0, ymm0
    vpackuswb   ymm1, ymm1, ymm1

    vpaddw      ymm2, ymm2, ymm4
    vpaddw      ymm3, ymm3, ymm5
    vpackuswb   ymm2, ymm2, ymm2
    vpackuswb   ymm3, ymm3, ymm3

    vpaddw      ymm4, ymm4, YMMWORD [wk(0)]
    vpaddw      ymm5, ymm5, YMMWORD [wk(1)]
    vpackuswb   ymm4, ymm4, ymm4
    vpackuswb   ymm5, ymm5, ymm5

%if RGB_PIXELSIZE == 3  ; ---------------

    vpunpcklbw  ymmA, ymmA, ymmC
    vpunpcklbw  ymmE, ymmE, ymmB
    vpunpcklbw  ymmD, ymmD, ymmF

    vpsrldq     ymmH, ymmA, 2
    vpunpckhwd  ymmG, ymmA, ymmE
    vpunpcklwd  ymmA, ymmA, ymmE

    vpsrldq     ymmE, ymmE, 2

    vmovdqa     ymmC, ymmD
    vpsrldq     ymmB, ymmD, 2
    vpunpckhwd  ymmC, ymmD, ymmH
    vpunpcklwd  ymmD, ymmD, ymmH

    vpunpckhwd  ymmF, ymmE, ymmB
    vpunpcklwd  ymmE, ymmE, ymmB

    vpshufd     ymmH, ymmA, 0x4E
    vpunpckldq  ymmA, ymmA, ymmD
    vpunpckhdq  ymmD, ymmD, ymmE
    vpunpckldq  ymmE, ymmE, ymmH

    vpshufd     ymmH, ymmG, 0x4E
    vpunpckldq  ymmG, ymmG, ymmC
    vpunpckhdq  ymmC, ymmC, ymmF
    vpunpckldq  ymmF, ymmF, ymmH

    vpunpcklqdq ymmH, ymmA, ymmE
    vpunpcklqdq ymmG, ymmD, ymmG
    vpunpcklqdq ymmC, ymmF, ymmC

    vperm2i128  ymmA, ymmH, ymmG, 0x20
    vperm2i128  ymmD, ymmC, ymmH, 0x30
    vperm2i128  ymmF, ymmG, ymmC, 0x31

    cmp         rcx, byte SIZEOF_YMMWORD
    jb          short .column_st64

    test        rdi, SIZEOF_YMMWORD-1
    jnz         short .out1
    ; --(aligned)-------------------
    vmovntdq    YMMWORD [rdi+0*SIZEOF_YMMWORD], ymmA
    vmovntdq    YMMWORD [rdi+1*SIZEOF_YMMWORD], ymmD
    vmovntdq    YMMWORD [rdi+2*SIZEOF_YMMWORD], ymmF
    jmp         short .out0
.out1:  ; --(unaligned)-----------------
    vmovdqu     YMMWORD [rdi+0*SIZEOF_YMMWORD], ymmA
    vmovdqu     YMMWORD [rdi+1*SIZEOF_YMMWORD], ymmD
    vmovdqu     YMMWORD [rdi+2*SIZEOF_YMMWORD], ymmF
.out0:
    add         rdi, byte RGB_PIXELSIZE*SIZEOF_YMMWORD  ; outptr
    sub         rcx, byte SIZEOF_YMMWORD
    jz          near .nextrow

    add         rsi, byte SIZEOF_YMMWORD  ; inptr0
    add         rbx, byte SIZEOF_YMMWORD  ; inptr1
    add         rdx, byte SIZEOF_YMMWORD  ; inptr2
    jmp         near .columnloop

.column_st64:
    lea         rcx, [rcx+rcx*2]            ; imul ecx, RGB_PIXELSIZE
    cmp         rcx, byte 2*SIZEOF_YMMWORD
    jb          short .column_st32
    vmovdqu     YMMWORD [rdi+0*SIZEOF_YMMWORD], ymmA
    vmovdqu     YMMWORD [rdi+1*SIZEOF_YMMWORD], ymmD
    add         rdi, byte 2*SIZEOF_YMMWORD  ; outptr
    vmovdqa     ymmA, ymmF
    sub         rcx, byte 2*SIZEOF_YMMWORD
    jmp         short .column_st31
.column_st32:
    cmp         rcx, byte SIZEOF_YMMWORD
    jb          short .column_st31
    vmovdqu     YMMWORD [rdi+0*SIZEOF_YMMWORD], ymmA
    add         rdi, byte SIZEOF_YMMWORD    ; outptr
    vmovdqa     ymmA, ymmD
    sub         rcx, byte SIZEOF_YMMWORD
    jmp         short .column_st31
.column_st31:
    cmp         rcx, byte SIZEOF_XMMWORD
    jb          short .column_st15
    vmovdqu     XMMWORD [rdi+0*SIZEOF_XMMWORD], xmmA
    add         rdi, byte SIZEOF_XMMWORD    ; outptr
    vperm2i128  ymmA, ymmA, ymmA, 1
    sub         rcx, byte SIZEOF_XMMWORD
.column_st15:
    ; Store the lower 8 bytes of xmmA to the output when it has enough
    ; space.
    cmp         rcx, byte SIZEOF_MMWORD
    jb          short .column_st7
    vmovq       XMM_MMWORD [rdi], xmmA
    add         rdi, byte SIZEOF_MMWORD
    sub         rcx, byte SIZEOF_MMWORD
    vpsrldq     xmmA, xmmA, SIZEOF_MMWORD
.column_st7:
    ; Store the lower 4 bytes of xmmA to the output when it has enough
    ; space.
    cmp         rcx, byte SIZEOF_DWORD
    jb          short .column_st3
    vmovd       XMM_DWORD [rdi], xmmA
    add         rdi, byte SIZEOF_DWORD
    sub         rcx, byte SIZEOF_DWORD
    vpsrldq     xmmA, xmmA, SIZEOF_DWORD
.column_st3:
    ; Store the lower 2 bytes of rax to the output when it has enough
    ; space.
    vmovd       eax, xmmA
    cmp         rcx, byte SIZEOF_WORD
    jb          short .column_st1
    mov         WORD [rdi], ax
    add         rdi, byte SIZEOF_WORD
    sub         rcx, byte SIZEOF_WORD
    shr         rax, 16
.column_st1:
    ; Store the lower 1 byte of rax to the output when it has enough
    ; space.
    test        rcx, rcx
    jz          short .nextrow
    mov         BYTE [rdi], al

%else  ; RGB_PIXELSIZE == 4 ; -----------

%ifdef RGBX_FILLER_0XFF
    vpcmpeqb    ymm6, ymm6, ymm6
    vpcmpeqb    ymm7, ymm7, ymm7
%else
    vpxor       ymm6, ymm6, ymm6
    vpxor       ymm7, ymm7, ymm7
%endif

    vpunpcklbw  ymmA, ymmA, ymmC
    vpunpcklbw  ymmE, ymmE, ymmG
    vpunpcklbw  ymmB, ymmB, ymmD
    vpunpcklbw  ymmF, ymmF, ymmH

    vpunpckhwd  ymmC, ymmA, ymmE
    vpunpcklwd  ymmA, ymmA, ymmE
    vpunpckhwd  ymmG, ymmB, ymmF
    vpunpcklwd  ymmB, ymmB, ymmF

    vpunpckhdq  ymmE, ymmA, ymmB
    vpunpckldq  ymmB, ymmA, ymmB
    vpunpckhdq  ymmF, ymmC, ymmG
    vpunpckldq  ymmG, ymmC, ymmG


    vperm2i128  ymmA, ymmB, ymmE, 0x20
    vperm2i128  ymmD, ymmG, ymmF, 0x20
    vperm2i128  ymmC, ymmB, ymmE, 0x31
    vperm2i128  ymmH, ymmG, ymmF, 0x31

    cmp         rcx, byte SIZEOF_YMMWORD
    jb          short .column_st64

    test        rdi, SIZEOF_YMMWORD-1
    jnz         short .out1
    ; --(aligned)-------------------
    vmovntdq    YMMWORD [rdi+0*SIZEOF_YMMWORD], ymmA
    vmovntdq    YMMWORD [rdi+1*SIZEOF_YMMWORD], ymmD
    vmovntdq    YMMWORD [rdi+2*SIZEOF_YMMWORD], ymmC
    vmovntdq    YMMWORD [rdi+3*SIZEOF_YMMWORD], ymmH
    jmp         short .out0
.out1:  ; --(unaligned)-----------------
    vmovdqu     YMMWORD [rdi+0*SIZEOF_YMMWORD], ymmA
    vmovdqu     YMMWORD [rdi+1*SIZEOF_YMMWORD], ymmD
    vmovdqu     YMMWORD [rdi+2*SIZEOF_YMMWORD], ymmC
    vmovdqu     YMMWORD [rdi+3*SIZEOF_YMMWORD], ymmH
.out0:
    add         rdi, RGB_PIXELSIZE*SIZEOF_YMMWORD  ; outptr
    sub         rcx, byte SIZEOF_YMMWORD
    jz          near .nextrow

    add         rsi, byte SIZEOF_YMMWORD  ; inptr0
    add         rbx, byte SIZEOF_YMMWORD  ; inptr1
    add         rdx, byte SIZEOF_YMMWORD  ; inptr2
    jmp         near .columnloop

.column_st64:
    cmp         rcx, byte SIZEOF_YMMWORD/2
    jb          short .column_st32
    vmovdqu     YMMWORD [rdi+0*SIZEOF_YMMWORD], ymmA
    vmovdqu     YMMWORD [rdi+1*SIZEOF_YMMWORD], ymmD
    add         rdi, byte 2*SIZEOF_YMMWORD  ; outptr
    vmovdqa     ymmA, ymmC
    vmovdqa     ymmD, ymmH
    sub         rcx, byte SIZEOF_YMMWORD/2
.column_st32:
    cmp         rcx, byte SIZEOF_YMMWORD/4
    jb          short .column_st16
    vmovdqu     YMMWORD [rdi+0*SIZEOF_YMMWORD], ymmA
    add         rdi, byte SIZEOF_YMMWORD    ; outptr
    vmovdqa     ymmA, ymmD
    sub         rcx, byte SIZEOF_YMMWORD/4
.column_st16:
    cmp         rcx, byte SIZEOF_YMMWORD/8
    jb          short .column_st15
    vmovdqu     XMMWORD [rdi+0*SIZEOF_XMMWORD], xmmA
    vperm2i128  ymmA, ymmA, ymmA, 1
    add         rdi, byte SIZEOF_XMMWORD    ; outptr
    sub         rcx, byte SIZEOF_YMMWORD/8
.column_st15:
    ; Store two pixels (8 bytes) of ymmA to the output when it has enough
    ; space.
    cmp         rcx, byte SIZEOF_YMMWORD/16
    jb          short .column_st7
    vmovq       MMWORD [rdi], xmmA
    add         rdi, byte SIZEOF_YMMWORD/16*4
    sub         rcx, byte SIZEOF_YMMWORD/16
    vpsrldq     xmmA, SIZEOF_YMMWORD/16*4
.column_st7:
    ; Store one pixel (4 bytes) of ymmA to the output when it has enough
    ; space.
    test        rcx, rcx
    jz          short .nextrow
    vmovd       XMM_DWORD [rdi], xmmA

%endif  ; RGB_PIXELSIZE ; ---------------

.nextrow:
    pop         rcx
    pop         rsi
    pop         rbx
    pop         rdx
    pop         rdi
    pop         rax

    add         rsi, byte SIZEOF_JSAMPROW
    add         rbx, byte SIZEOF_JSAMPROW
    add         rdx, byte SIZEOF_JSAMPROW
    add         rdi, byte SIZEOF_JSAMPROW  ; output_buf
    dec         rax                        ; num_rows
    jg          near .rowloop

    sfence                              ; flush the write buffer

.return:
    pop         rbx
    uncollect_args 5
    mov         rsp, rbp                ; rsp <- aligned rbp
    pop         rsp                     ; rsp <- original rbp
    pop         rbp
    ret

; For some reason, the OS X linker does not honor the request to align the
; segment unless we do this.
    align       32

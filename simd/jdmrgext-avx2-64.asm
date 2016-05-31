;
; jdmrgext.asm - merged upsampling/color conversion (64-bit AVX2)
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
; Upsample and color convert for the case of 2:1 horizontal and 1:1 vertical.
;
; GLOBAL(void)
; jsimd_h2v1_merged_upsample_avx2 (JDIMENSION output_width,
;                                  JSAMPIMAGE input_buf,
;                                  JDIMENSION in_row_group_ctr,
;                                  JSAMPARRAY output_buf);
;

; r10d = JDIMENSION output_width
; r11 = JSAMPIMAGE input_buf
; r12d = JDIMENSION in_row_group_ctr
; r13 = JSAMPARRAY output_buf

%define wk(i)   rbp-(WK_NUM-(i))*SIZEOF_YMMWORD  ; ymmword wk[WK_NUM]
%define WK_NUM  3

    align       32
    global      EXTN(jsimd_h2v1_merged_upsample_avx2)

EXTN(jsimd_h2v1_merged_upsample_avx2):
    push        rbp
    mov         rax, rsp                     ; rax = original rbp
    sub         rsp, byte 4
    and         rsp, byte (-SIZEOF_YMMWORD)  ; align to 256 bits
    mov         [rsp], rax
    mov         rbp, rsp                     ; rbp = aligned rbp
    lea         rsp, [wk(0)]
    collect_args 4
    push        rbx

    mov         ecx, r10d               ; col
    test        rcx, rcx
    jz          near .return

    push        rcx

    mov         rdi, r11
    mov         ecx, r12d
    mov         rsi, JSAMPARRAY [rdi+0*SIZEOF_JSAMPARRAY]
    mov         rbx, JSAMPARRAY [rdi+1*SIZEOF_JSAMPARRAY]
    mov         rdx, JSAMPARRAY [rdi+2*SIZEOF_JSAMPARRAY]
    mov         rdi, r13
    mov         rsi, JSAMPROW [rsi+rcx*SIZEOF_JSAMPROW]  ; inptr0
    mov         rbx, JSAMPROW [rbx+rcx*SIZEOF_JSAMPROW]  ; inptr1
    mov         rdx, JSAMPROW [rdx+rcx*SIZEOF_JSAMPROW]  ; inptr2
    mov         rdi, JSAMPROW [rdi]                      ; outptr

    pop         rcx                     ; col

.columnloop:

    vmovdqu     ymm6, YMMWORD [rbx]     ; ymm6=Cb(0123456789ABCDEF)
    vmovdqu     ymm7, YMMWORD [rdx]     ; ymm7=Cr(0123456789ABCDEF)

    vpxor       ymm1, ymm1, ymm1        ; ymm1=(all 0's)
    vpcmpeqw    ymm3, ymm3, ymm3
    vpsllw      ymm3, ymm3, 7           ; ymm3={0xFF80 0xFF80 0xFF80 0xFF80 ..}

    vpermq      ymm6, ymm6, 0xd8
    vpermq      ymm7, ymm7, 0xd8
    vpunpcklbw  ymm4, ymm6, ymm1        ; ymm4=Cb(01234567)=CbL
    vpunpckhbw  ymm6, ymm6, ymm1        ; ymm6=Cb(89ABCDEF)=CbH
    vpunpcklbw  ymm0, ymm7, ymm1        ; ymm0=Cr(01234567)=CrL
    vpunpckhbw  ymm7, ymm7, ymm1        ; ymm7=Cr(89ABCDEF)=CrH

    vpaddw      ymm5, ymm6, ymm3
    vpaddw      ymm2, ymm4, ymm3
    vpaddw      ymm1, ymm7, ymm3
    vpaddw      ymm3, ymm0, ymm3

    ; (Original)
    ; R = Y                + 1.40200 * Cr
    ; G = Y - 0.34414 * Cb - 0.71414 * Cr
    ; B = Y + 1.77200 * Cb
    ;
    ; (This implementation)
    ; R = Y                + 0.40200 * Cr + Cr
    ; G = Y - 0.34414 * Cb + 0.28586 * Cr - Cr
    ; B = Y - 0.22800 * Cb + Cb + Cb

    vpaddw      ymm6, ymm5, ymm5             ; ymm6=2*CbH
    vpaddw      ymm4, ymm2, ymm2             ; ymm4=2*CbL
    vpaddw      ymm7, ymm1, ymm1             ; ymm7=2*CrH
    vpaddw      ymm0, ymm3, ymm3             ; ymm0=2*CrL

    vpmulhw     ymm6, ymm6, [rel PW_MF0228]  ; ymm6=(2*CbH * -FIX(0.22800))
    vpmulhw     ymm4, ymm4, [rel PW_MF0228]  ; ymm4=(2*CbL * -FIX(0.22800))
    vpmulhw     ymm7, ymm7, [rel PW_F0402]   ; ymm7=(2*CrH * FIX(0.40200))
    vpmulhw     ymm0, ymm0, [rel PW_F0402]   ; ymm0=(2*CrL * FIX(0.40200))

    vpaddw      ymm6, ymm6, [rel PW_ONE]
    vpaddw      ymm4, ymm4, [rel PW_ONE]
    vpsraw      ymm6, ymm6, 1                ; ymm6=(CbH * -FIX(0.22800))
    vpsraw      ymm4, ymm4, 1                ; ymm4=(CbL * -FIX(0.22800))
    vpaddw      ymm7, ymm7, [rel PW_ONE]
    vpaddw      ymm0, ymm0, [rel PW_ONE]
    vpsraw      ymm7, ymm7, 1                ; ymm7=(CrH * FIX(0.40200))
    vpsraw      ymm0, ymm0, 1                ; ymm0=(CrL * FIX(0.40200))

    vpaddw      ymm6, ymm6, ymm5
    vpaddw      ymm4, ymm4, ymm2
    vpaddw      ymm6, ymm6, ymm5             ; ymm6=(CbH * FIX(1.77200))=(B-Y)H
    vpaddw      ymm4, ymm4, ymm2             ; ymm4=(CbL * FIX(1.77200))=(B-Y)L
    vpaddw      ymm7, ymm7, ymm1             ; ymm7=(CrH * FIX(1.40200))=(R-Y)H
    vpaddw      ymm0, ymm0, ymm3             ; ymm0=(CrL * FIX(1.40200))=(R-Y)L

    vmovdqa     YMMWORD [wk(0)], ymm6        ; wk(0)=(B-Y)H
    vmovdqa     YMMWORD [wk(1)], ymm7        ; wk(1)=(R-Y)H

    vpunpckhwd  ymm6, ymm5, ymm1
    vpunpcklwd  ymm5, ymm5, ymm1
    vpmaddwd    ymm5, ymm5, [rel PW_MF0344_F0285]
    vpmaddwd    ymm6, ymm6, [rel PW_MF0344_F0285]
    vpunpckhwd  ymm7, ymm2, ymm3
    vpunpcklwd  ymm2, ymm2, ymm3
    vpmaddwd    ymm2, ymm2, [rel PW_MF0344_F0285]
    vpmaddwd    ymm7, ymm7, [rel PW_MF0344_F0285]

    vpaddd      ymm5, ymm5, [rel PD_ONEHALF]
    vpaddd      ymm6, ymm6, [rel PD_ONEHALF]
    vpsrad      ymm5, ymm5, SCALEBITS
    vpsrad      ymm6, ymm6, SCALEBITS
    vpaddd      ymm2, ymm2, [rel PD_ONEHALF]
    vpaddd      ymm7, ymm7, [rel PD_ONEHALF]
    vpsrad      ymm2, ymm2, SCALEBITS
    vpsrad      ymm7, ymm7, SCALEBITS

    vpackssdw   ymm5, ymm5, ymm6        ; ymm5=CbH*-FIX(0.344)+CrH*FIX(0.285)
    vpackssdw   ymm2, ymm2, ymm7        ; ymm2=CbL*-FIX(0.344)+CrL*FIX(0.285)
    vpsubw      ymm5, ymm5, ymm1        ; ymm5=CbH*-FIX(0.344)+CrH*-FIX(0.714)=(G-Y)H
    vpsubw      ymm2, ymm2, ymm3        ; ymm2=CbL*-FIX(0.344)+CrL*-FIX(0.714)=(G-Y)L

    vmovdqa     YMMWORD [wk(2)], ymm5   ; wk(2)=(G-Y)H

    mov         al, 2                   ; Yctr
    jmp         short .Yloop_1st

.Yloop_2nd:
    vmovdqa     ymm0, YMMWORD [wk(1)]   ; ymm0=(R-Y)H
    vmovdqa     ymm2, YMMWORD [wk(2)]   ; ymm2=(G-Y)H
    vmovdqa     ymm4, YMMWORD [wk(0)]   ; ymm4=(B-Y)H

.Yloop_1st:
    vmovdqu     ymm7, YMMWORD [rsi]     ; ymm7=Y(0123456789ABCDEF)

    vpcmpeqw    ymm6, ymm6, ymm6
    vpsrlw      ymm6, ymm6, BYTE_BIT    ; ymm6={0xFF 0x00 0xFF 0x00 ..}
    vpand       ymm6, ymm6, ymm7        ; ymm6=Y(02468ACE)=YE
    vpsrlw      ymm7, ymm7, BYTE_BIT    ; ymm7=Y(13579BDF)=YO

    vmovdqa     ymm1, ymm0              ; ymm1=ymm0=(R-Y)(L/H)
    vmovdqa     ymm3, ymm2              ; ymm3=ymm2=(G-Y)(L/H)
    vmovdqa     ymm5, ymm4              ; ymm5=ymm4=(B-Y)(L/H)

    vpaddw      ymm0, ymm0, ymm6        ; ymm0=((R-Y)+YE)=RE=R(02468ACE)
    vpaddw      ymm1, ymm1, ymm7        ; ymm1=((R-Y)+YO)=RO=R(13579BDF)
    vpackuswb   ymm0, ymm0, ymm0        ; ymm0=R(02468ACE********)
    vpackuswb   ymm1, ymm1, ymm1        ; ymm1=R(13579BDF********)

    vpaddw      ymm2, ymm2, ymm6        ; ymm2=((G-Y)+YE)=GE=G(02468ACE)
    vpaddw      ymm3, ymm3, ymm7        ; ymm3=((G-Y)+YO)=GO=G(13579BDF)
    vpackuswb   ymm2, ymm2, ymm2        ; ymm2=G(02468ACE********)
    vpackuswb   ymm3, ymm3, ymm3        ; ymm3=G(13579BDF********)

    vpaddw      ymm4, ymm4, ymm6        ; ymm4=((B-Y)+YE)=BE=B(02468ACE)
    vpaddw      ymm5, ymm5, ymm7        ; ymm5=((B-Y)+YO)=BO=B(13579BDF)
    vpackuswb   ymm4, ymm4, ymm4        ; ymm4=B(02468ACE********)
    vpackuswb   ymm5, ymm5, ymm5        ; ymm5=B(13579BDF********)

%if RGB_PIXELSIZE == 3  ; ---------------

    vpunpcklbw  ymmA, ymmA, ymmC
    vpunpcklbw  ymmE, ymmE, ymmB
    vpunpcklbw  ymmD, ymmD, ymmF

    vmovdqa     ymmG, ymmA
    vmovdqa     ymmH, ymmA
    vpunpcklwd  ymmA, ymmA, ymmE
    vpunpckhwd  ymmG, ymmG, ymmE

    vpsrldq     ymmH, ymmH, 2
    vpsrldq     ymmE, ymmE, 2

    vmovdqa     ymmC, ymmD
    vmovdqa     ymmB, ymmD
    vpunpcklwd  ymmD, ymmD, ymmH
    vpunpckhwd  ymmC, ymmC, ymmH

    vpsrldq     ymmB, ymmB, 2

    vmovdqa     ymmF, ymmE
    vpunpcklwd  ymmE, ymmE, ymmB
    vpunpckhwd  ymmF, ymmF, ymmB

    vpshufd     ymmH, ymmA, 0x4E
    vmovdqa     ymmB, ymmE
    vpunpckldq  ymmA, ymmA, ymmD
    vpunpckldq  ymmE, ymmE, ymmH
    vpunpckhdq  ymmD, ymmD, ymmB

    vpshufd     ymmH, ymmG, 0x4E
    vmovdqa     ymmB, ymmF
    vpunpckldq  ymmG, ymmG, ymmC
    vpunpckldq  ymmF, ymmF, ymmH
    vpunpckhdq  ymmC, ymmC, ymmB

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
    jz          near .endcolumn

    add         rsi, byte SIZEOF_YMMWORD  ; inptr0
    dec         al                        ; Yctr
    jnz         near .Yloop_2nd

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
    jz          short .endcolumn
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
    jz          near .endcolumn

    add         rsi, byte SIZEOF_YMMWORD  ; inptr0
    dec         al
    jnz         near .Yloop_2nd

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
    add         rdi, byte SIZEOF_XMMWORD    ; outptr
    vperm2i128  ymmA, ymmA, ymmA, 1
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
    jz          short .endcolumn
    vmovd       XMM_DWORD [rdi], xmmA

%endif  ; RGB_PIXELSIZE ; ---------------

.endcolumn:
    sfence                              ; flush the write buffer

.return:
    pop         rbx
    uncollect_args 4
    mov         rsp, rbp                ; rsp <- aligned rbp
    pop         rsp                     ; rsp <- original rbp
    pop         rbp
    ret

; --------------------------------------------------------------------------
;
; Upsample and color convert for the case of 2:1 horizontal and 2:1 vertical.
;
; GLOBAL(void)
; jsimd_h2v2_merged_upsample_avx2 (JDIMENSION output_width,
;                                  JSAMPIMAGE input_buf,
;                                  JDIMENSION in_row_group_ctr,
;                                  JSAMPARRAY output_buf);
;

; r10d = JDIMENSION output_width
; r11 = JSAMPIMAGE input_buf
; r12d = JDIMENSION in_row_group_ctr
; r13 = JSAMPARRAY output_buf

    align       32
    global      EXTN(jsimd_h2v2_merged_upsample_avx2)

EXTN(jsimd_h2v2_merged_upsample_avx2):
    push        rbp
    mov         rax, rsp
    mov         rbp, rsp
    collect_args 4
    push        rbx

    mov         eax, r10d

    mov         rdi, r11
    mov         ecx, r12d
    mov         rsi, JSAMPARRAY [rdi+0*SIZEOF_JSAMPARRAY]
    mov         rbx, JSAMPARRAY [rdi+1*SIZEOF_JSAMPARRAY]
    mov         rdx, JSAMPARRAY [rdi+2*SIZEOF_JSAMPARRAY]
    mov         rdi, r13
    lea         rsi, [rsi+rcx*SIZEOF_JSAMPROW]

    push        rdx                     ; inptr2
    push        rbx                     ; inptr1
    push        rsi                     ; inptr00
    mov         rbx, rsp

    push        rdi
    push        rcx
    push        rax

    %ifdef WIN64
    mov         r8, rcx
    mov         r9, rdi
    mov         rcx, rax
    mov         rdx, rbx
    %else
    mov         rdx, rcx
    mov         rcx, rdi
    mov         rdi, rax
    mov         rsi, rbx
    %endif

    call        EXTN(jsimd_h2v1_merged_upsample_avx2)

    pop         rax
    pop         rcx
    pop         rdi
    pop         rsi
    pop         rbx
    pop         rdx

    add         rdi, byte SIZEOF_JSAMPROW  ; outptr1
    add         rsi, byte SIZEOF_JSAMPROW  ; inptr01

    push        rdx                     ; inptr2
    push        rbx                     ; inptr1
    push        rsi                     ; inptr00
    mov         rbx, rsp

    push        rdi
    push        rcx
    push        rax

    %ifdef WIN64
    mov         r8, rcx
    mov         r9, rdi
    mov         rcx, rax
    mov         rdx, rbx
    %else
    mov         rdx, rcx
    mov         rcx, rdi
    mov         rdi, rax
    mov         rsi, rbx
    %endif

    call        EXTN(jsimd_h2v1_merged_upsample_avx2)

    pop         rax
    pop         rcx
    pop         rdi
    pop         rsi
    pop         rbx
    pop         rdx

    pop         rbx
    uncollect_args 4
    pop         rbp
    ret

; For some reason, the OS X linker does not honor the request to align the
; segment unless we do this.
    align       32

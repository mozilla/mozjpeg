;
; jccolext.asm - colorspace conversion (64-bit AVX2)
;
; Copyright (C) 2009, 2016, D. R. Commander.
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
; jsimd_rgb_ycc_convert_avx2 (JDIMENSION img_width,
;                             JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
;                             JDIMENSION output_row, int num_rows);
;

; r10d = JDIMENSION img_width
; r11 = JSAMPARRAY input_buf
; r12 = JSAMPIMAGE output_buf
; r13d = JDIMENSION output_row
; r14d = int num_rows

%define wk(i)   rbp-(WK_NUM-(i))*SIZEOF_YMMWORD  ; ymmword wk[WK_NUM]
%define WK_NUM  8

    align       32

    global      EXTN(jsimd_rgb_ycc_convert_avx2)

EXTN(jsimd_rgb_ycc_convert_avx2):
    push        rbp
    mov         rax, rsp                     ; rax = original rbp
    sub         rsp, byte 4
    and         rsp, byte (-SIZEOF_YMMWORD)  ; align to 256 bits
    mov         [rsp], rax
    mov         rbp, rsp                     ; rbp = aligned rbp
    lea         rsp, [wk(0)]
    collect_args 5
    push        rbx

    mov         ecx, r10d
    test        rcx, rcx
    jz          near .return

    push        rcx

    mov         rsi, r12
    mov         ecx, r13d
    mov         rdi, JSAMPARRAY [rsi+0*SIZEOF_JSAMPARRAY]
    mov         rbx, JSAMPARRAY [rsi+1*SIZEOF_JSAMPARRAY]
    mov         rdx, JSAMPARRAY [rsi+2*SIZEOF_JSAMPARRAY]
    lea         rdi, [rdi+rcx*SIZEOF_JSAMPROW]
    lea         rbx, [rbx+rcx*SIZEOF_JSAMPROW]
    lea         rdx, [rdx+rcx*SIZEOF_JSAMPROW]

    pop         rcx

    mov         rsi, r11
    mov         eax, r14d
    test        rax, rax
    jle         near .return
.rowloop:
    push        rdx
    push        rbx
    push        rdi
    push        rsi
    push        rcx                     ; col

    mov         rsi, JSAMPROW [rsi]     ; inptr
    mov         rdi, JSAMPROW [rdi]     ; outptr0
    mov         rbx, JSAMPROW [rbx]     ; outptr1
    mov         rdx, JSAMPROW [rdx]     ; outptr2

    cmp         rcx, byte SIZEOF_YMMWORD
    jae         near .columnloop

%if RGB_PIXELSIZE == 3  ; ---------------

.column_ld1:
    push        rax
    push        rdx
    lea         rcx, [rcx+rcx*2]        ; imul ecx,RGB_PIXELSIZE
    test        cl, SIZEOF_BYTE
    jz          short .column_ld2
    sub         rcx, byte SIZEOF_BYTE
    movzx       rax, BYTE [rsi+rcx]
.column_ld2:
    test        cl, SIZEOF_WORD
    jz          short .column_ld4
    sub         rcx, byte SIZEOF_WORD
    movzx       rdx, WORD [rsi+rcx]
    shl         rax, WORD_BIT
    or          rax, rdx
.column_ld4:
    vmovd       xmmA, eax
    pop         rdx
    pop         rax
    test        cl, SIZEOF_DWORD
    jz          short .column_ld8
    sub         rcx, byte SIZEOF_DWORD
    vmovd       xmmF, XMM_DWORD [rsi+rcx]
    vpslldq     xmmA, xmmA, SIZEOF_DWORD
    vpor        xmmA, xmmA, xmmF
.column_ld8:
    test        cl, SIZEOF_MMWORD
    jz          short .column_ld16
    sub         rcx, byte SIZEOF_MMWORD
    vmovq       xmmB, XMM_MMWORD [rsi+rcx]
    vpslldq     xmmA, xmmA, SIZEOF_MMWORD
    vpor        xmmA, xmmA, xmmB
.column_ld16:
    test        cl, SIZEOF_XMMWORD
    jz          short .column_ld32
    sub         rcx, byte SIZEOF_XMMWORD
    vmovdqu     xmmB, XMM_MMWORD [rsi+rcx]
    vperm2i128  ymmA, ymmA, ymmA, 1
    vpor        ymmA, ymmB
.column_ld32:
    test        cl, SIZEOF_YMMWORD
    jz          short .column_ld64
    sub         rcx, byte SIZEOF_YMMWORD
    vmovdqa     ymmF, ymmA
    vmovdqu     ymmA, YMMWORD [rsi+0*SIZEOF_YMMWORD]
.column_ld64:
    test        cl, 2*SIZEOF_YMMWORD
    mov         rcx, SIZEOF_YMMWORD
    jz          short .rgb_ycc_cnv
    vmovdqa     ymmB, ymmA
    vmovdqu     ymmA, YMMWORD [rsi+0*SIZEOF_YMMWORD]
    vmovdqu     ymmF, YMMWORD [rsi+1*SIZEOF_YMMWORD]
    jmp         short .rgb_ycc_cnv

.columnloop:
    vmovdqu     ymmA, YMMWORD [rsi+0*SIZEOF_YMMWORD]
    vmovdqu     ymmF, YMMWORD [rsi+1*SIZEOF_YMMWORD]
    vmovdqu     ymmB, YMMWORD [rsi+2*SIZEOF_YMMWORD]

.rgb_ycc_cnv:

    vmovdqu     ymmC, ymmA
    vinserti128 ymmA, ymmF, xmmA, 0
    vinserti128 ymmC, ymmC, xmmB, 0
    vinserti128 ymmB, ymmB, xmmF, 0
    vperm2i128  ymmF, ymmC, ymmC, 1

    vmovdqa     ymmG, ymmA
    vpslldq     ymmA, ymmA, 8
    vpsrldq     ymmG, ymmG, 8

    vpunpckhbw  ymmA, ymmA, ymmF
    vpslldq     ymmF, ymmF, 8

    vpunpcklbw  ymmG, ymmG, ymmB
    vpunpckhbw  ymmF, ymmF, ymmB

    vmovdqa     ymmD, ymmA
    vpslldq     ymmA, ymmA, 8
    vpsrldq     ymmD, ymmD, 8

    vpunpckhbw  ymmA, ymmA, ymmG
    vpslldq     ymmG, ymmG, 8

    vpunpcklbw  ymmD, ymmD, ymmF
    vpunpckhbw  ymmG, ymmG, ymmF

    vmovdqa     ymmE, ymmA
    vpslldq     ymmA, ymmA, 8
    vpsrldq     ymmE, ymmE, 8

    vpunpckhbw  ymmA, ymmA, ymmD
    vpslldq     ymmD, ymmD, 8

    vpunpcklbw  ymmE, ymmE, ymmG
    vpunpckhbw  ymmD, ymmD, ymmG

    vpxor       ymmH, ymmH, ymmH

    vmovdqa     ymmC, ymmA
    vpunpcklbw  ymmA, ymmA, ymmH
    vpunpckhbw  ymmC, ymmC, ymmH

    vmovdqa     ymmB, ymmE
    vpunpcklbw  ymmE, ymmE, ymmH
    vpunpckhbw  ymmB, ymmB, ymmH

    vmovdqa     ymmF, ymmD
    vpunpcklbw  ymmD, ymmD, ymmH
    vpunpckhbw  ymmF, ymmF, ymmH

%else  ; RGB_PIXELSIZE == 4 ; -----------

.column_ld1:
    test        cl, SIZEOF_XMMWORD/16
    jz          short .column_ld2
    sub         rcx, byte SIZEOF_XMMWORD/16
    vmovd       xmmA, XMM_DWORD [rsi+rcx*RGB_PIXELSIZE]
.column_ld2:
    test        cl, SIZEOF_XMMWORD/8
    jz          short .column_ld4
    sub         rcx, byte SIZEOF_XMMWORD/8
    vmovq       xmmE, XMM_MMWORD [rsi+rcx*RGB_PIXELSIZE]
    vpslldq     xmmA, xmmA, SIZEOF_MMWORD
    vpor        xmmA, xmmA, xmmE
.column_ld4:
    test        cl, SIZEOF_XMMWORD/4
    jz          short .column_ld8
    sub         rcx, byte SIZEOF_XMMWORD/4
    vmovdqa     xmmE, xmmA
    vperm2i128  ymmE, ymmE, ymmE, 1
    vmovdqu     xmmA, XMMWORD [rsi+rcx*RGB_PIXELSIZE]
    vpor        ymmA, ymmA, ymmE
.column_ld8:
    test        cl, SIZEOF_XMMWORD/2
    jz          short .column_ld16
    sub         rcx, byte SIZEOF_XMMWORD/2
    vmovdqa     ymmE, ymmA
    vmovdqu     ymmA, YMMWORD [rsi+rcx*RGB_PIXELSIZE]
.column_ld16:
    test        cl, SIZEOF_XMMWORD
    mov         rcx, SIZEOF_YMMWORD
    jz          short .rgb_ycc_cnv
    vmovdqa     ymmF, ymmA
    vmovdqa     ymmH, ymmE
    vmovdqu     ymmA, YMMWORD [rsi+0*SIZEOF_YMMWORD]
    vmovdqu     ymmE, YMMWORD [rsi+1*SIZEOF_YMMWORD]
    jmp         short .rgb_ycc_cnv

.columnloop:
    vmovdqu     ymmA, YMMWORD [rsi+0*SIZEOF_YMMWORD]
    vmovdqu     ymmE, YMMWORD [rsi+1*SIZEOF_YMMWORD]
    vmovdqu     ymmF, YMMWORD [rsi+2*SIZEOF_YMMWORD]
    vmovdqu     ymmH, YMMWORD [rsi+3*SIZEOF_YMMWORD]

.rgb_ycc_cnv:
    vmovdqa     ymmB, ymmA
    vinserti128 ymmA, ymmA, xmmF, 1
    vperm2i128  ymmF, ymmB, ymmF, 0x31

    vmovdqa     ymmB, ymmE
    vinserti128 ymmE, ymmE, xmmH, 1
    vperm2i128  ymmH, ymmB, ymmH, 0x31

    vmovdqa     ymmB, ymmE
    vmovdqa     ymmE, ymmF
    vmovdqa     ymmF, ymmB

    vmovdqa     ymmD, ymmA
    vpunpcklbw  ymmA, ymmA, ymmE
    vpunpckhbw  ymmD, ymmD, ymmE

    vmovdqa     ymmC, ymmF
    vpunpcklbw  ymmF, ymmF, ymmH
    vpunpckhbw  ymmC, ymmC, ymmH

    vmovdqa     ymmB, ymmA
    vpunpcklwd  ymmA, ymmA, ymmF
    vpunpckhwd  ymmB, ymmB, ymmF

    vmovdqa     ymmG, ymmD
    vpunpcklwd  ymmD, ymmD, ymmC
    vpunpckhwd  ymmG, ymmG, ymmC

    vmovdqa     ymmE, ymmA
    vpunpcklbw  ymmA, ymmA, ymmD
    vpunpckhbw  ymmE, ymmE, ymmD

    vmovdqa     ymmH, ymmB
    vpunpcklbw  ymmB, ymmB, ymmG
    vpunpckhbw  ymmH, ymmH, ymmG

    vpxor       ymmF, ymmF, ymmF

    vmovdqa     ymmC, ymmA
    vpunpcklbw  ymmA, ymmA, ymmF
    vpunpckhbw  ymmC, ymmC, ymmF

    vmovdqa     ymmD, ymmB
    vpunpcklbw  ymmB, ymmB, ymmF
    vpunpckhbw  ymmD, ymmD, ymmF

    vmovdqa     ymmG, ymmE
    vpunpcklbw  ymmE, ymmE, ymmF
    vpunpckhbw  ymmG, ymmG, ymmF

    vpunpcklbw  ymmF, ymmF, ymmH
    vpunpckhbw  ymmH, ymmH, ymmH
    vpsrlw      ymmF, ymmF, BYTE_BIT
    vpsrlw      ymmH, ymmH, BYTE_BIT

%endif  ; RGB_PIXELSIZE ; ---------------

    ; ymm0=R(02468ACE)=RE, ymm2=G(02468ACE)=GE, ymm4=B(02468ACE)=BE
    ; ymm1=R(13579BDF)=RO, ymm3=G(13579BDF)=GO, ymm5=B(13579BDF)=BO

    ; (Original)
    ; Y  =  0.29900 * R + 0.58700 * G + 0.11400 * B
    ; Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B + CENTERJSAMPLE
    ; Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B + CENTERJSAMPLE
    ;
    ; (This implementation)
    ; Y  =  0.29900 * R + 0.33700 * G + 0.11400 * B + 0.25000 * G
    ; Cb = -0.16874 * R - 0.33126 * G + 0.50000 * B + CENTERJSAMPLE
    ; Cr =  0.50000 * R - 0.41869 * G - 0.08131 * B + CENTERJSAMPLE

    vmovdqa     YMMWORD [wk(0)], ymm0   ; wk(0)=RE
    vmovdqa     YMMWORD [wk(1)], ymm1   ; wk(1)=RO
    vmovdqa     YMMWORD [wk(2)], ymm4   ; wk(2)=BE
    vmovdqa     YMMWORD [wk(3)], ymm5   ; wk(3)=BO

    vmovdqa     ymm6, ymm1
    vpunpcklwd  ymm1, ymm1, ymm3
    vpunpckhwd  ymm6, ymm6, ymm3
    vmovdqa     ymm7, ymm1
    vmovdqa     ymm4, ymm6
    vpmaddwd    ymm1, ymm1, [rel PW_F0299_F0337]  ; ymm1=ROL*FIX(0.299)+GOL*FIX(0.337)
    vpmaddwd    ymm6, ymm6, [rel PW_F0299_F0337]  ; ymm6=ROH*FIX(0.299)+GOH*FIX(0.337)
    vpmaddwd    ymm7, ymm7, [rel PW_MF016_MF033]  ; ymm7=ROL*-FIX(0.168)+GOL*-FIX(0.331)
    vpmaddwd    ymm4, ymm4, [rel PW_MF016_MF033]  ; ymm4=ROH*-FIX(0.168)+GOH*-FIX(0.331)

    vmovdqa     YMMWORD [wk(4)], ymm1   ; wk(4)=ROL*FIX(0.299)+GOL*FIX(0.337)
    vmovdqa     YMMWORD [wk(5)], ymm6   ; wk(5)=ROH*FIX(0.299)+GOH*FIX(0.337)

    vpxor       ymm1, ymm1, ymm1
    vpxor       ymm6, ymm6, ymm6
    vpunpcklwd  ymm1, ymm1, ymm5        ; ymm1=BOL
    vpunpckhwd  ymm6, ymm6, ymm5        ; ymm6=BOH
    vpsrld      ymm1, ymm1, 1           ; ymm1=BOL*FIX(0.500)
    vpsrld      ymm6, ymm6, 1           ; ymm6=BOH*FIX(0.500)

    vmovdqa     ymm5, [rel PD_ONEHALFM1_CJ]  ; ymm5=[PD_ONEHALFM1_CJ]

    vpaddd      ymm7, ymm7, ymm1
    vpaddd      ymm4, ymm4, ymm6
    vpaddd      ymm7, ymm7, ymm5
    vpaddd      ymm4, ymm4, ymm5
    vpsrld      ymm7, ymm7, SCALEBITS   ; ymm7=CbOL
    vpsrld      ymm4, ymm4, SCALEBITS   ; ymm4=CbOH
    vpackssdw   ymm7, ymm7, ymm4        ; ymm7=CbO

    vmovdqa     ymm1, YMMWORD [wk(2)]   ; ymm1=BE

    vmovdqa     ymm6, ymm0
    vpunpcklwd  ymm0, ymm0, ymm2
    vpunpckhwd  ymm6, ymm6, ymm2
    vmovdqa     ymm5, ymm0
    vmovdqa     ymm4, ymm6
    vpmaddwd    ymm0, ymm0, [rel PW_F0299_F0337]  ; ymm0=REL*FIX(0.299)+GEL*FIX(0.337)
    vpmaddwd    ymm6, ymm6, [rel PW_F0299_F0337]  ; ymm6=REH*FIX(0.299)+GEH*FIX(0.337)
    vpmaddwd    ymm5, ymm5, [rel PW_MF016_MF033]  ; ymm5=REL*-FIX(0.168)+GEL*-FIX(0.331)
    vpmaddwd    ymm4, ymm4, [rel PW_MF016_MF033]  ; ymm4=REH*-FIX(0.168)+GEH*-FIX(0.331)

    vmovdqa     YMMWORD [wk(6)], ymm0   ; wk(6)=REL*FIX(0.299)+GEL*FIX(0.337)
    vmovdqa     YMMWORD [wk(7)], ymm6   ; wk(7)=REH*FIX(0.299)+GEH*FIX(0.337)

    vpxor       ymm0, ymm0, ymm0
    vpxor       ymm6, ymm6, ymm6
    vpunpcklwd  ymm0, ymm0, ymm1        ; ymm0=BEL
    vpunpckhwd  ymm6, ymm6, ymm1        ; ymm6=BEH
    vpsrld      ymm0, ymm0, 1           ; ymm0=BEL*FIX(0.500)
    vpsrld      ymm6, ymm6, 1           ; ymm6=BEH*FIX(0.500)


    vmovdqa     ymm1, [rel PD_ONEHALFM1_CJ]  ; ymm1=[PD_ONEHALFM1_CJ]

    vpaddd      ymm5, ymm5, ymm0
    vpaddd      ymm4, ymm4, ymm6
    vpaddd      ymm5, ymm5, ymm1
    vpaddd      ymm4, ymm4, ymm1
    vpsrld      ymm5, ymm5, SCALEBITS   ; ymm5=CbEL
    vpsrld      ymm4, ymm4, SCALEBITS   ; ymm4=CbEH
    vpackssdw   ymm5, ymm5, ymm4        ; ymm5=CbE

    vpsllw      ymm7, ymm7, BYTE_BIT
    vpor        ymm5, ymm5, ymm7        ; ymm5=Cb
    vmovdqu     YMMWORD [rbx], ymm5     ; Save Cb

    vmovdqa     ymm0, YMMWORD [wk(3)]   ; ymm0=BO
    vmovdqa     ymm6, YMMWORD [wk(2)]   ; ymm6=BE
    vmovdqa     ymm1, YMMWORD [wk(1)]   ; ymm1=RO

    vmovdqa     ymm4, ymm0
    vpunpcklwd  ymm0, ymm0, ymm3
    vpunpckhwd  ymm4, ymm4, ymm3
    vmovdqa     ymm7, ymm0
    vmovdqa     ymm5, ymm4
    vpmaddwd    ymm0, ymm0, [rel PW_F0114_F0250]  ; ymm0=BOL*FIX(0.114)+GOL*FIX(0.250)
    vpmaddwd    ymm4, ymm4, [rel PW_F0114_F0250]  ; ymm4=BOH*FIX(0.114)+GOH*FIX(0.250)
    vpmaddwd    ymm7, ymm7, [rel PW_MF008_MF041]  ; ymm7=BOL*-FIX(0.081)+GOL*-FIX(0.418)
    vpmaddwd    ymm5, ymm5, [rel PW_MF008_MF041]  ; ymm5=BOH*-FIX(0.081)+GOH*-FIX(0.418)

    vmovdqa     ymm3, [rel PD_ONEHALF]            ; ymm3=[PD_ONEHALF]

    vpaddd      ymm0, ymm0, YMMWORD [wk(4)]
    vpaddd      ymm4, ymm4, YMMWORD [wk(5)]
    vpaddd      ymm0, ymm0, ymm3
    vpaddd      ymm4, ymm4, ymm3
    vpsrld      ymm0, ymm0, SCALEBITS   ; ymm0=YOL
    vpsrld      ymm4, ymm4, SCALEBITS   ; ymm4=YOH
    vpackssdw   ymm0, ymm0, ymm4        ; ymm0=YO

    vpxor       ymm3, ymm3, ymm3
    vpxor       ymm4, ymm4, ymm4
    vpunpcklwd  ymm3, ymm3, ymm1        ; ymm3=ROL
    vpunpckhwd  ymm4, ymm4, ymm1        ; ymm4=ROH
    vpsrld      ymm3, ymm3, 1           ; ymm3=ROL*FIX(0.500)
    vpsrld      ymm4, ymm4, 1           ; ymm4=ROH*FIX(0.500)

    vmovdqa     ymm1, [rel PD_ONEHALFM1_CJ]  ; ymm1=[PD_ONEHALFM1_CJ]

    vpaddd      ymm7, ymm7, ymm3
    vpaddd      ymm5, ymm5, ymm4
    vpaddd      ymm7, ymm7, ymm1
    vpaddd      ymm5, ymm5, ymm1
    vpsrld      ymm7, ymm7, SCALEBITS   ; ymm7=CrOL
    vpsrld      ymm5, ymm5, SCALEBITS   ; ymm5=CrOH
    vpackssdw   ymm7, ymm7, ymm5        ; ymm7=CrO

    vmovdqa     ymm3, YMMWORD [wk(0)]   ; ymm3=RE

    vmovdqa     ymm4, ymm6
    vpunpcklwd  ymm6, ymm6, ymm2
    vpunpckhwd  ymm4, ymm4, ymm2
    vmovdqa     ymm1, ymm6
    vmovdqa     ymm5, ymm4
    vpmaddwd    ymm6, ymm6, [rel PW_F0114_F0250]  ; ymm6=BEL*FIX(0.114)+GEL*FIX(0.250)
    vpmaddwd    ymm4, ymm4, [rel PW_F0114_F0250]  ; ymm4=BEH*FIX(0.114)+GEH*FIX(0.250)
    vpmaddwd    ymm1, ymm1, [rel PW_MF008_MF041]  ; ymm1=BEL*-FIX(0.081)+GEL*-FIX(0.418)
    vpmaddwd    ymm5, ymm5, [rel PW_MF008_MF041]  ; ymm5=BEH*-FIX(0.081)+GEH*-FIX(0.418)

    vmovdqa     ymm2, [rel PD_ONEHALF]            ; ymm2=[PD_ONEHALF]

    vpaddd      ymm6, ymm6, YMMWORD [wk(6)]
    vpaddd      ymm4, ymm4, YMMWORD [wk(7)]
    vpaddd      ymm6, ymm6, ymm2
    vpaddd      ymm4, ymm4, ymm2
    vpsrld      ymm6, ymm6, SCALEBITS   ; ymm6=YEL
    vpsrld      ymm4, ymm4, SCALEBITS   ; ymm4=YEH
    vpackssdw   ymm6, ymm6, ymm4        ; ymm6=YE

    vpsllw      ymm0, ymm0, BYTE_BIT
    vpor        ymm6, ymm6, ymm0        ; ymm6=Y
    vmovdqu     YMMWORD [rdi], ymm6     ; Save Y

    vpxor       ymm2, ymm2, ymm2
    vpxor       ymm4, ymm4, ymm4
    vpunpcklwd  ymm2, ymm2, ymm3        ; ymm2=REL
    vpunpckhwd  ymm4, ymm4, ymm3        ; ymm4=REH
    vpsrld      ymm2, ymm2, 1           ; ymm2=REL*FIX(0.500)
    vpsrld      ymm4, ymm4, 1           ; ymm4=REH*FIX(0.500)

    vmovdqa     ymm0, [rel PD_ONEHALFM1_CJ]  ; ymm0=[PD_ONEHALFM1_CJ]

    vpaddd      ymm1, ymm1, ymm2
    vpaddd      ymm5, ymm5, ymm4
    vpaddd      ymm1, ymm1, ymm0
    vpaddd      ymm5, ymm5, ymm0
    vpsrld      ymm1, ymm1, SCALEBITS   ; ymm1=CrEL
    vpsrld      ymm5, ymm5, SCALEBITS   ; ymm5=CrEH
    vpackssdw   ymm1, ymm1, ymm5        ; ymm1=CrE

    vpsllw      ymm7, ymm7, BYTE_BIT
    vpor        ymm1, ymm1, ymm7        ; ymm1=Cr
    vmovdqu     YMMWORD [rdx], ymm1     ; Save Cr

    sub         rcx, byte SIZEOF_YMMWORD
    add         rsi, RGB_PIXELSIZE*SIZEOF_YMMWORD  ; inptr
    add         rdi, byte SIZEOF_YMMWORD           ; outptr0
    add         rbx, byte SIZEOF_YMMWORD           ; outptr1
    add         rdx, byte SIZEOF_YMMWORD           ; outptr2
    cmp         rcx, byte SIZEOF_YMMWORD
    jae         near .columnloop
    test        rcx, rcx
    jnz         near .column_ld1

    pop         rcx                     ; col
    pop         rsi
    pop         rdi
    pop         rbx
    pop         rdx

    add         rsi, byte SIZEOF_JSAMPROW  ; input_buf
    add         rdi, byte SIZEOF_JSAMPROW
    add         rbx, byte SIZEOF_JSAMPROW
    add         rdx, byte SIZEOF_JSAMPROW
    dec         rax                        ; num_rows
    jg          near .rowloop

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

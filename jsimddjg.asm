;
; jsimddjg.asm - SIMD instruction support check (for DJGPP V.2)
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
; Last Modified : September 26, 2004
;
; [TAB8]

%include "jsimdext.inc"

; --------------------------------------------------------------------------
	SECTION	SEG_TEXT
	BITS	32
;
; Check if the OS supports SIMD instructions (DJGPP V.2)
;
; GLOBAL(unsigned int)
; jpeg_simd_os_support (unsigned int simd)
;

%define EXCEPTION_ILLEGAL_INSTRUCTION	6	; vector number of #UD

%define simd	ebp+8			; unsigned int simd
%define mxcsr	ebp-4			; unsigned int mxcsr = 0x1F80

	align	16
	global	EXTN(jpeg_simd_os_support)

EXTN(jpeg_simd_os_support):
	push	ebp
	mov	ebp,esp
	push	dword 0x1F80		; default value of MXCSR register
	push	ebx

	push	DWORD [simd]	; simd_flags - modified from exception_handler

	mov	bl, EXCEPTION_ILLEGAL_INSTRUCTION
	mov	ax, 0x0202	; Get Processor Exception Handler Vector
	int	0x31		; DPMI function call
	push	ecx		; selector of old exception handler
	push	edx		; offset   of old exception handler

	mov	ecx,cs
	mov	edx, exception_handler
	mov	bl, EXCEPTION_ILLEGAL_INSTRUCTION
	mov	ax, 0x0203	; Set Processor Exception Handler Vector
	int	0x31		; DPMI function call

	mov	eax, DWORD [simd]

	; If floating point emulation is enabled (CR0.EM = 1),
	; executing an MMX/3DNow! instruction generates invalid
	; opcode exception (#UD).

	push	byte (.mmx_1 - .mmx_0)		; inst_bytes
	push	byte (JSIMD_MMX | JSIMD_3DNOW)	; test_flags
	test	eax, DWORD [esp]
	jz	short .mmx_1
.mmx_0:	emms				; executing MMX instruction
.mmx_1:	add	esp, byte 8

	push	byte (.sse_1 - .sse_0)
	push	byte (JSIMD_SSE | JSIMD_SSE2)
	test	eax, DWORD [esp]
	jz	short .sse_1
.sse_0:	ldmxcsr	DWORD [mxcsr]		; executing SSE instruction
.sse_1:	add	esp, byte 8

	pop	edx		; offset   of old exception handler
	pop	ecx		; selector of old exception handler
	mov	bl, EXCEPTION_ILLEGAL_INSTRUCTION
	mov	ax, 0x0203	; Set Processor Exception Handler Vector
	int	0x31		; DPMI function call

	pop	eax		; return simd_flags
	and	eax, byte JSIMD_ALL

	pop	ebx
	mov	esp,ebp
	pop	ebp
	ret

; --------------------------------------------------------------------------
;
; LOCAL(void) far
; exception_handler (unsigned long error_code,
;                    void * context_eip, unsigned short context_cs,
;                    unsigned long context_eflags,
;                    void * context_esp, unsigned short context_ss);
;

%define error_code	esp+12+8	; unsigned long error_code
%define context_eip	esp+12+12	; void * context_eip
%define context_cs	esp+12+16	; unsigned short context_cs
%define context_eflags	esp+12+20	; unsigned long context_eflags
%define context_esp	esp+12+24	; void * context_esp
%define context_ss	esp+12+28	; unsigned short context_ss

%define test_flags(b)	(b)+0
%define inst_bytes(b)	(b)+4
%define simd_flags(b)	(b)+16

	align	16

exception_handler:
	push	eax
	push	ecx
	push	edx

	mov	eax, POINTER [context_esp]
	mov	ecx, DWORD [test_flags(eax)]
	mov	edx, DWORD [inst_bytes(eax)]
	not	ecx
	add	POINTER [context_eip], edx	; next instruction
	and	DWORD [simd_flags(eax)], ecx	; turn off flag

	pop	edx
	pop	ecx
	pop	eax
	retf


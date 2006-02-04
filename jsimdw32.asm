;
; jsimdw32.asm - SIMD instruction support check (for Win32)
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
; Check if the OS supports SIMD instructions (Win32)
;
; Reference: "Win32 Exception handling for assembler programmers"
;               http://www.jorgon.freeserve.co.uk/Except/Except.htm
;
; GLOBAL(unsigned int)
; jpeg_simd_os_support (unsigned int simd)
;

%define simd	ebp+8			; unsigned int simd
%define mxcsr	ebp-4			; unsigned int mxcsr = 0x1F80

	align	16
	global	EXTN(jpeg_simd_os_support)

EXTN(jpeg_simd_os_support):
	push	ebp
	mov	ebp,esp
	push	dword 0x1F80		; default value of MXCSR register
	push	exception_handler
	push	POINTER [fs:0]		; prev_record_ptr
	mov	POINTER [fs:0], esp	; this_record_ptr

	mov	eax, DWORD [simd]
	and	eax, byte JSIMD_ALL
	xor	ecx,ecx
	xor	edx,edx

	; If floating point emulation is enabled (CR0.EM = 1),
	; executing an MMX/3DNow! instruction generates invalid
	; opcode exception (#UD).

	mov	cl, (JSIMD_MMX | JSIMD_3DNOW)
	mov	dl, (.mmx_1 - .mmx_0)
	test	al,cl
	jz	short .mmx_1
.mmx_0:	emms				; executing MMX instruction
.mmx_1:
	mov	cl, (JSIMD_SSE | JSIMD_SSE2)
	mov	dl, (.sse_1 - .sse_0)
	test	al,cl
	jz	short .sse_1
.sse_0:	ldmxcsr	DWORD [mxcsr]		; executing SSE instruction
.sse_1:

	pop	POINTER [fs:0]		; prev_record_ptr
	mov	esp,ebp
	pop	ebp
	ret

; --------------------------------------------------------------------------
;
; LOCAL(EXCEPTION_DISPOSITION)
; exception_handler (struct _EXCEPTION_RECORD * ExceptionRecord,
;                    void * EstablisherFrame, struct _CONTEXT * ContextRecord,
;                    void * DispatcherContext);
;

%define ExceptionContinueExecution  0	; from <excpt.h>
%define ExceptionContinueSearch     1	; typedef enum _EXCEPTION_DISPOSITION {
%define ExceptionNestedException    2	;   ...
%define ExceptionCollidedUnwind     3	; } EXCEPTION_DISPOSITION

%define EXCEPTION_ILLEGAL_INSTRUCTION   0xC000001D	; from <winbase.h>

%define ExceptionRecord     esp+4	; struct _EXCEPTION_RECORD *
%define EstablisherFrame    esp+8	; void * EstablisherFrame
%define ContextRecord       esp+12	; struct _CONTEXT * ContextRecord
%define DispatcherContext   esp+16	; void * DispatcherContext

%define ExceptionCode(b)    (b)+0	; ExceptionRecord->ExceptionCode
%define ExceptionFlags(b)   (b)+4	; ExceptionRecord->ExceptionFlags
%define Context_Edx(b)      (b)+168	; ContextRecord->Edx
%define Context_Ecx(b)      (b)+172	; ContextRecord->Ecx
%define Context_Eax(b)      (b)+176	; ContextRecord->Eax
%define Context_Eip(b)      (b)+184	; ContextRecord->Eip

	align	16

exception_handler:
	mov	edx, POINTER [ExceptionRecord]
	mov	eax, ExceptionContinueSearch

	cmp	DWORD [ExceptionFlags(edx)], byte 0
	jne	short .return			; noncontinuable exception
	cmp	DWORD [ExceptionCode(edx)], EXCEPTION_ILLEGAL_INSTRUCTION
	jne	short .return			; not a #UD exception

	mov	eax, POINTER [ContextRecord]
	mov	ecx, DWORD [Context_Ecx(eax)]
	mov	edx, DWORD [Context_Edx(eax)]
	not	ecx
	add	DWORD [Context_Eip(eax)], edx	; next instruction
	and	DWORD [Context_Eax(eax)], ecx	; turn off flag
	mov	eax, ExceptionContinueExecution
.return:
	ret


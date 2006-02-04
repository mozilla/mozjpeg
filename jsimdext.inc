;
; jsimdext.inc - common declarations
;
; x86 SIMD extension for IJG JPEG library - version 1.02
;
; Copyright (C) 1999-2006, MIYASAKA Masaru.
;
; This software is provided 'as-is', without any express or implied
; warranty.  In no event will the authors be held liable for any damages
; arising from the use of this software.
;
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it and redistribute it
; freely, subject to the following restrictions:
;
; 1. The origin of this software must not be misrepresented; you must not
;    claim that you wrote the original software. If you use this software
;    in a product, an acknowledgment in the product documentation would be
;    appreciated but is not required.
; 2. Altered source versions must be plainly marked as such, and must not be
;    misrepresented as being the original software.
; 3. This notice may not be removed or altered from any source distribution.
;
; Last Modified : February 4, 2006
;
; [TAB8]

%ifndef JSIMDCFG_INCLUDED	; in case jsimdcfg.inc already did
%include "jsimdcfg.inc"		; configuration declarations
%endif

; ==========================================================================
;  System-dependent configurations

%ifdef WIN32	; ----(nasm -fwin32 -DWIN32 ...)--------
; * Microsoft Visual C++
; * MinGW (Minimalist GNU for Windows)
; * CygWin
; * LCC-Win32

; -- segment definition --
;
%define SEG_TEXT    .text  align=16 public use32 class=CODE
%define SEG_CONST   .rdata align=16 public use32 class=CONST

%elifdef OBJ32	; ----(nasm -fobj -DOBJ32 ...)----------
; * Borland C++ (Win32)

; -- segment definition --
;
%define SEG_TEXT    .text  align=16 public use32 class=CODE
%define SEG_CONST   .data  align=16 public use32 class=DATA

%elifdef ELF	; ----(nasm -felf -DELF ...)------------
; * Linux
; * *BSD family Unix using elf format
; * Unix System V, including Solaris x86, UnixWare and SCO Unix

; -- segment definition --
;
%define SEG_TEXT    .text   progbits alloc exec   nowrite align=16
%define SEG_CONST   .rodata progbits alloc noexec nowrite align=16

; To make the code position-independent, append -DPIC to the commandline
;
%define GOT_SYMBOL  _GLOBAL_OFFSET_TABLE_	; ELF supports PIC
%define EXTN(name)  name			; foo() -> foo

%elifdef AOUT	; ----(nasm -faoutb/aout -DAOUT ...)----
; * Older Linux using a.out format  (nasm -f aout -DAOUT ...)
; * *BSD family Unix using a.out format  (nasm -f aoutb -DAOUT ...)

; -- segment definition --
;
%define SEG_TEXT    .text
%define SEG_CONST   .data

; To make the code position-independent, append -DPIC to the commandline
;
%define GOT_SYMBOL  __GLOBAL_OFFSET_TABLE_	; BSD-style a.out supports PIC

%elifdef MACHO	; ----(nasm -fmacho -DMACHO ...)--------
; * NeXTstep/OpenStep/Rhapsody/Darwin/MacOS X (Mach-O format)

; -- segment definition --
;
%define SEG_TEXT    .text  ;align=16	; nasm doesn't accept align=16. why?
%define SEG_CONST   .rodata align=16

; The generation of position-independent code (PIC) is the default on Darwin.
;
%define PIC
%define GOT_SYMBOL  _MACHO_PIC_		; Mach-O style code-relative addressing

%else		; ----(Other case)----------------------

; -- segment definition --
;
%define SEG_TEXT    .text
%define SEG_CONST   .data

%endif	; ----------------------------------------------

; ==========================================================================

; ---- jpeglib.h -----------------------------------------------------------

%define DCTSIZE		8	; The basic DCT block is 8x8 samples
%define DCTSIZE2	64	; DCTSIZE squared; # of elements in a block

%define JSIMD_NONE	0x00	; bitflags for jpeg_simd_*_support()
%define JSIMD_MMX	0x01
%define JSIMD_3DNOW	0x02
%define JSIMD_SSE	0x04
%define JSIMD_SSE2	0x08
%define JSIMD_ALL	(JSIMD_MMX | JSIMD_3DNOW | JSIMD_SSE | JSIMD_SSE2)

; ---- jpegint.h -----------------------------------------------------------

; Short forms of external names for systems with brain-damaged linkers.
;
%ifdef NEED_SHORT_EXTERNAL_NAMES
%define jpeg_simd_cpu_support	jSiCpuSupport
%define jpeg_simd_os_support	jSiOsSupport
%endif ; NEED_SHORT_EXTERNAL_NAMES

; ---- jmorecfg.h ----------------------------------------------------------
;
; BITS_IN_JSAMPLE==8 (8-bit sample values) is the only valid setting
; on this SIMD implementation.
;
%define BITS_IN_JSAMPLE	8	; Caution: Cannot be changed

; Representation of a single sample (pixel element value).
; On this SIMD implementation, this must be 'unsigned char'.
;
%define JSAMPLE		byte		; unsigned char
%define SIZEOF_JSAMPLE	SIZEOF_BYTE	; sizeof(JSAMPLE)
%define MAXJSAMPLE	255
%define CENTERJSAMPLE	128

; Representation of a DCT frequency coefficient.
; On this SIMD implementation, this must be 'short'.
;
%define JCOEF		word		; short
%define SIZEOF_JCOEF	SIZEOF_WORD	; sizeof(JCOEF)

; INT32 must hold at least signed 32-bit values.
; On this SIMD implementation, this must be 'long'.
;
%define INT32		dword		; long
%define SIZEOF_INT32	SIZEOF_DWORD	; sizeof(INT32)

; Datatype used for image dimensions.
; On this SIMD implementation, this must be 'unsigned int'.
;
%define JDIMENSION		dword		; unsigned int
%define SIZEOF_JDIMENSION	SIZEOF_DWORD	; sizeof(JDIMENSION)

; --------------------------------------------------------------------------

%define JSAMPROW		POINTER		; JSAMPLE FAR * (jpeglib.h)
%define JSAMPARRAY		POINTER		; JSAMPROW *    (jpeglib.h)
%define JSAMPIMAGE		POINTER		; JSAMPARRAY *  (jpeglib.h)
%define JCOEFPTR		POINTER		; JCOEF FAR *   (jpeglib.h)
%define SIZEOF_JSAMPROW		SIZEOF_POINTER	; sizeof(JSAMPROW)
%define SIZEOF_JSAMPARRAY	SIZEOF_POINTER	; sizeof(JSAMPARRAY)
%define SIZEOF_JSAMPIMAGE	SIZEOF_POINTER	; sizeof(JSAMPIMAGE)
%define SIZEOF_JCOEFPTR		SIZEOF_POINTER	; sizeof(JCOEFPTR)

%define POINTER			dword		; general pointer type
%define SIZEOF_POINTER		SIZEOF_DWORD	; sizeof(POINTER)
%define POINTER_BIT		DWORD_BIT	; sizeof(POINTER)*BYTE_BIT

%define INT			dword		; signed integer type
%define SIZEOF_INT		SIZEOF_DWORD	; sizeof(INT)
%define INT_BIT			DWORD_BIT	; sizeof(INT)*BYTE_BIT

%define FP32			dword		; IEEE754 single
%define SIZEOF_FP32		SIZEOF_DWORD	; sizeof(FP32)
%define FP32_BIT		DWORD_BIT	; sizeof(FP32)*BYTE_BIT

%define FP64			qword		; IEEE754 double
%define SIZEOF_FP64		SIZEOF_QWORD	; sizeof(FP64)
%define FP64_BIT		QWORD_BIT	; sizeof(FP64)*BYTE_BIT

%define FP80			tword		; IEEE754 double-extended(x86)
%define SIZEOF_FP80		SIZEOF_TWORD	; sizeof(FP80)
%define FP80_BIT		TWORD_BIT	; sizeof(FP80)*BYTE_BIT

%define MMWORD			qword		; int64  (MMX register)
%define SIZEOF_MMWORD		SIZEOF_QWORD	; sizeof(MMWORD)
%define MMWORD_BIT		QWORD_BIT	; sizeof(MMWORD)*BYTE_BIT

%define XMMWORD			dqword		; int128 (SSE register)
%define SIZEOF_XMMWORD		SIZEOF_DQWORD	; sizeof(XMMWORD)
%define XMMWORD_BIT		DQWORD_BIT	; sizeof(XMMWORD)*BYTE_BIT

%define SIZEOF_BYTE		1		; sizeof(BYTE)
%define SIZEOF_WORD		2		; sizeof(WORD)
%define SIZEOF_DWORD		4		; sizeof(DWORD)
%define SIZEOF_QWORD		8		; sizeof(QWORD)
%define SIZEOF_TBYTE		10		; sizeof(TBYTE)
%define SIZEOF_TWORD		10		; sizeof(TWORD)
%define SIZEOF_DQWORD		16		; sizeof(DQWORD)

%define BYTE_BIT		8		; CHAR_BIT in C
%define WORD_BIT		16		; sizeof(WORD)*BYTE_BIT
%define DWORD_BIT		32		; sizeof(DWORD)*BYTE_BIT
%define QWORD_BIT		64		; sizeof(QWORD)*BYTE_BIT
%define TBYTE_BIT		80		; sizeof(TBYTE)*BYTE_BIT
%define TWORD_BIT		80		; sizeof(TWORD)*BYTE_BIT
%define DQWORD_BIT		128		; sizeof(DQWORD)*BYTE_BIT

%idefine TBYTE	TWORD	; NASM uses the keyword 'TWORD' instead of 'TBYTE'
%idefine DQWORD		; currently not supported by NASM
%idefine _MMWORD	;
%idefine _DWORD		;

; --------------------------------------------------------------------------
;  External Symbol Name
;
%ifndef EXTN
%define EXTN(name)   _ %+ name		; foo() -> _foo
%endif

; --------------------------------------------------------------------------
;  Macros for position-independent code (PIC) support
;
%ifndef GOT_SYMBOL
%undef PIC
%endif

%ifdef PIC ; -------------------------------------------

%ifidn GOT_SYMBOL,_MACHO_PIC_ ; --------------------

; At present, nasm doesn't seem to support PIC generation for Mach-O.
; The PIC support code below is a little tricky.

	SECTION	SEG_CONST
const_base:

%define GOTOFF(got,sym) (got) + (sym) - const_base

%imacro get_GOT	1
	; NOTE: this macro destroys ecx resister.
	call	%%geteip
	add	ecx, byte (%%ref - $)
	jmp	short %%adjust
%%geteip:
	mov	ecx, POINTER [esp]
	ret
%%adjust:
	push	ebp
	xor	ebp,ebp		; ebp = 0
%ifidni %1,ebx	; (%1 == ebx)
	; db 0x8D,0x9C + jmp near const_base =
	;   lea ebx, [ecx+ebp*8+(const_base-%%ref)] ; 8D,9C,E9,(offset32)
	db	0x8D,0x9C		; 8D,9C
	jmp	near const_base		; E9,(const_base-%%ref)
%%ref:
%else  ; (%1 != ebx)
	; db 0x8D,0x8C + jmp near const_base =
	;   lea ecx, [ecx+ebp*8+(const_base-%%ref)] ; 8D,8C,E9,(offset32)
	db	0x8D,0x8C		; 8D,8C
	jmp	near const_base		; E9,(const_base-%%ref)
%%ref:	mov	%1, ecx
%endif ; (%1 == ebx)
	pop	ebp
%endmacro

%else	; GOT_SYMBOL != _MACHO_PIC_ ----------------

%define GOTOFF(got,sym) (got) + (sym) wrt ..gotoff

%imacro get_GOT	1
	extern	GOT_SYMBOL
	call	%%geteip
	add	%1, GOT_SYMBOL + $$ - $ wrt ..gotpc
	jmp	short %%done
%%geteip:
	mov	%1, POINTER [esp]
	ret
%%done:
%endmacro

%endif	; GOT_SYMBOL == _MACHO_PIC_ ----------------

%imacro pushpic	1.nolist
	push	%1
%endmacro
%imacro poppic	1.nolist
	pop	%1
%endmacro
%imacro movpic	2.nolist
	mov	%1,%2
%endmacro

%else	; !PIC -----------------------------------------

%define GOTOFF(got,sym) (sym)

%imacro get_GOT	1.nolist
%endmacro
%imacro pushpic	1.nolist
%endmacro
%imacro poppic	1.nolist
%endmacro
%imacro movpic	2.nolist
%endmacro

%endif	;  PIC -----------------------------------------

; --------------------------------------------------------------------------
;  Align the next instruction on {2,4,8,16,..}-byte boundary.
;  ".balign n,,m" in GNU as
;
%define MSKLE(x,y)  (~(((y) & 0xFFFF) - ((x) & 0xFFFF)) >> 16)
%define FILLB(b,n)  (($$-(b)) & ((n)-1))

%imacro alignx 1-2.nolist 0xFFFF
%%bs:	times MSKLE(FILLB(%%bs,%1),%2) & MSKLE(16,FILLB($,%1)) & FILLB($,%1) \
	       db 0x90                               ; nop
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/9 \
	       db 0x8D,0x9C,0x23,0x00,0x00,0x00,0x00 ; lea ebx,[ebx+0x00000000]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/7 \
	       db 0x8D,0xAC,0x25,0x00,0x00,0x00,0x00 ; lea ebp,[ebp+0x00000000]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/6 \
	       db 0x8D,0xAD,0x00,0x00,0x00,0x00      ; lea ebp,[ebp+0x00000000]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/4 \
	       db 0x8D,0x6C,0x25,0x00                ; lea ebp,[ebp+0x00]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/3 \
	       db 0x8D,0x6D,0x00                     ; lea ebp,[ebp+0x00]
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/2 \
	       db 0x8B,0xED                          ; mov ebp,ebp
	times MSKLE(FILLB(%%bs,%1),%2) & FILLB($,%1)/1 \
	       db 0x90                               ; nop
%endmacro

; Align the next data on {2,4,8,16,..}-byte boundary.
;
%imacro alignz 1.nolist
	align %1, db 0		; filling zeros
%endmacro

; --------------------------------------------------------------------------

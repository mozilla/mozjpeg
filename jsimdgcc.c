/*
 * jsimdgcc.c - SIMD instruction support check (gcc)
 *
 * x86 SIMD extension for IJG JPEG library
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 * For conditions of distribution and use, see copyright notice in jsimdext.inc
 *
 * Last Modified : January 24, 2006
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"

#include <setjmp.h>
#include <signal.h>


static volatile int lockf /* = 0 */;
static jmp_buf jmpbuf;


/*
 * Exception handler for signal()
 */

LOCAL(void)
exception_handler (int sig)
{
  signal(SIGILL, SIG_DFL);
  longjmp(jmpbuf, 1);
}


/*
 * Check if the OS supports SIMD instructions
 */

GLOBAL(unsigned int)
jpeg_simd_os_support (unsigned int simd)
{
#ifdef __GNUC__		/* gcc (i386) */
  unsigned int mxcsr = 0x1F80;

  /* enter critical section */
  __asm__ __volatile__ (
  "get_lock:                  \n\t"
    "movl  $1,%%eax           \n\t"
    "xchgl %0,%%eax           \n\t"	/* try to get lock */
    "cmpl  $0,%%eax           \n\t"	/* test if successful */
    "je    critical_section   \n"
  "spin_loop:                 \n\t"
  /*".byte 0xF3,0x90          \n\t"*/	/* "pause" on P4 (short delay) */
    "cmpl  $0,%0              \n\t"	/* check if lock is free */
    "jne   spin_loop          \n\t"
    "jmp   get_lock           \n"
  "critical_section:          \n\t"
     : "=m" (lockf) : "m" (lockf) : "%eax"
  );

  /* If floating point emulation is enabled (CR0.EM = 1),
   * executing an MMX/3DNow! instruction generates invalid
   * opcode exception (#UD).
   */
  if (simd & (JSIMD_MMX | JSIMD_3DNOW)) {
    if (!setjmp(jmpbuf)) {
      signal(SIGILL, exception_handler);
      __asm__ __volatile__ (
        ".byte 0x0F,0x77"		/* emms */
      );
      signal(SIGILL, SIG_DFL);
    } else {
      simd &= ~(JSIMD_MMX | JSIMD_3DNOW);
    }
  }
  if (simd & (JSIMD_SSE | JSIMD_SSE2)) {
    if (!setjmp(jmpbuf)) {
      signal(SIGILL, exception_handler);
      __asm__ __volatile__ (
        "leal  %0,%%eax        \n\t"
        ".byte 0x0F,0xAE,0x10  \n\t"	/* ldmxcsr [eax] */
         : : "m" (mxcsr) : "%eax"
      );
      signal(SIGILL, SIG_DFL);
    } else {
      simd &= ~(JSIMD_SSE | JSIMD_SSE2);
    }
  }

  /* leave critical section */
  lockf = 0;	/* release lock */
#endif	/* __GNUC__ */

  return simd;
}

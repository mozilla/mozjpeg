/*
 * jdarith.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains arithmetic entropy decoding routines.
 * These routines are invoked via the methods entropy_decode
 * and entropy_decode_init/term.
 */

#include "jinclude.h"

#ifdef D_ARITH_CODING_SUPPORTED


/*
 * The arithmetic coding option of the JPEG standard specifies Q-coding,
 * which is covered by patents held by IBM (and possibly AT&T and Mitsubishi).
 * At this time it does not appear to be legal for the Independent JPEG
 * Group to distribute software that implements arithmetic coding.
 * We have therefore removed arithmetic coding support from the
 * distributed source code.
 *
 * We're not happy about it either.
 */


/*
 * The method selection routine for arithmetic entropy decoding.
 */

GLOBAL void
jseldarithmetic (decompress_info_ptr cinfo)
{
  if (cinfo->arith_code) {
    ERREXIT(cinfo->emethods, "Sorry, there are legal restrictions on arithmetic coding");
  }
}

#endif /* D_ARITH_CODING_SUPPORTED */

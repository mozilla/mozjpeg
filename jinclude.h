/*
 * jinclude.h
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This is the central file that's #include'd by all the JPEG .c files.
 * Its purpose is to provide a single place to fix any problems with
 * including the wrong system include files.
 * You can edit these declarations if you use a system with nonstandard
 * system include files.
 */


/*
 * <stdio.h> is included to get the FILE typedef and NULL macro.
 * Note that the core portable-JPEG files do not actually do any I/O
 * using the stdio library; only the user interface, error handler,
 * and file reading/writing modules invoke any stdio functions.
 * (Well, we did cheat a bit in jvirtmem.c, but only if MEM_STATS is defined.)
 */

#include <stdio.h>

/*
 * We need the size_t typedef, which defines the parameter type of malloc().
 * In an ANSI-conforming implementation this is provided by <stdio.h>,
 * but on non-ANSI systems it's more likely to be in <sys/types.h>.
 */

#ifndef __STDC__		/* shouldn't need this if __STDC__ */
#include <sys/types.h>
#endif

/*
 * In ANSI C, and indeed any rational implementation, size_t is also the
 * type returned by sizeof().  However, it seems there are some irrational
 * implementations out there, in which sizeof() returns an int even though
 * size_t is defined as long or unsigned long.  To ensure consistent results
 * we always use this SIZEOF() macro in place of using sizeof() directly.
 */

#define SIZEOF(object)	((size_t) sizeof(object))

/*
 * We need the memcpy() and strcmp() functions, plus memory zeroing.
 * ANSI and System V implementations declare these in <string.h>.
 * BSD doesn't have the mem() functions, but it does have bcopy()/bzero().
 * NOTE: we assume the size parameters to these functions are of type size_t.
 * Insert casts in these macros if not!
 */

#ifdef __STDC__
#include <string.h>
#define MEMZERO(voidptr,size)	memset((voidptr), 0, (size))
#else /* not STDC */
#ifdef BSD
#include <strings.h>
#define MEMZERO(voidptr,size)	bzero((voidptr), (size))
#define memcpy(dest,src,size)	bcopy((src), (dest), (size))
#else /* not BSD, assume Sys V or compatible */
#include <string.h>
#define MEMZERO(voidptr,size)	memset((voidptr), 0, (size))
#endif /* BSD */
#endif /* STDC */


/* Now include the portable JPEG definition files. */

#include "jconfig.h"

#include "jpegdata.h"

/*
 * jinclude.h
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
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
 * Normally the __STDC__ macro can be taken as indicating that the system
 * include files conform to the ANSI C standard.  However, if you are running
 * GCC on a machine with non-ANSI system include files, that is not the case.
 * In that case change the following, or add -DNONANSI_INCLUDES to your CFLAGS.
 */

#ifdef __STDC__
#ifndef NONANSI_INCLUDES
#define INCLUDES_ARE_ANSI	/* this is what's tested before including */
#endif
#endif

/*
 * <stdio.h> is included to get the FILE typedef and NULL macro.
 * Note that the core portable-JPEG files do not actually do any I/O
 * using the stdio library; only the user interface, error handler,
 * and file reading/writing modules invoke any stdio functions.
 * (Well, we did cheat a bit in jmemmgr.c, but only if MEM_STATS is defined.)
 */

#include <stdio.h>

/*
 * We need the size_t typedef, which defines the parameter type of malloc().
 * In an ANSI-conforming implementation this is provided by <stdio.h>,
 * but on non-ANSI systems it's more likely to be in <sys/types.h>.
 * On some not-quite-ANSI systems you may find it in <stddef.h>.
 */

#ifndef INCLUDES_ARE_ANSI	/* shouldn't need this if ANSI C */
#include <sys/types.h>
#endif
#ifdef __SASC			/* Amiga SAS C provides it in stddef.h. */
#include <stddef.h>
#endif

/*
 * In ANSI C, and indeed any rational implementation, size_t is also the
 * type returned by sizeof().  However, it seems there are some irrational
 * implementations out there, in which sizeof() returns an int even though
 * size_t is defined as long or unsigned long.  To ensure consistent results
 * we always use this SIZEOF() macro in place of using sizeof() directly.
 */

#undef SIZEOF			/* in case you included X11/xmd.h */
#define SIZEOF(object)	((size_t) sizeof(object))

/*
 * fread() and fwrite() are always invoked through these macros.
 * On some systems you may need to twiddle the argument casts.
 * CAUTION: argument order is different from underlying functions!
 */

#define JFREAD(file,buf,sizeofbuf)  \
  ((size_t) fread((void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))
#define JFWRITE(file,buf,sizeofbuf)  \
  ((size_t) fwrite((const void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))

/*
 * We need the memcpy() and strcmp() functions, plus memory zeroing.
 * ANSI and System V implementations declare these in <string.h>.
 * BSD doesn't have the mem() functions, but it does have bcopy()/bzero().
 * Some systems may declare memset and memcpy in <memory.h>.
 *
 * NOTE: we assume the size parameters to these functions are of type size_t.
 * Change the casts in these macros if not!
 */

#ifdef INCLUDES_ARE_ANSI
#include <string.h>
#define MEMZERO(target,size)	memset((void *)(target), 0, (size_t)(size))
#define MEMCOPY(dest,src,size)	memcpy((void *)(dest), (const void *)(src), (size_t)(size))
#else /* not ANSI */
#ifdef BSD
#include <strings.h>
#define MEMZERO(target,size)	bzero((void *)(target), (size_t)(size))
#define MEMCOPY(dest,src,size)	bcopy((const void *)(src), (void *)(dest), (size_t)(size))
#else /* not BSD, assume Sys V or compatible */
#include <string.h>
#define MEMZERO(target,size)	memset((void *)(target), 0, (size_t)(size))
#define MEMCOPY(dest,src,size)	memcpy((void *)(dest), (const void *)(src), (size_t)(size))
#endif /* BSD */
#endif /* ANSI */


/* Now include the portable JPEG definition files. */

#include "jconfig.h"

#include "jpegdata.h"

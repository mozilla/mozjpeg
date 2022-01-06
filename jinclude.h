/*
 * jinclude.h
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1991-1994, Thomas G. Lane.
 * It was modified by The libjpeg-turbo Project to include only code relevant
 * to libjpeg-turbo.
 * For conditions of distribution and use, see the accompanying README.ijg
 * file.
 *
 * This file exists to provide a single place to fix any problems with
 * including the wrong system include files.  (Common problems are taken
 * care of by the standard jconfig symbols, but on really weird systems
 * you may have to edit this file.)
 *
 * NOTE: this file is NOT intended to be included by applications using the
 * JPEG library.  Most applications need only include jpeglib.h.
 */


/* Include auto-config file to find out which system include files we need. */

#include "jconfig.h"            /* auto configuration options */
#define JCONFIG_INCLUDED        /* so that jpeglib.h doesn't do it again */

/*
 * Note that the core JPEG library does not require <stdio.h>;
 * only the default error handler and data source/destination modules do.
 * But we must pull it in because of the references to FILE in jpeglib.h.
 * You can remove those references if you want to compile without <stdio.h>.
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * The modules that use fread() and fwrite() always invoke them through
 * these macros.  On some systems you may need to twiddle the argument casts.
 * CAUTION: argument order is different from underlying functions!
 */

#define JFREAD(file, buf, sizeofbuf) \
  ((size_t)fread((void *)(buf), (size_t)1, (size_t)(sizeofbuf), (file)))
#define JFWRITE(file, buf, sizeofbuf) \
  ((size_t)fwrite((const void *)(buf), (size_t)1, (size_t)(sizeofbuf), (file)))

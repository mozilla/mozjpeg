/*
 * jmemname.c  (jmemsys.c)
 *
 * Copyright (C) 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides a generic implementation of the system-dependent
 * portion of the JPEG memory manager.  This implementation assumes that
 * you must explicitly construct a name for each temp file.
 * Also, the problem of determining the amount of memory available
 * is shoved onto the user.
 */

#include "jinclude.h"
#include "jmemsys.h"

#ifdef INCLUDES_ARE_ANSI
#include <stdlib.h>		/* to declare malloc(), free() */
#else
extern void * malloc PP((size_t size));
extern void free PP((void *ptr));
#endif

#ifndef SEEK_SET		/* pre-ANSI systems may not define this; */
#define SEEK_SET  0		/* if not, assume 0 is correct */
#endif

#ifdef DONT_USE_B_MODE		/* define mode parameters for fopen() */
#define READ_BINARY	"r"
#define RW_BINARY	"w+"
#else
#define READ_BINARY	"rb"
#define RW_BINARY	"w+b"
#endif


static external_methods_ptr methods; /* saved for access to error_exit */

static long total_used;		/* total memory requested so far */


/*
 * Selection of a file name for a temporary file.
 * This is system-dependent!
 *
 * The code as given is suitable for most Unix systems, and it is easily
 * modified for most non-Unix systems.  Some notes:
 *  1.  The temp file is created in the directory named by TEMP_DIRECTORY.
 *      The default value is /usr/tmp, which is the conventional place for
 *      creating large temp files on Unix.  On other systems you'll probably
 *      want to change the file location.  You can do this by editing the
 *      #define, or by defining TEMP_DIRECTORY in CFLAGS in the Makefile.
 *      For example, you might say
 *          CFLAGS= ... '-DTEMP_DIRECTORY="/tmp/"'
 *      Note that double quotes are needed in the text of the macro.
 *      With most make systems you have to put single quotes around the
 *      -D construct to preserve the double quotes.
 *	(Amiga SAS C has trouble with ":" and such in command-line options,
 *	so we've put in a special case for the preferred Amiga temp directory.)
 *
 *  2.  If you need to change the file name as well as its location,
 *      you can override the TEMP_FILE_NAME macro.  (Note that this is
 *      actually a printf format string; it must contain %s and %d.)
 *      Few people should need to do this.
 *
 *  3.  mktemp() is used to ensure that multiple processes running
 *      simultaneously won't select the same file names.  If your system
 *      doesn't have mktemp(), define NO_MKTEMP to do it the hard way.
 *
 *  4.  You probably want to define NEED_SIGNAL_CATCHER so that jcmain/jdmain
 *      will cause the temp files to be removed if you stop the program early.
 */

#ifndef TEMP_DIRECTORY		/* so can override from Makefile */
#ifdef AMIGA
#define TEMP_DIRECTORY  "JPEGTMP:"  /* recommended setting for Amiga */
#else
#define TEMP_DIRECTORY  "/usr/tmp/" /* recommended setting for Unix */
#endif
#endif

static int next_file_num;	/* to distinguish among several temp files */

#ifdef NO_MKTEMP

#ifndef TEMP_FILE_NAME		/* so can override from Makefile */
#define TEMP_FILE_NAME  "%sJPG%03d.TMP"
#endif

LOCAL void
select_file_name (char * fname)
{
  FILE * tfile;

  /* Keep generating file names till we find one that's not in use */
  for (;;) {
    next_file_num++;		/* advance counter */
    sprintf(fname, TEMP_FILE_NAME, TEMP_DIRECTORY, next_file_num);
    if ((tfile = fopen(fname, READ_BINARY)) == NULL)
      break;
    fclose(tfile);		/* oops, it's there; close tfile & try again */
  }
}

#else /* ! NO_MKTEMP */

/* Note that mktemp() requires the initial filename to end in six X's */
#ifndef TEMP_FILE_NAME		/* so can override from Makefile */
#define TEMP_FILE_NAME  "%sJPG%dXXXXXX"
#endif

LOCAL void
select_file_name (char * fname)
{
  next_file_num++;		/* advance counter */
  sprintf(fname, TEMP_FILE_NAME, TEMP_DIRECTORY, next_file_num);
  mktemp(fname);		/* make sure file name is unique */
  /* mktemp replaces the trailing XXXXXX with a unique string of characters */
}

#endif /* NO_MKTEMP */


/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */

GLOBAL void *
jget_small (size_t sizeofobject)
{
  total_used += sizeofobject;
  return (void *) malloc(sizeofobject);
}

GLOBAL void
jfree_small (void * object)
{
  free(object);
}

/*
 * We assume NEED_FAR_POINTERS is not defined and so the separate entry points
 * jget_large, jfree_large are not needed.
 */


/*
 * This routine computes the total memory space available for allocation.
 * It's impossible to do this in a portable way; our current solution is
 * to make the user tell us (with a default value set at compile time).
 * If you can actually get the available space, it's a good idea to subtract
 * a slop factor of 5% or so.
 */

#ifndef DEFAULT_MAX_MEM		/* so can override from makefile */
#define DEFAULT_MAX_MEM		1000000L /* default: one megabyte */
#endif

GLOBAL long
jmem_available (long min_bytes_needed, long max_bytes_needed)
{
  return methods->max_memory_to_use - total_used;
}


/*
 * Backing store (temporary file) management.
 * Backing store objects are only used when the value returned by
 * jmem_available is less than the total space needed.  You can dispense
 * with these routines if you have plenty of virtual memory; see jmemnobs.c.
 */


METHODDEF void
read_backing_store (backing_store_ptr info, void FAR * buffer_address,
		    long file_offset, long byte_count)
{
  if (fseek(info->temp_file, file_offset, SEEK_SET))
    ERREXIT(methods, "fseek failed on temporary file");
  if (JFREAD(info->temp_file, buffer_address, byte_count)
      != (size_t) byte_count)
    ERREXIT(methods, "fread failed on temporary file");
}


METHODDEF void
write_backing_store (backing_store_ptr info, void FAR * buffer_address,
		     long file_offset, long byte_count)
{
  if (fseek(info->temp_file, file_offset, SEEK_SET))
    ERREXIT(methods, "fseek failed on temporary file");
  if (JFWRITE(info->temp_file, buffer_address, byte_count)
      != (size_t) byte_count)
    ERREXIT(methods, "fwrite failed on temporary file --- out of disk space?");
}


METHODDEF void
close_backing_store (backing_store_ptr info)
{
  fclose(info->temp_file);	/* close the file */
  unlink(info->temp_name);	/* delete the file */
/* If your system doesn't have unlink(), use remove() instead.
 * remove() is the ANSI-standard name for this function, but if
 * your system was ANSI you'd be using jmemansi.c, right?
 */
}


GLOBAL void
jopen_backing_store (backing_store_ptr info, long total_bytes_needed)
{
  char tracemsg[TEMP_NAME_LENGTH+40];

  select_file_name(info->temp_name);
  if ((info->temp_file = fopen(info->temp_name, RW_BINARY)) == NULL) {
    /* hack to get around ERREXIT's inability to handle string parameters */
    sprintf(tracemsg, "Failed to create temporary file %s", info->temp_name);
    ERREXIT(methods, tracemsg);
  }
  info->read_backing_store = read_backing_store;
  info->write_backing_store = write_backing_store;
  info->close_backing_store = close_backing_store;
  /* hack to get around TRACEMS' inability to handle string parameters */
  sprintf(tracemsg, "Using temp file %s", info->temp_name);
  TRACEMS(methods, 1, tracemsg);
}


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.  Keep in mind that jmem_term may be called more than
 * once.
 */

GLOBAL void
jmem_init (external_methods_ptr emethods)
{
  methods = emethods;		/* save struct addr for error exit access */
  emethods->max_memory_to_use = DEFAULT_MAX_MEM;
  total_used = 0;
  next_file_num = 0;
}

GLOBAL void
jmem_term (void)
{
  /* no work */
}

/*
 * jmemnobs.c  (jmemsys.c)
 *
 * Copyright (C) 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides a really simple implementation of the system-
 * dependent portion of the JPEG memory manager.  This implementation
 * assumes that no backing-store files are needed: all required space
 * can be obtained from malloc().
 * This is very portable in the sense that it'll compile on almost anything,
 * but you'd better have lots of main memory (or virtual memory) if you want
 * to process big images.
 * Note that the max_memory_to_use option is ignored by this implementation.
 */

#include "jinclude.h"
#include "jmemsys.h"

#ifdef INCLUDES_ARE_ANSI
#include <stdlib.h>		/* to declare malloc(), free() */
#else
extern void * malloc PP((size_t size));
extern void free PP((void *ptr));
#endif


static external_methods_ptr methods; /* saved for access to error_exit */


/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */

GLOBAL void *
jget_small (size_t sizeofobject)
{
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
 * Here we always say, "we got all you want bud!"
 */

GLOBAL long
jmem_available (long min_bytes_needed, long max_bytes_needed)
{
  return max_bytes_needed;
}


/*
 * Backing store (temporary file) management.
 * This should never be called and we just error out.
 */

GLOBAL void
jopen_backing_store (backing_store_ptr info, long total_bytes_needed)
{
  ERREXIT(methods, "Backing store not supported");
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
  emethods->max_memory_to_use = 0;
}

GLOBAL void
jmem_term (void)
{
  /* no work */
}

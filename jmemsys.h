/*
 * jmemsys.h
 *
 * Copyright (C) 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This include file defines the interface between the system-independent
 * and system-dependent portions of the JPEG memory manager.  (The system-
 * independent portion is jmemmgr.c; there are several different versions
 * of the system-dependent portion, and of this file for that matter.)
 *
 * This is a "generic" skeleton that may need to be modified for particular
 * systems.  It should be usable as-is on the majority of non-MSDOS machines.
 */


/*
 * These two functions are used to allocate and release small chunks of
 * memory (typically the total amount requested through jget_small is
 * no more than 20Kb or so).  Behavior should be the same as for the
 * standard library functions malloc and free; in particular, jget_small
 * returns NULL on failure.  On most systems, these ARE malloc and free.
 * On an 80x86 machine using small-data memory model, these manage near heap.
 */

EXTERN void * jget_small PP((size_t sizeofobject));
EXTERN void jfree_small PP((void * object));

/*
 * These two functions are used to allocate and release large chunks of
 * memory (up to the total free space designated by jmem_available).
 * The interface is the same as above, except that on an 80x86 machine,
 * far pointers are used.  On other systems these ARE the same as above.
 */

#ifdef NEED_FAR_POINTERS	/* typically not needed except on 80x86 */
EXTERN void FAR * jget_large PP((size_t sizeofobject));
EXTERN void jfree_large PP((void FAR * object));
#else
#define jget_large(sizeofobject)	jget_small(sizeofobject)
#define jfree_large(object)		jfree_small(object)
#endif

/*
 * The macro MAX_ALLOC_CHUNK designates the maximum number of bytes that may
 * be requested in a single call on jget_large (and jget_small for that
 * matter, but that case should never come into play).  This macro is needed
 * to model the 64Kb-segment-size limit of far addressing on 80x86 machines.
 * On machines with flat address spaces, any large constant may be used here.
 */

#define MAX_ALLOC_CHUNK		1000000000L

/*
 * This routine computes the total space available for allocation by
 * jget_large.  If more space than this is needed, backing store will be used.
 * NOTE: any memory already allocated must not be counted.
 *
 * There is a minimum space requirement, corresponding to the minimum
 * feasible buffer sizes; jmemmgr.c will request that much space even if
 * jmem_available returns zero.  The maximum space needed, enough to hold
 * all working storage in memory, is also passed in case it is useful.
 *
 * It is OK for jmem_available to underestimate the space available (that'll
 * just lead to more backing-store access than is really necessary).
 * However, an overestimate will lead to failure.  Hence it's wise to subtract
 * a slop factor from the true available space, especially if jget_small space
 * comes from the same pool.  5% should be enough.
 *
 * On machines with lots of virtual memory, any large constant may be returned.
 * Conversely, zero may be returned to always use the minimum amount of memory.
 */

EXTERN long jmem_available PP((long min_bytes_needed, long max_bytes_needed));


/*
 * This structure holds whatever state is needed to access a single
 * backing-store object.  The read/write/close method pointers are called
 * by jmemmgr.c to manipulate the backing-store object; all other fields
 * are private to the system-dependent backing store routines.
 */

#define TEMP_NAME_LENGTH   64	/* max length of a temporary file's name */

typedef struct backing_store_struct * backing_store_ptr;

typedef struct backing_store_struct {
	/* Methods for reading/writing/closing this backing-store object */
	METHOD(void, read_backing_store, (backing_store_ptr info,
					  void FAR * buffer_address,
					  long file_offset, long byte_count));
	METHOD(void, write_backing_store, (backing_store_ptr info,
					   void FAR * buffer_address,
					   long file_offset, long byte_count));
	METHOD(void, close_backing_store, (backing_store_ptr info));
	/* Private fields for system-dependent backing-store management */
	/* For a typical implementation with temp files, we might need: */
	FILE * temp_file;	/* stdio reference to temp file */
	char temp_name[TEMP_NAME_LENGTH]; /* name of temp file */
      } backing_store_info;

/*
 * Initial opening of a backing-store object.  This must fill in the
 * read/write/close pointers in the object.  The read/write routines
 * may take an error exit if the specified maximum file size is exceeded.
 * (If jmem_available always returns a large value, this routine can just
 * take an error exit.)
 */

EXTERN void jopen_backing_store PP((backing_store_ptr info,
				    long total_bytes_needed));


/*
 * These routines take care of any system-dependent initialization and
 * cleanup required.  The system methods struct address should be saved
 * by jmem_init in case an error exit must be taken.  jmem_term may assume
 * that all requested memory has been freed and that all opened backing-
 * store objects have been closed.
 * NB: jmem_term may be called more than once, and must behave reasonably
 * if that happens.
 */

EXTERN void jmem_init PP((external_methods_ptr emethods));
EXTERN void jmem_term PP((void));

/*
 * jvirtmem.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides the system-dependent memory allocation routines
 * for the case where we can rely on virtual memory to handle large arrays.
 *
 * This includes some MS-DOS code just for trial purposes; "big" arrays will
 * have to be handled with temp files on MS-DOS, so a real implementation of
 * a DOS memory manager will probably be a separate file.  (See additional
 * comments about big arrays, below.)
 * 
 * NB: allocation routines never return NULL.
 * They should exit to error_exit if unsuccessful.
 */

#define AM_MEMORY_MANAGER	/* we define big_Xarray_control structs */

#include "jinclude.h"

#ifdef INCLUDES_ARE_ANSI
#include <stdlib.h>		/* to declare malloc(), free() */
#else
extern void * malloc PP((size_t size));
extern void free PP((void *ptr));
#endif


/* Insert system-specific definitions of far_malloc, far_free here. */

#ifndef NEED_FAR_POINTERS	/* Generic for non-braindamaged CPUs */

#define far_malloc(x)	malloc(x)
#define far_free(x)	free(x)

#else /* NEED_FAR_POINTERS */

#ifdef __TURBOC__
/* These definitions work for Turbo C */
#include <alloc.h>		/* need farmalloc(), farfree() */
#define far_malloc(x)	farmalloc(x)
#define far_free(x)	farfree(x)
#else
#ifdef MSDOS
/* These definitions work for Microsoft C and compatible compilers */
#include <malloc.h>		/* need _fmalloc(), _ffree() */
#define far_malloc(x)	_fmalloc(x)
#define far_free(x)	_ffree(x)
#endif
#endif

#endif /* NEED_FAR_POINTERS */

/*
 * When allocating 2-D arrays we can either ask malloc() for each row
 * individually, or grab the whole space in one chunk.  The latter is
 * a lot faster on large arrays, but fails if malloc can't handle big
 * requests, as is typically true on MS-DOS.
 * We assume here that big malloc requests are safe whenever
 * NEED_FAR_POINTERS is not defined, but you can change this if you are
 * on a weird machine.
 */

#ifndef NEED_FAR_POINTERS
#define BIG_MALLOCS_OK		/* safe to ask far_malloc for > 64Kb */
#endif


/*
 * Some important notes:
 *   The array alloc/dealloc routines are not merely a convenience;
 *   on 80x86 machines the bottom-level pointers in an array are FAR
 *   and thus may not be allocatable by alloc_small.
 *
 *   Also, it's not a good idea to try to merge the sarray and barray
 *   routines, even though they are textually almost the same, because
 *   samples are usually stored as bytes while coefficients are shorts.
 *   Thus, in machines where byte pointers have a different representation
 *   from word pointers, the resulting machine code could not be the same.
 */


static external_methods_ptr methods; /* saved for access to error_exit */


#ifdef MEM_STATS		/* optional extra stuff for statistics */

#define MALLOC_OVERHEAD  (SIZEOF(char *)) /* assumed overhead per request */
#define MALLOC_FAR_OVERHEAD  (SIZEOF(char FAR *)) /* for "far" storage */

static long total_num_small = 0;	/* total # of small objects alloced */
static long total_bytes_small = 0;	/* total bytes requested */
static long cur_num_small = 0;		/* # currently alloced */
static long max_num_small = 0;		/* max simultaneously alloced */

#ifdef NEED_FAR_POINTERS
static long total_num_medium = 0;	/* total # of medium objects alloced */
static long total_bytes_medium = 0;	/* total bytes requested */
static long cur_num_medium = 0;		/* # currently alloced */
static long max_num_medium = 0;		/* max simultaneously alloced */
#endif

static long total_num_sarray = 0;	/* total # of sarray objects alloced */
static long total_bytes_sarray = 0;	/* total bytes requested */
static long cur_num_sarray = 0;		/* # currently alloced */
static long max_num_sarray = 0;		/* max simultaneously alloced */

static long total_num_barray = 0;	/* total # of barray objects alloced */
static long total_bytes_barray = 0;	/* total bytes requested */
static long cur_num_barray = 0;		/* # currently alloced */
static long max_num_barray = 0;		/* max simultaneously alloced */


GLOBAL void
j_mem_stats (void)
{
  /* since this is only a debugging stub, we can cheat a little on the
   * trace message mechanism... helps 'cuz trace can't handle longs.
   */
  fprintf(stderr, "total_num_small = %ld\n", total_num_small);
  fprintf(stderr, "total_bytes_small = %ld\n", total_bytes_small);
  if (cur_num_small)
    fprintf(stderr, "CUR_NUM_SMALL = %ld\n", cur_num_small);
  fprintf(stderr, "max_num_small = %ld\n", max_num_small);
  
#ifdef NEED_FAR_POINTERS
  fprintf(stderr, "total_num_medium = %ld\n", total_num_medium);
  fprintf(stderr, "total_bytes_medium = %ld\n", total_bytes_medium);
  if (cur_num_medium)
    fprintf(stderr, "CUR_NUM_MEDIUM = %ld\n", cur_num_medium);
  fprintf(stderr, "max_num_medium = %ld\n", max_num_medium);
#endif
  
  fprintf(stderr, "total_num_sarray = %ld\n", total_num_sarray);
  fprintf(stderr, "total_bytes_sarray = %ld\n", total_bytes_sarray);
  if (cur_num_sarray)
    fprintf(stderr, "CUR_NUM_SARRAY = %ld\n", cur_num_sarray);
  fprintf(stderr, "max_num_sarray = %ld\n", max_num_sarray);
  
  fprintf(stderr, "total_num_barray = %ld\n", total_num_barray);
  fprintf(stderr, "total_bytes_barray = %ld\n", total_bytes_barray);
  if (cur_num_barray)
    fprintf(stderr, "CUR_NUM_BARRAY = %ld\n", cur_num_barray);
  fprintf(stderr, "max_num_barray = %ld\n", max_num_barray);
}

#endif /* MEM_STATS */


LOCAL void
out_of_memory (int which)
/* Report an out-of-memory error and stop execution */
/* If we compiled MEM_STATS support, report alloc requests before dying */
{
#ifdef MEM_STATS
  j_mem_stats();
#endif
  ERREXIT1(methods, "Insufficient memory (case %d)", which);
}



METHODDEF void *
alloc_small (size_t sizeofobject)
/* Allocate a "small" (all-in-memory) object */
{
  void * result;

#ifdef MEM_STATS
  total_num_small++;
  total_bytes_small += sizeofobject + MALLOC_OVERHEAD;
  cur_num_small++;
  if (cur_num_small > max_num_small) max_num_small = cur_num_small;
#endif

  result = malloc(sizeofobject);
  if (result == NULL)
    out_of_memory(1);
  return result;
}


METHODDEF void
free_small (void *ptr)
/* Free a "small" (all-in-memory) object */
{
  free(ptr);

#ifdef MEM_STATS
  cur_num_small--;
#endif
}


#ifdef NEED_FAR_POINTERS

METHODDEF void FAR *
alloc_medium (size_t sizeofobject)
/* Allocate a "medium" (all in memory, but in far heap) object */
{
  void FAR * result;

#ifdef MEM_STATS
  total_num_medium++;
  total_bytes_medium += sizeofobject + MALLOC_FAR_OVERHEAD;
  cur_num_medium++;
  if (cur_num_medium > max_num_medium) max_num_medium = cur_num_medium;
#endif

  result = far_malloc(sizeofobject);
  if (result == NULL)
    out_of_memory(2);
  return result;
}


METHODDEF void
free_medium (void FAR *ptr)
/* Free a "medium" (all in memory, but in far heap) object */
{
  far_free(ptr);

#ifdef MEM_STATS
  cur_num_medium--;
#endif
}

#endif /* NEED_FAR_POINTERS */


METHODDEF JSAMPARRAY
alloc_small_sarray (long samplesperrow, long numrows)
/* Allocate a "small" (all-in-memory) 2-D sample array */
{
  JSAMPARRAY result;
#ifdef BIG_MALLOCS_OK
  JSAMPROW workspace;
#endif
  long i;

#ifdef MEM_STATS
  total_num_sarray++;
#ifdef BIG_MALLOCS_OK
  total_bytes_sarray += numrows * samplesperrow * SIZEOF(JSAMPLE)
			+ MALLOC_FAR_OVERHEAD;
#else
  total_bytes_sarray += (samplesperrow * SIZEOF(JSAMPLE) + MALLOC_FAR_OVERHEAD)
			* numrows;
#endif
  cur_num_sarray++;
  if (cur_num_sarray > max_num_sarray) max_num_sarray = cur_num_sarray;
#endif

  /* Get space for row pointers; this is always "near" on 80x86 */
  result = (JSAMPARRAY) alloc_small((size_t) (numrows * SIZEOF(JSAMPROW)));

  /* Get the rows themselves; on 80x86 these are "far" */

#ifdef BIG_MALLOCS_OK
  workspace = (JSAMPROW) far_malloc((size_t)
				(numrows * samplesperrow * SIZEOF(JSAMPLE)));
  if (workspace == NULL)
    out_of_memory(3);
  for (i = 0; i < numrows; i++) {
    result[i] = workspace;
    workspace += samplesperrow;
  }
#else
  for (i = 0; i < numrows; i++) {
    result[i] = (JSAMPROW) far_malloc((size_t)
				      (samplesperrow * SIZEOF(JSAMPLE)));
    if (result[i] == NULL)
      out_of_memory(3);
  }
#endif

  return result;
}


METHODDEF void
free_small_sarray (JSAMPARRAY ptr, long numrows)
/* Free a "small" (all-in-memory) 2-D sample array */
{
  /* Free the rows themselves; on 80x86 these are "far" */
#ifdef BIG_MALLOCS_OK
  far_free((void FAR *) ptr[0]);
#else
  long i;

  for (i = 0; i < numrows; i++) {
    far_free((void FAR *) ptr[i]);
  }
#endif

  /* Free space for row pointers; this is always "near" on 80x86 */
  free_small((void *) ptr);

#ifdef MEM_STATS
  cur_num_sarray--;
#endif
}


METHODDEF JBLOCKARRAY
alloc_small_barray (long blocksperrow, long numrows)
/* Allocate a "small" (all-in-memory) 2-D coefficient-block array */
{
  JBLOCKARRAY result;
#ifdef BIG_MALLOCS_OK
  JBLOCKROW workspace;
#endif
  long i;

#ifdef MEM_STATS
  total_num_barray++;
#ifdef BIG_MALLOCS_OK
  total_bytes_barray += numrows * blocksperrow * SIZEOF(JBLOCK)
			+ MALLOC_FAR_OVERHEAD;
#else
  total_bytes_barray += (blocksperrow * SIZEOF(JBLOCK) + MALLOC_FAR_OVERHEAD)
			* numrows;
#endif
  cur_num_barray++;
  if (cur_num_barray > max_num_barray) max_num_barray = cur_num_barray;
#endif

  /* Get space for row pointers; this is always "near" on 80x86 */
  result = (JBLOCKARRAY) alloc_small((size_t) (numrows * SIZEOF(JBLOCKROW)));

  /* Get the rows themselves; on 80x86 these are "far" */

#ifdef BIG_MALLOCS_OK
  workspace = (JBLOCKROW) far_malloc((size_t)
				(numrows * blocksperrow * SIZEOF(JBLOCK)));
  if (workspace == NULL)
    out_of_memory(4);
  for (i = 0; i < numrows; i++) {
    result[i] = workspace;
    workspace += blocksperrow;
  }
#else
  for (i = 0; i < numrows; i++) {
    result[i] = (JBLOCKROW) far_malloc((size_t)
				       (blocksperrow * SIZEOF(JBLOCK)));
    if (result[i] == NULL)
      out_of_memory(4);
  }
#endif

  return result;
}


METHODDEF void
free_small_barray (JBLOCKARRAY ptr, long numrows)
/* Free a "small" (all-in-memory) 2-D coefficient-block array */
{
  /* Free the rows themselves; on 80x86 these are "far" */
#ifdef BIG_MALLOCS_OK
  far_free((void FAR *) ptr[0]);
#else
  long i;

  for (i = 0; i < numrows; i++) {
    far_free((void FAR *) ptr[i]);
  }
#endif

  /* Free space for row pointers; this is always "near" on 80x86 */
  free_small((void *) ptr);

#ifdef MEM_STATS
  cur_num_barray--;
#endif
}



/*
 * About "big" array management:
 *
 * To allow machines with limited memory to handle large images,
 * all processing in the JPEG system is done a few pixel or block rows
 * at a time.  The above "small" array routines are only used to allocate
 * strip buffers (as wide as the image, but just a few rows high).
 * In some cases multiple passes must be made over the data.  In these
 * cases the "big" array routines are used.  The array is still accessed
 * a strip at a time, but the memory manager must save the whole array
 * for repeated accesses.  The intended implementation is that there is
 * a strip buffer in memory (as high as is possible given the desired memory
 * limit), plus a backing file that holds the rest of the array.
 *
 * The request_big_array routines are told the total size of the image (in case
 * it is useful to know the total file size that will be needed).  They are
 * also given the unit height, which is the number of rows that will be
 * accessed at once; the in-memory buffer should usually be made a multiple of
 * this height for best efficiency.
 *
 * The request routines create control blocks (and may open backing files),
 * but they don't create the in-memory buffers.  This is postponed until
 * alloc_big_arrays is called.  At that time the total amount of space needed
 * is known (approximately, anyway), so free memory can be divided up fairly.
 *
 * The access_big_array routines are responsible for making a specific strip
 * area accessible (after reading or writing the backing file, if necessary).
 * Note that the access routines are told whether the caller intends to modify
 * the accessed strip; during a read-only pass this saves having to rewrite
 * data to disk.
 *
 * The typical access pattern is one top-to-bottom pass to write the data,
 * followed by one or more read-only top-to-bottom passes.  However, other
 * access patterns may occur while reading.  For example, translation of image
 * formats that use bottom-to-top scan order will require bottom-to-top read
 * passes.  The memory manager need not support multiple write passes nor
 * funny write orders (meaning that rearranging rows must be handled while
 * reading data out of the big array, not while putting it in).
 *
 * In current usage, the access requests are always for nonoverlapping strips;
 * that is, successive access start_row numbers always differ by exactly the
 * unitheight.  This allows fairly simple buffer dump/reload logic if the
 * in-memory buffer is made a multiple of the unitheight.  It would be
 * possible to keep subsampled rather than fullsize data in the "big" arrays,
 * thus reducing temp file size, if we supported overlapping strip access
 * (access requests differing by less than the unitheight).  At the moment
 * I don't believe this is worth the extra complexity.
 *
 * This particular implementation doesn't use temp files; the whole of a big
 * array is allocated in (virtual) memory, and any swapping is done behind the
 * scenes by the operating system.
 */



/* The control blocks for virtual arrays.
 * These are pretty minimal in this implementation.
 * Note: in this implementation we could realize big arrays
 * at request time and make alloc_big_arrays a no-op;
 * however, doing it separately keeps callers honest.
 */

struct big_sarray_control {
	JSAMPARRAY mem_buffer;	/* memory buffer (the whole thing, here) */
	long rows_in_mem;	/* Height of memory buffer */
	long samplesperrow;	/* Width of memory buffer */
	long unitheight;	/* # of rows accessed by access_big_sarray() */
	big_sarray_ptr next;	/* list link for unrealized arrays */
};

struct big_barray_control {
	JBLOCKARRAY mem_buffer;	/* memory buffer (the whole thing, here) */
	long rows_in_mem;	/* Height of memory buffer */
	long blocksperrow;	/* Width of memory buffer */
	long unitheight;	/* # of rows accessed by access_big_barray() */
	big_barray_ptr next;	/* list link for unrealized arrays */
};


/* Headers of lists of control blocks for unrealized big arrays */
static big_sarray_ptr unalloced_sarrays;
static big_barray_ptr unalloced_barrays;


METHODDEF big_sarray_ptr
request_big_sarray (long samplesperrow, long numrows, long unitheight)
/* Request a "big" (virtual-memory) 2-D sample array */
{
  big_sarray_ptr result;

  /* get control block */
  result = (big_sarray_ptr) alloc_small(SIZEOF(struct big_sarray_control));

  result->mem_buffer = NULL;	/* lets access routine spot premature access */
  result->rows_in_mem = numrows;
  result->samplesperrow = samplesperrow;
  result->unitheight = unitheight;
  result->next = unalloced_sarrays; /* add to list of unallocated arrays */
  unalloced_sarrays = result;

  return result;
}


METHODDEF big_barray_ptr
request_big_barray (long blocksperrow, long numrows, long unitheight)
/* Request a "big" (virtual-memory) 2-D coefficient-block array */
{
  big_barray_ptr result;

  /* get control block */
  result = (big_barray_ptr) alloc_small(SIZEOF(struct big_barray_control));

  result->mem_buffer = NULL;	/* lets access routine spot premature access */
  result->rows_in_mem = numrows;
  result->blocksperrow = blocksperrow;
  result->unitheight = unitheight;
  result->next = unalloced_barrays; /* add to list of unallocated arrays */
  unalloced_barrays = result;

  return result;
}


METHODDEF void
alloc_big_arrays (long extra_small_samples, long extra_small_blocks,
		  long extra_medium_space)
/* Allocate the in-memory buffers for any unrealized "big" arrays */
/* 'extra' values are upper bounds for total future small-array requests */
/* and far-heap requests */
{
  /* In this implementation we just malloc the whole arrays */
  /* and expect the system's virtual memory to worry about swapping them */
  big_sarray_ptr sptr;
  big_barray_ptr bptr;

  for (sptr = unalloced_sarrays; sptr != NULL; sptr = sptr->next) {
    sptr->mem_buffer = alloc_small_sarray(sptr->samplesperrow,
					  sptr->rows_in_mem);
  }

  for (bptr = unalloced_barrays; bptr != NULL; bptr = bptr->next) {
    bptr->mem_buffer = alloc_small_barray(bptr->blocksperrow,
					  bptr->rows_in_mem);
  }

  unalloced_sarrays = NULL;	/* reset for possible future cycles */
  unalloced_barrays = NULL;
}


METHODDEF JSAMPARRAY
access_big_sarray (big_sarray_ptr ptr, long start_row, boolean writable)
/* Access the part of a "big" sample array starting at start_row */
/* and extending for ptr->unitheight rows.  writable is true if  */
/* caller intends to modify the accessed area. */
{
  /* debugging check */
  if (start_row < 0 || start_row+ptr->unitheight > ptr->rows_in_mem ||
      ptr->mem_buffer == NULL)
    ERREXIT(methods, "Bogus access_big_sarray request");

  return ptr->mem_buffer + start_row;
}


METHODDEF JBLOCKARRAY
access_big_barray (big_barray_ptr ptr, long start_row, boolean writable)
/* Access the part of a "big" coefficient-block array starting at start_row */
/* and extending for ptr->unitheight rows.  writable is true if  */
/* caller intends to modify the accessed area. */
{
  /* debugging check */
  if (start_row < 0 || start_row+ptr->unitheight > ptr->rows_in_mem ||
      ptr->mem_buffer == NULL)
    ERREXIT(methods, "Bogus access_big_barray request");

  return ptr->mem_buffer + start_row;
}


METHODDEF void
free_big_sarray (big_sarray_ptr ptr)
/* Free a "big" (virtual-memory) 2-D sample array */
{
  free_small_sarray(ptr->mem_buffer, ptr->rows_in_mem);
  free_small((void *) ptr);	/* free the control block too */
}


METHODDEF void
free_big_barray (big_barray_ptr ptr)
/* Free a "big" (virtual-memory) 2-D coefficient-block array */
{
  free_small_barray(ptr->mem_buffer, ptr->rows_in_mem);
  free_small((void *) ptr);	/* free the control block too */
}



/*
 * The method selection routine for virtual memory systems.
 * The system-dependent setup routine should call this routine
 * to install the necessary method pointers in the supplied struct.
 */

GLOBAL void
jselvirtmem (external_methods_ptr emethods)
{
  methods = emethods;		/* save struct addr for error exit access */

  emethods->alloc_small = alloc_small;
  emethods->free_small = free_small;
#ifdef NEED_FAR_POINTERS
  emethods->alloc_medium = alloc_medium;
  emethods->free_medium = free_medium;
#endif
  emethods->alloc_small_sarray = alloc_small_sarray;
  emethods->free_small_sarray = free_small_sarray;
  emethods->alloc_small_barray = alloc_small_barray;
  emethods->free_small_barray = free_small_barray;
  emethods->request_big_sarray = request_big_sarray;
  emethods->request_big_barray = request_big_barray;
  emethods->alloc_big_arrays = alloc_big_arrays;
  emethods->access_big_sarray = access_big_sarray;
  emethods->access_big_barray = access_big_barray;
  emethods->free_big_sarray = free_big_sarray;
  emethods->free_big_barray = free_big_barray;

  unalloced_sarrays = NULL;	/* make sure list headers are empty */
  unalloced_barrays = NULL;
}

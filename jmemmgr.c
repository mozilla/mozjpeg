/*
 * jmemmgr.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides the standard system-independent memory management
 * routines.  This code is usable across a wide variety of machines; most
 * of the system dependencies have been isolated in a separate file.
 * The major functions provided here are:
 *   * bookkeeping to allow all allocated memory to be freed upon exit;
 *   * policy decisions about how to divide available memory among the
 *     various large arrays;
 *   * control logic for swapping virtual arrays between main memory and
 *     backing storage.
 * The separate system-dependent file provides the actual backing-storage
 * access code, and it contains the policy decision about how much total
 * main memory to use.
 * This file is system-dependent in the sense that some of its functions
 * are unnecessary in some systems.  For example, if there is enough virtual
 * memory so that backing storage will never be used, much of the big-array
 * control logic could be removed.  (Of course, if you have that much memory
 * then you shouldn't care about a little bit of unused code...)
 *
 * These routines are invoked via the methods alloc_small, free_small,
 * alloc_medium, free_medium, alloc_small_sarray, free_small_sarray,
 * alloc_small_barray, free_small_barray, request_big_sarray,
 * request_big_barray, alloc_big_arrays, access_big_sarray, access_big_barray,
 * free_big_sarray, free_big_barray, and free_all.
 */

#define AM_MEMORY_MANAGER	/* we define big_Xarray_control structs */

#include "jinclude.h"
#include "jmemsys.h"		/* import the system-dependent declarations */

#ifndef NO_GETENV
#ifdef INCLUDES_ARE_ANSI
#include <stdlib.h>		/* to declare getenv() */
#else
extern char * getenv PP((const char * name));
#endif
#endif


/*
 * On many systems it is not necessary to distinguish alloc_small from
 * alloc_medium; the main case where they must be distinguished is when
 * FAR pointers are distinct from regular pointers.  However, you might
 * want to keep them separate if you have different system-dependent logic
 * for small and large memory requests (i.e., jget_small and jget_large
 * do different things).
 */

#ifdef NEED_FAR_POINTERS
#define NEED_ALLOC_MEDIUM	/* flags alloc_medium really exists */
#endif


/*
 * Many machines require storage alignment: longs must start on 4-byte
 * boundaries, doubles on 8-byte boundaries, etc.  On such machines, malloc()
 * always returns pointers that are multiples of the worst-case alignment
 * requirement, and we had better do so too.  This means the headers that
 * we tack onto allocated structures had better have length a multiple of
 * the alignment requirement.
 * There isn't any really portable way to determine the worst-case alignment
 * requirement.  In this code we assume that the alignment requirement is
 * multiples of sizeof(align_type).  Here we define align_type as double;
 * with this definition, the code will run on all machines known to me.
 * If your machine has lesser alignment needs, you can save a few bytes
 * by making align_type smaller.
 */

typedef double align_type;


/*
 * Some important notes:
 *   The allocation routines provided here must never return NULL.
 *   They should exit to error_exit if unsuccessful.
 *
 *   It's not a good idea to try to merge the sarray and barray routines,
 *   even though they are textually almost the same, because samples are
 *   usually stored as bytes while coefficients are shorts.  Thus, in machines
 *   where byte pointers have a different representation from word pointers,
 *   the resulting machine code could not be the same.
 */


static external_methods_ptr methods; /* saved for access to error_exit */


#ifdef MEM_STATS		/* optional extra stuff for statistics */

/* These macros are the assumed overhead per block for malloc().
 * They don't have to be accurate, but the printed statistics will be
 * off a little bit if they are not.
 */
#define MALLOC_OVERHEAD  (SIZEOF(void *)) /* overhead for jget_small() */
#define MALLOC_FAR_OVERHEAD  (SIZEOF(void FAR *)) /* for jget_large() */

static long total_num_small = 0;	/* total # of small objects alloced */
static long total_bytes_small = 0;	/* total bytes requested */
static long cur_num_small = 0;		/* # currently alloced */
static long max_num_small = 0;		/* max simultaneously alloced */

#ifdef NEED_ALLOC_MEDIUM
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


LOCAL void
print_mem_stats (void)
{
  /* since this is only a debugging stub, we can cheat a little on the
   * trace message mechanism... helpful 'cuz trace_message can't handle longs.
   */
  fprintf(stderr, "total_num_small = %ld\n", total_num_small);
  fprintf(stderr, "total_bytes_small = %ld\n", total_bytes_small);
  if (cur_num_small)
    fprintf(stderr, "cur_num_small = %ld\n", cur_num_small);
  fprintf(stderr, "max_num_small = %ld\n", max_num_small);
  
#ifdef NEED_ALLOC_MEDIUM
  fprintf(stderr, "total_num_medium = %ld\n", total_num_medium);
  fprintf(stderr, "total_bytes_medium = %ld\n", total_bytes_medium);
  if (cur_num_medium)
    fprintf(stderr, "cur_num_medium = %ld\n", cur_num_medium);
  fprintf(stderr, "max_num_medium = %ld\n", max_num_medium);
#endif
  
  fprintf(stderr, "total_num_sarray = %ld\n", total_num_sarray);
  fprintf(stderr, "total_bytes_sarray = %ld\n", total_bytes_sarray);
  if (cur_num_sarray)
    fprintf(stderr, "cur_num_sarray = %ld\n", cur_num_sarray);
  fprintf(stderr, "max_num_sarray = %ld\n", max_num_sarray);
  
  fprintf(stderr, "total_num_barray = %ld\n", total_num_barray);
  fprintf(stderr, "total_bytes_barray = %ld\n", total_bytes_barray);
  if (cur_num_barray)
    fprintf(stderr, "cur_num_barray = %ld\n", cur_num_barray);
  fprintf(stderr, "max_num_barray = %ld\n", max_num_barray);
}

#endif /* MEM_STATS */


LOCAL void
out_of_memory (int which)
/* Report an out-of-memory error and stop execution */
/* If we compiled MEM_STATS support, report alloc requests before dying */
{
#ifdef MEM_STATS
  if (methods->trace_level <= 0) /* don't do it if free_all() will */
    print_mem_stats();		/* print optional memory usage statistics */
#endif
  ERREXIT1(methods, "Insufficient memory (case %d)", which);
}


/*
 * Management of "small" objects.
 * These are all-in-memory, and are in near-heap space on an 80x86.
 */

typedef union small_struct * small_ptr;

typedef union small_struct {
	small_ptr next;		/* next in list of allocated objects */
	align_type dummy;	/* ensures alignment of following storage */
      } small_hdr;

static small_ptr small_list;	/* head of list */


METHODDEF void *
alloc_small (size_t sizeofobject)
/* Allocate a "small" object */
{
  small_ptr result;

  sizeofobject += SIZEOF(small_hdr); /* add space for header */

#ifdef MEM_STATS
  total_num_small++;
  total_bytes_small += sizeofobject + MALLOC_OVERHEAD;
  cur_num_small++;
  if (cur_num_small > max_num_small) max_num_small = cur_num_small;
#endif

  result = (small_ptr) jget_small(sizeofobject);
  if (result == NULL)
    out_of_memory(1);

  result->next = small_list;
  small_list = result;
  result++;			/* advance past header */

  return (void *) result;
}


METHODDEF void
free_small (void *ptr)
/* Free a "small" object */
{
  small_ptr hdr;
  small_ptr * llink;

  hdr = (small_ptr) ptr;
  hdr--;			/* point back to header */

  /* Remove item from list -- linear search is fast enough */
  llink = &small_list;
  while (*llink != hdr) {
    if (*llink == NULL)
      ERREXIT(methods, "Bogus free_small request");
    llink = &( (*llink)->next );
  }
  *llink = hdr->next;

  jfree_small((void *) hdr);

#ifdef MEM_STATS
  cur_num_small--;
#endif
}


/*
 * Management of "medium-size" objects.
 * These are just like small objects except they are in the FAR heap.
 */

#ifdef NEED_ALLOC_MEDIUM

typedef union medium_struct FAR * medium_ptr;

typedef union medium_struct {
	medium_ptr next;	/* next in list of allocated objects */
	align_type dummy;	/* ensures alignment of following storage */
      } medium_hdr;

static medium_ptr medium_list;	/* head of list */


METHODDEF void FAR *
alloc_medium (size_t sizeofobject)
/* Allocate a "medium-size" object */
{
  medium_ptr result;

  sizeofobject += SIZEOF(medium_hdr); /* add space for header */

#ifdef MEM_STATS
  total_num_medium++;
  total_bytes_medium += sizeofobject + MALLOC_FAR_OVERHEAD;
  cur_num_medium++;
  if (cur_num_medium > max_num_medium) max_num_medium = cur_num_medium;
#endif

  result = (medium_ptr) jget_large(sizeofobject);
  if (result == NULL)
    out_of_memory(2);

  result->next = medium_list;
  medium_list = result;
  result++;			/* advance past header */

  return (void FAR *) result;
}


METHODDEF void
free_medium (void FAR *ptr)
/* Free a "medium-size" object */
{
  medium_ptr hdr;
  medium_ptr FAR * llink;

  hdr = (medium_ptr) ptr;
  hdr--;			/* point back to header */

  /* Remove item from list -- linear search is fast enough */
  llink = (medium_ptr FAR *) &medium_list;
  while (*llink != hdr) {
    if (*llink == NULL)
      ERREXIT(methods, "Bogus free_medium request");
    llink = &( (*llink)->next );
  }
  *llink = hdr->next;

  jfree_large((void FAR *) hdr);

#ifdef MEM_STATS
  cur_num_medium--;
#endif
}

#endif /* NEED_ALLOC_MEDIUM */


/*
 * Management of "small" (all-in-memory) 2-D sample arrays.
 * The pointers are in near heap, the samples themselves in FAR heap.
 * The header structure is adjacent to the row pointers.
 * To minimize allocation overhead and to allow I/O of large contiguous
 * blocks, we allocate the sample rows in groups of as many rows as possible
 * without exceeding MAX_ALLOC_CHUNK total bytes per allocation request.
 * Note that the big-array control routines, later in this file, know about
 * this chunking of rows ... and also how to get the rowsperchunk value!
 */

typedef struct small_sarray_struct * small_sarray_ptr;

typedef struct small_sarray_struct {
	small_sarray_ptr next;	/* next in list of allocated sarrays */
	long numrows;		/* # of rows in this array */
	long rowsperchunk;	/* max # of rows per allocation chunk */
	JSAMPROW dummy;		/* ensures alignment of following storage */
      } small_sarray_hdr;

static small_sarray_ptr small_sarray_list; /* head of list */


METHODDEF JSAMPARRAY
alloc_small_sarray (long samplesperrow, long numrows)
/* Allocate a "small" (all-in-memory) 2-D sample array */
{
  small_sarray_ptr hdr;
  JSAMPARRAY result;
  JSAMPROW workspace;
  long rowsperchunk, currow, i;

#ifdef MEM_STATS
  total_num_sarray++;
  cur_num_sarray++;
  if (cur_num_sarray > max_num_sarray) max_num_sarray = cur_num_sarray;
#endif

  /* Calculate max # of rows allowed in one allocation chunk */
  rowsperchunk = MAX_ALLOC_CHUNK / (samplesperrow * SIZEOF(JSAMPLE));
  if (rowsperchunk <= 0)
      ERREXIT(methods, "Image too wide for this implementation");

  /* Get space for header and row pointers; this is always "near" on 80x86 */
  hdr = (small_sarray_ptr) alloc_small((size_t) (numrows * SIZEOF(JSAMPROW)
						 + SIZEOF(small_sarray_hdr)));

  result = (JSAMPARRAY) (hdr+1); /* advance past header */

  /* Insert into list now so free_all does right thing if I fail */
  /* after allocating only some of the rows... */
  hdr->next = small_sarray_list;
  hdr->numrows = 0;
  hdr->rowsperchunk = rowsperchunk;
  small_sarray_list = hdr;

  /* Get the rows themselves; on 80x86 these are "far" */
  currow = 0;
  while (currow < numrows) {
    rowsperchunk = MIN(rowsperchunk, numrows - currow);
#ifdef MEM_STATS
    total_bytes_sarray += rowsperchunk * samplesperrow * SIZEOF(JSAMPLE)
			  + MALLOC_FAR_OVERHEAD;
#endif
    workspace = (JSAMPROW) jget_large((size_t) (rowsperchunk * samplesperrow
						* SIZEOF(JSAMPLE)));
    if (workspace == NULL)
      out_of_memory(3);
    for (i = rowsperchunk; i > 0; i--) {
      result[currow++] = workspace;
      workspace += samplesperrow;
    }
    hdr->numrows = currow;
  }

  return result;
}


METHODDEF void
free_small_sarray (JSAMPARRAY ptr)
/* Free a "small" (all-in-memory) 2-D sample array */
{
  small_sarray_ptr hdr;
  small_sarray_ptr * llink;
  long i;

  hdr = (small_sarray_ptr) ptr;
  hdr--;			/* point back to header */

  /* Remove item from list -- linear search is fast enough */
  llink = &small_sarray_list;
  while (*llink != hdr) {
    if (*llink == NULL)
      ERREXIT(methods, "Bogus free_small_sarray request");
    llink = &( (*llink)->next );
  }
  *llink = hdr->next;

  /* Free the rows themselves; on 80x86 these are "far" */
  /* Note we only free the row-group headers! */
  for (i = 0; i < hdr->numrows; i += hdr->rowsperchunk) {
    jfree_large((void FAR *) ptr[i]);
  }

  /* Free header and row pointers */
  free_small((void *) hdr);

#ifdef MEM_STATS
  cur_num_sarray--;
#endif
}


/*
 * Management of "small" (all-in-memory) 2-D coefficient-block arrays.
 * This is essentially the same as the code for sample arrays, above.
 */

typedef struct small_barray_struct * small_barray_ptr;

typedef struct small_barray_struct {
	small_barray_ptr next;	/* next in list of allocated barrays */
	long numrows;		/* # of rows in this array */
	long rowsperchunk;	/* max # of rows per allocation chunk */
	JBLOCKROW dummy;	/* ensures alignment of following storage */
      } small_barray_hdr;

static small_barray_ptr small_barray_list; /* head of list */


METHODDEF JBLOCKARRAY
alloc_small_barray (long blocksperrow, long numrows)
/* Allocate a "small" (all-in-memory) 2-D coefficient-block array */
{
  small_barray_ptr hdr;
  JBLOCKARRAY result;
  JBLOCKROW workspace;
  long rowsperchunk, currow, i;

#ifdef MEM_STATS
  total_num_barray++;
  cur_num_barray++;
  if (cur_num_barray > max_num_barray) max_num_barray = cur_num_barray;
#endif

  /* Calculate max # of rows allowed in one allocation chunk */
  rowsperchunk = MAX_ALLOC_CHUNK / (blocksperrow * SIZEOF(JBLOCK));
  if (rowsperchunk <= 0)
      ERREXIT(methods, "Image too wide for this implementation");

  /* Get space for header and row pointers; this is always "near" on 80x86 */
  hdr = (small_barray_ptr) alloc_small((size_t) (numrows * SIZEOF(JBLOCKROW)
						 + SIZEOF(small_barray_hdr)));

  result = (JBLOCKARRAY) (hdr+1); /* advance past header */

  /* Insert into list now so free_all does right thing if I fail */
  /* after allocating only some of the rows... */
  hdr->next = small_barray_list;
  hdr->numrows = 0;
  hdr->rowsperchunk = rowsperchunk;
  small_barray_list = hdr;

  /* Get the rows themselves; on 80x86 these are "far" */
  currow = 0;
  while (currow < numrows) {
    rowsperchunk = MIN(rowsperchunk, numrows - currow);
#ifdef MEM_STATS
    total_bytes_barray += rowsperchunk * blocksperrow * SIZEOF(JBLOCK)
			  + MALLOC_FAR_OVERHEAD;
#endif
    workspace = (JBLOCKROW) jget_large((size_t) (rowsperchunk * blocksperrow
						 * SIZEOF(JBLOCK)));
    if (workspace == NULL)
      out_of_memory(4);
    for (i = rowsperchunk; i > 0; i--) {
      result[currow++] = workspace;
      workspace += blocksperrow;
    }
    hdr->numrows = currow;
  }

  return result;
}


METHODDEF void
free_small_barray (JBLOCKARRAY ptr)
/* Free a "small" (all-in-memory) 2-D coefficient-block array */
{
  small_barray_ptr hdr;
  small_barray_ptr * llink;
  long i;

  hdr = (small_barray_ptr) ptr;
  hdr--;			/* point back to header */

  /* Remove item from list -- linear search is fast enough */
  llink = &small_barray_list;
  while (*llink != hdr) {
    if (*llink == NULL)
      ERREXIT(methods, "Bogus free_small_barray request");
    llink = &( (*llink)->next );
  }
  *llink = hdr->next;

  /* Free the rows themselves; on 80x86 these are "far" */
  /* Note we only free the row-group headers! */
  for (i = 0; i < hdr->numrows; i += hdr->rowsperchunk) {
    jfree_large((void FAR *) ptr[i]);
  }

  /* Free header and row pointers */
  free_small((void *) hdr);

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
 * accessed at once; the in-memory buffer should be made a multiple of
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
 * possible to keep downsampled rather than fullsize data in the "big" arrays,
 * thus reducing temp file size, if we supported overlapping strip access
 * (access requests differing by less than the unitheight).  At the moment
 * I don't believe this is worth the extra complexity.
 */



/* The control blocks for virtual arrays.
 * System-dependent info for the associated backing store is hidden inside
 * the backing_store_info struct.
 */

struct big_sarray_control {
	long rows_in_array;	/* total virtual array height */
	long samplesperrow;	/* width of array (and of memory buffer) */
	long unitheight;	/* # of rows accessed by access_big_sarray() */
	JSAMPARRAY mem_buffer;	/* the in-memory buffer */
	long rows_in_mem;	/* height of memory buffer */
	long rowsperchunk;	/* allocation chunk size in mem_buffer */
	long cur_start_row;	/* first logical row # in the buffer */
	boolean dirty;		/* do current buffer contents need written? */
	boolean b_s_open;	/* is backing-store data valid? */
	big_sarray_ptr next;	/* link to next big sarray control block */
	backing_store_info b_s_info; /* System-dependent control info */
};

static big_sarray_ptr big_sarray_list; /* head of list */

struct big_barray_control {
	long rows_in_array;	/* total virtual array height */
	long blocksperrow;	/* width of array (and of memory buffer) */
	long unitheight;	/* # of rows accessed by access_big_barray() */
	JBLOCKARRAY mem_buffer;	/* the in-memory buffer */
	long rows_in_mem;	/* height of memory buffer */
	long rowsperchunk;	/* allocation chunk size in mem_buffer */
	long cur_start_row;	/* first logical row # in the buffer */
	boolean dirty;		/* do current buffer contents need written? */
	boolean b_s_open;	/* is backing-store data valid? */
	big_barray_ptr next;	/* link to next big barray control block */
	backing_store_info b_s_info; /* System-dependent control info */
};

static big_barray_ptr big_barray_list; /* head of list */


METHODDEF big_sarray_ptr
request_big_sarray (long samplesperrow, long numrows, long unitheight)
/* Request a "big" (virtual-memory) 2-D sample array */
{
  big_sarray_ptr result;

  /* get control block */
  result = (big_sarray_ptr) alloc_small(SIZEOF(struct big_sarray_control));

  result->rows_in_array = numrows;
  result->samplesperrow = samplesperrow;
  result->unitheight = unitheight;
  result->mem_buffer = NULL;	/* marks array not yet realized */
  result->b_s_open = FALSE;	/* no associated backing-store object */
  result->next = big_sarray_list; /* add to list of big arrays */
  big_sarray_list = result;

  return result;
}


METHODDEF big_barray_ptr
request_big_barray (long blocksperrow, long numrows, long unitheight)
/* Request a "big" (virtual-memory) 2-D coefficient-block array */
{
  big_barray_ptr result;

  /* get control block */
  result = (big_barray_ptr) alloc_small(SIZEOF(struct big_barray_control));

  result->rows_in_array = numrows;
  result->blocksperrow = blocksperrow;
  result->unitheight = unitheight;
  result->mem_buffer = NULL;	/* marks array not yet realized */
  result->b_s_open = FALSE;	/* no associated backing-store object */
  result->next = big_barray_list; /* add to list of big arrays */
  big_barray_list = result;

  return result;
}


METHODDEF void
alloc_big_arrays (long extra_small_samples, long extra_small_blocks,
		  long extra_medium_space)
/* Allocate the in-memory buffers for any unrealized "big" arrays */
/* 'extra' values are upper bounds for total future small-array requests */
/* and far-heap requests */
{
  long total_extra_space = extra_small_samples * SIZEOF(JSAMPLE)
			   + extra_small_blocks * SIZEOF(JBLOCK)
			   + extra_medium_space;
  long space_per_unitheight, maximum_space, avail_mem;
  long unitheights, max_unitheights;
  big_sarray_ptr sptr;
  big_barray_ptr bptr;

  /* Compute the minimum space needed (unitheight rows in each buffer)
   * and the maximum space needed (full image height in each buffer).
   * These may be of use to the system-dependent jmem_available routine.
   */
  space_per_unitheight = 0;
  maximum_space = total_extra_space;
  for (sptr = big_sarray_list; sptr != NULL; sptr = sptr->next) {
    if (sptr->mem_buffer == NULL) { /* if not realized yet */
      space_per_unitheight += sptr->unitheight *
			      sptr->samplesperrow * SIZEOF(JSAMPLE);
      maximum_space += sptr->rows_in_array *
		       sptr->samplesperrow * SIZEOF(JSAMPLE);
    }
  }
  for (bptr = big_barray_list; bptr != NULL; bptr = bptr->next) {
    if (bptr->mem_buffer == NULL) { /* if not realized yet */
      space_per_unitheight += bptr->unitheight *
			      bptr->blocksperrow * SIZEOF(JBLOCK);
      maximum_space += bptr->rows_in_array *
		       bptr->blocksperrow * SIZEOF(JBLOCK);
    }
  }

  if (space_per_unitheight <= 0)
    return;			/* no unrealized arrays, no work */

  /* Determine amount of memory to actually use; this is system-dependent. */
  avail_mem = jmem_available(space_per_unitheight + total_extra_space,
			     maximum_space);

  /* If the maximum space needed is available, make all the buffers full
   * height; otherwise parcel it out with the same number of unitheights
   * in each buffer.
   */
  if (avail_mem >= maximum_space)
    max_unitheights = 1000000000L;
  else {
    max_unitheights = (avail_mem - total_extra_space) / space_per_unitheight;
    /* If there doesn't seem to be enough space, try to get the minimum
     * anyway.  This allows a "stub" implementation of jmem_available().
     */
    if (max_unitheights <= 0)
      max_unitheights = 1;
  }

  /* Allocate the in-memory buffers and initialize backing store as needed. */

  for (sptr = big_sarray_list; sptr != NULL; sptr = sptr->next) {
    if (sptr->mem_buffer == NULL) { /* if not realized yet */
      unitheights = (sptr->rows_in_array + sptr->unitheight - 1L)
		    / sptr->unitheight;
      if (unitheights <= max_unitheights) {
	/* This buffer fits in memory */
	sptr->rows_in_mem = sptr->rows_in_array;
      } else {
	/* It doesn't fit in memory, create backing store. */
	sptr->rows_in_mem = max_unitheights * sptr->unitheight;
	jopen_backing_store(& sptr->b_s_info,
			    (long) (sptr->rows_in_array *
				    sptr->samplesperrow * SIZEOF(JSAMPLE)));
	sptr->b_s_open = TRUE;
      }
      sptr->mem_buffer = alloc_small_sarray(sptr->samplesperrow,
					    sptr->rows_in_mem);
      /* Reach into the small_sarray header and get the rowsperchunk field.
       * Yes, I know, this is horrible coding practice.
       */
      sptr->rowsperchunk =
	((small_sarray_ptr) sptr->mem_buffer)[-1].rowsperchunk;
      sptr->cur_start_row = 0;
      sptr->dirty = FALSE;
    }
  }

  for (bptr = big_barray_list; bptr != NULL; bptr = bptr->next) {
    if (bptr->mem_buffer == NULL) { /* if not realized yet */
      unitheights = (bptr->rows_in_array + bptr->unitheight - 1L)
		    / bptr->unitheight;
      if (unitheights <= max_unitheights) {
	/* This buffer fits in memory */
	bptr->rows_in_mem = bptr->rows_in_array;
      } else {
	/* It doesn't fit in memory, create backing store. */
	bptr->rows_in_mem = max_unitheights * bptr->unitheight;
	jopen_backing_store(& bptr->b_s_info,
			    (long) (bptr->rows_in_array *
				    bptr->blocksperrow * SIZEOF(JBLOCK)));
	bptr->b_s_open = TRUE;
      }
      bptr->mem_buffer = alloc_small_barray(bptr->blocksperrow,
					    bptr->rows_in_mem);
      /* Reach into the small_barray header and get the rowsperchunk field. */
      bptr->rowsperchunk =
	((small_barray_ptr) bptr->mem_buffer)[-1].rowsperchunk;
      bptr->cur_start_row = 0;
      bptr->dirty = FALSE;
    }
  }
}


LOCAL void
do_sarray_io (big_sarray_ptr ptr, boolean writing)
/* Do backing store read or write of a "big" sample array */
{
  long bytesperrow, file_offset, byte_count, rows, i;

  bytesperrow = ptr->samplesperrow * SIZEOF(JSAMPLE);
  file_offset = ptr->cur_start_row * bytesperrow;
  /* Loop to read or write each allocation chunk in mem_buffer */
  for (i = 0; i < ptr->rows_in_mem; i += ptr->rowsperchunk) {
    /* One chunk, but check for short chunk at end of buffer */
    rows = MIN(ptr->rowsperchunk, ptr->rows_in_mem - i);
    /* Transfer no more than fits in file */
    rows = MIN(rows, ptr->rows_in_array - (ptr->cur_start_row + i));
    if (rows <= 0)		/* this chunk might be past end of file! */
      break;
    byte_count = rows * bytesperrow;
    if (writing)
      (*ptr->b_s_info.write_backing_store) (& ptr->b_s_info,
					    (void FAR *) ptr->mem_buffer[i],
					    file_offset, byte_count);
    else
      (*ptr->b_s_info.read_backing_store) (& ptr->b_s_info,
					   (void FAR *) ptr->mem_buffer[i],
					   file_offset, byte_count);
    file_offset += byte_count;
  }
}


LOCAL void
do_barray_io (big_barray_ptr ptr, boolean writing)
/* Do backing store read or write of a "big" coefficient-block array */
{
  long bytesperrow, file_offset, byte_count, rows, i;

  bytesperrow = ptr->blocksperrow * SIZEOF(JBLOCK);
  file_offset = ptr->cur_start_row * bytesperrow;
  /* Loop to read or write each allocation chunk in mem_buffer */
  for (i = 0; i < ptr->rows_in_mem; i += ptr->rowsperchunk) {
    /* One chunk, but check for short chunk at end of buffer */
    rows = MIN(ptr->rowsperchunk, ptr->rows_in_mem - i);
    /* Transfer no more than fits in file */
    rows = MIN(rows, ptr->rows_in_array - (ptr->cur_start_row + i));
    if (rows <= 0)		/* this chunk might be past end of file! */
      break;
    byte_count = rows * bytesperrow;
    if (writing)
      (*ptr->b_s_info.write_backing_store) (& ptr->b_s_info,
					    (void FAR *) ptr->mem_buffer[i],
					    file_offset, byte_count);
    else
      (*ptr->b_s_info.read_backing_store) (& ptr->b_s_info,
					   (void FAR *) ptr->mem_buffer[i],
					   file_offset, byte_count);
    file_offset += byte_count;
  }
}


METHODDEF JSAMPARRAY
access_big_sarray (big_sarray_ptr ptr, long start_row, boolean writable)
/* Access the part of a "big" sample array starting at start_row */
/* and extending for ptr->unitheight rows.  writable is true if  */
/* caller intends to modify the accessed area. */
{
  /* debugging check */
  if (start_row < 0 || start_row+ptr->unitheight > ptr->rows_in_array ||
      ptr->mem_buffer == NULL)
    ERREXIT(methods, "Bogus access_big_sarray request");

  /* Make the desired part of the virtual array accessible */
  if (start_row < ptr->cur_start_row ||
      start_row+ptr->unitheight > ptr->cur_start_row+ptr->rows_in_mem) {
    if (! ptr->b_s_open)
      ERREXIT(methods, "Virtual array controller messed up");
    /* Flush old buffer contents if necessary */
    if (ptr->dirty) {
      do_sarray_io(ptr, TRUE);
      ptr->dirty = FALSE;
    }
    /* Decide what part of virtual array to access.
     * Algorithm: if target address > current window, assume forward scan,
     * load starting at target address.  If target address < current window,
     * assume backward scan, load so that target address is top of window.
     * Note that when switching from forward write to forward read, will have
     * start_row = 0, so the limiting case applies and we load from 0 anyway.
     */
    if (start_row > ptr->cur_start_row) {
      ptr->cur_start_row = start_row;
    } else {
      ptr->cur_start_row = start_row + ptr->unitheight - ptr->rows_in_mem;
      if (ptr->cur_start_row < 0)
	ptr->cur_start_row = 0;	/* don't fall off front end of file */
    }
    /* If reading, read in the selected part of the array. 
     * If we are writing, we need not pre-read the selected portion,
     * since the access sequence constraints ensure it would be garbage.
     */
    if (! writable) {
      do_sarray_io(ptr, FALSE);
    }
  }
  /* Flag the buffer dirty if caller will write in it */
  if (writable)
    ptr->dirty = TRUE;
  /* Return address of proper part of the buffer */
  return ptr->mem_buffer + (start_row - ptr->cur_start_row);
}


METHODDEF JBLOCKARRAY
access_big_barray (big_barray_ptr ptr, long start_row, boolean writable)
/* Access the part of a "big" coefficient-block array starting at start_row */
/* and extending for ptr->unitheight rows.  writable is true if  */
/* caller intends to modify the accessed area. */
{
  /* debugging check */
  if (start_row < 0 || start_row+ptr->unitheight > ptr->rows_in_array ||
      ptr->mem_buffer == NULL)
    ERREXIT(methods, "Bogus access_big_barray request");

  /* Make the desired part of the virtual array accessible */
  if (start_row < ptr->cur_start_row ||
      start_row+ptr->unitheight > ptr->cur_start_row+ptr->rows_in_mem) {
    if (! ptr->b_s_open)
      ERREXIT(methods, "Virtual array controller messed up");
    /* Flush old buffer contents if necessary */
    if (ptr->dirty) {
      do_barray_io(ptr, TRUE);
      ptr->dirty = FALSE;
    }
    /* Decide what part of virtual array to access.
     * Algorithm: if target address > current window, assume forward scan,
     * load starting at target address.  If target address < current window,
     * assume backward scan, load so that target address is top of window.
     * Note that when switching from forward write to forward read, will have
     * start_row = 0, so the limiting case applies and we load from 0 anyway.
     */
    if (start_row > ptr->cur_start_row) {
      ptr->cur_start_row = start_row;
    } else {
      ptr->cur_start_row = start_row + ptr->unitheight - ptr->rows_in_mem;
      if (ptr->cur_start_row < 0)
	ptr->cur_start_row = 0;	/* don't fall off front end of file */
    }
    /* If reading, read in the selected part of the array. 
     * If we are writing, we need not pre-read the selected portion,
     * since the access sequence constraints ensure it would be garbage.
     */
    if (! writable) {
      do_barray_io(ptr, FALSE);
    }
  }
  /* Flag the buffer dirty if caller will write in it */
  if (writable)
    ptr->dirty = TRUE;
  /* Return address of proper part of the buffer */
  return ptr->mem_buffer + (start_row - ptr->cur_start_row);
}


METHODDEF void
free_big_sarray (big_sarray_ptr ptr)
/* Free a "big" (virtual-memory) 2-D sample array */
{
  big_sarray_ptr * llink;

  /* Remove item from list -- linear search is fast enough */
  llink = &big_sarray_list;
  while (*llink != ptr) {
    if (*llink == NULL)
      ERREXIT(methods, "Bogus free_big_sarray request");
    llink = &( (*llink)->next );
  }
  *llink = ptr->next;

  if (ptr->b_s_open)		/* there may be no backing store */
    (*ptr->b_s_info.close_backing_store) (& ptr->b_s_info);

  if (ptr->mem_buffer != NULL)	/* just in case never realized */
    free_small_sarray(ptr->mem_buffer);

  free_small((void *) ptr);	/* free the control block too */
}


METHODDEF void
free_big_barray (big_barray_ptr ptr)
/* Free a "big" (virtual-memory) 2-D coefficient-block array */
{
  big_barray_ptr * llink;

  /* Remove item from list -- linear search is fast enough */
  llink = &big_barray_list;
  while (*llink != ptr) {
    if (*llink == NULL)
      ERREXIT(methods, "Bogus free_big_barray request");
    llink = &( (*llink)->next );
  }
  *llink = ptr->next;

  if (ptr->b_s_open)		/* there may be no backing store */
    (*ptr->b_s_info.close_backing_store) (& ptr->b_s_info);

  if (ptr->mem_buffer != NULL)	/* just in case never realized */
    free_small_barray(ptr->mem_buffer);

  free_small((void *) ptr);	/* free the control block too */
}


/*
 * Cleanup: free anything that's been allocated since jselmemmgr().
 */

METHODDEF void
free_all (void)
{
  /* First free any open "big" arrays -- these may release small arrays */
  while (big_sarray_list != NULL)
    free_big_sarray(big_sarray_list);
  while (big_barray_list != NULL)
    free_big_barray(big_barray_list);
  /* Free any open small arrays -- these may release small objects */
  /* +1's are because we must pass a pointer to the data, not the header */
  while (small_sarray_list != NULL)
    free_small_sarray((JSAMPARRAY) (small_sarray_list + 1));
  while (small_barray_list != NULL)
    free_small_barray((JBLOCKARRAY) (small_barray_list + 1));
  /* Free any remaining small objects */
  while (small_list != NULL)
    free_small((void *) (small_list + 1));
#ifdef NEED_ALLOC_MEDIUM
  while (medium_list != NULL)
    free_medium((void FAR *) (medium_list + 1));
#endif

  jmem_term();			/* system-dependent cleanup */

#ifdef MEM_STATS
  if (methods->trace_level > 0)
    print_mem_stats();		/* print optional memory usage statistics */
#endif
}


/*
 * The method selection routine for virtual memory systems.
 * The system-dependent setup routine should call this routine
 * to install the necessary method pointers in the supplied struct.
 */

GLOBAL void
jselmemmgr (external_methods_ptr emethods)
{
  methods = emethods;		/* save struct addr for error exit access */

  emethods->alloc_small = alloc_small;
  emethods->free_small = free_small;
#ifdef NEED_ALLOC_MEDIUM
  emethods->alloc_medium = alloc_medium;
  emethods->free_medium = free_medium;
#else
  emethods->alloc_medium = alloc_small;
  emethods->free_medium = free_small;
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
  emethods->free_all = free_all;

  /* Initialize list headers to empty */
  small_list = NULL;
#ifdef NEED_ALLOC_MEDIUM
  medium_list = NULL;
#endif
  small_sarray_list = NULL;
  small_barray_list = NULL;
  big_sarray_list = NULL;
  big_barray_list = NULL;

  jmem_init(emethods);		/* system-dependent initialization */

  /* Check for an environment variable JPEGMEM; if found, override the
   * default max_memory setting from jmem_init.  Note that a command line
   * -m argument may again override this value.
   * If your system doesn't support getenv(), define NO_GETENV to disable
   * this feature.
   */
#ifndef NO_GETENV
  { char * memenv;

    if ((memenv = getenv("JPEGMEM")) != NULL) {
      long lval;
      char ch = 'x';

      if (sscanf(memenv, "%ld%c", &lval, &ch) > 0) {
	if (ch == 'm' || ch == 'M')
	  lval *= 1000L;
	emethods->max_memory_to_use = lval * 1000L;
      }
    }
  }
#endif

}

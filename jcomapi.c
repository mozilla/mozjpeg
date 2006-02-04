/*
 * jcomapi.c
 *
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * ---------------------------------------------------------------------
 * x86 SIMD extension for IJG JPEG library
 * Copyright (C) 1999-2006, MIYASAKA Masaru.
 * This file has been modified for SIMD extension.
 * Last Modified : March 11, 2005
 * ---------------------------------------------------------------------
 *
 * This file contains application interface routines that are used for both
 * compression and decompression.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/*
 * Abort processing of a JPEG compression or decompression operation,
 * but don't destroy the object itself.
 *
 * For this, we merely clean up all the nonpermanent memory pools.
 * Note that temp files (virtual arrays) are not allowed to belong to
 * the permanent pool, so we will be able to close all temp files here.
 * Closing a data source or destination, if necessary, is the application's
 * responsibility.
 */

GLOBAL(void)
jpeg_abort (j_common_ptr cinfo)
{
  int pool;

  /* Do nothing if called on a not-initialized or destroyed JPEG object. */
  if (cinfo->mem == NULL)
    return;

  /* Releasing pools in reverse order might help avoid fragmentation
   * with some (brain-damaged) malloc libraries.
   */
  for (pool = JPOOL_NUMPOOLS-1; pool > JPOOL_PERMANENT; pool--) {
    (*cinfo->mem->free_pool) (cinfo, pool);
  }

  /* Reset overall state for possible reuse of object */
  if (cinfo->is_decompressor) {
    cinfo->global_state = DSTATE_START;
    /* Try to keep application from accessing now-deleted marker list.
     * A bit kludgy to do it here, but this is the most central place.
     */
    ((j_decompress_ptr) cinfo)->marker_list = NULL;
  } else {
    cinfo->global_state = CSTATE_START;
  }
}


/*
 * Destruction of a JPEG object.
 *
 * Everything gets deallocated except the master jpeg_compress_struct itself
 * and the error manager struct.  Both of these are supplied by the application
 * and must be freed, if necessary, by the application.  (Often they are on
 * the stack and so don't need to be freed anyway.)
 * Closing a data source or destination, if necessary, is the application's
 * responsibility.
 */

GLOBAL(void)
jpeg_destroy (j_common_ptr cinfo)
{
  /* We need only tell the memory manager to release everything. */
  /* NB: mem pointer is NULL if memory mgr failed to initialize. */
  if (cinfo->mem != NULL)
    (*cinfo->mem->self_destruct) (cinfo);
  cinfo->mem = NULL;		/* be safe if jpeg_destroy is called twice */
  cinfo->global_state = 0;	/* mark it destroyed */
}


/*
 * Convenience routines for allocating quantization and Huffman tables.
 * (Would jutils.c be a more reasonable place to put these?)
 */

GLOBAL(JQUANT_TBL *)
jpeg_alloc_quant_table (j_common_ptr cinfo)
{
  JQUANT_TBL *tbl;

  tbl = (JQUANT_TBL *)
    (*cinfo->mem->alloc_small) (cinfo, JPOOL_PERMANENT, SIZEOF(JQUANT_TBL));
  tbl->sent_table = FALSE;	/* make sure this is false in any new table */
  return tbl;
}


GLOBAL(JHUFF_TBL *)
jpeg_alloc_huff_table (j_common_ptr cinfo)
{
  JHUFF_TBL *tbl;

  tbl = (JHUFF_TBL *)
    (*cinfo->mem->alloc_small) (cinfo, JPOOL_PERMANENT, SIZEOF(JHUFF_TBL));
  tbl->sent_table = FALSE;	/* make sure this is false in any new table */
  return tbl;
}


/*
 * SIMD Ext: Checking for support of SIMD instruction set.
 */

GLOBAL(unsigned int)
jpeg_simd_support (j_common_ptr cinfo)
{
  enum { JSIMD_INVALID = ~0 };
  static volatile unsigned int simd_supported = JSIMD_INVALID;

  if (simd_supported == JSIMD_INVALID)
    simd_supported = jpeg_simd_os_support(jpeg_simd_cpu_support());

#ifndef JSIMD_MASKFUNC_NOT_SUPPORTED
  if (cinfo != NULL)	/* Turn off the masked flags */
    return simd_supported & ~jpeg_simd_mask(cinfo, JSIMD_NONE, JSIMD_NONE);
#endif
  return simd_supported;
}

#ifndef JSIMD_MASKFUNC_NOT_SUPPORTED

/*
 * SIMD Ext: modify/retrieve SIMD instruction mask
 */

GLOBAL(unsigned int)
jpeg_simd_mask (j_common_ptr cinfo, unsigned int remove, unsigned int add)
{
  unsigned long *gp;
  unsigned int oldmask;

  if (cinfo->is_decompressor)
    gp = (unsigned long *) &((j_decompress_ptr) cinfo)->output_gamma;
  else	/* compressor */
    gp = (unsigned long *) &((j_compress_ptr) cinfo)->input_gamma;

  if ((gp[1] == 0x3FF00000 || gp[1] == 0x00000000) &&	/* +1.0 or +0.0 */
      (gp[0] & ~JSIMD_ALL) == 0) {
    oldmask = gp[0];
    if (((remove | add) & ~JSIMD_ALL) == 0)
      gp[0] = (oldmask & ~remove) | add;
  } else {
    oldmask = 0;	/* error */
  }
  return oldmask;
}

#endif /* !JSIMD_MASKFUNC_NOT_SUPPORTED */

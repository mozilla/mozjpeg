/*
 * jccoefct.c
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1994-1997, Thomas G. Lane.
 * libjpeg-turbo Modifications:
 * Copyright (C) 2022, 2024, D. R. Commander.
 * Copyright (C) 2014, Mozilla Corporation.
 * For conditions of distribution and use, see the accompanying README.ijg
 * file.
 *
 * This file contains the coefficient buffer controller for compression.
 * This controller is the top level of the lossy JPEG compressor proper.
 * The coefficient buffer lies between forward-DCT and entropy encoding steps.
 */

#define JPEG_INTERNALS
#include "jinclude.h"
#include "jpeglib.h"
#include "jsamplecomp.h"
#include "jchuff.h"
#ifdef WITH_TRELLIS_THREADS
#include <pthread.h>
#include <unistd.h>
#endif

/* Upper bound on JINT_TRELLIS_THREADS */
#define MAX_TRELLIS_THREADS  64

/* We use a full-image coefficient buffer when doing Huffman optimization,
 * and also for writing multiple-scan JPEG files.  In all cases, the DCT
 * step is run during the first pass, and subsequent passes need only read
 * the buffered coefficients.
 */
#ifdef ENTROPY_OPT_SUPPORTED
#define FULL_COEF_BUFFER_SUPPORTED
#else
#ifdef C_MULTISCAN_FILES_SUPPORTED
#define FULL_COEF_BUFFER_SUPPORTED
#endif
#endif


/* Private buffer controller object */

typedef struct {
  struct jpeg_c_coef_controller pub; /* public fields */

  JDIMENSION iMCU_row_num;      /* iMCU row # within image */
  JDIMENSION mcu_ctr;           /* counts MCUs processed in current row */
  int MCU_vert_offset;          /* counts MCU rows within iMCU row */
  int MCU_rows_per_iMCU_row;    /* number of such rows needed */

  /* For single-pass compression, it's sufficient to buffer just one MCU
   * (although this may prove a bit slow in practice).  We allocate a
   * workspace of C_MAX_BLOCKS_IN_MCU coefficient blocks, and reuse it for each
   * MCU constructed and sent.  In multi-pass modes, this array points to the
   * current MCU's blocks within the virtual arrays.
   */
  JBLOCKROW MCU_buffer[C_MAX_BLOCKS_IN_MCU];

  /* In multi-pass modes, we need a virtual block array for each component. */
  jvirt_barray_ptr whole_image[MAX_COMPONENTS];

  /* when using trellis quantization, need to keep a copy of all unquantized coefficients */
  jvirt_barray_ptr whole_image_uq[MAX_COMPONENTS];

  /* Trellis quantization working memory, one per thread */
  trellis_scratch *scratch[MAX_TRELLIS_THREADS];

} my_coef_controller;

typedef my_coef_controller *my_coef_ptr;


/* Forward declarations */
METHODDEF(boolean) compress_data(j_compress_ptr cinfo, _JSAMPIMAGE input_buf);
#ifdef FULL_COEF_BUFFER_SUPPORTED
METHODDEF(boolean) compress_first_pass(j_compress_ptr cinfo,
                                       _JSAMPIMAGE input_buf);
METHODDEF(boolean) compress_output(j_compress_ptr cinfo,
                                   _JSAMPIMAGE input_buf);
#endif
METHODDEF(boolean) compress_trellis_pass
        (j_compress_ptr cinfo, JSAMPIMAGE input_buf);


LOCAL(void)
start_iMCU_row(j_compress_ptr cinfo)
/* Reset within-iMCU-row counters for a new row */
{
  my_coef_ptr coef = (my_coef_ptr)cinfo->coef;

  /* In an interleaved scan, an MCU row is the same as an iMCU row.
   * In a noninterleaved scan, an iMCU row has v_samp_factor MCU rows.
   * But at the bottom of the image, process only what's left.
   */
  if (cinfo->comps_in_scan > 1) {
    coef->MCU_rows_per_iMCU_row = 1;
  } else {
    if (coef->iMCU_row_num < (cinfo->total_iMCU_rows - 1))
      coef->MCU_rows_per_iMCU_row = cinfo->cur_comp_info[0]->v_samp_factor;
    else
      coef->MCU_rows_per_iMCU_row = cinfo->cur_comp_info[0]->last_row_height;
  }

  coef->mcu_ctr = 0;
  coef->MCU_vert_offset = 0;
}


/*
 * Initialize for a processing pass.
 */

METHODDEF(void)
start_pass_coef(j_compress_ptr cinfo, J_BUF_MODE pass_mode)
{
  my_coef_ptr coef = (my_coef_ptr)cinfo->coef;

  coef->iMCU_row_num = 0;
  start_iMCU_row(cinfo);

  switch (pass_mode) {
  case JBUF_PASS_THRU:
    if (coef->whole_image[0] != NULL)
      ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    coef->pub._compress_data = compress_data;
    break;
#ifdef FULL_COEF_BUFFER_SUPPORTED
  case JBUF_SAVE_AND_PASS:
    if (coef->whole_image[0] == NULL)
      ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    coef->pub._compress_data = compress_first_pass;
    break;
  case JBUF_CRANK_DEST:
    if (coef->whole_image[0] == NULL)
      ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    coef->pub._compress_data = compress_output;
    break;
#endif
#if BITS_IN_JSAMPLE == 8
  case JBUF_REQUANT:
    if (coef->whole_image[0] == NULL)
      ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    coef->pub.compress_data = compress_trellis_pass;
    break;
#endif
      
  default:
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
    break;
  }
}


/*
 * Process some data in the single-pass case.
 * We process the equivalent of one fully interleaved MCU row ("iMCU" row)
 * per call, ie, v_samp_factor block rows for each component in the image.
 * Returns TRUE if the iMCU row is completed, FALSE if suspended.
 *
 * NB: input_buf contains a plane for each component in image,
 * which we index according to the component's SOF position.
 */

METHODDEF(boolean)
compress_data(j_compress_ptr cinfo, _JSAMPIMAGE input_buf)
{
  my_coef_ptr coef = (my_coef_ptr)cinfo->coef;
  JDIMENSION MCU_col_num;       /* index of current MCU within row */
  JDIMENSION last_MCU_col = cinfo->MCUs_per_row - 1;
  JDIMENSION last_iMCU_row = cinfo->total_iMCU_rows - 1;
  int blkn, bi, ci, yindex, yoffset, blockcnt;
  JDIMENSION ypos, xpos;
  jpeg_component_info *compptr;

  /* Loop to write as much as one whole iMCU row */
  for (yoffset = coef->MCU_vert_offset; yoffset < coef->MCU_rows_per_iMCU_row;
       yoffset++) {
    for (MCU_col_num = coef->mcu_ctr; MCU_col_num <= last_MCU_col;
         MCU_col_num++) {
      /* Determine where data comes from in input_buf and do the DCT thing.
       * Each call on forward_DCT processes a horizontal row of DCT blocks
       * as wide as an MCU; we rely on having allocated the MCU_buffer[] blocks
       * sequentially.  Dummy blocks at the right or bottom edge are filled in
       * specially.  The data in them does not matter for image reconstruction,
       * so we fill them with values that will encode to the smallest amount of
       * data, viz: all zeroes in the AC entries, DC entries equal to previous
       * block's DC value.  (Thanks to Thomas Kinsman for this idea.)
       */
      blkn = 0;
      for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
        compptr = cinfo->cur_comp_info[ci];
        blockcnt = (MCU_col_num < last_MCU_col) ? compptr->MCU_width :
                                                  compptr->last_col_width;
        xpos = MCU_col_num * compptr->MCU_sample_width;
        ypos = yoffset * DCTSIZE; /* ypos == (yoffset+yindex) * DCTSIZE */
        for (yindex = 0; yindex < compptr->MCU_height; yindex++) {
          if (coef->iMCU_row_num < last_iMCU_row ||
              yoffset + yindex < compptr->last_row_height) {
            (*cinfo->fdct->_forward_DCT) (cinfo, compptr,
                                          input_buf[compptr->component_index],
                                          coef->MCU_buffer[blkn],
                                          ypos, xpos, (JDIMENSION)blockcnt,
                                          NULL);
            if (blockcnt < compptr->MCU_width) {
              /* Create some dummy blocks at the right edge of the image. */
              jzero_far((void *)coef->MCU_buffer[blkn + blockcnt],
                        (compptr->MCU_width - blockcnt) * sizeof(JBLOCK));
              for (bi = blockcnt; bi < compptr->MCU_width; bi++) {
                coef->MCU_buffer[blkn + bi][0][0] =
                  coef->MCU_buffer[blkn + bi - 1][0][0];
              }
            }
          } else {
            /* Create a row of dummy blocks at the bottom of the image. */
            jzero_far((void *)coef->MCU_buffer[blkn],
                      compptr->MCU_width * sizeof(JBLOCK));
            for (bi = 0; bi < compptr->MCU_width; bi++) {
              coef->MCU_buffer[blkn + bi][0][0] =
                coef->MCU_buffer[blkn - 1][0][0];
            }
          }
          blkn += compptr->MCU_width;
          ypos += DCTSIZE;
        }
      }
      /* Try to write the MCU.  In event of a suspension failure, we will
       * re-DCT the MCU on restart (a bit inefficient, could be fixed...)
       */
      if (!(*cinfo->entropy->encode_mcu) (cinfo, coef->MCU_buffer)) {
        /* Suspension forced; update state counters and exit */
        coef->MCU_vert_offset = yoffset;
        coef->mcu_ctr = MCU_col_num;
        return FALSE;
      }
    }
    /* Completed an MCU row, but perhaps not an iMCU row */
    coef->mcu_ctr = 0;
  }
  /* Completed the iMCU row, advance counters for next one */
  coef->iMCU_row_num++;
  start_iMCU_row(cinfo);
  return TRUE;
}


#ifdef FULL_COEF_BUFFER_SUPPORTED

/*
 * Process some data in the first pass of a multi-pass case.
 * We process the equivalent of one fully interleaved MCU row ("iMCU" row)
 * per call, ie, v_samp_factor block rows for each component in the image.
 * This amount of data is read from the source buffer, DCT'd and quantized,
 * and saved into the virtual arrays.  We also generate suitable dummy blocks
 * as needed at the right and lower edges.  (The dummy blocks are constructed
 * in the virtual arrays, which have been padded appropriately.)  This makes
 * it possible for subsequent passes not to worry about real vs. dummy blocks.
 *
 * We must also emit the data to the entropy encoder.  This is conveniently
 * done by calling compress_output() after we've loaded the current strip
 * of the virtual arrays.
 *
 * NB: input_buf contains a plane for each component in image.  All
 * components are DCT'd and loaded into the virtual arrays in this pass.
 * However, it may be that only a subset of the components are emitted to
 * the entropy encoder during this first pass; be careful about looking
 * at the scan-dependent variables (MCU dimensions, etc).
 */

METHODDEF(boolean)
compress_first_pass(j_compress_ptr cinfo, _JSAMPIMAGE input_buf)
{
  my_coef_ptr coef = (my_coef_ptr)cinfo->coef;
  JDIMENSION last_iMCU_row = cinfo->total_iMCU_rows - 1;
  JDIMENSION blocks_across, MCUs_across, MCUindex;
  int bi, ci, h_samp_factor, block_row, block_rows, ndummy;
  JCOEF lastDC;
  jpeg_component_info *compptr;
  JBLOCKARRAY buffer;
  JBLOCKROW thisblockrow, lastblockrow;
  JBLOCKARRAY buffer_dst;

  for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
       ci++, compptr++) {
    /* Align the virtual buffer for this component. */
    buffer = (*cinfo->mem->access_virt_barray)
      ((j_common_ptr)cinfo, coef->whole_image[ci],
       coef->iMCU_row_num * compptr->v_samp_factor,
       (JDIMENSION)compptr->v_samp_factor, TRUE);
    
    buffer_dst = (*cinfo->mem->access_virt_barray)
      ((j_common_ptr) cinfo, coef->whole_image_uq[ci],
       coef->iMCU_row_num * compptr->v_samp_factor,
       (JDIMENSION)compptr->v_samp_factor, TRUE);
    
    /* Count non-dummy DCT block rows in this iMCU row. */
    if (coef->iMCU_row_num < last_iMCU_row)
      block_rows = compptr->v_samp_factor;
    else {
      /* NB: can't use last_row_height here, since may not be set! */
      block_rows = (int)(compptr->height_in_blocks % compptr->v_samp_factor);
      if (block_rows == 0) block_rows = compptr->v_samp_factor;
    }
    blocks_across = compptr->width_in_blocks;
    h_samp_factor = compptr->h_samp_factor;
    /* Count number of dummy blocks to be added at the right margin. */
    ndummy = (int)(blocks_across % h_samp_factor);
    if (ndummy > 0)
      ndummy = h_samp_factor - ndummy;
    /* Perform DCT for all non-dummy blocks in this iMCU row.  Each call
     * on forward_DCT processes a complete horizontal row of DCT blocks.
     */
    for (block_row = 0; block_row < block_rows; block_row++) {
      thisblockrow = buffer[block_row];
      (*cinfo->fdct->_forward_DCT) (cinfo, compptr,
                                    input_buf[ci], thisblockrow,
                                    (JDIMENSION)(block_row * DCTSIZE),
                                    (JDIMENSION)0, blocks_across,
                                    buffer_dst[block_row]);
      if (ndummy > 0) {
        /* Create dummy blocks at the right edge of the image. */
        thisblockrow += blocks_across; /* => first dummy block */
        jzero_far((void *)thisblockrow, ndummy * sizeof(JBLOCK));
        lastDC = thisblockrow[-1][0];
        for (bi = 0; bi < ndummy; bi++) {
          thisblockrow[bi][0] = lastDC;
        }
      }
    }
    /* If at end of image, create dummy block rows as needed.
     * The tricky part here is that within each MCU, we want the DC values
     * of the dummy blocks to match the last real block's DC value.
     * This squeezes a few more bytes out of the resulting file...
     */
    if (coef->iMCU_row_num == last_iMCU_row) {
      blocks_across += ndummy;  /* include lower right corner */
      MCUs_across = blocks_across / h_samp_factor;
      for (block_row = block_rows; block_row < compptr->v_samp_factor;
           block_row++) {
        thisblockrow = buffer[block_row];
        lastblockrow = buffer[block_row - 1];
        jzero_far((void *)thisblockrow,
                  (size_t)(blocks_across * sizeof(JBLOCK)));
        for (MCUindex = 0; MCUindex < MCUs_across; MCUindex++) {
          lastDC = lastblockrow[h_samp_factor - 1][0];
          for (bi = 0; bi < h_samp_factor; bi++) {
            thisblockrow[bi][0] = lastDC;
          }
          thisblockrow += h_samp_factor; /* advance to next MCU in row */
          lastblockrow += h_samp_factor;
        }
      }
    }
  }
  /* NB: compress_output will increment iMCU_row_num if successful.
   * A suspension return will result in redoing all the work above next time.
   */

  /* Emit data to the entropy encoder, sharing code with subsequent passes */
  return compress_output(cinfo, input_buf);
}

#if BITS_IN_JSAMPLE == 8
/*
 * Trellis-quantize one iMCU row of one component.  An iMCU row depends only
 * on its own unquantized coefficients and on Huffman tables that are fixed for
 * the pass: the DC predictor restarts at 0, and the block row above is only
 * consulted within the iMCU row.  That is what lets rows run in parallel.
 */
LOCAL(void)
trellis_iMCU_row(j_compress_ptr cinfo, jpeg_component_info *compptr,
                 c_derived_tbl *dctbl, c_derived_tbl *actbl, void *arith_r,
                 trellis_scratch *scratch, JBLOCKARRAY buffer,
                 JBLOCKARRAY buffer_dst, JDIMENSION iMCU_row_num)
{
  JDIMENSION last_iMCU_row = cinfo->total_iMCU_rows - 1;
  JDIMENSION blocks_across, MCUs_across, MCUindex;
  int bi, h_samp_factor, block_row, block_rows, ndummy;
  JCOEF lastDC;
  JBLOCKROW thisblockrow, lastblockrow;

  /* Count non-dummy DCT block rows in this iMCU row. */
  if (iMCU_row_num < last_iMCU_row)
    block_rows = compptr->v_samp_factor;
  else {
    /* NB: can't use last_row_height here, since may not be set! */
    block_rows = (int) (compptr->height_in_blocks % compptr->v_samp_factor);
    if (block_rows == 0) block_rows = compptr->v_samp_factor;
  }
  blocks_across = compptr->width_in_blocks;
  h_samp_factor = compptr->h_samp_factor;
  /* Count number of dummy blocks to be added at the right margin. */
  ndummy = (int) (blocks_across % h_samp_factor);
  if (ndummy > 0)
    ndummy = h_samp_factor - ndummy;

  lastDC = 0;

  /* Perform DCT for all non-dummy blocks in this iMCU row.  Each call
   * on forward_DCT processes a complete horizontal row of DCT blocks.
   */
  for (block_row = 0; block_row < block_rows; block_row++) {
    thisblockrow = buffer[block_row];
    lastblockrow = (block_row > 0) ? buffer[block_row-1] : NULL;
#ifdef C_ARITH_CODING_SUPPORTED
    if (cinfo->arith_code)
      quantize_trellis_arith(cinfo, (arith_rates *) arith_r, thisblockrow,
                             buffer_dst[block_row], blocks_across,
                             cinfo->quant_tbl_ptrs[compptr->quant_tbl_no],
                             cinfo->master->norm_src[compptr->quant_tbl_no],
                             cinfo->master->norm_coef[compptr->quant_tbl_no],
                             &lastDC, lastblockrow, buffer_dst[block_row-1]);
    else
#endif
      quantize_trellis(cinfo, dctbl, actbl, thisblockrow,
                       buffer_dst[block_row], blocks_across,
                       cinfo->quant_tbl_ptrs[compptr->quant_tbl_no],
                       cinfo->master->norm_src[compptr->quant_tbl_no],
                       cinfo->master->norm_coef[compptr->quant_tbl_no],
                       &lastDC, lastblockrow, buffer_dst[block_row-1],
                       scratch);

    if (ndummy > 0) {
      /* Create dummy blocks at the right edge of the image. */
      thisblockrow += blocks_across; /* => first dummy block */
      jzero_far((void *) thisblockrow, ndummy * sizeof(JBLOCK));
      lastDC = thisblockrow[-1][0];
      for (bi = 0; bi < ndummy; bi++) {
        thisblockrow[bi][0] = lastDC;
      }
    }
  }
  /* If at end of image, create dummy block rows as needed.
   * The tricky part here is that within each MCU, we want the DC values
   * of the dummy blocks to match the last real block's DC value.
   * This squeezes a few more bytes out of the resulting file...
   */
  if (iMCU_row_num == last_iMCU_row) {
    blocks_across += ndummy;  /* include lower right corner */
    MCUs_across = blocks_across / h_samp_factor;
    for (block_row = block_rows; block_row < compptr->v_samp_factor;
         block_row++) {
      thisblockrow = buffer[block_row];
      lastblockrow = buffer[block_row-1];
      jzero_far((void *) thisblockrow,
                (size_t) (blocks_across * sizeof(JBLOCK)));
      for (MCUindex = 0; MCUindex < MCUs_across; MCUindex++) {
        lastDC = lastblockrow[h_samp_factor-1][0];
        for (bi = 0; bi < h_samp_factor; bi++) {
          thisblockrow[bi][0] = lastDC;
        }
        thisblockrow += h_samp_factor; /* advance to next MCU in row */
        lastblockrow += h_samp_factor;
      }
    }
  }
}

/* Scratch space for one trellis thread, allocated on first use and reused by
 * every later pass.
 */
LOCAL(trellis_scratch *)
get_trellis_scratch(j_compress_ptr cinfo, int thread)
{
  my_coef_ptr coef = (my_coef_ptr) cinfo->coef;
  JDIMENSION max_blocks = 0;
  int ci;

  if (coef->scratch[thread] == NULL) {
    for (ci = 0; ci < cinfo->num_components; ci++)
      max_blocks = MAX(max_blocks, cinfo->comp_info[ci].width_in_blocks);
    coef->scratch[thread] = jalloc_trellis_scratch(cinfo, max_blocks);
  }
  return coef->scratch[thread];
}

#ifdef WITH_TRELLIS_THREADS

typedef struct {
  j_compress_ptr cinfo;
  c_derived_tbl dctbl[MAX_COMPS_IN_SCAN], actbl[MAX_COMPS_IN_SCAN];
  JBLOCKARRAY *buffer, *buffer_dst;     /* per job: component-major */
  JDIMENSION num_jobs, next_job;
  pthread_mutex_t lock;
} trellis_pass;

typedef struct {
  trellis_pass *pass;
  trellis_scratch *scratch;
} trellis_worker;

/* Runs on each thread, including the calling one.  Nothing here may call the
 * error handler: it would longjmp (or exit) on a thread that does not own
 * cinfo.  That is why the scratch space is allocated before the threads start.
 */
static void *
trellis_worker_main(void *arg)
{
  trellis_worker *worker = (trellis_worker *) arg;
  trellis_pass *pass = worker->pass;
  j_compress_ptr cinfo = pass->cinfo;
  JDIMENSION job, rows = cinfo->total_iMCU_rows;
  int ci;

  for (;;) {
    pthread_mutex_lock(&pass->lock);
    job = pass->next_job++;
    pthread_mutex_unlock(&pass->lock);
    if (job >= pass->num_jobs)
      break;
    ci = (int) (job / rows);
    trellis_iMCU_row(cinfo, cinfo->cur_comp_info[ci], &pass->dctbl[ci],
                     &pass->actbl[ci], NULL, worker->scratch,
                     pass->buffer[job], pass->buffer_dst[job], job % rows);
  }
  return NULL;
}

LOCAL(int)
trellis_thread_count(j_compress_ptr cinfo)
{
  int threads = cinfo->master->trellis_threads;

  if (threads == 0) {
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    threads = cores > 0 ? (int) cores : 1;
  }
  return MIN(threads, MAX_TRELLIS_THREADS);
}

/*
 * Trellis-quantize every iMCU row of the scan up front, across threads.  Rows
 * are independent (see trellis_iMCU_row) and each writes only its own blocks,
 * so the result is the single-threaded one byte for byte.  Returns FALSE,
 * having done nothing, when the pass must run row by row instead:
 * trellis_q_opt sums floats across rows, so their order matters, and
 * arithmetic coding is not parallelized.
 *
 * The virtual arrays are accessed here, serially, because access_virt_barray
 * is not thread-safe.  Holding every row's pointers at once is valid because
 * this library never uses a backing store: the arrays are fully in memory.
 */
LOCAL(boolean)
trellis_all_rows(j_compress_ptr cinfo)
{
  my_coef_ptr coef = (my_coef_ptr) cinfo->coef;
  JDIMENSION rows = cinfo->total_iMCU_rows, row, job;
  trellis_pass pass;
  trellis_worker workers[MAX_TRELLIS_THREADS];
  pthread_t threads[MAX_TRELLIS_THREADS];
  int num_threads, started, ci, i;

  if (cinfo->arith_code || cinfo->master->trellis_q_opt)
    return FALSE;
  pass.num_jobs = rows * (JDIMENSION) cinfo->comps_in_scan;
  num_threads = trellis_thread_count(cinfo);
  if ((JDIMENSION) num_threads > pass.num_jobs)
    num_threads = (int) pass.num_jobs;
  if (num_threads < 2)
    return FALSE;

  pass.cinfo = cinfo;
  pass.next_job = 0;
  pass.buffer = (JBLOCKARRAY *)
    (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
                                2 * pass.num_jobs * sizeof(JBLOCKARRAY));
  pass.buffer_dst = pass.buffer + pass.num_jobs;
  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    jpeg_component_info *compptr = cinfo->cur_comp_info[ci];
    c_derived_tbl *dctbl = &pass.dctbl[ci], *actbl = &pass.actbl[ci];

    jpeg_make_c_derived_tbl(cinfo, TRUE, compptr->dc_tbl_no, &dctbl);
    jpeg_make_c_derived_tbl(cinfo, FALSE, compptr->ac_tbl_no, &actbl);
    for (row = 0; row < rows; row++) {
      job = (JDIMENSION) ci * rows + row;
      pass.buffer[job] = (*cinfo->mem->access_virt_barray)
        ((j_common_ptr) cinfo, coef->whole_image[compptr->component_index],
         row * compptr->v_samp_factor, (JDIMENSION) compptr->v_samp_factor,
         TRUE);
      pass.buffer_dst[job] = (*cinfo->mem->access_virt_barray)
        ((j_common_ptr) cinfo, coef->whole_image_uq[compptr->component_index],
         row * compptr->v_samp_factor, (JDIMENSION) compptr->v_samp_factor,
         TRUE);
    }
  }
  for (i = 0; i < num_threads; i++) {
    workers[i].pass = &pass;
    workers[i].scratch = get_trellis_scratch(cinfo, i);
  }

  if (pthread_mutex_init(&pass.lock, NULL) != 0)
    return FALSE;
  /* If a thread fails to start, the others take its share of the rows. */
  for (started = 1; started < num_threads; started++)
    if (pthread_create(&threads[started], NULL, trellis_worker_main,
                       &workers[started]) != 0)
      break;
  trellis_worker_main(&workers[0]);
  for (i = 1; i < started; i++)
    pthread_join(threads[i], NULL);
  pthread_mutex_destroy(&pass.lock);
  return TRUE;
}

#else

LOCAL(boolean)
trellis_all_rows(j_compress_ptr cinfo)
{
  return FALSE;
}

#endif /* WITH_TRELLIS_THREADS */

METHODDEF(boolean)
compress_trellis_pass_emit(j_compress_ptr cinfo, JSAMPIMAGE input_buf)
{
  /* Every row was quantized when the pass started; only emit them now. */
  return compress_output(cinfo, input_buf);
}

METHODDEF(boolean)
compress_trellis_pass (j_compress_ptr cinfo, JSAMPIMAGE input_buf)
{
  my_coef_ptr coef = (my_coef_ptr) cinfo->coef;
  int ci;
  jpeg_component_info *compptr;
  JBLOCKARRAY buffer;
  JBLOCKARRAY buffer_dst;

  if (coef->iMCU_row_num == 0 && trellis_all_rows(cinfo)) {
    coef->pub.compress_data = compress_trellis_pass_emit;
    return compress_output(cinfo, input_buf);
  }

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    c_derived_tbl dctbl_data;
    c_derived_tbl *dctbl = &dctbl_data;
    c_derived_tbl actbl_data;
    c_derived_tbl *actbl = &actbl_data;
    void *arith_r = NULL;

#ifdef C_ARITH_CODING_SUPPORTED
    arith_rates arith_r_data;
#endif

    compptr = cinfo->cur_comp_info[ci];

#ifdef C_ARITH_CODING_SUPPORTED
    if (cinfo->arith_code) {
      jget_arith_rates(cinfo, compptr->dc_tbl_no, compptr->ac_tbl_no,
                       &arith_r_data);
      arith_r = &arith_r_data;
    } else
#endif
    {
      jpeg_make_c_derived_tbl(cinfo, TRUE, compptr->dc_tbl_no, &dctbl);
      jpeg_make_c_derived_tbl(cinfo, FALSE, compptr->ac_tbl_no, &actbl);
    }

    /* Align the virtual buffer for this component. */
    buffer = (*cinfo->mem->access_virt_barray)
    ((j_common_ptr) cinfo, coef->whole_image[compptr->component_index],
     coef->iMCU_row_num * compptr->v_samp_factor,
     (JDIMENSION)compptr->v_samp_factor, TRUE);
    
    buffer_dst = (*cinfo->mem->access_virt_barray)
    ((j_common_ptr) cinfo, coef->whole_image_uq[compptr->component_index],
     coef->iMCU_row_num * compptr->v_samp_factor,
     (JDIMENSION)compptr->v_samp_factor, TRUE);

    trellis_iMCU_row(cinfo, compptr, dctbl, actbl, arith_r,
                     get_trellis_scratch(cinfo, 0), buffer, buffer_dst,
                     coef->iMCU_row_num);
  }

  /* NB: compress_output will increment iMCU_row_num if successful.
   * A suspension return will result in redoing all the work above next time.
   */
  
  /* Emit data to the entropy encoder, sharing code with subsequent passes */
  return compress_output(cinfo, input_buf);
}
#endif

/*
 * Process some data in subsequent passes of a multi-pass case.
 * We process the equivalent of one fully interleaved MCU row ("iMCU" row)
 * per call, ie, v_samp_factor block rows for each component in the scan.
 * The data is obtained from the virtual arrays and fed to the entropy coder.
 * Returns TRUE if the iMCU row is completed, FALSE if suspended.
 *
 * NB: input_buf is ignored; it is likely to be a NULL pointer.
 */

METHODDEF(boolean)
compress_output(j_compress_ptr cinfo, _JSAMPIMAGE input_buf)
{
  my_coef_ptr coef = (my_coef_ptr)cinfo->coef;
  JDIMENSION MCU_col_num;       /* index of current MCU within row */
  int blkn, ci, xindex, yindex, yoffset;
  JDIMENSION start_col;
  JBLOCKARRAY buffer[MAX_COMPS_IN_SCAN];
  JBLOCKROW buffer_ptr;
  jpeg_component_info *compptr;

  /* Align the virtual buffers for the components used in this scan.
   * NB: during first pass, this is safe only because the buffers will
   * already be aligned properly, so jmemmgr.c won't need to do any I/O.
   */
  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    buffer[ci] = (*cinfo->mem->access_virt_barray)
      ((j_common_ptr)cinfo, coef->whole_image[compptr->component_index],
       coef->iMCU_row_num * compptr->v_samp_factor,
       (JDIMENSION)compptr->v_samp_factor, FALSE);
  }

  /* Loop to process one whole iMCU row */
  for (yoffset = coef->MCU_vert_offset; yoffset < coef->MCU_rows_per_iMCU_row;
       yoffset++) {
    for (MCU_col_num = coef->mcu_ctr; MCU_col_num < cinfo->MCUs_per_row;
         MCU_col_num++) {
      /* Construct list of pointers to DCT blocks belonging to this MCU */
      blkn = 0;                 /* index of current DCT block within MCU */
      for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
        compptr = cinfo->cur_comp_info[ci];
        start_col = MCU_col_num * compptr->MCU_width;
        for (yindex = 0; yindex < compptr->MCU_height; yindex++) {
          buffer_ptr = buffer[ci][yindex + yoffset] + start_col;
          for (xindex = 0; xindex < compptr->MCU_width; xindex++) {
            coef->MCU_buffer[blkn++] = buffer_ptr++;
          }
        }
      }
      /* Try to write the MCU. */
      if (!(*cinfo->entropy->encode_mcu) (cinfo, coef->MCU_buffer)) {
        /* Suspension forced; update state counters and exit */
        coef->MCU_vert_offset = yoffset;
        coef->mcu_ctr = MCU_col_num;
        return FALSE;
      }
    }
    /* Completed an MCU row, but perhaps not an iMCU row */
    coef->mcu_ctr = 0;
  }
  /* Completed the iMCU row, advance counters for next one */
  coef->iMCU_row_num++;
  start_iMCU_row(cinfo);
  return TRUE;
}

#endif /* FULL_COEF_BUFFER_SUPPORTED */


/*
 * Initialize coefficient buffer controller.
 */

GLOBAL(void)
_jinit_c_coef_controller(j_compress_ptr cinfo, boolean need_full_buffer)
{
  my_coef_ptr coef;

  if (cinfo->data_precision != BITS_IN_JSAMPLE)
    ERREXIT1(cinfo, JERR_BAD_PRECISION, cinfo->data_precision);

  coef = (my_coef_ptr)
    (*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_IMAGE,
                                sizeof(my_coef_controller));
  memset(coef, 0, sizeof(my_coef_controller));
  cinfo->coef = (struct jpeg_c_coef_controller *)coef;
  coef->pub.start_pass = start_pass_coef;

  /* Create the coefficient buffer. */
  if (need_full_buffer) {
#ifdef FULL_COEF_BUFFER_SUPPORTED
    /* Allocate a full-image virtual array for each component, */
    /* padded to a multiple of samp_factor DCT blocks in each direction. */
    int ci;
    jpeg_component_info *compptr;

    for (ci = 0, compptr = cinfo->comp_info; ci < cinfo->num_components;
         ci++, compptr++) {
      coef->whole_image[ci] = (*cinfo->mem->request_virt_barray)
        ((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
         (JDIMENSION)jround_up((long) compptr->width_in_blocks,
                                (long) compptr->h_samp_factor),
         (JDIMENSION)jround_up((long) compptr->height_in_blocks,
                                (long) compptr->v_samp_factor),
         (JDIMENSION)compptr->v_samp_factor);
      
      coef->whole_image_uq[ci] = (*cinfo->mem->request_virt_barray)
        ((j_common_ptr) cinfo, JPOOL_IMAGE, FALSE,
         (JDIMENSION)jround_up((long) compptr->width_in_blocks,
                                (long) compptr->h_samp_factor),
         (JDIMENSION)jround_up((long) compptr->height_in_blocks,
                                (long) compptr->v_samp_factor),
         (JDIMENSION)compptr->v_samp_factor);
    }
#else
    ERREXIT(cinfo, JERR_BAD_BUFFER_MODE);
#endif
  } else {
    /* We only need a single-MCU buffer. */
    JBLOCKROW buffer;
    int i;

    buffer = (JBLOCKROW)
      (*cinfo->mem->alloc_large) ((j_common_ptr)cinfo, JPOOL_IMAGE,
                                  C_MAX_BLOCKS_IN_MCU * sizeof(JBLOCK));
    for (i = 0; i < C_MAX_BLOCKS_IN_MCU; i++) {
      coef->MCU_buffer[i] = buffer + i;
    }
    coef->whole_image[0] = NULL; /* flag for no virtual arrays */
  }
}

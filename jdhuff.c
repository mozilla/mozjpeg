/*
 * jdhuff.c
 *
 * Copyright (C) 1991, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains Huffman entropy decoding routines.
 * These routines are invoked via the methods entropy_decode
 * and entropy_decoder_init/term.
 */

#include "jinclude.h"


/* Static variables to avoid passing 'round extra parameters */

static decompress_info_ptr dcinfo;

static unsigned int get_buffer; /* current bit-extraction buffer */
static int bits_left;		/* # of unused bits in it */


LOCAL void
fix_huff_tbl (HUFF_TBL * htbl)
/* Compute derived values for a Huffman table */
{
  int p, i, l, lastp, si;
  char huffsize[257];
  UINT16 huffcode[257];
  UINT16 code;
  
  /* Figure 7.3.5.4.2.1: make table of Huffman code length for each symbol */
  /* Note that this is in code-length order. */

  p = 0;
  for (l = 1; l <= 16; l++) {
    for (i = 1; i <= htbl->bits[l]; i++)
      huffsize[p++] = l;
  }
  huffsize[p] = 0;
  lastp = p;
  
  /* Figure 7.3.5.4.2.2: generate the codes themselves */
  /* Note that this is in code-length order. */
  
  code = 0;
  si = huffsize[0];
  p = 0;
  while (huffsize[p]) {
    while (huffsize[p] == si) {
      huffcode[p++] = code;
      code++;
    }
    code <<= 1;
    si++;
  }
  
  /* Figure 7.3.5.4.2.3: generate encoding tables */
  /* These are code and size indexed by symbol value */

  for (p = 0; p < lastp; p++) {
    htbl->ehufco[htbl->huffval[p]] = huffcode[p];
    htbl->ehufsi[htbl->huffval[p]] = huffsize[p];
  }
  
  /* Figure 13.4.2.3.1: generate decoding tables */

  p = 0;
  for (l = 1; l <= 16; l++) {
    if (htbl->bits[l]) {
      htbl->valptr[l] = p;	/* huffval[] index of 1st sym of code len l */
      htbl->mincode[l] = huffcode[p]; /* minimum code of length l */
      p += htbl->bits[l];
      htbl->maxcode[l] = huffcode[p-1];	/* maximum code of length l */
    } else {
      htbl->maxcode[l] = -1;
    }
  }
}


/* Extract the next N bits from the input stream (N <= 8) */

LOCAL int
get_bits (int nbits)
{
  int result;
  
  while (nbits > bits_left) {
    int c = JGETC(dcinfo);
    
    get_buffer = (get_buffer << 8) + c;
    bits_left += 8;
    /* If it's 0xFF, check and discard stuffed zero byte */
    if (c == 0xff) {
      c = JGETC(dcinfo);  /* Byte stuffing */
      if (c != 0)
	ERREXIT1(dcinfo->emethods,
		 "Unexpected marker 0x%02x in compressed data", c);
    }
  }
  
  bits_left -= nbits;
  result = (get_buffer >> bits_left) & ((1 << nbits) - 1);
  return result;
}

/* Macro to make things go at some speed! */

#define get_bit()	(bits_left ? \
			 ((get_buffer >> (--bits_left)) & 1) : \
			 get_bits(1))


/* Figure 13.4.2.3.2: extract next coded symbol from input stream */
  
LOCAL int
huff_DECODE (HUFF_TBL * htbl)
{
  int l, p;
  INT32 code;
  
  code = get_bit();
  l = 1;
  while (code > htbl->maxcode[l]) {
    code = (code << 1) + get_bit();
    l++;
  }
  
  p = htbl->valptr[l] + (code - htbl->mincode[l]);
  
  return htbl->huffval[p];
}


/* Figure 13.4.2.1.1: extend sign bit */

#define huff_EXTEND(x, s)	((x) < (1 << ((s)-1)) ? \
				 (x) + (-1 << (s)) + 1 : \
				 (x))


/* Decode a single block's worth of coefficients */
/* Note that only the difference is returned for the DC coefficient */

LOCAL void
decode_one_block (JBLOCK block, HUFF_TBL *dctbl, HUFF_TBL *actbl)
{
  int s, k, r, n;

  /* zero out the coefficient block */

  MEMZERO((void *) block, SIZEOF(JBLOCK));
  
  /* Section 13.4.2.1: decode the DC coefficient difference */

  s = huff_DECODE(dctbl);
  r = get_bits(s);
  block[0] = huff_EXTEND(r, s);
  
  /* Section 13.4.2.2: decode the AC coefficients */
  
  for (k = 1; k < DCTSIZE2; k++) {
    r = huff_DECODE(actbl);
    
    s = r & 15;
    n = r >> 4;
    
    if (s) {
      k = k + n;
      r = get_bits(s);
      block[k] = huff_EXTEND(r, s);
    } else {
      if (n != 15)
	break;
      k += 15;
    }
  }
}


/*
 * Initialize for a Huffman-compressed scan.
 * This is invoked after reading the SOS marker.
 */

METHODDEF void
huff_decoder_init (decompress_info_ptr cinfo)
{
  short ci;
  jpeg_component_info * compptr;

  /* Initialize static variables */
  dcinfo = cinfo;
  bits_left = 0;

  for (ci = 0; ci < cinfo->comps_in_scan; ci++) {
    compptr = cinfo->cur_comp_info[ci];
    /* Make sure requested tables are present */
    if (cinfo->dc_huff_tbl_ptrs[compptr->dc_tbl_no] == NULL ||
	cinfo->ac_huff_tbl_ptrs[compptr->ac_tbl_no] == NULL)
      ERREXIT(cinfo->emethods, "Use of undefined Huffman table");
    /* Compute derived values for Huffman tables */
    /* We may do this more than once for same table, but it's not a big deal */
    fix_huff_tbl(cinfo->dc_huff_tbl_ptrs[compptr->dc_tbl_no]);
    fix_huff_tbl(cinfo->ac_huff_tbl_ptrs[compptr->ac_tbl_no]);
    /* Initialize DC predictions to 0 */
    cinfo->last_dc_val[ci] = 0;
  }

  /* Initialize restart stuff */
  cinfo->restarts_to_go = cinfo->restart_interval;
  cinfo->next_restart_num = 0;
}


/*
 * Check for a restart marker & resynchronize decoder.
 */

LOCAL void
process_restart (decompress_info_ptr cinfo)
{
  int c, nbytes;
  short ci;

  /* Throw away any partial unread byte */
  bits_left = 0;

  /* Scan for next JPEG marker */
  nbytes = 0;
  do {
    do {			/* skip any non-FF bytes */
      nbytes++;
      c = JGETC(cinfo);
    } while (c != 0xFF);
    do {			/* skip any duplicate FFs */
      nbytes++;
      c = JGETC(cinfo);
    } while (c == 0xFF);
  } while (c == 0);		/* repeat if it was a stuffed FF/00 */

  if (c != (RST0 + cinfo->next_restart_num))
    ERREXIT2(cinfo->emethods, "Found 0x%02x marker instead of RST%d",
	     c, cinfo->next_restart_num);

  if (nbytes != 2)
    TRACEMS2(cinfo->emethods, 1, "Skipped %d bytes before RST%d",
	     nbytes-2, cinfo->next_restart_num);
  else
    TRACEMS1(cinfo->emethods, 2, "RST%d", cinfo->next_restart_num);

  /* Re-initialize DC predictions to 0 */
  for (ci = 0; ci < cinfo->comps_in_scan; ci++)
    cinfo->last_dc_val[ci] = 0;

  /* Update restart state */
  cinfo->restarts_to_go = cinfo->restart_interval;
  cinfo->next_restart_num++;
  cinfo->next_restart_num &= 7;
}


/*
 * Decode and return one MCU's worth of Huffman-compressed coefficients.
 */

METHODDEF void
huff_decode (decompress_info_ptr cinfo, JBLOCK *MCU_data)
{
  short blkn, ci;
  jpeg_component_info * compptr;

  /* Account for restart interval, process restart marker if needed */
  if (cinfo->restart_interval) {
    if (cinfo->restarts_to_go == 0)
      process_restart(cinfo);
    cinfo->restarts_to_go--;
  }

  for (blkn = 0; blkn < cinfo->blocks_in_MCU; blkn++) {
    ci = cinfo->MCU_membership[blkn];
    compptr = cinfo->cur_comp_info[ci];
    decode_one_block(MCU_data[blkn],
		     cinfo->dc_huff_tbl_ptrs[compptr->dc_tbl_no],
		     cinfo->ac_huff_tbl_ptrs[compptr->ac_tbl_no]);
    /* Convert DC difference to actual value, update last_dc_val */
    MCU_data[blkn][0] += cinfo->last_dc_val[ci];
    cinfo->last_dc_val[ci] = MCU_data[blkn][0];
  }
}


/*
 * Finish up at the end of a Huffman-compressed scan.
 */

METHODDEF void
huff_decoder_term (decompress_info_ptr cinfo)
{
  /* No work needed */
}


/*
 * The method selection routine for Huffman entropy decoding.
 */

GLOBAL void
jseldhuffman (decompress_info_ptr cinfo)
{
  if (! cinfo->arith_code) {
    cinfo->methods->entropy_decoder_init = huff_decoder_init;
    cinfo->methods->entropy_decode = huff_decode;
    cinfo->methods->entropy_decoder_term = huff_decoder_term;
  }
}

/*
 * jwrjfif.c
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to write standard JPEG file headers/markers.
 * The file format created is a raw JPEG data stream with (optionally) an
 * APP0 marker per the JFIF spec.  This will handle baseline and
 * JFIF-convention JPEG files, although there is currently no provision
 * for inserting a thumbnail image in the JFIF header.
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume output to
 * an ordinary stdio stream.  However, the changes to write to something
 * else are localized in the macros appearing just below.
 *
 * These routines are invoked via the methods write_file_header,
 * write_scan_header, write_jpeg_data, write_scan_trailer, and
 * write_file_trailer.
 */

#include "jinclude.h"

#ifdef JFIF_SUPPORTED


/*
 * To output to something other than a stdio stream, you'd need to redefine
 * these macros.
 */

/* Write a single byte */
#define emit_byte(cinfo,x)  putc((x), cinfo->output_file)

/* Write some bytes from a (char *) buffer */
#define WRITE_BYTES(cinfo,dataptr,datacount)  \
  { if (JFWRITE(cinfo->output_file, dataptr, datacount) \
	!= (size_t) (datacount)) \
      ERREXIT(cinfo->emethods, "Output file write error --- out of disk space?"); }

/* Clean up and verify successful output */
#define CHECK_OUTPUT(cinfo)  \
  { fflush(cinfo->output_file); \
    if (ferror(cinfo->output_file)) \
      ERREXIT(cinfo->emethods, "Output file write error --- out of disk space?"); }


/* End of stdio-specific code. */


typedef enum {			/* JPEG marker codes */
  M_SOF0  = 0xc0,
  M_SOF1  = 0xc1,
  M_SOF2  = 0xc2,
  M_SOF3  = 0xc3,
  
  M_SOF5  = 0xc5,
  M_SOF6  = 0xc6,
  M_SOF7  = 0xc7,
  
  M_JPG   = 0xc8,
  M_SOF9  = 0xc9,
  M_SOF10 = 0xca,
  M_SOF11 = 0xcb,
  
  M_SOF13 = 0xcd,
  M_SOF14 = 0xce,
  M_SOF15 = 0xcf,
  
  M_DHT   = 0xc4,
  
  M_DAC   = 0xcc,
  
  M_RST0  = 0xd0,
  M_RST1  = 0xd1,
  M_RST2  = 0xd2,
  M_RST3  = 0xd3,
  M_RST4  = 0xd4,
  M_RST5  = 0xd5,
  M_RST6  = 0xd6,
  M_RST7  = 0xd7,
  
  M_SOI   = 0xd8,
  M_EOI   = 0xd9,
  M_SOS   = 0xda,
  M_DQT   = 0xdb,
  M_DNL   = 0xdc,
  M_DRI   = 0xdd,
  M_DHP   = 0xde,
  M_EXP   = 0xdf,
  
  M_APP0  = 0xe0,
  M_APP15 = 0xef,
  
  M_JPG0  = 0xf0,
  M_JPG13 = 0xfd,
  M_COM   = 0xfe,
  
  M_TEM   = 0x01,
  
  M_ERROR = 0x100
} JPEG_MARKER;


LOCAL void
emit_marker (compress_info_ptr cinfo, JPEG_MARKER mark)
/* Emit a marker code */
{
  emit_byte(cinfo, 0xFF);
  emit_byte(cinfo, mark);
}


LOCAL void
emit_2bytes (compress_info_ptr cinfo, int value)
/* Emit a 2-byte integer; these are always MSB first in JPEG files */
{
  emit_byte(cinfo, (value >> 8) & 0xFF);
  emit_byte(cinfo, value & 0xFF);
}


LOCAL int
emit_dqt (compress_info_ptr cinfo, int index)
/* Emit a DQT marker */
/* Returns the precision used (0 = 8bits, 1 = 16bits) for baseline checking */
{
  QUANT_TBL_PTR data = cinfo->quant_tbl_ptrs[index];
  int prec = 0;
  int i;
  
  for (i = 0; i < DCTSIZE2; i++) {
    if (data[i] > 255)
      prec = 1;
  }

  emit_marker(cinfo, M_DQT);
  
  emit_2bytes(cinfo, prec ? DCTSIZE2*2 + 1 + 2 : DCTSIZE2 + 1 + 2);
  
  emit_byte(cinfo, index + (prec<<4));
  
  for (i = 0; i < DCTSIZE2; i++) {
    if (prec)
      emit_byte(cinfo, data[i] >> 8);
    emit_byte(cinfo, data[i] & 0xFF);
  }

  return prec;
}


LOCAL void
emit_dht (compress_info_ptr cinfo, int index, boolean is_ac)
/* Emit a DHT marker */
{
  HUFF_TBL * htbl;
  int length, i;
  
  if (is_ac) {
    htbl = cinfo->ac_huff_tbl_ptrs[index];
    index += 0x10;		/* output index has AC bit set */
  } else {
    htbl = cinfo->dc_huff_tbl_ptrs[index];
  }

  if (htbl == NULL)
    ERREXIT1(cinfo->emethods, "Huffman table 0x%02x was not defined", index);
  
  if (! htbl->sent_table) {
    emit_marker(cinfo, M_DHT);
    
    length = 0;
    for (i = 1; i <= 16; i++)
      length += htbl->bits[i];
    
    emit_2bytes(cinfo, length + 2 + 1 + 16);
    emit_byte(cinfo, index);
    
    for (i = 1; i <= 16; i++)
      emit_byte(cinfo, htbl->bits[i]);
    
    for (i = 0; i < length; i++)
      emit_byte(cinfo, htbl->huffval[i]);
    
    htbl->sent_table = TRUE;
  }
}


LOCAL void
emit_dac (compress_info_ptr cinfo)
/* Emit a DAC marker */
/* Since the useful info is so small, we want to emit all the tables in */
/* one DAC marker.  Therefore this routine does its own scan of the table. */
{
  char dc_in_use[NUM_ARITH_TBLS];
  char ac_in_use[NUM_ARITH_TBLS];
  int length, i;
  
  for (i = 0; i < NUM_ARITH_TBLS; i++)
    dc_in_use[i] = ac_in_use[i] = 0;
  
  for (i = 0; i < cinfo->num_components; i++) {
    dc_in_use[cinfo->comp_info[i].dc_tbl_no] = 1;
    ac_in_use[cinfo->comp_info[i].ac_tbl_no] = 1;
  }
  
  length = 0;
  for (i = 0; i < NUM_ARITH_TBLS; i++)
    length += dc_in_use[i] + ac_in_use[i];
  
  emit_marker(cinfo, M_DAC);
  
  emit_2bytes(cinfo, length*2 + 2);
  
  for (i = 0; i < NUM_ARITH_TBLS; i++) {
    if (dc_in_use[i]) {
      emit_byte(cinfo, i);
      emit_byte(cinfo, cinfo->arith_dc_L[i] + (cinfo->arith_dc_U[i]<<4));
    }
    if (ac_in_use[i]) {
      emit_byte(cinfo, i + 0x10);
      emit_byte(cinfo, cinfo->arith_ac_K[i]);
    }
  }
}


LOCAL void
emit_dri (compress_info_ptr cinfo)
/* Emit a DRI marker */
{
  emit_marker(cinfo, M_DRI);
  
  emit_2bytes(cinfo, 4);	/* fixed length */

  emit_2bytes(cinfo, (int) cinfo->restart_interval);
}


LOCAL void
emit_sof (compress_info_ptr cinfo, JPEG_MARKER code)
/* Emit a SOF marker */
{
  int i;
  
  emit_marker(cinfo, code);
  
  emit_2bytes(cinfo, 3 * cinfo->num_components + 2 + 5 + 1); /* length */

  if (cinfo->image_height > 65535L || cinfo->image_width > 65535L)
    ERREXIT(cinfo->emethods, "Maximum image dimension for JFIF is 65535 pixels");

  emit_byte(cinfo, cinfo->data_precision);
  emit_2bytes(cinfo, (int) cinfo->image_height);
  emit_2bytes(cinfo, (int) cinfo->image_width);

  emit_byte(cinfo, cinfo->num_components);

  for (i = 0; i < cinfo->num_components; i++) {
    emit_byte(cinfo, cinfo->comp_info[i].component_id);
    emit_byte(cinfo, (cinfo->comp_info[i].h_samp_factor << 4)
		     + cinfo->comp_info[i].v_samp_factor);
    emit_byte(cinfo, cinfo->comp_info[i].quant_tbl_no);
  }
}


LOCAL void
emit_sos (compress_info_ptr cinfo)
/* Emit a SOS marker */
{
  int i;
  
  emit_marker(cinfo, M_SOS);
  
  emit_2bytes(cinfo, 2 * cinfo->comps_in_scan + 2 + 1 + 3); /* length */
  
  emit_byte(cinfo, cinfo->comps_in_scan);
  
  for (i = 0; i < cinfo->comps_in_scan; i++) {
    emit_byte(cinfo, cinfo->cur_comp_info[i]->component_id);
    emit_byte(cinfo, (cinfo->cur_comp_info[i]->dc_tbl_no << 4)
		     + cinfo->cur_comp_info[i]->ac_tbl_no);
  }

  emit_byte(cinfo, 0);		/* Spectral selection start */
  emit_byte(cinfo, DCTSIZE2-1);	/* Spectral selection end */
  emit_byte(cinfo, 0);		/* Successive approximation */
}


LOCAL void
emit_jfif_app0 (compress_info_ptr cinfo)
/* Emit a JFIF-compliant APP0 marker */
{
  /*
   * Length of APP0 block	(2 bytes)
   * Block ID			(4 bytes - ASCII "JFIF")
   * Zero byte			(1 byte to terminate the ID string)
   * Version Major, Minor	(2 bytes - 0x01, 0x01)
   * Units			(1 byte - 0x00 = none, 0x01 = inch, 0x02 = cm)
   * Xdpu			(2 bytes - dots per unit horizontal)
   * Ydpu			(2 bytes - dots per unit vertical)
   * Thumbnail X size		(1 byte)
   * Thumbnail Y size		(1 byte)
   */
  
  emit_marker(cinfo, M_APP0);
  
  emit_2bytes(cinfo, 2 + 4 + 1 + 2 + 1 + 2 + 2 + 1 + 1); /* length */

  emit_byte(cinfo, 0x4A);	/* Identifier: ASCII "JFIF" */
  emit_byte(cinfo, 0x46);
  emit_byte(cinfo, 0x49);
  emit_byte(cinfo, 0x46);
  emit_byte(cinfo, 0);
  emit_byte(cinfo, 1);		/* Major version */
  emit_byte(cinfo, 1);		/* Minor version */
  emit_byte(cinfo, cinfo->density_unit); /* Pixel size information */
  emit_2bytes(cinfo, (int) cinfo->X_density);
  emit_2bytes(cinfo, (int) cinfo->Y_density);
  emit_byte(cinfo, 0);		/* No thumbnail image */
  emit_byte(cinfo, 0);
}


LOCAL void
emit_com (compress_info_ptr cinfo, char * dataptr, size_t datalen)
/* Emit a COM marker */
{
  if ((unsigned) datalen <= (unsigned) 65533) {	/* safety check */
    emit_marker(cinfo, M_COM);
  
    emit_2bytes(cinfo, (int) (datalen + 2)); /* length */

    while (datalen--) {
      emit_byte(cinfo, *dataptr);
      dataptr++;
    }
  }
}


/*
 * Write the file header.
 */


METHODDEF void
write_file_header (compress_info_ptr cinfo)
{
  char qt_in_use[NUM_QUANT_TBLS];
  int i, prec;
  boolean is_baseline;
  
  emit_marker(cinfo, M_SOI);	/* first the SOI */

  if (cinfo->write_JFIF_header)	/* next an optional JFIF APP0 */
    emit_jfif_app0(cinfo);

  if (cinfo->comment_text != NULL) /* and an optional COM block */
    emit_com(cinfo, cinfo->comment_text,
	     (size_t) (strlen(cinfo->comment_text)));

  /* Emit DQT for each quantization table. */
  /* Note that doing it here means we can't adjust the QTs on-the-fly. */
  /* If we did want to do that, we'd have a problem with checking precision */
  /* for the is_baseline determination. */

  for (i = 0; i < NUM_QUANT_TBLS; i++)
    qt_in_use[i] = 0;

  for (i = 0; i < cinfo->num_components; i++)
    qt_in_use[cinfo->comp_info[i].quant_tbl_no] = 1;

  prec = 0;
  for (i = 0; i < NUM_QUANT_TBLS; i++) {
    if (qt_in_use[i])
      prec += emit_dqt(cinfo, i);
  }
  /* now prec is nonzero iff there are any 16-bit quant tables. */

  /* Check for a non-baseline specification. */
  /* Note we assume that Huffman table numbers won't be changed later. */
  is_baseline = TRUE;
  if (cinfo->arith_code || (cinfo->data_precision != 8))
    is_baseline = FALSE;
  for (i = 0; i < cinfo->num_components; i++) {
    if (cinfo->comp_info[i].dc_tbl_no > 1 || cinfo->comp_info[i].ac_tbl_no > 1)
      is_baseline = FALSE;
  }
  if (prec && is_baseline) {
    is_baseline = FALSE;
    /* If it's baseline except for quantizer size, warn the user */
    TRACEMS(cinfo->emethods, 0,
	    "Caution: quantization tables are too coarse for baseline JPEG");
  }


  /* Emit the proper SOF marker */
  if (cinfo->arith_code)
    emit_sof(cinfo, M_SOF9);	/* SOF code for arithmetic coding */
  else if (is_baseline)
    emit_sof(cinfo, M_SOF0);	/* SOF code for baseline implementation */
  else
    emit_sof(cinfo, M_SOF1);	/* SOF code for non-baseline Huffman file */
}


/*
 * Write the start of a scan (everything through the SOS marker).
 */

METHODDEF void
write_scan_header (compress_info_ptr cinfo)
{
  int i;

  if (cinfo->arith_code) {
    /* Emit arith conditioning info.  We will have some duplication
     * if the file has multiple scans, but it's so small it's hardly
     * worth worrying about.
     */
    emit_dac(cinfo);
  } else {
    /* Emit Huffman tables.  Note that emit_dht takes care of
     * suppressing duplicate tables.
     */
    for (i = 0; i < cinfo->comps_in_scan; i++) {
      emit_dht(cinfo, cinfo->cur_comp_info[i]->dc_tbl_no, FALSE);
      emit_dht(cinfo, cinfo->cur_comp_info[i]->ac_tbl_no, TRUE);
    }
  }

  /* Emit DRI if required --- note that DRI value could change for each scan.
   * If it doesn't, a tiny amount of space is wasted in multiple-scan files.
   * We assume DRI will never be nonzero for one scan and zero for a later one.
   */
  if (cinfo->restart_interval)
    emit_dri(cinfo);

  emit_sos(cinfo);
}


/*
 * Write some bytes of compressed data within a scan.
 */

METHODDEF void
write_jpeg_data (compress_info_ptr cinfo, char *dataptr, int datacount)
{
  WRITE_BYTES(cinfo, dataptr, datacount);
}


/*
 * Finish up after a compressed scan (series of write_jpeg_data calls).
 */

METHODDEF void
write_scan_trailer (compress_info_ptr cinfo)
{
  /* no work needed in this format */
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
write_file_trailer (compress_info_ptr cinfo)
{
  emit_marker(cinfo, M_EOI);
  /* Make sure we wrote the output file OK */
  CHECK_OUTPUT(cinfo);
}


/*
 * The method selection routine for standard JPEG header writing.
 * This should be called from c_ui_method_selection if appropriate.
 */

GLOBAL void
jselwjfif (compress_info_ptr cinfo)
{
  cinfo->methods->write_file_header = write_file_header;
  cinfo->methods->write_scan_header = write_scan_header;
  cinfo->methods->write_jpeg_data = write_jpeg_data;
  cinfo->methods->write_scan_trailer = write_scan_trailer;
  cinfo->methods->write_file_trailer = write_file_trailer;
}

#endif /* JFIF_SUPPORTED */

/*
 * jrdjfif.c
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to decode standard JPEG file headers/markers.
 * This code will handle "raw JPEG" and JFIF-convention JPEG files.
 *
 * You can also use this module to decode a raw-JPEG or JFIF-standard data
 * stream that is embedded within a larger file.  To do that, you must
 * position the file to the JPEG SOI marker (0xFF/0xD8) that begins the
 * data sequence to be decoded.  If nothing better is possible, you can scan
 * the file until you see the SOI marker, then use JUNGETC to push it back.
 *
 * This module relies on the JGETC macro and the read_jpeg_data method (which
 * is provided by the user interface) to read from the JPEG data stream.
 * Therefore, this module is not dependent on any particular assumption about
 * the data source; it need not be a stdio stream at all.  (This fact does
 * NOT carry over to more complex JPEG file formats such as JPEG-in-TIFF;
 * those format control modules may well need to assume stdio input.)
 *
 * These routines are invoked via the methods read_file_header,
 * read_scan_header, read_jpeg_data, read_scan_trailer, and read_file_trailer.
 */

#include "jinclude.h"

#ifdef JFIF_SUPPORTED


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


/*
 * Reload the input buffer after it's been emptied, and return the next byte.
 * This is exported for direct use by the entropy decoder.
 * See the JGETC macro for calling conditions.  Note in particular that
 * read_jpeg_data may NOT return EOF.  If no more data is available, it must
 * exit via ERREXIT, or perhaps synthesize fake data (such as an RST marker).
 * For error recovery purposes, synthesizing an EOI marker is probably best.
 *
 * For this header control module, read_jpeg_data is supplied by the
 * user interface.  However, header formats that require random access
 * to the input file would need to supply their own code.  This code is
 * left here to indicate what is required.
 */

#if 0				/* not needed in this module */

METHODDEF int
read_jpeg_data (decompress_info_ptr cinfo)
{
  cinfo->next_input_byte = cinfo->input_buffer + MIN_UNGET;

  cinfo->bytes_in_buffer = (int) JFREAD(cinfo->input_file,
					cinfo->next_input_byte,
					JPEG_BUF_SIZE);
  
  if (cinfo->bytes_in_buffer <= 0) {
    WARNMS(cinfo->emethods, "Premature EOF in JPEG file");
    cinfo->next_input_byte[0] = (char) 0xFF;
    cinfo->next_input_byte[1] = (char) M_EOI;
    cinfo->bytes_in_buffer = 2;
  }

  return JGETC(cinfo);
}

#endif


/*
 * Routines to parse JPEG markers & save away the useful info.
 */


LOCAL INT32
get_2bytes (decompress_info_ptr cinfo)
/* Get a 2-byte unsigned integer (e.g., a marker parameter length field) */
{
  INT32 a;
  
  a = JGETC(cinfo);
  return (a << 8) + JGETC(cinfo);
}


LOCAL void
skip_variable (decompress_info_ptr cinfo, int code)
/* Skip over an unknown or uninteresting variable-length marker */
{
  INT32 length;
  
  length = get_2bytes(cinfo);
  
  TRACEMS2(cinfo->emethods, 1,
	   "Skipping marker 0x%02x, length %u", code, (int) length);
  
  for (length -= 2; length > 0; length--)
    (void) JGETC(cinfo);
}


LOCAL void
get_dht (decompress_info_ptr cinfo)
/* Process a DHT marker */
{
  INT32 length;
  UINT8 bits[17];
  UINT8 huffval[256];
  int i, index, count;
  HUFF_TBL **htblptr;
  
  length = get_2bytes(cinfo)-2;
  
  while (length > 0) {
    index = JGETC(cinfo);

    TRACEMS1(cinfo->emethods, 1, "Define Huffman Table 0x%02x", index);
      
    bits[0] = 0;
    count = 0;
    for (i = 1; i <= 16; i++) {
      bits[i] = (UINT8) JGETC(cinfo);
      count += bits[i];
    }

    length -= 1 + 16;

    TRACEMS8(cinfo->emethods, 2, "        %3d %3d %3d %3d %3d %3d %3d %3d",
	     bits[1], bits[2], bits[3], bits[4],
	     bits[5], bits[6], bits[7], bits[8]);
    TRACEMS8(cinfo->emethods, 2, "        %3d %3d %3d %3d %3d %3d %3d %3d",
	     bits[9], bits[10], bits[11], bits[12],
	     bits[13], bits[14], bits[15], bits[16]);

    if (count > 256 || ((INT32) count) > length)
      ERREXIT(cinfo->emethods, "Bogus DHT counts");

    for (i = 0; i < count; i++)
      huffval[i] = (UINT8) JGETC(cinfo);

    length -= count;

    if (index & 0x10) {		/* AC table definition */
      index -= 0x10;
      htblptr = &cinfo->ac_huff_tbl_ptrs[index];
    } else {			/* DC table definition */
      htblptr = &cinfo->dc_huff_tbl_ptrs[index];
    }

    if (index < 0 || index >= NUM_HUFF_TBLS)
      ERREXIT1(cinfo->emethods, "Bogus DHT index %d", index);

    if (*htblptr == NULL)
      *htblptr = (HUFF_TBL *) (*cinfo->emethods->alloc_small) (SIZEOF(HUFF_TBL));
  
    MEMCOPY((*htblptr)->bits, bits, SIZEOF((*htblptr)->bits));
    MEMCOPY((*htblptr)->huffval, huffval, SIZEOF((*htblptr)->huffval));
    }
}


LOCAL void
get_dac (decompress_info_ptr cinfo)
/* Process a DAC marker */
{
  INT32 length;
  int index, val;

  length = get_2bytes(cinfo)-2;
  
  while (length > 0) {
    index = JGETC(cinfo);
    val = JGETC(cinfo);

    TRACEMS2(cinfo->emethods, 1,
	     "Define Arithmetic Table 0x%02x: 0x%02x", index, val);

    if (index < 0 || index >= (2*NUM_ARITH_TBLS))
      ERREXIT1(cinfo->emethods, "Bogus DAC index %d", index);

    if (index >= NUM_ARITH_TBLS) { /* define AC table */
      cinfo->arith_ac_K[index-NUM_ARITH_TBLS] = (UINT8) val;
    } else {			/* define DC table */
      cinfo->arith_dc_L[index] = (UINT8) (val & 0x0F);
      cinfo->arith_dc_U[index] = (UINT8) (val >> 4);
      if (cinfo->arith_dc_L[index] > cinfo->arith_dc_U[index])
	ERREXIT1(cinfo->emethods, "Bogus DAC value 0x%x", val);
    }

    length -= 2;
  }
}


LOCAL void
get_dqt (decompress_info_ptr cinfo)
/* Process a DQT marker */
{
  INT32 length;
  int n, i, prec;
  UINT16 tmp;
  QUANT_TBL_PTR quant_ptr;
  
  length = get_2bytes(cinfo) - 2;
  
  while (length > 0) {
    n = JGETC(cinfo);
    prec = n >> 4;
    n &= 0x0F;

    TRACEMS2(cinfo->emethods, 1,
	     "Define Quantization Table %d  precision %d", n, prec);

    if (n >= NUM_QUANT_TBLS)
      ERREXIT1(cinfo->emethods, "Bogus table number %d", n);
      
    if (cinfo->quant_tbl_ptrs[n] == NULL)
      cinfo->quant_tbl_ptrs[n] = (QUANT_TBL_PTR)
	(*cinfo->emethods->alloc_small) (SIZEOF(QUANT_TBL));
    quant_ptr = cinfo->quant_tbl_ptrs[n];

    for (i = 0; i < DCTSIZE2; i++) {
      tmp = JGETC(cinfo);
      if (prec)
	tmp = (tmp<<8) + JGETC(cinfo);
      quant_ptr[i] = tmp;
    }

    for (i = 0; i < DCTSIZE2; i += 8) {
      TRACEMS8(cinfo->emethods, 2, "        %4u %4u %4u %4u %4u %4u %4u %4u",
	       quant_ptr[i  ], quant_ptr[i+1], quant_ptr[i+2], quant_ptr[i+3],
	       quant_ptr[i+4], quant_ptr[i+5], quant_ptr[i+6], quant_ptr[i+7]);
    }

    length -= DCTSIZE2+1;
    if (prec) length -= DCTSIZE2;
  }
}


LOCAL void
get_dri (decompress_info_ptr cinfo)
/* Process a DRI marker */
{
  if (get_2bytes(cinfo) != 4)
    ERREXIT(cinfo->emethods, "Bogus length in DRI");

  cinfo->restart_interval = (UINT16) get_2bytes(cinfo);

  TRACEMS1(cinfo->emethods, 1,
	   "Define Restart Interval %u", cinfo->restart_interval);
}


LOCAL void
get_app0 (decompress_info_ptr cinfo)
/* Process an APP0 marker */
{
#define JFIF_LEN 14
  INT32 length;
  UINT8 b[JFIF_LEN];
  int buffp;

  length = get_2bytes(cinfo) - 2;

  /* See if a JFIF APP0 marker is present */

  if (length >= JFIF_LEN) {
    for (buffp = 0; buffp < JFIF_LEN; buffp++)
      b[buffp] = (UINT8) JGETC(cinfo);
    length -= JFIF_LEN;

    if (b[0]==0x4A && b[1]==0x46 && b[2]==0x49 && b[3]==0x46 && b[4]==0) {
      /* Found JFIF APP0 marker: check version */
      /* Major version must be 1 */
      if (b[5] != 1)
	ERREXIT2(cinfo->emethods, "Unsupported JFIF revision number %d.%02d",
		 b[5], b[6]);
      /* Minor version should be 0..2, but try to process anyway if newer */
      if (b[6] > 2)
	TRACEMS2(cinfo->emethods, 1, "Warning: unknown JFIF revision number %d.%02d",
		 b[5], b[6]);
      /* Save info */
      cinfo->density_unit = b[7];
      cinfo->X_density = (b[8] << 8) + b[9];
      cinfo->Y_density = (b[10] << 8) + b[11];
      /* Assume colorspace is YCbCr, unless UI has overridden me */
      if (cinfo->jpeg_color_space == CS_UNKNOWN)
	cinfo->jpeg_color_space = CS_YCbCr;
      TRACEMS3(cinfo->emethods, 1, "JFIF APP0 marker, density %dx%d  %d",
	       cinfo->X_density, cinfo->Y_density, cinfo->density_unit);
      if (b[12] | b[13])
	TRACEMS2(cinfo->emethods, 1, "    with %d x %d thumbnail image",
		 b[12], b[13]);
      if (length != ((INT32) b[12] * (INT32) b[13] * (INT32) 3))
	TRACEMS1(cinfo->emethods, 1,
		 "Warning: thumbnail image size does not match data length %u",
		 (int) length);
    } else {
      TRACEMS1(cinfo->emethods, 1, "Unknown APP0 marker (not JFIF), length %u",
	       (int) length + JFIF_LEN);
    }
  } else {
    TRACEMS1(cinfo->emethods, 1, "Short APP0 marker, length %u", (int) length);
  }

  while (--length >= 0)		/* skip any remaining data */
    (void) JGETC(cinfo);
}


LOCAL void
get_com (decompress_info_ptr cinfo)
/* Process a COM marker */
/* Actually we just pass this off to an application-supplied routine */
{
  INT32 length;
  
  length = get_2bytes(cinfo) - 2;
  
  TRACEMS1(cinfo->emethods, 1, "Comment, length %u", (int) length);
  
  (*cinfo->methods->process_comment) (cinfo, (long) length);
}


LOCAL void
get_sof (decompress_info_ptr cinfo, int code)
/* Process a SOFn marker */
{
  INT32 length;
  short ci;
  int c;
  jpeg_component_info * compptr;
  
  length = get_2bytes(cinfo);
  
  cinfo->data_precision = JGETC(cinfo);
  cinfo->image_height   = get_2bytes(cinfo);
  cinfo->image_width    = get_2bytes(cinfo);
  cinfo->num_components = JGETC(cinfo);

  TRACEMS4(cinfo->emethods, 1,
	   "Start Of Frame 0x%02x: width=%u, height=%u, components=%d",
	   code, (int) cinfo->image_width, (int) cinfo->image_height,
	   cinfo->num_components);

  /* We don't support files in which the image height is initially specified */
  /* as 0 and is later redefined by DNL.  As long as we have to check that,  */
  /* might as well have a general sanity check. */
  if (cinfo->image_height <= 0 || cinfo->image_width <= 0
      || cinfo->num_components <= 0)
    ERREXIT(cinfo->emethods, "Empty JPEG image (DNL not supported)");

#ifdef EIGHT_BIT_SAMPLES
  if (cinfo->data_precision != 8)
    ERREXIT(cinfo->emethods, "Unsupported JPEG data precision");
#endif
#ifdef TWELVE_BIT_SAMPLES
  if (cinfo->data_precision != 12) /* this needs more thought?? */
    ERREXIT(cinfo->emethods, "Unsupported JPEG data precision");
#endif
#ifdef SIXTEEN_BIT_SAMPLES
  if (cinfo->data_precision != 16) /* this needs more thought?? */
    ERREXIT(cinfo->emethods, "Unsupported JPEG data precision");
#endif

  if (length != (cinfo->num_components * 3 + 8))
    ERREXIT(cinfo->emethods, "Bogus SOF length");

  cinfo->comp_info = (jpeg_component_info *) (*cinfo->emethods->alloc_small)
			(cinfo->num_components * SIZEOF(jpeg_component_info));
  
  for (ci = 0; ci < cinfo->num_components; ci++) {
    compptr = &cinfo->comp_info[ci];
    compptr->component_index = ci;
    compptr->component_id = JGETC(cinfo);
    c = JGETC(cinfo);
    compptr->h_samp_factor = (c >> 4) & 15;
    compptr->v_samp_factor = (c     ) & 15;
    compptr->quant_tbl_no  = JGETC(cinfo);
    compptr->component_needed = TRUE; /* assume all components are wanted */

    TRACEMS4(cinfo->emethods, 1, "    Component %d: %dhx%dv q=%d",
	     compptr->component_id, compptr->h_samp_factor,
	     compptr->v_samp_factor, compptr->quant_tbl_no);
  }
}


LOCAL void
get_sos (decompress_info_ptr cinfo)
/* Process a SOS marker */
{
  INT32 length;
  int i, ci, n, c, cc;
  jpeg_component_info * compptr;
  
  length = get_2bytes(cinfo);
  
  n = JGETC(cinfo);  /* Number of components */
  cinfo->comps_in_scan = n;
  length -= 3;
  
  if (length != (n * 2 + 3) || n < 1 || n > MAX_COMPS_IN_SCAN)
    ERREXIT(cinfo->emethods, "Bogus SOS length");

  TRACEMS1(cinfo->emethods, 1, "Start Of Scan: %d components", n);
  
  for (i = 0; i < n; i++) {
    cc = JGETC(cinfo);
    c = JGETC(cinfo);
    length -= 2;
    
    for (ci = 0; ci < cinfo->num_components; ci++)
      if (cc == cinfo->comp_info[ci].component_id)
	break;
    
    if (ci >= cinfo->num_components)
      ERREXIT(cinfo->emethods, "Invalid component number in SOS");
    
    compptr = &cinfo->comp_info[ci];
    cinfo->cur_comp_info[i] = compptr;
    compptr->dc_tbl_no = (c >> 4) & 15;
    compptr->ac_tbl_no = (c     ) & 15;
    
    TRACEMS3(cinfo->emethods, 1, "    c%d: [dc=%d ac=%d]", cc,
	     compptr->dc_tbl_no, compptr->ac_tbl_no);
  }
  
  while (length > 0) {
    (void) JGETC(cinfo);
    length--;
  }
}


LOCAL void
get_soi (decompress_info_ptr cinfo)
/* Process an SOI marker */
{
  int i;
  
  TRACEMS(cinfo->emethods, 1, "Start of Image");

  /* Reset all parameters that are defined to be reset by SOI */

  for (i = 0; i < NUM_ARITH_TBLS; i++) {
    cinfo->arith_dc_L[i] = 0;
    cinfo->arith_dc_U[i] = 1;
    cinfo->arith_ac_K[i] = 5;
  }
  cinfo->restart_interval = 0;

  cinfo->density_unit = 0;	/* set default JFIF APP0 values */
  cinfo->X_density = 1;
  cinfo->Y_density = 1;

  cinfo->CCIR601_sampling = FALSE; /* Assume non-CCIR sampling */
}


LOCAL int
next_marker (decompress_info_ptr cinfo)
/* Find the next JPEG marker */
/* Note that the output might not be a valid marker code, */
/* but it will never be 0 or FF */
{
  int c, nbytes;

  nbytes = 0;
  do {
    do {			/* skip any non-FF bytes */
      nbytes++;
      c = JGETC(cinfo);
    } while (c != 0xFF);
    do {			/* skip any duplicate FFs */
      /* we don't increment nbytes here since extra FFs are legal */
      c = JGETC(cinfo);
    } while (c == 0xFF);
  } while (c == 0);		/* repeat if it was a stuffed FF/00 */

  if (nbytes != 1)
    WARNMS2(cinfo->emethods,
	    "Corrupt JPEG data: %d extraneous bytes before marker 0x%02x",
	    nbytes-1, c);

  return c;
}


LOCAL int
process_tables (decompress_info_ptr cinfo)
/* Scan and process JPEG markers that can appear in any order */
/* Return when an SOI, EOI, SOFn, or SOS is found */
{
  int c;

  for (;;) {
    c = next_marker(cinfo);
      
    switch (c) {
    case M_SOF0:
    case M_SOF1:
    case M_SOF2:
    case M_SOF3:
    case M_SOF5:
    case M_SOF6:
    case M_SOF7:
    case M_JPG:
    case M_SOF9:
    case M_SOF10:
    case M_SOF11:
    case M_SOF13:
    case M_SOF14:
    case M_SOF15:
    case M_SOI:
    case M_EOI:
    case M_SOS:
      return c;
      
    case M_DHT:
      get_dht(cinfo);
      break;
      
    case M_DAC:
      get_dac(cinfo);
      break;
      
    case M_DQT:
      get_dqt(cinfo);
      break;
      
    case M_DRI:
      get_dri(cinfo);
      break;
      
    case M_APP0:
      get_app0(cinfo);
      break;
      
    case M_COM:
      get_com(cinfo);
      break;

    case M_RST0:		/* these are all parameterless */
    case M_RST1:
    case M_RST2:
    case M_RST3:
    case M_RST4:
    case M_RST5:
    case M_RST6:
    case M_RST7:
    case M_TEM:
      TRACEMS1(cinfo->emethods, 1, "Unexpected marker 0x%02x", c);
      break;

    default:	/* must be DNL, DHP, EXP, APPn, JPGn, or RESn */
      skip_variable(cinfo, c);
      break;
    }
  }
}



/*
 * Initialize and read the file header (everything through the SOF marker).
 */

METHODDEF void
read_file_header (decompress_info_ptr cinfo)
{
  int c;

  /* Demand an SOI marker at the start of the file --- otherwise it's
   * probably not a JPEG file at all.  If the user interface wants to support
   * nonstandard headers in front of the SOI, it must skip over them itself
   * before calling jpeg_decompress().
   */
  if (JGETC(cinfo) != 0xFF  ||  JGETC(cinfo) != (int) M_SOI)
    ERREXIT(cinfo->emethods, "Not a JPEG file");

  get_soi(cinfo);		/* OK, process SOI */

  /* Process markers until SOF */
  c = process_tables(cinfo);

  switch (c) {
  case M_SOF0:
  case M_SOF1:
    get_sof(cinfo, c);
    cinfo->arith_code = FALSE;
    break;
      
  case M_SOF9:
    get_sof(cinfo, c);
    cinfo->arith_code = TRUE;
    break;

  default:
    ERREXIT1(cinfo->emethods, "Unsupported SOF marker type 0x%02x", c);
    break;
  }

  /* Figure out what colorspace we have */
  /* (too bad the JPEG committee didn't provide a real way to specify this) */

  switch (cinfo->num_components) {
  case 1:
    cinfo->jpeg_color_space = CS_GRAYSCALE;
    break;

  case 3:
    /* if we saw a JFIF marker, leave it set to YCbCr; */
    /* also leave it alone if UI has provided a value */
    if (cinfo->jpeg_color_space == CS_UNKNOWN) {
      short cid0 = cinfo->comp_info[0].component_id;
      short cid1 = cinfo->comp_info[1].component_id;
      short cid2 = cinfo->comp_info[2].component_id;

      if (cid0 == 1 && cid1 == 2 && cid2 == 3)
	cinfo->jpeg_color_space = CS_YCbCr; /* assume it's JFIF w/out marker */
      else if (cid0 == 1 && cid1 == 4 && cid2 == 5)
	cinfo->jpeg_color_space = CS_YIQ; /* prototype's YIQ matrix */
      else {
	TRACEMS3(cinfo->emethods, 1,
		 "Unrecognized component IDs %d %d %d, assuming YCbCr",
		 cid0, cid1, cid2);
	cinfo->jpeg_color_space = CS_YCbCr;
      }
    }
    break;

  case 4:
    cinfo->jpeg_color_space = CS_CMYK;
    break;

  default:
    cinfo->jpeg_color_space = CS_UNKNOWN;
    break;
  }
}


/*
 * Read the start of a scan (everything through the SOS marker).
 * Return TRUE if find SOS, FALSE if find EOI.
 */

METHODDEF boolean
read_scan_header (decompress_info_ptr cinfo)
{
  int c;
  
  /* Process markers until SOS or EOI */
  c = process_tables(cinfo);
  
  switch (c) {
  case M_SOS:
    get_sos(cinfo);
    return TRUE;
    
  case M_EOI:
    TRACEMS(cinfo->emethods, 1, "End Of Image");
    return FALSE;

  default:
    ERREXIT1(cinfo->emethods, "Unexpected marker 0x%02x", c);
    break;
  }
  return FALSE;			/* keeps lint happy */
}


/*
 * The entropy decoder calls this routine if it finds a marker other than
 * the restart marker it was expecting.  (This code is *not* used unless
 * a nonzero restart interval has been declared.)  The passed parameter is
 * the marker code actually found (might be anything, except 0 or FF).
 * The desired restart marker is that indicated by cinfo->next_restart_num.
 * This routine is supposed to apply whatever error recovery strategy seems
 * appropriate in order to position the input stream to the next data segment.
 * For some file formats (eg, TIFF) extra information such as tile boundary
 * pointers may be available to help in this decision.
 *
 * This implementation is substantially constrained by wanting to treat the
 * input as a data stream; this means we can't back up.  (For instance, we
 * generally can't fseek() if the input is a Unix pipe.)  Therefore, we have
 * only the following actions to work with:
 *   1. Do nothing, let the entropy decoder resume at next byte of file.
 *   2. Read forward until we find another marker, discarding intervening
 *      data.  (In theory we could look ahead within the current bufferload,
 *      without having to discard data if we don't find the desired marker.
 *      This idea is not implemented here, in part because it makes behavior
 *      dependent on buffer size and chance buffer-boundary positions.)
 *   3. Push back the passed marker (with JUNGETC).  This will cause the
 *      entropy decoder to process an empty data segment, inserting dummy
 *      zeroes, and then re-read the marker we pushed back.
 * #2 is appropriate if we think the desired marker lies ahead, while #3 is
 * appropriate if the found marker is a future restart marker (indicating
 * that we have missed the desired restart marker, probably because it got
 * corrupted).

 * We apply #2 or #3 if the found marker is a restart marker no more than
 * two counts behind or ahead of the expected one.  We also apply #2 if the
 * found marker is not a legal JPEG marker code (it's certainly bogus data).
 * If the found marker is a restart marker more than 2 counts away, we do #1
 * (too much risk that the marker is erroneous; with luck we will be able to
 * resync at some future point).
 * For any valid non-restart JPEG marker, we apply #3.  This keeps us from
 * overrunning the end of a scan.  An implementation limited to single-scan
 * files might find it better to apply #2 for markers other than EOI, since
 * any other marker would have to be bogus data in that case.
 */

METHODDEF void
resync_to_restart (decompress_info_ptr cinfo, int marker)
{
  int desired = cinfo->next_restart_num;
  int action = 1;

  /* Always put up a warning. */
  WARNMS2(cinfo->emethods,
	  "Corrupt JPEG data: found 0x%02x marker instead of RST%d",
	  marker, desired);
  /* Outer loop handles repeated decision after scanning forward. */
  for (;;) {
    if (marker < (int) M_SOF0)
      action = 2;		/* invalid marker */
    else if (marker < (int) M_RST0 || marker > (int) M_RST7)
      action = 3;		/* valid non-restart marker */
    else {
      if (marker == ((int) M_RST0 + ((desired+1) & 7)) ||
	  marker == ((int) M_RST0 + ((desired+2) & 7)))
	action = 3;		/* one of the next two expected restarts */
      else if (marker == ((int) M_RST0 + ((desired-1) & 7)) ||
	       marker == ((int) M_RST0 + ((desired-2) & 7)))
	action = 2;		/* a prior restart, so advance */
      else
	action = 1;		/* desired restart or too far away */
    }
    TRACEMS2(cinfo->emethods, 4,
	     "At marker 0x%02x, recovery action %d", marker, action);
    switch (action) {
    case 1:
      /* Let entropy decoder resume processing. */
      return;
    case 2:
      /* Scan to the next marker, and repeat the decision loop. */
      marker = next_marker(cinfo);
      break;
    case 3:
      /* Put back this marker & return. */
      /* Entropy decoder will be forced to process an empty segment. */
      JUNGETC(marker, cinfo);
      JUNGETC(0xFF, cinfo);
      return;
    }
  }
}


/*
 * Finish up after a compressed scan (series of read_jpeg_data calls);
 * prepare for another read_scan_header call.
 */

METHODDEF void
read_scan_trailer (decompress_info_ptr cinfo)
{
  /* no work needed */
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
read_file_trailer (decompress_info_ptr cinfo)
{
  /* no work needed */
}


/*
 * The method selection routine for standard JPEG header reading.
 * Note that this must be called by the user interface before calling
 * jpeg_decompress.  When a non-JFIF file is to be decompressed (TIFF,
 * perhaps), the user interface must discover the file type and call
 * the appropriate method selection routine.
 */

GLOBAL void
jselrjfif (decompress_info_ptr cinfo)
{
  cinfo->methods->read_file_header = read_file_header;
  cinfo->methods->read_scan_header = read_scan_header;
  /* For JFIF/raw-JPEG format, the user interface supplies read_jpeg_data. */
#if 0
  cinfo->methods->read_jpeg_data = read_jpeg_data;
#endif
  cinfo->methods->resync_to_restart = resync_to_restart;
  cinfo->methods->read_scan_trailer = read_scan_trailer;
  cinfo->methods->read_file_trailer = read_file_trailer;
}

#endif /* JFIF_SUPPORTED */

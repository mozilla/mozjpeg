/*
 * jrdgif.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in GIF format.
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume input from
 * an ordinary stdio stream.  They further assume that reading begins
 * at the start of the file; input_init may need work if the
 * user interface has already read some data (e.g., to determine that
 * the file is indeed GIF format).
 *
 * These routines are invoked via the methods get_input_row
 * and input_init/term.
 */

/*
 * This code is loosely based on giftoppm from the PBMPLUS distribution
 * of Feb. 1991.  That file contains the following copyright notice:
 * +-------------------------------------------------------------------+
 * | Copyright 1990, David Koblas.                                     |
 * |   Permission to use, copy, modify, and distribute this software   |
 * |   and its documentation for any purpose and without fee is hereby |
 * |   granted, provided that the above copyright notice appear in all |
 * |   copies and that both that copyright notice and this permission  |
 * |   notice appear in supporting documentation.  This software is    |
 * |   provided "as is" without express or implied warranty.           |
 * +-------------------------------------------------------------------+
 *
 * We are also required to state that
 *    "The Graphics Interchange Format(c) is the Copyright property of
 *    CompuServe Incorporated. GIF(sm) is a Service Mark property of
 *    CompuServe Incorporated."
 */

#include "jinclude.h"

#ifdef GIF_SUPPORTED


#define	MAXCOLORMAPSIZE	256	/* max # of colors in a GIF colormap */
#define NUMCOLORS	3	/* # of colors */
#define CM_RED		0	/* color component numbers */
#define CM_GREEN	1
#define CM_BLUE		2

static JSAMPARRAY colormap;	/* the colormap to use */
/* colormap[i][j] = value of i'th color component for pixel value j */

#define	MAX_LZW_BITS	12	/* maximum LZW code size */
#define LZW_TABLE_SIZE	(1<<MAX_LZW_BITS) /* # of possible LZW symbols */

/* Macros for extracting header data --- note we assume chars may be signed */

#define LM_to_uint(a,b)		((((b)&0xFF) << 8) | ((a)&0xFF))

#define BitSet(byte, bit)	((byte) & (bit))
#define INTERLACE	0x40	/* mask for bit signifying interlaced image */
#define COLORMAPFLAG	0x80	/* mask for bit signifying colormap presence */

#define	ReadOK(file,buffer,len)	(JFREAD(file,buffer,len) == ((size_t) (len)))

/* Static vars for GetCode and LZWReadByte */

static char code_buf[256+4];	/* current input data block */
static int last_byte;		/* # of bytes in code_buf */
static int last_bit;		/* # of bits in code_buf */
static int cur_bit;		/* next bit index to read */
static boolean out_of_blocks;	/* TRUE if hit terminator data block */

static int input_code_size;	/* codesize given in GIF file */
static int clear_code,end_code; /* values for Clear and End codes */

static int code_size;		/* current actual code size */
static int limit_code;		/* 2^code_size */
static int max_code;		/* first unused code value */
static boolean first_time;	/* flags first call to LZWReadByte */

/* LZW decompression tables:
 *   symbol_head[K] = prefix symbol of any LZW symbol K (0..LZW_TABLE_SIZE-1)
 *   symbol_tail[K] = suffix byte   of any LZW symbol K (0..LZW_TABLE_SIZE-1)
 * Note that entries 0..end_code of the above tables are not used,
 * since those symbols represent raw bytes or special codes.
 *
 * The stack represents the not-yet-used expansion of the last LZW symbol.
 * In the worst case, a symbol could expand to as many bytes as there are
 * LZW symbols, so we allocate LZW_TABLE_SIZE bytes for the stack.
 * (This is conservative since that number includes the raw-byte symbols.)
 *
 * The tables are allocated from FAR heap space since they would use up
 * rather a lot of the near data space in a PC.
 */

static UINT16 FAR *symbol_head; /* => table of prefix symbols */
static UINT8  FAR *symbol_tail; /* => table of suffix bytes */
static UINT8  FAR *symbol_stack; /* stack for symbol expansions */
static UINT8  FAR *sp;		/* stack pointer */

/* Static state for interlaced image processing */

static boolean is_interlaced;	/* TRUE if have interlaced image */
static big_sarray_ptr interlaced_image;	/* full image in interlaced order */
static long cur_row_number;	/* need to know actual row number */
static long pass2_offset;	/* # of pixel rows in pass 1 */
static long pass3_offset;	/* # of pixel rows in passes 1&2 */
static long pass4_offset;	/* # of pixel rows in passes 1,2,3 */


/* Forward declarations */
METHODDEF void load_interlaced_image PP((compress_info_ptr cinfo, JSAMPARRAY pixel_row));
METHODDEF void get_interlaced_row PP((compress_info_ptr cinfo, JSAMPARRAY pixel_row));



LOCAL int
ReadByte (compress_info_ptr cinfo)
/* Read next byte from GIF file */
{
  register FILE * infile = cinfo->input_file;
  int c;

  if ((c = getc(infile)) == EOF)
    ERREXIT(cinfo->emethods, "Premature EOF in GIF file");
  return c;
}


LOCAL int
GetDataBlock (compress_info_ptr cinfo, char *buf)
/* Read a GIF data block, which has a leading count byte */
/* A zero-length block marks the end of a data block sequence */
{
  int count;

  count = ReadByte(cinfo);
  if (count > 0) {
    if (! ReadOK(cinfo->input_file, buf, count))
      ERREXIT(cinfo->emethods, "Premature EOF in GIF file");
  }
  return count;
}


LOCAL void
SkipDataBlocks (compress_info_ptr cinfo)
/* Skip a series of data blocks, until a block terminator is found */
{
  char buf[256];

  while (GetDataBlock(cinfo, buf) > 0)
    /* skip */;
}


LOCAL void
ReInitLZW (void)
/* (Re)initialize LZW state; shared code for startup and Clear processing */
{
  code_size = input_code_size+1;
  limit_code = clear_code << 1;	/* 2^code_size */
  max_code = clear_code + 2;	/* first unused code value */
  sp = symbol_stack;		/* init stack to empty */
}


LOCAL void
InitLZWCode (void)
/* Initialize for a series of LZWReadByte (and hence GetCode) calls */
{
  /* GetCode initialization */
  last_byte = 2;		/* make safe to "recopy last two bytes" */
  last_bit = 0;			/* nothing in the buffer */
  cur_bit = 0;			/* force buffer load on first call */
  out_of_blocks = FALSE;

  /* LZWReadByte initialization */
  clear_code = 1 << input_code_size; /* compute special code values */
  end_code = clear_code + 1;	/* note that these do not change */
  first_time = TRUE;
  ReInitLZW();
}


LOCAL int
GetCode (compress_info_ptr cinfo)
/* Fetch the next code_size bits from the GIF data */
/* We assume code_size is less than 16 */
{
  register INT32 accum;
  int offs, ret, count;

  if ( (cur_bit+code_size) > last_bit) {
    /* Time to reload the buffer */
    if (out_of_blocks) {
      WARNMS(cinfo->emethods, "Ran out of GIF bits");
      return end_code;		/* fake something useful */
    }
    /* preserve last two bytes of what we have -- assume code_size <= 16 */
    code_buf[0] = code_buf[last_byte-2];
    code_buf[1] = code_buf[last_byte-1];
    /* Load more bytes; set flag if we reach the terminator block */
    if ((count = GetDataBlock(cinfo, &code_buf[2])) == 0) {
      out_of_blocks = TRUE;
      WARNMS(cinfo->emethods, "Ran out of GIF bits");
      return end_code;		/* fake something useful */
    }
    /* Reset counters */
    cur_bit = (cur_bit - last_bit) + 16;
    last_byte = 2 + count;
    last_bit = last_byte * 8;
  }

  /* Form up next 24 bits in accum */
  offs = cur_bit >> 3;		/* byte containing cur_bit */
#ifdef CHAR_IS_UNSIGNED
  accum = code_buf[offs+2];
  accum <<= 8;
  accum |= code_buf[offs+1];
  accum <<= 8;
  accum |= code_buf[offs];
#else
  accum = code_buf[offs+2] & 0xFF;
  accum <<= 8;
  accum |= code_buf[offs+1] & 0xFF;
  accum <<= 8;
  accum |= code_buf[offs] & 0xFF;
#endif

  /* Right-align cur_bit in accum, then mask off desired number of bits */
  accum >>= (cur_bit & 7);
  ret = ((int) accum) & ((1 << code_size) - 1);
  
  cur_bit += code_size;
  return ret;
}


LOCAL int
LZWReadByte (compress_info_ptr cinfo)
/* Read an LZW-compressed byte */
{
  static int oldcode;		/* previous LZW symbol */
  static int firstcode;		/* first byte of oldcode's expansion */
  register int code;		/* current working code */
  int incode;			/* saves actual input code */

  /* First time, just eat the expected Clear code(s) and return next code, */
  /* which is expected to be a raw byte. */
  if (first_time) {
    first_time = FALSE;
    code = clear_code;		/* enables sharing code with Clear case */
  } else {

    /* If any codes are stacked from a previously read symbol, return them */
    if (sp > symbol_stack)
      return (int) *(--sp);

    /* Time to read a new symbol */
    code = GetCode(cinfo);

  }

  if (code == clear_code) {
    /* Reinit static state, swallow any extra Clear codes, and */
    /* return next code, which is expected to be a raw byte. */
    ReInitLZW();
    do {
      code = GetCode(cinfo);
    } while (code == clear_code);
    if (code > clear_code) {	/* make sure it is a raw byte */
      WARNMS(cinfo->emethods, "Corrupt data in GIF file");
      code = 0;			/* use something valid */
    }
    firstcode = oldcode = code;	/* make firstcode, oldcode valid! */
    return code;
  }

  if (code == end_code) {
    /* Skip the rest of the image, unless GetCode already read terminator */
    if (! out_of_blocks) {
      SkipDataBlocks(cinfo);
      out_of_blocks = TRUE;
    }
    /* Complain that there's not enough data */
    WARNMS(cinfo->emethods, "Premature end of GIF image");
    /* Pad data with 0's */
    return 0;			/* fake something usable */
  }

  /* Got normal raw byte or LZW symbol */
  incode = code;		/* save for a moment */
  
  if (code >= max_code) {	/* special case for not-yet-defined symbol */
    /* code == max_code is OK; anything bigger is bad data */
    if (code > max_code) {
      WARNMS(cinfo->emethods, "Corrupt data in GIF file");
      incode = 0;		/* prevent creation of loops in symbol table */
    }
    *sp++ = (UINT8) firstcode;	/* it will be defined as oldcode/firstcode */
    code = oldcode;
  }

  /* If it's a symbol, expand it into the stack */
  while (code >= clear_code) {
    *sp++ = symbol_tail[code];	/* tail of symbol: a simple byte value */
    code = symbol_head[code];	/* head of symbol: another LZW symbol */
  }
  /* At this point code just represents a raw byte */
  firstcode = code;		/* save for possible future use */

  /* If there's room in table, */
  if ((code = max_code) < LZW_TABLE_SIZE) {
    /* Define a new symbol = prev sym + head of this sym's expansion */
    symbol_head[code] = oldcode;
    symbol_tail[code] = (UINT8) firstcode;
    max_code++;
    /* Is it time to increase code_size? */
    if ((max_code >= limit_code) && (code_size < MAX_LZW_BITS)) {
      code_size++;
      limit_code <<= 1;		/* keep equal to 2^code_size */
    }
  }
  
  oldcode = incode;		/* save last input symbol for future use */
  return firstcode;		/* return first byte of symbol's expansion */
}


LOCAL void
ReadColorMap (compress_info_ptr cinfo, int cmaplen, JSAMPARRAY cmap)
/* Read a GIF colormap */
{
  int i;

  for (i = 0; i < cmaplen; i++) {
    cmap[CM_RED][i]   = (JSAMPLE) ReadByte(cinfo);
    cmap[CM_GREEN][i] = (JSAMPLE) ReadByte(cinfo);
    cmap[CM_BLUE][i]  = (JSAMPLE) ReadByte(cinfo);
  }
}


LOCAL void
DoExtension (compress_info_ptr cinfo)
/* Process an extension block */
/* Currently we ignore 'em all */
{
  int extlabel;

  /* Read extension label byte */
  extlabel = ReadByte(cinfo);
  TRACEMS1(cinfo->emethods, 1, "Ignoring GIF extension block of type 0x%02x",
	   extlabel);
  /* Skip the data block(s) associated with the extension */
  SkipDataBlocks(cinfo);
}


/*
 * Read the file header; return image size and component count.
 */

METHODDEF void
input_init (compress_info_ptr cinfo)
{
  char hdrbuf[10];		/* workspace for reading control blocks */
  UINT16 width, height;		/* image dimensions */
  int colormaplen, aspectRatio;
  int c;

  /* Allocate space to store the colormap */
  colormap = (*cinfo->emethods->alloc_small_sarray)
		((long) MAXCOLORMAPSIZE, (long) NUMCOLORS);

  /* Read and verify GIF Header */
  if (! ReadOK(cinfo->input_file, hdrbuf, 6))
    ERREXIT(cinfo->emethods, "Not a GIF file");
  if (hdrbuf[0] != 'G' || hdrbuf[1] != 'I' || hdrbuf[2] != 'F')
    ERREXIT(cinfo->emethods, "Not a GIF file");
  /* Check for expected version numbers.
   * If unknown version, give warning and try to process anyway;
   * this is per recommendation in GIF89a standard.
   */
  if ((hdrbuf[3] != '8' || hdrbuf[4] != '7' || hdrbuf[5] != 'a') &&
      (hdrbuf[3] != '8' || hdrbuf[4] != '9' || hdrbuf[5] != 'a'))
    TRACEMS3(cinfo->emethods, 1,
	     "Warning: unexpected GIF version number '%c%c%c'",
	     hdrbuf[3], hdrbuf[4], hdrbuf[5]);

  /* Read and decipher Logical Screen Descriptor */
  if (! ReadOK(cinfo->input_file, hdrbuf, 7))
    ERREXIT(cinfo->emethods, "Premature EOF in GIF file");
  width = LM_to_uint(hdrbuf[0],hdrbuf[1]);
  height = LM_to_uint(hdrbuf[2],hdrbuf[3]);
  colormaplen = 2 << (hdrbuf[4] & 0x07);
  /* we ignore the color resolution, sort flag, and background color index */
  aspectRatio = hdrbuf[6] & 0xFF;
  if (aspectRatio != 0 && aspectRatio != 49)
    TRACEMS(cinfo->emethods, 1, "Warning: nonsquare pixels in input");

  /* Read global colormap if header indicates it is present */
  if (BitSet(hdrbuf[4], COLORMAPFLAG))
    ReadColorMap(cinfo, colormaplen, colormap);

  /* Scan until we reach start of desired image.
   * We don't currently support skipping images, but could add it easily.
   */
  for (;;) {
    c = ReadByte(cinfo);

    if (c == ';')		/* GIF terminator?? */
      ERREXIT(cinfo->emethods, "Too few images in GIF file");

    if (c == '!') {		/* Extension */
      DoExtension(cinfo);
      continue;
    }
    
    if (c != ',') {		/* Not an image separator? */
      TRACEMS1(cinfo->emethods, 1, "Bogus input char 0x%02x, ignoring", c);
      continue;
    }

    /* Read and decipher Local Image Descriptor */
    if (! ReadOK(cinfo->input_file, hdrbuf, 9))
      ERREXIT(cinfo->emethods, "Premature EOF in GIF file");
    /* we ignore top/left position info, also sort flag */
    width = LM_to_uint(hdrbuf[4],hdrbuf[5]);
    height = LM_to_uint(hdrbuf[6],hdrbuf[7]);
    is_interlaced = BitSet(hdrbuf[8], INTERLACE);

    /* Read local colormap if header indicates it is present */
    /* Note: if we wanted to support skipping images, */
    /* we'd need to skip rather than read colormap for ignored images */
    if (BitSet(hdrbuf[8], COLORMAPFLAG)) {
      colormaplen = 2 << (hdrbuf[8] & 0x07);
      ReadColorMap(cinfo, colormaplen, colormap);
    }

    input_code_size = ReadByte(cinfo); /* get minimum-code-size byte */
    if (input_code_size < 2 || input_code_size >= MAX_LZW_BITS)
      ERREXIT1(cinfo->emethods, "Bogus codesize %d", input_code_size);

    /* Reached desired image, so break out of loop */
    /* If we wanted to skip this image, */
    /* we'd call SkipDataBlocks and then continue the loop */
    break;
  }

  /* Prepare to read selected image: first initialize LZW decompressor */
  symbol_head = (UINT16 FAR *) (*cinfo->emethods->alloc_medium)
				(LZW_TABLE_SIZE * SIZEOF(UINT16));
  symbol_tail = (UINT8 FAR *) (*cinfo->emethods->alloc_medium)
				(LZW_TABLE_SIZE * SIZEOF(UINT8));
  symbol_stack = (UINT8 FAR *) (*cinfo->emethods->alloc_medium)
				(LZW_TABLE_SIZE * SIZEOF(UINT8));
  InitLZWCode();

  /*
   * If image is interlaced, we read it into a full-size sample array,
   * decompressing as we go; then get_input_row selects rows from the
   * sample array in the proper order.
   */
  if (is_interlaced) {
    /* We request the big array now, but can't access it until the pipeline
     * controller causes all the big arrays to be allocated.  Hence, the
     * actual work of reading the image is postponed until the first call
     * of get_input_row.
     */
    interlaced_image = (*cinfo->emethods->request_big_sarray)
		((long) width, (long) height, 1L);
    cinfo->methods->get_input_row = load_interlaced_image;
    cinfo->total_passes++;	/* count file reading as separate pass */
  }

  /* Return info about the image. */
  cinfo->input_components = NUMCOLORS;
  cinfo->in_color_space = CS_RGB;
  cinfo->image_width = width;
  cinfo->image_height = height;
  cinfo->data_precision = 8;	/* always, even if 12-bit JSAMPLEs */

  TRACEMS3(cinfo->emethods, 1, "%ux%ux%d GIF image",
	   (unsigned int) width, (unsigned int) height, colormaplen);
}


/*
 * Read one row of pixels.
 * This version is used for noninterlaced GIF images:
 * we read directly from the GIF file.
 */

METHODDEF void
get_input_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
{
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  register int c;
  
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  for (col = cinfo->image_width; col > 0; col--) {
    c = LZWReadByte(cinfo);
    *ptr0++ = colormap[CM_RED][c];
    *ptr1++ = colormap[CM_GREEN][c];
    *ptr2++ = colormap[CM_BLUE][c];
  }
}


/*
 * Read one row of pixels.
 * This version is used for the first call on get_input_row when
 * reading an interlaced GIF file: we read the whole image into memory.
 */

METHODDEF void
load_interlaced_image (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
{
  JSAMPARRAY image_ptr;
  register JSAMPROW sptr;
  register long col;
  long row;

  /* Read the interlaced image into the big array we've created. */
  for (row = 0; row < cinfo->image_height; row++) {
    (*cinfo->methods->progress_monitor) (cinfo, row, cinfo->image_height);
    image_ptr = (*cinfo->emethods->access_big_sarray)
			(interlaced_image, row, TRUE);
    sptr = image_ptr[0];
    for (col = cinfo->image_width; col > 0; col--) {
      *sptr++ = (JSAMPLE) LZWReadByte(cinfo);
    }
  }
  cinfo->completed_passes++;

  /* Replace method pointer so subsequent calls don't come here. */
  cinfo->methods->get_input_row = get_interlaced_row;
  /* Initialize for get_interlaced_row, and perform first call on it. */
  cur_row_number = 0;
  pass2_offset = (cinfo->image_height + 7L) / 8L;
  pass3_offset = pass2_offset + (cinfo->image_height + 3L) / 8L;
  pass4_offset = pass3_offset + (cinfo->image_height + 1L) / 4L;

  get_interlaced_row(cinfo, pixel_row);
}


/*
 * Read one row of pixels.
 * This version is used for interlaced GIF images:
 * we read from the big in-memory image.
 */

METHODDEF void
get_interlaced_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
{
  JSAMPARRAY image_ptr;
  register JSAMPROW sptr, ptr0, ptr1, ptr2;
  register long col;
  register int c;
  long irow;

  /* Figure out which row of interlaced image is needed, and access it. */
  switch ((int) (cur_row_number & 7L)) {
  case 0:			/* first-pass row */
    irow = cur_row_number >> 3;
    break;
  case 4:			/* second-pass row */
    irow = (cur_row_number >> 3) + pass2_offset;
    break;
  case 2:			/* third-pass row */
  case 6:
    irow = (cur_row_number >> 2) + pass3_offset;
    break;
  default:			/* fourth-pass row */
    irow = (cur_row_number >> 1) + pass4_offset;
    break;
  }
  image_ptr = (*cinfo->emethods->access_big_sarray)
			(interlaced_image, irow, FALSE);
  /* Scan the row, expand colormap, and output */
  sptr = image_ptr[0];
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  for (col = cinfo->image_width; col > 0; col--) {
    c = GETJSAMPLE(*sptr++);
    *ptr0++ = colormap[CM_RED][c];
    *ptr1++ = colormap[CM_GREEN][c];
    *ptr2++ = colormap[CM_BLUE][c];
  }
  cur_row_number++;		/* for next time */
}


/*
 * Finish up at the end of the file.
 */

METHODDEF void
input_term (compress_info_ptr cinfo)
{
  /* no work (we let free_all release the workspace) */
}


/*
 * The method selection routine for GIF format input.
 * Note that this must be called by the user interface before calling
 * jpeg_compress.  If multiple input formats are supported, the
 * user interface is responsible for discovering the file format and
 * calling the appropriate method selection routine.
 */

GLOBAL void
jselrgif (compress_info_ptr cinfo)
{
  cinfo->methods->input_init = input_init;
  cinfo->methods->get_input_row = get_input_row; /* assume uninterlaced */
  cinfo->methods->input_term = input_term;
}

#endif /* GIF_SUPPORTED */

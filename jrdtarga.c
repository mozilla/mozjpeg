/*
 * jrdtarga.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains routines to read input images in Targa format.
 *
 * These routines may need modification for non-Unix environments or
 * specialized applications.  As they stand, they assume input from
 * an ordinary stdio stream.  They further assume that reading begins
 * at the start of the file; input_init may need work if the
 * user interface has already read some data (e.g., to determine that
 * the file is indeed Targa format).
 *
 * These routines are invoked via the methods get_input_row
 * and input_init/term.
 *
 * Based on code contributed by Lee Daniel Crocker.
 */

#include "jinclude.h"

#ifdef TARGA_SUPPORTED


/* Macros to deal with unsigned chars as efficiently as compiler allows */

#ifdef HAVE_UNSIGNED_CHAR
typedef unsigned char U_CHAR;
#define UCH(x)	((int) (x))
#else /* !HAVE_UNSIGNED_CHAR */
#ifdef CHAR_IS_UNSIGNED
typedef char U_CHAR;
#define UCH(x)	((int) (x))
#else
typedef char U_CHAR;
#define UCH(x)	((int) (x) & 0xFF)
#endif
#endif /* HAVE_UNSIGNED_CHAR */


#define	ReadOK(file,buffer,len)	(JFREAD(file,buffer,len) == ((size_t) (len)))


static JSAMPARRAY colormap;	/* Targa colormap (converted to my format) */

static big_sarray_ptr whole_image; /* Needed if funny input row order */
static long current_row;	/* Current logical row number to read */

/* Pointer to routine to extract next Targa pixel from input file */
static void (*read_pixel) PP((compress_info_ptr cinfo));

/* Result of read_pixel is delivered here: */
static U_CHAR tga_pixel[4];

static int pixel_size;		/* Bytes per Targa pixel (1 to 4) */

/* State info for reading RLE-coded pixels; both counts must be init to 0 */
static int block_count;		/* # of pixels remaining in RLE block */
static int dup_pixel_count;	/* # of times to duplicate previous pixel */

/* This saves the correct pixel-row-expansion method for preload_image */
static void (*get_pixel_row) PP((compress_info_ptr cinfo,
				 JSAMPARRAY pixel_row));


/* For expanding 5-bit pixel values to 8-bit with best rounding */

static const UINT8 c5to8bits[32] = {
    0,   8,  16,  24,  32,  41,  49,  57,
   65,  74,  82,  90,  98, 106, 115, 123,
  131, 139, 148, 156, 164, 172, 180, 189,
  197, 205, 213, 222, 230, 238, 246, 255
};



LOCAL int
read_byte (compress_info_ptr cinfo)
/* Read next byte from Targa file */
{
  register FILE *infile = cinfo->input_file;
  register int c;

  if ((c = getc(infile)) == EOF)
    ERREXIT(cinfo->emethods, "Premature EOF in Targa file");
  return c;
}


LOCAL void
read_colormap (compress_info_ptr cinfo, int cmaplen, int mapentrysize)
/* Read the colormap from a Targa file */
{
  int i;

  /* Presently only handles 24-bit BGR format */
  if (mapentrysize != 24)
    ERREXIT(cinfo->emethods, "Unsupported Targa colormap format");

  for (i = 0; i < cmaplen; i++) {
    colormap[2][i] = (JSAMPLE) read_byte(cinfo);
    colormap[1][i] = (JSAMPLE) read_byte(cinfo);
    colormap[0][i] = (JSAMPLE) read_byte(cinfo);
  }
}


/*
 * read_pixel methods: get a single pixel from Targa file into tga_pixel[]
 */

LOCAL void
read_non_rle_pixel (compress_info_ptr cinfo)
/* Read one Targa pixel from the input file; no RLE expansion */
{
  register FILE * infile = cinfo->input_file;
  register int i;

  for (i = 0; i < pixel_size; i++) {
    tga_pixel[i] = (U_CHAR) getc(infile);
  }
}


LOCAL void
read_rle_pixel (compress_info_ptr cinfo)
/* Read one Targa pixel from the input file, expanding RLE data as needed */
{
  register FILE * infile = cinfo->input_file;
  register int i;

  /* Duplicate previously read pixel? */
  if (dup_pixel_count > 0) {
    dup_pixel_count--;
    return;
  }

  /* Time to read RLE block header? */
  if (--block_count < 0) {	/* decrement pixels remaining in block */
    i = read_byte(cinfo);
    if (i & 0x80) {		/* Start of duplicate-pixel block? */
      dup_pixel_count = i & 0x7F; /* number of duplications after this one */
      block_count = 0;		/* then read new block header */
    } else {
      block_count = i & 0x7F;	/* number of pixels after this one */
    }
  }

  /* Read next pixel */
  for (i = 0; i < pixel_size; i++) {
    tga_pixel[i] = (U_CHAR) getc(infile);
  }
}


/*
 * Read one row of pixels.
 *
 * We provide several different versions depending on input file format.
 */


METHODDEF void
get_8bit_gray_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading 8-bit grayscale pixels */
{
  register JSAMPROW ptr0;
  register long col;
  
  ptr0 = pixel_row[0];
  for (col = cinfo->image_width; col > 0; col--) {
    (*read_pixel) (cinfo);	/* Load next pixel into tga_pixel */
    *ptr0++ = (JSAMPLE) UCH(tga_pixel[0]);
  }
}

METHODDEF void
get_8bit_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading 8-bit colormap indexes */
{
  register int t;
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  for (col = cinfo->image_width; col > 0; col--) {
    (*read_pixel) (cinfo);	/* Load next pixel into tga_pixel */
    t = UCH(tga_pixel[0]);
    *ptr0++ = colormap[0][t];
    *ptr1++ = colormap[1][t];
    *ptr2++ = colormap[2][t];
  }
}

METHODDEF void
get_16bit_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading 16-bit pixels */
{
  register int t;
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  for (col = cinfo->image_width; col > 0; col--) {
    (*read_pixel) (cinfo);	/* Load next pixel into tga_pixel */
    t = UCH(tga_pixel[0]);
    t += UCH(tga_pixel[1]) << 8;
    /* We expand 5 bit data to 8 bit sample width.
     * The format of the 16-bit (LSB first) input word is
     *     xRRRRRGGGGGBBBBB
     */
    *ptr2++ = (JSAMPLE) c5to8bits[t & 0x1F];
    t >>= 5;
    *ptr1++ = (JSAMPLE) c5to8bits[t & 0x1F];
    t >>= 5;
    *ptr0++ = (JSAMPLE) c5to8bits[t & 0x1F];
  }
}

METHODDEF void
get_24bit_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading 24-bit pixels */
{
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  for (col = cinfo->image_width; col > 0; col--) {
    (*read_pixel) (cinfo);	/* Load next pixel into tga_pixel */
    *ptr0++ = (JSAMPLE) UCH(tga_pixel[2]); /* convert BGR to RGB order */
    *ptr1++ = (JSAMPLE) UCH(tga_pixel[1]);
    *ptr2++ = (JSAMPLE) UCH(tga_pixel[0]);
  }
}

/*
 * Targa also defines a 32-bit pixel format with order B,G,R,A.
 * We presently ignore the attribute byte, so the code for reading
 * these pixels is identical to the 24-bit routine above.
 * This works because the actual pixel length is only known to read_pixel.
 */

#define get_32bit_row  get_24bit_row


/*
 * This method is for re-reading the input data in standard top-down
 * row order.  The entire image has already been read into whole_image
 * with proper conversion of pixel format, but it's in a funny row order.
 */

METHODDEF void
get_memory_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
{
  JSAMPARRAY image_ptr;
  long source_row;

  /* Compute row of source that maps to current_row of normal order */
  /* For now, assume image is bottom-up and not interlaced. */
  /* NEEDS WORK to support interlaced images! */
  source_row = cinfo->image_height - current_row - 1;

  /* Fetch that row from virtual array */
  image_ptr = (*cinfo->emethods->access_big_sarray)
		(whole_image, source_row * cinfo->input_components, FALSE);

  jcopy_sample_rows(image_ptr, 0, pixel_row, 0,
		    cinfo->input_components, cinfo->image_width);

  current_row++;
}


/*
 * This method loads the image into whole_image during the first call on
 * get_input_row.  The get_input_row pointer is then adjusted to call
 * get_memory_row on subsequent calls.
 */

METHODDEF void
preload_image (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
{
  JSAMPARRAY image_ptr;
  long row;

  /* Read the data into a virtual array in input-file row order */
  for (row = 0; row < cinfo->image_height; row++) {
    (*cinfo->methods->progress_monitor) (cinfo, row, cinfo->image_height);
    image_ptr = (*cinfo->emethods->access_big_sarray)
			(whole_image, row * cinfo->input_components, TRUE);
    (*get_pixel_row) (cinfo, image_ptr);
  }
  cinfo->completed_passes++;

  /* Set up to read from the virtual array in unscrambled order */
  cinfo->methods->get_input_row = get_memory_row;
  current_row = 0;
  /* And read the first row */
  get_memory_row(cinfo, pixel_row);
}


/*
 * Read the file header; return image size and component count.
 */

METHODDEF void
input_init (compress_info_ptr cinfo)
{
  U_CHAR targaheader[18];
  int idlen, cmaptype, subtype, flags, interlace_type, components;
  UINT16 width, height, maplen;
  boolean is_bottom_up;

#define GET_2B(offset)	((unsigned int) UCH(targaheader[offset]) + \
			 (((unsigned int) UCH(targaheader[offset+1])) << 8))
  
  if (! ReadOK(cinfo->input_file, targaheader, 18))
    ERREXIT(cinfo->emethods, "Unexpected end of file");

  /* Pretend "15-bit" pixels are 16-bit --- we ignore attribute bit anyway */
  if (targaheader[16] == 15)
    targaheader[16] = 16;

  idlen = UCH(targaheader[0]);
  cmaptype = UCH(targaheader[1]);
  subtype = UCH(targaheader[2]);
  maplen = GET_2B(5);
  width = GET_2B(12);
  height = GET_2B(14);
  pixel_size = UCH(targaheader[16]) >> 3;
  flags = UCH(targaheader[17]);	/* Image Descriptor byte */

  is_bottom_up = ((flags & 0x20) == 0);	/* bit 5 set => top-down */
  interlace_type = flags >> 6;	/* bits 6/7 are interlace code */

  if (cmaptype > 1 ||		/* cmaptype must be 0 or 1 */
      pixel_size < 1 || pixel_size > 4 ||
      (UCH(targaheader[16]) & 7) != 0 || /* bits/pixel must be multiple of 8 */
      interlace_type != 0)	/* currently don't allow interlaced image */
    ERREXIT(cinfo->emethods, "Invalid or unsupported Targa file");
  
  if (subtype > 8) {
    /* It's an RLE-coded file */
    read_pixel = read_rle_pixel;
    block_count = dup_pixel_count = 0;
    subtype -= 8;
  } else {
    /* Non-RLE file */
    read_pixel = read_non_rle_pixel;
  }

  /* Now should have subtype 1, 2, or 3 */
  components = 3;		/* until proven different */
  cinfo->in_color_space = CS_RGB;

  switch (subtype) {
  case 1:			/* colormapped image */
    if (pixel_size == 1 && cmaptype == 1)
      get_pixel_row = get_8bit_row;
    else
      ERREXIT(cinfo->emethods, "Invalid or unsupported Targa file");
    TRACEMS2(cinfo->emethods, 1, "%ux%u colormapped Targa image",
	     width, height);
    break;
  case 2:			/* RGB image */
    switch (pixel_size) {
    case 2:
      get_pixel_row = get_16bit_row;
      break;
    case 3:
      get_pixel_row = get_24bit_row;
      break;
    case 4:
      get_pixel_row = get_32bit_row;
      break;
    default:
      ERREXIT(cinfo->emethods, "Invalid or unsupported Targa file");
      break;
    }
    TRACEMS2(cinfo->emethods, 1, "%ux%u RGB Targa image",
	     width, height);
    break;
  case 3:			/* Grayscale image */
    components = 1;
    cinfo->in_color_space = CS_GRAYSCALE;
    if (pixel_size == 1)
      get_pixel_row = get_8bit_gray_row;
    else
      ERREXIT(cinfo->emethods, "Invalid or unsupported Targa file");
    TRACEMS2(cinfo->emethods, 1, "%ux%u grayscale Targa image",
	     width, height);
    break;
  default:
    ERREXIT(cinfo->emethods, "Invalid or unsupported Targa file");
    break;
  }

  if (is_bottom_up) {
    whole_image = (*cinfo->emethods->request_big_sarray)
			((long) width, (long) height * components,
			 (long) components);
    cinfo->methods->get_input_row = preload_image;
    cinfo->total_passes++;	/* count file reading as separate pass */
  } else {
    whole_image = NULL;
    cinfo->methods->get_input_row = get_pixel_row;
  }
  
  while (idlen--)		/* Throw away ID field */
    (void) read_byte(cinfo);

  if (maplen > 0) {
    if (maplen > 256 || GET_2B(3) != 0)
      ERREXIT(cinfo->emethods, "Colormap too large");
    /* Allocate space to store the colormap */
    colormap = (*cinfo->emethods->alloc_small_sarray)
			((long) maplen, 3L);
    /* and read it from the file */
    read_colormap(cinfo, (int) maplen, UCH(targaheader[7]));
  } else {
    if (cmaptype)		/* but you promised a cmap! */
      ERREXIT(cinfo->emethods, "Invalid or unsupported Targa file");
    colormap = NULL;
  }

  cinfo->input_components = components;
  cinfo->image_width = width;
  cinfo->image_height = height;
  cinfo->data_precision = 8;	/* always, even if 12-bit JSAMPLEs */
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
 * The method selection routine for Targa format input.
 * Note that this must be called by the user interface before calling
 * jpeg_compress.  If multiple input formats are supported, the
 * user interface is responsible for discovering the file format and
 * calling the appropriate method selection routine.
 */

GLOBAL void
jselrtarga (compress_info_ptr cinfo)
{
  cinfo->methods->input_init = input_init;
  /* cinfo->methods->get_input_row is set by input_init */
  cinfo->methods->input_term = input_term;
}

#endif /* TARGA_SUPPORTED */

/*
 * jpegdata.h
 *
 * Copyright (C) 1991, 1992, 1993, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file defines shared data structures for the various JPEG modules.
 */


/*
 * You might need to change some of the following declarations if you are
 * using the JPEG software within a surrounding application program
 * or porting it to an unusual system.
 */


/* If the source or destination of image data is not to be stdio streams,
 * these types may need work.  You can replace them with some kind of
 * pointer or indicator that is useful to you, or just ignore 'em.
 * Note that the user interface and the various jrdxxx/jwrxxx modules
 * will also need work for non-stdio input/output.
 */

typedef FILE * JFILEREF;	/* source or dest of JPEG-compressed data */

typedef FILE * IFILEREF;	/* source or dest of non-JPEG image data */


/* These defines are used in all function definitions and extern declarations.
 * You could modify them if you need to change function linkage conventions,
 * as is shown below for use with C++.  Another application would be to make
 * all functions global for use with code profilers that require it.
 * NOTE: the C++ test does the right thing if you are reading this include
 * file in a C++ application to link to JPEG code that's been compiled with a
 * regular C compiler.  I'm not sure it works if you try to compile the JPEG
 * code with C++.
 */

#define METHODDEF static	/* a function called through method pointers */
#define LOCAL	  static	/* a function used only in its module */
#define GLOBAL			/* a function referenced thru EXTERNs */
#ifdef __cplusplus
#define EXTERN	  extern "C"	/* a reference to a GLOBAL function */
#else
#define EXTERN	  extern	/* a reference to a GLOBAL function */
#endif


/* Here is the pseudo-keyword for declaring pointers that must be "far"
 * on 80x86 machines.  Most of the specialized coding for 80x86 is handled
 * by just saying "FAR *" where such a pointer is needed.  In a few places
 * explicit coding is needed; see uses of the NEED_FAR_POINTERS symbol.
 */

#ifdef NEED_FAR_POINTERS
#define FAR  far
#else
#define FAR
#endif



/* The remaining declarations are not system-dependent, we hope. */


/*
 * NOTE: if you have an ancient, strict-K&R C compiler, it may choke on the
 * similarly-named fields in Compress_info_struct and Decompress_info_struct.
 * If this happens, you can get around it by rearranging the two structs so
 * that the similarly-named fields appear first and in the same order in
 * each struct.  Since such compilers are now pretty rare, we haven't done
 * this in the portable code, preferring to maintain a logical ordering.
 */



/* This macro is used to declare a "method", that is, a function pointer. */
/* We want to supply prototype parameters if the compiler can cope. */
/* Note that the arglist parameter must be parenthesized! */

#ifdef PROTO
#define METHOD(type,methodname,arglist)  type (*methodname) arglist
#else
#define METHOD(type,methodname,arglist)  type (*methodname) ()
#endif

/* Forward references to lists of method pointers */
typedef struct External_methods_struct * external_methods_ptr;
typedef struct Compress_methods_struct * compress_methods_ptr;
typedef struct Decompress_methods_struct * decompress_methods_ptr;


/* Data structures for images containing either samples or coefficients. */
/* Note that the topmost (leftmost) index is always color component. */
/* On 80x86 machines, the image arrays are too big for near pointers, */
/* but the pointer arrays can fit in near memory. */

typedef JSAMPLE FAR *JSAMPROW;	/* ptr to one image row of pixel samples. */
typedef JSAMPROW *JSAMPARRAY;	/* ptr to some rows (a 2-D sample array) */
typedef JSAMPARRAY *JSAMPIMAGE;	/* a 3-D sample array: top index is color */


#define DCTSIZE		8	/* The basic DCT block is 8x8 samples */
#define DCTSIZE2	64	/* DCTSIZE squared; # of elements in a block */

typedef JCOEF JBLOCK[DCTSIZE2];	/* one block of coefficients */
typedef JBLOCK FAR *JBLOCKROW;	/* pointer to one row of coefficient blocks */
typedef JBLOCKROW *JBLOCKARRAY;		/* a 2-D array of coefficient blocks */
typedef JBLOCKARRAY *JBLOCKIMAGE;	/* a 3-D array of coefficient blocks */

typedef JCOEF FAR *JCOEFPTR;	/* useful in a couple of places */


/* The input and output data of the DCT transform subroutines are of
 * the following type, which need not be the same as JCOEF.
 * For example, on a machine with fast floating point, it might make sense
 * to recode the DCT routines to use floating point; then DCTELEM would be
 * 'float' or 'double'.
 */

typedef JCOEF DCTELEM;
typedef DCTELEM DCTBLOCK[DCTSIZE2];


/* Types for JPEG compression parameters and working tables. */


typedef enum {			/* defines known color spaces */
	CS_UNKNOWN,		/* error/unspecified */
	CS_GRAYSCALE,		/* monochrome (only 1 component) */
	CS_RGB,			/* red/green/blue */
	CS_YCbCr,		/* Y/Cb/Cr (also known as YUV) */
	CS_YIQ,			/* Y/I/Q */
	CS_CMYK			/* C/M/Y/K */
} COLOR_SPACE;


typedef struct {		/* Basic info about one component */
  /* These values are fixed over the whole image */
  /* For compression, they must be supplied by the user interface; */
  /* for decompression, they are read from the SOF marker. */
	short component_id;	/* identifier for this component (0..255) */
	short component_index;	/* its index in SOF or cinfo->comp_info[] */
	short h_samp_factor;	/* horizontal sampling factor (1..4) */
	short v_samp_factor;	/* vertical sampling factor (1..4) */
	short quant_tbl_no;	/* quantization table selector (0..3) */
  /* These values may vary between scans */
  /* For compression, they must be supplied by the user interface; */
  /* for decompression, they are read from the SOS marker. */
	short dc_tbl_no;	/* DC entropy table selector (0..3) */
	short ac_tbl_no;	/* AC entropy table selector (0..3) */
  /* These values are computed during compression or decompression startup */
	long true_comp_width;	/* component's image width in samples */
	long true_comp_height;	/* component's image height in samples */
	/* the above are the logical dimensions of the downsampled image */
  /* These values are computed before starting a scan of the component */
	short MCU_width;	/* number of blocks per MCU, horizontally */
	short MCU_height;	/* number of blocks per MCU, vertically */
	short MCU_blocks;	/* MCU_width * MCU_height */
	long downsampled_width;	/* image width in samples, after expansion */
	long downsampled_height; /* image height in samples, after expansion */
	/* the above are the true_comp_xxx values rounded up to multiples of */
	/* the MCU dimensions; these are the working dimensions of the array */
	/* as it is passed through the DCT or IDCT step.  NOTE: these values */
	/* differ depending on whether the component is interleaved or not!! */
  /* This flag is used only for decompression.  In cases where some of the */
  /* components will be ignored (eg grayscale output from YCbCr image), */
  /* we can skip IDCT etc. computations for the unused components. */
	boolean component_needed; /* do we need the value of this component? */
} jpeg_component_info;


/* DCT coefficient quantization tables.
 * For 8-bit precision, 'INT16' should be good enough for quantization values;
 * for more precision, we go for the full 16 bits.  'INT16' provides a useful
 * speedup on many machines (multiplication & division of JCOEFs by
 * quantization values is a significant chunk of the runtime).
 * Note: the values in a QUANT_TBL are always given in zigzag order.
 */
#ifdef EIGHT_BIT_SAMPLES
typedef INT16 QUANT_VAL;	/* element of a quantization table */
#else
typedef UINT16 QUANT_VAL;	/* element of a quantization table */
#endif
typedef QUANT_VAL QUANT_TBL[DCTSIZE2];	/* A quantization table */
typedef QUANT_VAL * QUANT_TBL_PTR;	/* pointer to same */


/* Huffman coding tables.
 */

#define HUFF_LOOKAHEAD	8	/* # of bits of lookahead */

typedef struct {
  /* These two fields directly represent the contents of a JPEG DHT marker */
	UINT8 bits[17];		/* bits[k] = # of symbols with codes of */
				/* length k bits; bits[0] is unused */
	UINT8 huffval[256];	/* The symbols, in order of incr code length */
  /* This field is used only during compression.  It's initialized FALSE when
   * the table is created, and set TRUE when it's been output to the file.
   */
	boolean sent_table;	/* TRUE when table has been output */
  /* The remaining fields are computed from the above to allow more efficient
   * coding and decoding.  These fields should be considered private to the
   * Huffman compression & decompression modules.  We use a union since only
   * one set of fields is needed at a time.
   */
	union {
	  struct {		/* encoding tables: */
	    UINT16 ehufco[256];	/* code for each symbol */
	    char ehufsi[256];	/* length of code for each symbol */
	  } enc;
	  struct {		/* decoding tables: */
	    /* Basic tables: (element [0] of each array is unused) */
	    INT32 mincode[17];	/* smallest code of length k */
	    INT32 maxcode[18];	/* largest code of length k (-1 if none) */
	    /* (maxcode[17] is a sentinel to ensure huff_DECODE terminates) */
	    int valptr[17];	/* huffval[] index of 1st symbol of length k */
	    /* Lookahead tables: indexed by the next HUFF_LOOKAHEAD bits of
	     * the input data stream.  If the next Huffman code is no more
	     * than HUFF_LOOKAHEAD bits long, we can obtain its length and
	     * the corresponding symbol directly from these tables.
	     */
	    int look_nbits[1<<HUFF_LOOKAHEAD]; /* # bits, or 0 if too long */
	    UINT8 look_sym[1<<HUFF_LOOKAHEAD]; /* symbol, or unused */
	  } dec;
	} priv;
} HUFF_TBL;


#define NUM_QUANT_TBLS      4	/* quantization tables are numbered 0..3 */
#define NUM_HUFF_TBLS       4	/* Huffman tables are numbered 0..3 */
#define NUM_ARITH_TBLS      16	/* arith-coding tables are numbered 0..15 */
#define MAX_COMPS_IN_SCAN   4	/* JPEG limit on # of components in one scan */
#define MAX_SAMP_FACTOR     4	/* JPEG limit on sampling factors */
#define MAX_BLOCKS_IN_MCU   10	/* JPEG limit on # of blocks in an MCU */


/* Working data for compression */

struct Compress_info_struct {
/*
 * All of these fields shall be established by the user interface before
 * calling jpeg_compress, or by the input_init or c_ui_method_selection
 * methods.
 * Most parameters can be set to reasonable defaults by j_c_defaults.
 * Note that the UI must supply the storage for the main methods struct,
 * though it sets only a few of the methods there.
 */
	compress_methods_ptr methods; /* Points to list of methods to use */

	external_methods_ptr emethods; /* Points to list of methods to use */

	IFILEREF input_file;	/* tells input routines where to read image */
	JFILEREF output_file;	/* tells output routines where to write JPEG */

	long image_width;	/* input image width */
	long image_height;	/* input image height */
	short input_components;	/* # of color components in input image */

	short data_precision;	/* bits of precision in image data */

	COLOR_SPACE in_color_space; /* colorspace of input file */
	COLOR_SPACE jpeg_color_space; /* colorspace of JPEG file */

	double input_gamma;	/* image gamma of input file */

	boolean write_JFIF_header; /* should a JFIF marker be written? */
	/* These three values are not used by the JPEG code, only copied */
	/* into the JFIF APP0 marker.  density_unit can be 0 for unknown, */
	/* 1 for dots/inch, or 2 for dots/cm.  Note that the pixel aspect */
	/* ratio is defined by X_density/Y_density even when density_unit=0. */
	UINT8 density_unit;	/* JFIF code for pixel size units */
	UINT16 X_density;	/* Horizontal pixel density */
	UINT16 Y_density;	/* Vertical pixel density */

	char * comment_text;	/* Text for COM block, or NULL for no COM */
	/* note: JPEG library will not free() the comment string, */
	/* unless you allocate it via alloc_small(). */

	short num_components;	/* # of color components in JPEG image */
	jpeg_component_info * comp_info;
	/* comp_info[i] describes component that appears i'th in SOF */

	QUANT_TBL_PTR quant_tbl_ptrs[NUM_QUANT_TBLS];
	/* ptrs to coefficient quantization tables, or NULL if not defined */

	HUFF_TBL * dc_huff_tbl_ptrs[NUM_HUFF_TBLS];
	HUFF_TBL * ac_huff_tbl_ptrs[NUM_HUFF_TBLS];
	/* ptrs to Huffman coding tables, or NULL if not defined */

	UINT8 arith_dc_L[NUM_ARITH_TBLS]; /* L values for DC arithmetic-coding tables */
	UINT8 arith_dc_U[NUM_ARITH_TBLS]; /* U values for DC arithmetic-coding tables */
	UINT8 arith_ac_K[NUM_ARITH_TBLS]; /* Kx values for AC arithmetic-coding tables */

	boolean arith_code;	/* TRUE=arithmetic coding, FALSE=Huffman */
	boolean interleave;	/* TRUE=interleaved output, FALSE=not */
	boolean optimize_coding; /* TRUE=optimize entropy encoding parms */
	boolean CCIR601_sampling; /* TRUE=first samples are cosited */
	int smoothing_factor;	/* 1..100, or 0 for no input smoothing */

	/* The restart interval can be specified in absolute MCUs by setting
	 * restart_interval, or in MCU rows by setting restart_in_rows
	 * (in which case the correct restart_interval will be figured
	 * for each scan).
	 */
	UINT16 restart_interval;/* MCUs per restart interval, or 0 for no restart */
	int restart_in_rows;	/* if > 0, MCU rows per restart interval */

/*
 * These fields are computed during jpeg_compress startup
 */
	short max_h_samp_factor; /* largest h_samp_factor */
	short max_v_samp_factor; /* largest v_samp_factor */

/*
 * These fields may be useful for progress monitoring
 */

	int total_passes;	/* number of passes expected */
	int completed_passes;	/* number of passes completed so far */

/*
 * These fields are valid during any one scan
 */
	short comps_in_scan;	/* # of JPEG components output this time */
	jpeg_component_info * cur_comp_info[MAX_COMPS_IN_SCAN];
	/* *cur_comp_info[i] describes component that appears i'th in SOS */

	long MCUs_per_row;	/* # of MCUs across the image */
	long MCU_rows_in_scan;	/* # of MCU rows in the image */

	short blocks_in_MCU;	/* # of DCT blocks per MCU */
	short MCU_membership[MAX_BLOCKS_IN_MCU];
	/* MCU_membership[i] is index in cur_comp_info of component owning */
	/* i'th block in an MCU */

	/* these fields are private data for the entropy encoder */
	JCOEF last_dc_val[MAX_COMPS_IN_SCAN]; /* last DC coef for each comp */
	JCOEF last_dc_diff[MAX_COMPS_IN_SCAN]; /* last DC diff for each comp */
	UINT16 restarts_to_go;	/* MCUs left in this restart interval */
	short next_restart_num;	/* # of next RSTn marker (0..7) */
};

typedef struct Compress_info_struct * compress_info_ptr;


/* Working data for decompression */

struct Decompress_info_struct {
/*
 * These fields shall be established by the user interface before
 * calling jpeg_decompress.
 * Most parameters can be set to reasonable defaults by j_d_defaults.
 * Note that the UI must supply the storage for the main methods struct,
 * though it sets only a few of the methods there.
 */
	decompress_methods_ptr methods; /* Points to list of methods to use */

	external_methods_ptr emethods; /* Points to list of methods to use */

	JFILEREF input_file;	/* tells input routines where to read JPEG */
	IFILEREF output_file;	/* tells output routines where to write image */

	/* these can be set at d_ui_method_selection time: */

	COLOR_SPACE out_color_space; /* colorspace of output */

	double output_gamma;	/* image gamma wanted in output */

	boolean quantize_colors; /* T if output is a colormapped format */
	/* the following are ignored if not quantize_colors: */
	boolean two_pass_quantize;	/* use two-pass color quantization? */
	boolean use_dithering;		/* want color dithering? */
	int desired_number_of_colors;	/* max number of colors to use */

	boolean do_block_smoothing; /* T = apply cross-block smoothing */
	boolean do_pixel_smoothing; /* T = apply post-upsampling smoothing */

/*
 * These fields are used for efficient buffering of data between read_jpeg_data
 * and the entropy decoding object.  By using a shared buffer, we avoid copying
 * data and eliminate the need for an "unget" operation at the end of a scan.
 * The actual source of the data is known only to read_jpeg_data; see the
 * JGETC macro, below.
 * Note: the user interface is expected to allocate the input_buffer and
 * initialize bytes_in_buffer to 0.  Also, for JFIF/raw-JPEG input, the UI
 * actually supplies the read_jpeg_data method.  This is all handled by
 * j_d_defaults in a typical implementation.
 */
	char * input_buffer;	/* start of buffer (private to input code) */
	char * next_input_byte;	/* => next byte to read from buffer */
	int bytes_in_buffer;	/* # of bytes remaining in buffer */

/*
 * These fields are set by read_file_header or read_scan_header
 */
	long image_width;	/* overall image width */
	long image_height;	/* overall image height */

	short data_precision;	/* bits of precision in image data */

	COLOR_SPACE jpeg_color_space; /* colorspace of JPEG file */

        /* These three values are not used by the JPEG code, merely copied */
	/* from the JFIF APP0 marker (if any). */
	UINT8 density_unit;	/* JFIF code for pixel size units */
	UINT16 X_density;	/* Horizontal pixel density */
	UINT16 Y_density;	/* Vertical pixel density */

	short num_components;	/* # of color components in JPEG image */
	jpeg_component_info * comp_info;
	/* comp_info[i] describes component that appears i'th in SOF */

	QUANT_TBL_PTR quant_tbl_ptrs[NUM_QUANT_TBLS];
	/* ptrs to coefficient quantization tables, or NULL if not defined */

	HUFF_TBL * dc_huff_tbl_ptrs[NUM_HUFF_TBLS];
	HUFF_TBL * ac_huff_tbl_ptrs[NUM_HUFF_TBLS];
	/* ptrs to Huffman coding tables, or NULL if not defined */

	UINT8 arith_dc_L[NUM_ARITH_TBLS]; /* L values for DC arith-coding tables */
	UINT8 arith_dc_U[NUM_ARITH_TBLS]; /* U values for DC arith-coding tables */
	UINT8 arith_ac_K[NUM_ARITH_TBLS]; /* Kx values for AC arith-coding tables */

	boolean arith_code;	/* TRUE=arithmetic coding, FALSE=Huffman */
	boolean CCIR601_sampling; /* TRUE=first samples are cosited */

	UINT16 restart_interval;/* MCUs per restart interval, or 0 for no restart */

/*
 * These fields are computed during jpeg_decompress startup
 */
	short max_h_samp_factor; /* largest h_samp_factor */
	short max_v_samp_factor; /* largest v_samp_factor */

	short color_out_comps;	/* # of color components output by color_convert */
				/* (need not match num_components) */
	short final_out_comps;	/* # of color components sent to put_pixel_rows */
	/* (1 when quantizing colors, else same as color_out_comps) */

	JSAMPLE * sample_range_limit; /* table for fast range-limiting */

/*
 * When quantizing colors, the color quantizer leaves a pointer to the output
 * colormap in these fields.  The colormap is valid from the time put_color_map
 * is called (must be before any put_pixel_rows calls) until shutdown (more
 * specifically, until free_all is called to release memory).
 */
	int actual_number_of_colors; /* actual number of entries */
	JSAMPARRAY colormap;	/* NULL if not valid */
	/* map has color_out_comps rows * actual_number_of_colors columns */

/*
 * These fields may be useful for progress monitoring
 */

	int total_passes;	/* number of passes expected */
	int completed_passes;	/* number of passes completed so far */

/*
 * These fields are valid during any one scan
 */
	short comps_in_scan;	/* # of JPEG components input this time */
	jpeg_component_info * cur_comp_info[MAX_COMPS_IN_SCAN];
	/* *cur_comp_info[i] describes component that appears i'th in SOS */

	long MCUs_per_row;	/* # of MCUs across the image */
	long MCU_rows_in_scan;	/* # of MCU rows in the image */

	short blocks_in_MCU;	/* # of DCT blocks per MCU */
	short MCU_membership[MAX_BLOCKS_IN_MCU];
	/* MCU_membership[i] is index in cur_comp_info of component owning */
	/* i'th block in an MCU */

	/* these fields are private data for the entropy encoder */
	JCOEF last_dc_val[MAX_COMPS_IN_SCAN]; /* last DC coef for each comp */
	JCOEF last_dc_diff[MAX_COMPS_IN_SCAN]; /* last DC diff for each comp */
	UINT16 restarts_to_go;	/* MCUs left in this restart interval */
	short next_restart_num;	/* # of next RSTn marker (0..7) */
};

typedef struct Decompress_info_struct * decompress_info_ptr;


/* Macros for reading data from the decompression input buffer */

#ifdef CHAR_IS_UNSIGNED
#define JGETC(cinfo)	( --(cinfo)->bytes_in_buffer < 0 ? \
			 (*(cinfo)->methods->read_jpeg_data) (cinfo) : \
			 (int) (*(cinfo)->next_input_byte++) )
#else
#define JGETC(cinfo)	( --(cinfo)->bytes_in_buffer < 0 ? \
			 (*(cinfo)->methods->read_jpeg_data) (cinfo) : \
			 (int) (*(cinfo)->next_input_byte++) & 0xFF )
#endif

#define JUNGETC(ch,cinfo)  ((cinfo)->bytes_in_buffer++, \
			    *(--((cinfo)->next_input_byte)) = (char) (ch))

#define MIN_UNGET	4	/* may always do at least 4 JUNGETCs */


/* A virtual image has a control block whose contents are private to the
 * memory manager module (and may differ between managers).  The rest of the
 * code only refers to virtual images by these pointer types, and never
 * dereferences the pointer.
 */

typedef struct big_sarray_control * big_sarray_ptr;
typedef struct big_barray_control * big_barray_ptr;

/* Although a real ANSI C compiler can deal perfectly well with pointers to
 * unspecified structures (see "incomplete types" in the spec), a few pre-ANSI
 * and pseudo-ANSI compilers get confused.  To keep one of these bozos happy,
 * add -DINCOMPLETE_TYPES_BROKEN to CFLAGS in your Makefile.  Then we will
 * pseudo-define the structs as containing a single "dummy" field.
 * The memory managers #define AM_MEMORY_MANAGER before including this file,
 * so that they can make their own definitions of the structs.
 */

#ifdef INCOMPLETE_TYPES_BROKEN
#ifndef AM_MEMORY_MANAGER
struct big_sarray_control { long dummy; };
struct big_barray_control { long dummy; };
#endif
#endif


/* Method types that need typedefs */

typedef METHOD(void, MCU_output_method_ptr, (compress_info_ptr cinfo,
					     JBLOCK *MCU_data));
typedef METHOD(void, MCU_output_caller_ptr, (compress_info_ptr cinfo,
					     MCU_output_method_ptr output_method));
typedef METHOD(void, downsample_ptr, (compress_info_ptr cinfo,
				      int which_component,
				      long input_cols, int input_rows,
				      long output_cols, int output_rows,
				      JSAMPARRAY above,
				      JSAMPARRAY input_data,
				      JSAMPARRAY below,
				      JSAMPARRAY output_data));
typedef METHOD(void, upsample_ptr, (decompress_info_ptr cinfo,
				    int which_component,
				    long input_cols, int input_rows,
				    long output_cols, int output_rows,
				    JSAMPARRAY above,
				    JSAMPARRAY input_data,
				    JSAMPARRAY below,
				    JSAMPARRAY output_data));
typedef METHOD(void, quantize_method_ptr, (decompress_info_ptr cinfo,
					   int num_rows,
					   JSAMPIMAGE input_data,
					   JSAMPARRAY output_workspace));
typedef METHOD(void, quantize_caller_ptr, (decompress_info_ptr cinfo,
					   quantize_method_ptr quantize_method));


/* These structs contain function pointers for the various JPEG methods. */

/* Routines to be provided by the surrounding application, rather than the
 * portable JPEG code proper.  These are the same for compression and
 * decompression.
 */

struct External_methods_struct {
	/* User interface: error exit and trace message routines */
	/* NOTE: the string msgtext parameters will eventually be replaced
	 * by an enumerated-type code so that non-English error messages
	 * can be substituted easily.  This will not be done until all the
	 * code is in place, so that we know what messages are needed.
	 */
	METHOD(void, error_exit, (const char *msgtext));
	METHOD(void, trace_message, (const char *msgtext));

	/* Working data for error/trace facility */
	/* See macros below for the usage of these variables */
	int trace_level;	/* level of detail of tracing messages */
	/* Use level 0 for important warning messages (nonfatal errors) */
	/* Use levels 1, 2, 3 for successively more detailed trace options */

	/* For recoverable corrupt-data errors, we emit a warning message and
	 * keep going.  A surrounding application can check for bad data by
	 * seeing if num_warnings is nonzero at the end of processing.
	 */
	long num_warnings;	/* number of corrupt-data warnings */
	int first_warning_level; /* trace level for first warning */
	int more_warning_level;	/* trace level for subsequent warnings */

	int message_parm[8];	/* store numeric parms for messages here */

	/* Memory management */
	/* NB: alloc routines never return NULL. They exit to */
	/* error_exit if not successful. */
	METHOD(void *, alloc_small, (size_t sizeofobject));
	METHOD(void, free_small, (void *ptr));
	METHOD(void FAR *, alloc_medium, (size_t sizeofobject));
	METHOD(void, free_medium, (void FAR *ptr));
	METHOD(JSAMPARRAY, alloc_small_sarray, (long samplesperrow,
						long numrows));
	METHOD(void, free_small_sarray, (JSAMPARRAY ptr));
	METHOD(JBLOCKARRAY, alloc_small_barray, (long blocksperrow,
						 long numrows));
	METHOD(void, free_small_barray, (JBLOCKARRAY ptr));
	METHOD(big_sarray_ptr, request_big_sarray, (long samplesperrow,
						    long numrows,
						    long unitheight));
	METHOD(big_barray_ptr, request_big_barray, (long blocksperrow,
						    long numrows,
						    long unitheight));
	METHOD(void, alloc_big_arrays, (long extra_small_samples,
					long extra_small_blocks,
					long extra_medium_space));
	METHOD(JSAMPARRAY, access_big_sarray, (big_sarray_ptr ptr,
					       long start_row,
					       boolean writable));
	METHOD(JBLOCKARRAY, access_big_barray, (big_barray_ptr ptr,
						long start_row,
						boolean writable));
	METHOD(void, free_big_sarray, (big_sarray_ptr ptr));
	METHOD(void, free_big_barray, (big_barray_ptr ptr));
	METHOD(void, free_all, (void));

	long max_memory_to_use;	/* maximum amount of memory to use */
};

/* Macros to simplify using the error and trace message stuff */
/* The first parameter is generally cinfo->emethods */

/* Fatal errors (print message and exit) */
#define ERREXIT(emeth,msg)		((*(emeth)->error_exit) (msg))
#define ERREXIT1(emeth,msg,p1)		((emeth)->message_parm[0] = (p1), \
					 (*(emeth)->error_exit) (msg))
#define ERREXIT2(emeth,msg,p1,p2)	((emeth)->message_parm[0] = (p1), \
					 (emeth)->message_parm[1] = (p2), \
					 (*(emeth)->error_exit) (msg))
#define ERREXIT3(emeth,msg,p1,p2,p3)	((emeth)->message_parm[0] = (p1), \
					 (emeth)->message_parm[1] = (p2), \
					 (emeth)->message_parm[2] = (p3), \
					 (*(emeth)->error_exit) (msg))
#define ERREXIT4(emeth,msg,p1,p2,p3,p4) ((emeth)->message_parm[0] = (p1), \
					 (emeth)->message_parm[1] = (p2), \
					 (emeth)->message_parm[2] = (p3), \
					 (emeth)->message_parm[3] = (p4), \
					 (*(emeth)->error_exit) (msg))

#define MAKESTMT(stuff)		do { stuff } while (0)

/* Nonfatal errors (we'll keep going, but the data is probably corrupt) */
/* Note that warning count is incremented as a side-effect! */
#define WARNMS(emeth,msg)    \
  MAKESTMT( if ((emeth)->trace_level >= ((emeth)->num_warnings++ ? \
		(emeth)->more_warning_level : (emeth)->first_warning_level)){ \
		(*(emeth)->trace_message) (msg); } )
#define WARNMS1(emeth,msg,p1)    \
  MAKESTMT( if ((emeth)->trace_level >= ((emeth)->num_warnings++ ? \
		(emeth)->more_warning_level : (emeth)->first_warning_level)){ \
		(emeth)->message_parm[0] = (p1); \
		(*(emeth)->trace_message) (msg); } )
#define WARNMS2(emeth,msg,p1,p2)    \
  MAKESTMT( if ((emeth)->trace_level >= ((emeth)->num_warnings++ ? \
		(emeth)->more_warning_level : (emeth)->first_warning_level)){ \
		(emeth)->message_parm[0] = (p1); \
		(emeth)->message_parm[1] = (p2); \
		(*(emeth)->trace_message) (msg); } )

/* Informational/debugging messages */
#define TRACEMS(emeth,lvl,msg)    \
  MAKESTMT( if ((emeth)->trace_level >= (lvl)) { \
		(*(emeth)->trace_message) (msg); } )
#define TRACEMS1(emeth,lvl,msg,p1)    \
  MAKESTMT( if ((emeth)->trace_level >= (lvl)) { \
		(emeth)->message_parm[0] = (p1); \
		(*(emeth)->trace_message) (msg); } )
#define TRACEMS2(emeth,lvl,msg,p1,p2)    \
  MAKESTMT( if ((emeth)->trace_level >= (lvl)) { \
		(emeth)->message_parm[0] = (p1); \
		(emeth)->message_parm[1] = (p2); \
		(*(emeth)->trace_message) (msg); } )
#define TRACEMS3(emeth,lvl,msg,p1,p2,p3)    \
  MAKESTMT( if ((emeth)->trace_level >= (lvl)) { \
		int * _mp = (emeth)->message_parm; \
		*_mp++ = (p1); *_mp++ = (p2); *_mp = (p3); \
		(*(emeth)->trace_message) (msg); } )
#define TRACEMS4(emeth,lvl,msg,p1,p2,p3,p4)    \
  MAKESTMT( if ((emeth)->trace_level >= (lvl)) { \
		int * _mp = (emeth)->message_parm; \
		*_mp++ = (p1); *_mp++ = (p2); *_mp++ = (p3); *_mp = (p4); \
		(*(emeth)->trace_message) (msg); } )
#define TRACEMS8(emeth,lvl,msg,p1,p2,p3,p4,p5,p6,p7,p8)    \
  MAKESTMT( if ((emeth)->trace_level >= (lvl)) { \
		int * _mp = (emeth)->message_parm; \
		*_mp++ = (p1); *_mp++ = (p2); *_mp++ = (p3); *_mp++ = (p4); \
		*_mp++ = (p5); *_mp++ = (p6); *_mp++ = (p7); *_mp = (p8); \
		(*(emeth)->trace_message) (msg); } )


/* Methods used during JPEG compression. */

struct Compress_methods_struct {
	/* Hook for user interface to get control after input_init */
	METHOD(void, c_ui_method_selection, (compress_info_ptr cinfo));
	/* Hook for user interface to do progress monitoring */
	METHOD(void, progress_monitor, (compress_info_ptr cinfo,
					long loopcounter, long looplimit));
	/* Input image reading & conversion to standard form */
	METHOD(void, input_init, (compress_info_ptr cinfo));
	METHOD(void, get_input_row, (compress_info_ptr cinfo,
				     JSAMPARRAY pixel_row));
	METHOD(void, input_term, (compress_info_ptr cinfo));
	/* Color space and gamma conversion */
	METHOD(void, colorin_init, (compress_info_ptr cinfo));
	METHOD(void, get_sample_rows, (compress_info_ptr cinfo,
				       int rows_to_read,
				       JSAMPIMAGE image_data));
	METHOD(void, colorin_term, (compress_info_ptr cinfo));
	/* Expand picture data at edges */
	METHOD(void, edge_expand, (compress_info_ptr cinfo,
				   long input_cols, int input_rows,
				   long output_cols, int output_rows,
				   JSAMPIMAGE image_data));
	/* Downsample pixel values of a single component */
	/* There can be a different downsample method for each component */
	METHOD(void, downsample_init, (compress_info_ptr cinfo));
	downsample_ptr downsample[MAX_COMPS_IN_SCAN];
	METHOD(void, downsample_term, (compress_info_ptr cinfo));
	/* Extract samples in MCU order, process & hand off to output_method */
	/* The input is always exactly N MCU rows worth of data */
	METHOD(void, extract_init, (compress_info_ptr cinfo));
	METHOD(void, extract_MCUs, (compress_info_ptr cinfo,
				    JSAMPIMAGE image_data,
				    int num_mcu_rows,
				    MCU_output_method_ptr output_method));
	METHOD(void, extract_term, (compress_info_ptr cinfo));
	/* Entropy encoding parameter optimization */
	METHOD(void, entropy_optimize, (compress_info_ptr cinfo,
					MCU_output_caller_ptr source_method));
	/* Entropy encoding */
	METHOD(void, entropy_encode_init, (compress_info_ptr cinfo));
	METHOD(void, entropy_encode, (compress_info_ptr cinfo,
				      JBLOCK *MCU_data));
	METHOD(void, entropy_encode_term, (compress_info_ptr cinfo));
	/* JPEG file header construction */
	METHOD(void, write_file_header, (compress_info_ptr cinfo));
	METHOD(void, write_scan_header, (compress_info_ptr cinfo));
	METHOD(void, write_jpeg_data, (compress_info_ptr cinfo,
				       char *dataptr,
				       int datacount));
	METHOD(void, write_scan_trailer, (compress_info_ptr cinfo));
	METHOD(void, write_file_trailer, (compress_info_ptr cinfo));
	/* Pipeline control */
	METHOD(void, c_pipeline_controller, (compress_info_ptr cinfo));
	METHOD(void, entropy_output, (compress_info_ptr cinfo,
				      char *dataptr,
				      int datacount));
	/* Overall control */
	METHOD(void, c_per_scan_method_selection, (compress_info_ptr cinfo));
};

/* Methods used during JPEG decompression. */

struct Decompress_methods_struct {
	/* Hook for user interface to get control after reading file header */
	METHOD(void, d_ui_method_selection, (decompress_info_ptr cinfo));
	/* Hook for user interface to process comment blocks */
	METHOD(void, process_comment, (decompress_info_ptr cinfo,
				       long comment_length));
	/* Hook for user interface to do progress monitoring */
	METHOD(void, progress_monitor, (decompress_info_ptr cinfo,
					long loopcounter, long looplimit));
	/* JPEG file scanning */
	METHOD(void, read_file_header, (decompress_info_ptr cinfo));
	METHOD(boolean, read_scan_header, (decompress_info_ptr cinfo));
	METHOD(int, read_jpeg_data, (decompress_info_ptr cinfo));
	METHOD(void, resync_to_restart, (decompress_info_ptr cinfo,
					 int marker));
	METHOD(void, read_scan_trailer, (decompress_info_ptr cinfo));
	METHOD(void, read_file_trailer, (decompress_info_ptr cinfo));
	/* Entropy decoding */
	METHOD(void, entropy_decode_init, (decompress_info_ptr cinfo));
	METHOD(void, entropy_decode, (decompress_info_ptr cinfo,
				      JBLOCKROW *MCU_data));
	METHOD(void, entropy_decode_term, (decompress_info_ptr cinfo));
	/* MCU disassembly: fetch MCUs from entropy_decode, build coef array */
	/* The reverse_DCT step is in the same module for symmetry reasons */
	METHOD(void, disassemble_init, (decompress_info_ptr cinfo));
	METHOD(void, disassemble_MCU, (decompress_info_ptr cinfo,
				       JBLOCKIMAGE image_data));
	METHOD(void, reverse_DCT, (decompress_info_ptr cinfo,
				   JBLOCKIMAGE coeff_data,
				   JSAMPIMAGE output_data, int start_row));
	METHOD(void, disassemble_term, (decompress_info_ptr cinfo));
	/* Cross-block smoothing */
	METHOD(void, smooth_coefficients, (decompress_info_ptr cinfo,
					   jpeg_component_info *compptr,
					   JBLOCKROW above,
					   JBLOCKROW currow,
					   JBLOCKROW below,
					   JBLOCKROW output));
	/* Upsample pixel values of a single component */
	/* There can be a different upsample method for each component */
	METHOD(void, upsample_init, (decompress_info_ptr cinfo));
	upsample_ptr upsample[MAX_COMPS_IN_SCAN];
	METHOD(void, upsample_term, (decompress_info_ptr cinfo));
	/* Color space and gamma conversion */
	METHOD(void, colorout_init, (decompress_info_ptr cinfo));
	METHOD(void, color_convert, (decompress_info_ptr cinfo,
				     int num_rows, long num_cols,
				     JSAMPIMAGE input_data,
				     JSAMPIMAGE output_data));
	METHOD(void, colorout_term, (decompress_info_ptr cinfo));
	/* Color quantization */
	METHOD(void, color_quant_init, (decompress_info_ptr cinfo));
	METHOD(void, color_quantize, (decompress_info_ptr cinfo,
				      int num_rows,
				      JSAMPIMAGE input_data,
				      JSAMPARRAY output_data));
	METHOD(void, color_quant_prescan, (decompress_info_ptr cinfo,
					   int num_rows,
					   JSAMPIMAGE image_data,
					   JSAMPARRAY workspace));
	METHOD(void, color_quant_doit, (decompress_info_ptr cinfo,
					quantize_caller_ptr source_method));
	METHOD(void, color_quant_term, (decompress_info_ptr cinfo));
	/* Output image writing */
	METHOD(void, output_init, (decompress_info_ptr cinfo));
	METHOD(void, put_color_map, (decompress_info_ptr cinfo,
				     int num_colors, JSAMPARRAY colormap));
	METHOD(void, put_pixel_rows, (decompress_info_ptr cinfo,
				      int num_rows,
				      JSAMPIMAGE pixel_data));
	METHOD(void, output_term, (decompress_info_ptr cinfo));
	/* Pipeline control */
	METHOD(void, d_pipeline_controller, (decompress_info_ptr cinfo));
	/* Overall control */
	METHOD(void, d_per_scan_method_selection, (decompress_info_ptr cinfo));
};


/* External declarations for routines that aren't called via method ptrs. */
/* Note: use "j" as first char of names to minimize namespace pollution. */
/* The PP macro hides prototype parameters from compilers that can't cope. */

#ifdef PROTO
#define PP(arglist)	arglist
#else
#define PP(arglist)	()
#endif


/* main entry for compression */
EXTERN void jpeg_compress PP((compress_info_ptr cinfo));

/* default parameter setup for compression */
EXTERN void j_c_defaults PP((compress_info_ptr cinfo, int quality,
			     boolean force_baseline));
EXTERN void j_monochrome_default PP((compress_info_ptr cinfo));
EXTERN void j_set_quality PP((compress_info_ptr cinfo, int quality,
			      boolean force_baseline));
/* advanced compression parameter setup aids */
EXTERN void j_add_quant_table PP((compress_info_ptr cinfo, int which_tbl,
				  const QUANT_VAL *basic_table,
				  int scale_factor, boolean force_baseline));
EXTERN int j_quality_scaling PP((int quality));

/* main entry for decompression */
EXTERN void jpeg_decompress PP((decompress_info_ptr cinfo));

/* default parameter setup for decompression */
EXTERN void j_d_defaults PP((decompress_info_ptr cinfo,
			     boolean standard_buffering));

/* forward DCT */
EXTERN void j_fwd_dct PP((DCTBLOCK data));
/* inverse DCT */
EXTERN void j_rev_dct PP((DCTBLOCK data));

/* utility routines in jutils.c */
EXTERN long jround_up PP((long a, long b));
EXTERN void jcopy_sample_rows PP((JSAMPARRAY input_array, int source_row,
				  JSAMPARRAY output_array, int dest_row,
				  int num_rows, long num_cols));
EXTERN void jcopy_block_row PP((JBLOCKROW input_row, JBLOCKROW output_row,
				long num_blocks));
EXTERN void jzero_far PP((void FAR * target, size_t bytestozero));

/* method selection routines for compression modules */
EXTERN void jselcpipeline PP((compress_info_ptr cinfo)); /* jcpipe.c */
EXTERN void jselchuffman PP((compress_info_ptr cinfo)); /* jchuff.c */
EXTERN void jselcarithmetic PP((compress_info_ptr cinfo)); /* jcarith.c */
EXTERN void jselexpand PP((compress_info_ptr cinfo)); /* jcexpand.c */
EXTERN void jseldownsample PP((compress_info_ptr cinfo)); /* jcsample.c */
EXTERN void jselcmcu PP((compress_info_ptr cinfo)); /* jcmcu.c */
EXTERN void jselccolor PP((compress_info_ptr cinfo));	/* jccolor.c */
/* The user interface should call one of these to select input format: */
EXTERN void jselrgif PP((compress_info_ptr cinfo)); /* jrdgif.c */
EXTERN void jselrppm PP((compress_info_ptr cinfo)); /* jrdppm.c */
EXTERN void jselrrle PP((compress_info_ptr cinfo)); /* jrdrle.c */
EXTERN void jselrtarga PP((compress_info_ptr cinfo)); /* jrdtarga.c */
/* and one of these to select output header format: */
EXTERN void jselwjfif PP((compress_info_ptr cinfo)); /* jwrjfif.c */

/* method selection routines for decompression modules */
EXTERN void jseldpipeline PP((decompress_info_ptr cinfo)); /* jdpipe.c */
EXTERN void jseldhuffman PP((decompress_info_ptr cinfo)); /* jdhuff.c */
EXTERN void jseldarithmetic PP((decompress_info_ptr cinfo)); /* jdarith.c */
EXTERN void jseldmcu PP((decompress_info_ptr cinfo)); /* jdmcu.c */
EXTERN void jselbsmooth PP((decompress_info_ptr cinfo)); /* jbsmooth.c */
EXTERN void jselupsample PP((decompress_info_ptr cinfo)); /* jdsample.c */
EXTERN void jseldcolor PP((decompress_info_ptr cinfo));	/* jdcolor.c */
EXTERN void jsel1quantize PP((decompress_info_ptr cinfo)); /* jquant1.c */
EXTERN void jsel2quantize PP((decompress_info_ptr cinfo)); /* jquant2.c */
/* The user interface should call one of these to select input format: */
EXTERN void jselrjfif PP((decompress_info_ptr cinfo)); /* jrdjfif.c */
/* and one of these to select output image format: */
EXTERN void jselwgif PP((decompress_info_ptr cinfo)); /* jwrgif.c */
EXTERN void jselwppm PP((decompress_info_ptr cinfo)); /* jwrppm.c */
EXTERN void jselwrle PP((decompress_info_ptr cinfo)); /* jwrrle.c */
EXTERN void jselwtarga PP((decompress_info_ptr cinfo)); /* jwrtarga.c */

/* method selection routines for system-dependent modules */
EXTERN void jselerror PP((external_methods_ptr emethods)); /* jerror.c */
EXTERN void jselmemmgr PP((external_methods_ptr emethods)); /* jmemmgr.c */


/* We assume that right shift corresponds to signed division by 2 with
 * rounding towards minus infinity.  This is correct for typical "arithmetic
 * shift" instructions that shift in copies of the sign bit.  But some
 * C compilers implement >> with an unsigned shift.  For these machines you
 * must define RIGHT_SHIFT_IS_UNSIGNED.
 * RIGHT_SHIFT provides a proper signed right shift of an INT32 quantity.
 * It is only applied with constant shift counts.  SHIFT_TEMPS must be
 * included in the variables of any routine using RIGHT_SHIFT.
 */

#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define SHIFT_TEMPS	INT32 shift_temp;
#define RIGHT_SHIFT(x,shft)  \
	((shift_temp = (x)) < 0 ? \
	 (shift_temp >> (shft)) | ((~((INT32) 0)) << (32-(shft))) : \
	 (shift_temp >> (shft)))
#else
#define SHIFT_TEMPS
#define RIGHT_SHIFT(x,shft)	((x) >> (shft))
#endif


/* Miscellaneous useful macros */

#undef MAX
#define MAX(a,b)	((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b)	((a) < (b) ? (a) : (b))


#define RST0	0xD0		/* RST0 marker code */

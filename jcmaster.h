/*
 * jcmaster.h
 *
 * This file was part of the Independent JPEG Group's software:
 * Copyright (C) 1991-1995, Thomas G. Lane.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains master control structure for the JPEG compressor.
 */

/* Private state */

typedef enum {
	main_pass,		/* input data, also do first output step */
	huff_opt_pass,		/* Huffman code optimization pass */
	output_pass		/* data output pass */
} c_pass_type;

typedef struct {
  struct jpeg_comp_master pub;	/* public fields */

  c_pass_type pass_type;	/* the type of the current pass */

  int pass_number;		/* # of passes completed */
  int total_passes;		/* total # of passes needed */

  int scan_number;		/* current index in scan_info[] */
} my_comp_master;

typedef my_comp_master * my_master_ptr;

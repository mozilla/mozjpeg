/*
 * example.c
 *
 * This file is not actually part of the JPEG software.  Rather, it provides
 * a skeleton that may be useful for constructing applications that use the
 * JPEG software as subroutines.  This code will NOT do anything useful as is.
 *
 * This file illustrates how to use the JPEG code as a subroutine library
 * to read or write JPEG image files.  We assume here that you are not
 * merely interested in converting the image to yet another image file format
 * (if you are, you should be adding another I/O module to cjpeg/djpeg, not
 * constructing a new application).  Instead, we show how to pass the
 * decompressed image data into or out of routines that you provide.  For
 * example, a viewer program might use the JPEG decompressor together with
 * routines that write the decompressed image directly to a display.
 *
 * We present these routines in the same coding style used in the JPEG code
 * (ANSI function definitions, etc); but you are of course free to code your
 * routines in a different style if you prefer.
 */

/*
 * Include file for declaring JPEG data structures.
 * This file also includes some system headers like <stdio.h>;
 * if you prefer, you can include "jconfig.h" and "jpegdata.h" instead.
 */

#include "jinclude.h"

/*
 * <setjmp.h> is used for the optional error recovery mechanism shown in
 * the second part of the example.
 */

#include <setjmp.h>



/******************** JPEG COMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to feed data into the JPEG compressor.
 * We present a minimal version that does not worry about refinements such
 * as error recovery (the JPEG code will just exit() if it gets an error).
 */


/*
 * To supply the image data for compression, you must define three routines
 * input_init, get_input_row, and input_term.  These routines will be called
 * from the JPEG compressor via function pointer values that you store in the
 * cinfo data structure; hence they need not be globally visible and the exact
 * names don't matter.  (In fact, the "METHODDEF" macro expands to "static" if
 * you use the unmodified JPEG include files.)
 *
 * The input file reading modules (jrdppm.c, jrdgif.c, jrdtarga.c, etc) may be
 * useful examples of what these routines should actually do, although each of
 * them is encrusted with a lot of specialized code for its own file format.
 */


METHODDEF void
input_init (compress_info_ptr cinfo)
/* Initialize for input; return image size and component data. */
{
  /* This routine must return five pieces of information about the incoming
   * image, and must do any setup needed for the get_input_row routine.
   * The image information is returned in fields of the cinfo struct.
   * (If you don't care about modularity, you could initialize these fields
   * in the main JPEG calling routine, and make this routine be a no-op.)
   * We show some example values here.
   */
  cinfo->image_width = 640;		/* width in pixels */
  cinfo->image_height = 480;		/* height in pixels */
  /* JPEG views an image as being a rectangular array of pixels, with each
   * pixel having the same number of "component" values (color channels).
   * You must specify how many components there are and the colorspace
   * interpretation of the components.  Most applications will use RGB data or
   * grayscale data.  If you want to use something else, you'll need to study
   * and perhaps modify jcdeflts.c, jccolor.c, and jdcolor.c.
   */
  cinfo->input_components = 3;		/* or 1 for grayscale */
  cinfo->in_color_space = CS_RGB;	/* or CS_GRAYSCALE for grayscale */
  cinfo->data_precision = 8;		/* bits per pixel component value */
  /* In the current JPEG software, data_precision must be set equal to
   * BITS_IN_JSAMPLE, which is 8 unless you twiddle jconfig.h.  Future
   * versions might allow you to say either 8 or 12 if compiled with
   * 12-bit JSAMPLEs, or up to 16 in lossless mode.  In any case,
   * it is up to you to scale incoming pixel values to the range
   *   0 .. (1<<data_precision)-1.
   * If your image data format is fixed at a byte per component,
   * then saying "8" is probably the best long-term solution.
   */
}


/*
 * This function is called repeatedly and must supply the next row of pixels
 * on each call.  The rows MUST be returned in top-to-bottom order if you want
 * your JPEG files to be compatible with everyone else's.  (If you cannot
 * readily read your data in that order, you'll need an intermediate array to
 * hold the image.  See jrdtarga.c or jrdrle.c for examples of handling
 * bottom-to-top source data using the JPEG code's portable mechanisms.)
 * The data is to be returned into a 2-D array of JSAMPLEs, indexed as
 *		JSAMPLE pixel_row[component][column]
 * where component runs from 0 to cinfo->input_components-1, and column runs
 * from 0 to cinfo->image_width-1 (column 0 is left edge of image).  Note that
 * this is actually an array of pointers to arrays rather than a true 2D array,
 * since C does not support variable-size multidimensional arrays.
 * JSAMPLE is typically typedef'd as "unsigned char".
 */


METHODDEF void
get_input_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* Read next row of pixels into pixel_row[][] */
{
  /* This example shows how you might read RGB data (3 components)
   * from an input file in which the data is stored 3 bytes per pixel
   * in left-to-right, top-to-bottom order.
   */
  register FILE * infile = cinfo->input_file;
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  
  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];
  for (col = 0; col < cinfo->image_width; col++) {
    *ptr0++ = (JSAMPLE) getc(infile); /* red */
    *ptr1++ = (JSAMPLE) getc(infile); /* green */
    *ptr2++ = (JSAMPLE) getc(infile); /* blue */
  }
}


METHODDEF void
input_term (compress_info_ptr cinfo)
/* Finish up at the end of the input */
{
  /* This termination routine will very often have no work to do, */
  /* but you must provide it anyway. */
  /* Note that the JPEG code will only call it during successful exit; */
  /* if you want it called during error exit, you gotta do that yourself. */
}


/*
 * That's it for the routines that deal with reading the input image data.
 * Now we have overall control and parameter selection routines.
 */


/*
 * This routine must determine what output JPEG file format is to be written,
 * and make any other compression parameter changes that are desirable.
 * This routine gets control after the input file header has been read
 * (i.e., right after input_init has been called).  You could combine its
 * functions into input_init, or even into the main control routine, but
 * if you have several different input_init routines, it's a definite win
 * to keep this separate.  You MUST supply this routine even if it's a no-op.
 */

METHODDEF void
c_ui_method_selection (compress_info_ptr cinfo)
{
  /* If the input is gray scale, generate a monochrome JPEG file. */
  if (cinfo->in_color_space == CS_GRAYSCALE)
    j_monochrome_default(cinfo);
  /* For now, always select JFIF output format. */
  jselwjfif(cinfo);
}


/*
 * OK, here is the main function that actually causes everything to happen.
 * We assume here that the target filename is supplied by the caller of this
 * routine, and that all JPEG compression parameters can be default values.
 */

GLOBAL void
write_JPEG_file (char * filename)
{
  /* These three structs contain JPEG parameters and working data.
   * They must survive for the duration of parameter setup and one
   * call to jpeg_compress; typically, making them local data in the
   * calling routine is the best strategy.
   */
  struct Compress_info_struct cinfo;
  struct Compress_methods_struct c_methods;
  struct External_methods_struct e_methods;

  /* Initialize the system-dependent method pointers. */
  cinfo.methods = &c_methods;	/* links to method structs */
  cinfo.emethods = &e_methods;
  /* Here we use the default JPEG error handler, which will just print
   * an error message on stderr and call exit().  See the second half of
   * this file for an example of more graceful error recovery.
   */
  jselerror(&e_methods);	/* select std error/trace message routines */
  /* Here we use the standard memory manager provided with the JPEG code.
   * In some cases you might want to replace the memory manager, or at
   * least the system-dependent part of it, with your own code.
   */
  jselmemmgr(&e_methods);	/* select std memory allocation routines */
  /* If the compressor requires full-image buffers (for entropy-coding
   * optimization or a noninterleaved JPEG file), it will create temporary
   * files for anything that doesn't fit within the maximum-memory setting.
   * (Note that temp files are NOT needed if you use the default parameters.)
   * You can change the default maximum-memory setting by changing
   * e_methods.max_memory_to_use after jselmemmgr returns.
   * On some systems you may also need to set up a signal handler to
   * ensure that temporary files are deleted if the program is interrupted.
   * (This is most important if you are on MS-DOS and use the jmemdos.c
   * memory manager back end; it will try to grab extended memory for
   * temp files, and that space will NOT be freed automatically.)
   * See jcmain.c or jdmain.c for an example signal handler.
   */

  /* Here, set up pointers to your own routines for input data handling
   * and post-init parameter selection.
   */
  c_methods.input_init = input_init;
  c_methods.get_input_row = get_input_row;
  c_methods.input_term = input_term;
  c_methods.c_ui_method_selection = c_ui_method_selection;

  /* Set up default JPEG parameters in the cinfo data structure. */
  j_c_defaults(&cinfo, 75, FALSE);
  /* Note: 75 is the recommended default quality level; you may instead pass
   * a user-specified quality level.  Be aware that values below 25 will cause
   * non-baseline JPEG files to be created (and a warning message to that
   * effect to be emitted on stderr).  This won't bother our decoder, but some
   * commercial JPEG implementations may choke on non-baseline JPEG files.
   * If you want to force baseline compatibility, pass TRUE instead of FALSE.
   * (If non-baseline files are fine, but you could do without that warning
   * message, set e_methods.trace_level to -1.)
   */

  /* At this point you can modify the default parameters set by j_c_defaults
   * as needed.  For a minimal implementation, you shouldn't need to change
   * anything.  See jcmain.c for some examples of what you might change.
   */

  /* Select the input and output files.
   * Note that cinfo.input_file is only used if your input reading routines
   * use it; otherwise, you can just make it NULL.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to write binary files.
   */

  cinfo.input_file = NULL;	/* if no actual input file involved */

  if ((cinfo.output_file = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    exit(1);
  }

  /* Here we go! */
  jpeg_compress(&cinfo);

  /* That's it, son.  Nothin' else to do, except close files. */
  /* Here we assume only the output file need be closed. */
  fclose(cinfo.output_file);

  /* Note: if you want to compress more than one image, we recommend you
   * repeat this whole routine.  You MUST repeat the j_c_defaults()/alter
   * parameters/jpeg_compress() sequence, as some data structures allocated
   * in j_c_defaults are freed upon exit from jpeg_compress.
   */
}



/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to read data from the JPEG decompressor.
 * It's a little more refined than the above in that we show how to do your
 * own error recovery.  If you don't care about that, you don't need these
 * next two routines.
 */


/*
 * These routines replace the default trace/error routines included with the
 * JPEG code.  The example trace_message routine shown here is actually the
 * same as the standard one, but you could modify it if you don't want messages
 * sent to stderr.  The example error_exit routine is set up to return
 * control to read_JPEG_file() rather than calling exit().  You can use the
 * same routines for both compression and decompression error recovery.
 */

/* These static variables are needed by the error routines. */
static jmp_buf setjmp_buffer;	/* for return to caller */
static external_methods_ptr emethods; /* needed for access to message_parm */


/* This routine is used for any and all trace, debug, or error printouts
 * from the JPEG code.  The parameter is a printf format string; up to 8
 * integer data values for the format string have been stored in the
 * message_parm[] field of the external_methods struct.
 */

METHODDEF void
trace_message (const char *msgtext)
{
  fprintf(stderr, msgtext,
	  emethods->message_parm[0], emethods->message_parm[1],
	  emethods->message_parm[2], emethods->message_parm[3],
	  emethods->message_parm[4], emethods->message_parm[5],
	  emethods->message_parm[6], emethods->message_parm[7]);
  fprintf(stderr, "\n");	/* there is no \n in the format string! */
}

/*
 * The error_exit() routine should not return to its caller.  The default
 * routine calls exit(), but here we assume that we want to return to
 * read_JPEG_file, which has set up a setjmp context for the purpose.
 * You should make sure that the free_all method is called, either within
 * error_exit or after the return to the outer-level routine.
 */

METHODDEF void
error_exit (const char *msgtext)
{
  trace_message(msgtext);	/* report the error message */
  (*emethods->free_all) ();	/* clean up memory allocation & temp files */
  longjmp(setjmp_buffer, 1);	/* return control to outer routine */
}



/*
 * To accept the image data from decompression, you must define four routines
 * output_init, put_color_map, put_pixel_rows, and output_term.
 *
 * You must understand the distinction between full color output mode
 * (N independent color components) and colormapped output mode (a single
 * output component representing an index into a color map).  You should use
 * colormapped mode to write to a colormapped display screen or output file.
 * Colormapped mode is also useful for reducing grayscale output to a small
 * number of gray levels: when using the 1-pass quantizer on grayscale data,
 * the colormap entries will be evenly spaced from 0 to MAX_JSAMPLE, so you
 * can regard the indexes as directly representing gray levels at reduced
 * precision.  In any other case, you should not depend on the colormap
 * entries having any particular order.
 * To get colormapped output, set cinfo->quantize_colors to TRUE and set
 * cinfo->desired_number_of_colors to the maximum number of entries in the
 * colormap.  This can be done either in your main routine or in
 * d_ui_method_selection.  For grayscale quantization, also set
 * cinfo->two_pass_quantize to FALSE to ensure the 1-pass quantizer is used
 * (presently this is the default, but it may not be so in the future).
 *
 * The output file writing modules (jwrppm.c, jwrgif.c, jwrtarga.c, etc) may be
 * useful examples of what these routines should actually do, although each of
 * them is encrusted with a lot of specialized code for its own file format.
 */


METHODDEF void
output_init (decompress_info_ptr cinfo)
/* This routine should do any setup required */
{
  /* This routine can initialize for output based on the data passed in cinfo.
   * Useful fields include:
   *	image_width, image_height	Pretty obvious, I hope.
   *	data_precision			bits per pixel value; typically 8.
   *	out_color_space			output colorspace previously requested
   *	color_out_comps			number of color components in same
   *	final_out_comps			number of components actually output
   * final_out_comps is 1 if quantize_colors is true, else it is equal to
   * color_out_comps.
   *
   * If you have requested color quantization, the colormap is NOT yet set.
   * You may wish to defer output initialization until put_color_map is called.
   */
}


/*
 * This routine is called if and only if you have set cinfo->quantize_colors
 * to TRUE.  It is given the selected colormap and can complete any required
 * initialization.  This call will occur after output_init and before any
 * calls to put_pixel_rows.  Note that the colormap pointer is also placed
 * in a cinfo field, whence it can be used by put_pixel_rows or output_term.
 * num_colors will be less than or equal to desired_number_of_colors.
 *
 * The colormap data is supplied as a 2-D array of JSAMPLEs, indexed as
 *		JSAMPLE colormap[component][indexvalue]
 * where component runs from 0 to cinfo->color_out_comps-1, and indexvalue
 * runs from 0 to num_colors-1.  Note that this is actually an array of
 * pointers to arrays rather than a true 2D array, since C does not support
 * variable-size multidimensional arrays.
 * JSAMPLE is typically typedef'd as "unsigned char".  If you want your code
 * to be as portable as the JPEG code proper, you should always access JSAMPLE
 * values with the GETJSAMPLE() macro, which will do the right thing if the
 * machine has only signed chars.
 */

METHODDEF void
put_color_map (decompress_info_ptr cinfo, int num_colors, JSAMPARRAY colormap)
/* Write the color map */
{
  /* You need not provide this routine if you always set cinfo->quantize_colors
   * FALSE; but a safer practice is to provide it and have it just print an
   * error message, like this:
   */
  fprintf(stderr, "put_color_map called: there's a bug here somewhere!\n");
}


/*
 * This function is called repeatedly, with a few more rows of pixels supplied
 * on each call.  With the current JPEG code, some multiple of 8 rows will be
 * passed on each call except the last, but it is extremely bad form to depend
 * on this.  You CAN assume num_rows > 0.
 * The data is supplied in top-to-bottom row order (the standard order within
 * a JPEG file).  If you cannot readily use the data in that order, you'll
 * need an intermediate array to hold the image.  See jwrrle.c for an example
 * of outputting data in bottom-to-top order.
 *
 * The data is supplied as a 3-D array of JSAMPLEs, indexed as
 *		JSAMPLE pixel_data[component][row][column]
 * where component runs from 0 to cinfo->final_out_comps-1, row runs from 0 to
 * num_rows-1, and column runs from 0 to cinfo->image_width-1 (column 0 is
 * left edge of image).  Note that this is actually an array of pointers to
 * pointers to arrays rather than a true 3D array, since C does not support
 * variable-size multidimensional arrays.
 * JSAMPLE is typically typedef'd as "unsigned char".  If you want your code
 * to be as portable as the JPEG code proper, you should always access JSAMPLE
 * values with the GETJSAMPLE() macro, which will do the right thing if the
 * machine has only signed chars.
 *
 * If quantize_colors is true, then there is only one component, and its values
 * are indexes into the previously supplied colormap.  Otherwise the values
 * are actual data in your selected output colorspace.
 */


METHODDEF void
put_pixel_rows (decompress_info_ptr cinfo, int num_rows, JSAMPIMAGE pixel_data)
/* Write some rows of output data */
{
  /* This example shows how you might write full-color RGB data (3 components)
   * to an output file in which the data is stored 3 bytes per pixel.
   */
  register FILE * outfile = cinfo->output_file;
  register JSAMPROW ptr0, ptr1, ptr2;
  register long col;
  register int row;
  
  for (row = 0; row < num_rows; row++) {
    ptr0 = pixel_data[0][row];
    ptr1 = pixel_data[1][row];
    ptr2 = pixel_data[2][row];
    for (col = 0; col < cinfo->image_width; col++) {
      putc(GETJSAMPLE(*ptr0), outfile);	/* red */
      ptr0++;
      putc(GETJSAMPLE(*ptr1), outfile);	/* green */
      ptr1++;
      putc(GETJSAMPLE(*ptr2), outfile);	/* blue */
      ptr2++;
    }
  }
}


METHODDEF void
output_term (decompress_info_ptr cinfo)
/* Finish up at the end of the output */
{
  /* This termination routine may not need to do anything. */
  /* Note that the JPEG code will only call it during successful exit; */
  /* if you want it called during error exit, you gotta do that yourself. */
}


/*
 * That's it for the routines that deal with writing the output image.
 * Now we have overall control and parameter selection routines.
 */


/*
 * This routine gets control after the JPEG file header has been read;
 * at this point the image size and colorspace are known.
 * The routine must determine what output routines are to be used, and make
 * any decompression parameter changes that are desirable.  For example,
 * if it is found that the JPEG file is grayscale, you might want to do
 * things differently than if it is color.  You can also delay setting
 * quantize_colors and associated options until this point. 
 *
 * j_d_defaults initializes out_color_space to CS_RGB.  If you want grayscale
 * output you should set out_color_space to CS_GRAYSCALE.  Note that you can
 * force grayscale output from a color JPEG file (though not vice versa).
 */

METHODDEF void
d_ui_method_selection (decompress_info_ptr cinfo)
{
  /* if grayscale input, force grayscale output; */
  /* else leave the output colorspace as set by main routine. */
  if (cinfo->jpeg_color_space == CS_GRAYSCALE)
    cinfo->out_color_space = CS_GRAYSCALE;

  /* select output routines */
  cinfo->methods->output_init = output_init;
  cinfo->methods->put_color_map = put_color_map;
  cinfo->methods->put_pixel_rows = put_pixel_rows;
  cinfo->methods->output_term = output_term;
}


/*
 * OK, here is the main function that actually causes everything to happen.
 * We assume here that the JPEG filename is supplied by the caller of this
 * routine, and that all decompression parameters can be default values.
 * The routine returns 1 if successful, 0 if not.
 */

GLOBAL int
read_JPEG_file (char * filename)
{
  /* These three structs contain JPEG parameters and working data.
   * They must survive for the duration of parameter setup and one
   * call to jpeg_decompress; typically, making them local data in the
   * calling routine is the best strategy.
   */
  struct Decompress_info_struct cinfo;
  struct Decompress_methods_struct dc_methods;
  struct External_methods_struct e_methods;

  /* Select the input and output files.
   * In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * Note that cinfo.output_file is only used if your output handling routines
   * use it; otherwise, you can just make it NULL.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  if ((cinfo.input_file = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "can't open %s\n", filename);
    return 0;
  }

  cinfo.output_file = NULL;	/* if no actual output file involved */

  /* Initialize the system-dependent method pointers. */
  cinfo.methods = &dc_methods;	/* links to method structs */
  cinfo.emethods = &e_methods;
  /* Here we supply our own error handler; compare to use of standard error
   * handler in the previous write_JPEG_file example.
   */
  emethods = &e_methods;	/* save struct addr for possible access */
  e_methods.error_exit = error_exit; /* supply error-exit routine */
  e_methods.trace_message = trace_message; /* supply trace-message routine */
  e_methods.trace_level = 0;	/* default = no tracing */
  e_methods.num_warnings = 0;	/* no warnings emitted yet */
  e_methods.first_warning_level = 0; /* display first corrupt-data warning */
  e_methods.more_warning_level = 3; /* but suppress additional ones */

  /* prepare setjmp context for possible exit from error_exit */
  if (setjmp(setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * Memory allocation has already been cleaned up (see free_all call in
     * error_exit), but we need to close the input file before returning.
     * You might also need to close an output file, etc.
     */
    fclose(cinfo.input_file);
    return 0;
  }

  /* Here we use the standard memory manager provided with the JPEG code.
   * In some cases you might want to replace the memory manager, or at
   * least the system-dependent part of it, with your own code.
   */
  jselmemmgr(&e_methods);	/* select std memory allocation routines */
  /* If the decompressor requires full-image buffers (for two-pass color
   * quantization or a noninterleaved JPEG file), it will create temporary
   * files for anything that doesn't fit within the maximum-memory setting.
   * You can change the default maximum-memory setting by changing
   * e_methods.max_memory_to_use after jselmemmgr returns.
   * On some systems you may also need to set up a signal handler to
   * ensure that temporary files are deleted if the program is interrupted.
   * (This is most important if you are on MS-DOS and use the jmemdos.c
   * memory manager back end; it will try to grab extended memory for
   * temp files, and that space will NOT be freed automatically.)
   * See jcmain.c or jdmain.c for an example signal handler.
   */

  /* Here, set up the pointer to your own routine for post-header-reading
   * parameter selection.  You could also initialize the pointers to the
   * output data handling routines here, if they are not dependent on the
   * image type.
   */
  dc_methods.d_ui_method_selection = d_ui_method_selection;

  /* Set up default decompression parameters. */
  j_d_defaults(&cinfo, TRUE);
  /* TRUE indicates that an input buffer should be allocated.
   * In unusual cases you may want to allocate the input buffer yourself;
   * see jddeflts.c for commentary.
   */

  /* At this point you can modify the default parameters set by j_d_defaults
   * as needed; for example, you can request color quantization or force
   * grayscale output.  See jdmain.c for examples of what you might change.
   */

  /* Set up to read a JFIF or baseline-JPEG file. */
  /* This is the only JPEG file format currently supported. */
  jselrjfif(&cinfo);

  /* Here we go! */
  jpeg_decompress(&cinfo);

  /* That's it, son.  Nothin' else to do, except close files. */
  /* Here we assume only the input file need be closed. */
  fclose(cinfo.input_file);

  /* You might want to test e_methods.num_warnings to see if bad data was
   * detected.  In this example, we just blindly forge ahead.
   */
  return 1;			/* indicate success */

  /* Note: if you want to decompress more than one image, we recommend you
   * repeat this whole routine.  You MUST repeat the j_d_defaults()/alter
   * parameters/jpeg_decompress() sequence, as some data structures allocated
   * in j_d_defaults are freed upon exit from jpeg_decompress.
   */
}

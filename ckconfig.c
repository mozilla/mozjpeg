/*
 * ckconfig.c
 *
 * Copyright (C) 1991, 1992, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 */

/*
 * This program is intended to help you determine how to configure the JPEG
 * software for installation on a particular system.  The idea is to try to
 * compile and execute this program.  If your compiler fails to compile the
 * program, make changes as indicated in the comments below.  Once you can
 * compile the program, run it, and it will tell you how to set the various
 * switches in jconfig.h and in your Makefile.
 *
 * This could all be done automatically if we could assume we were on a Unix
 * system, but we don't want to assume that, so you'll have to edit and
 * recompile this program until it works.
 *
 * As a general rule, each time you try to compile this program,
 * pay attention only to the *first* error message you get from the compiler.
 * Many C compilers will issue lots of spurious error messages once they
 * have gotten confused.  Go to the line indicated in the first error message,
 * and read the comments preceding that line to see what to change.
 *
 * Almost all of the edits you may need to make to this program consist of
 * changing a line that reads "#define SOME_SYMBOL" to "#undef SOME_SYMBOL",
 * or vice versa.  This is called defining or undefining that symbol.
 */


/* First we must see if your system has the include files we need.
 * We start out with the assumption that your system follows the ANSI
 * conventions for include files.  If you get any error in the next dozen
 * lines, undefine INCLUDES_ARE_ANSI.
 */

#define INCLUDES_ARE_ANSI	/* replace 'define' by 'undef' if error here */

#ifdef INCLUDES_ARE_ANSI	/* this will be skipped if you undef... */
#include <stdio.h>		/* If you ain't got this, you ain't got C. */
#ifdef __SASC			/* Amiga SAS C provides size_t in stddef.h. */
#include <stddef.h>		/* (They are wrong...) */
#endif
#include <string.h>		/* size_t might be here too. */
typedef size_t my_size_t;	/* The payoff: do we have size_t now? */
#include <stdlib.h>		/* Check other ANSI includes we use. */
#endif


/* If your system doesn't follow the ANSI conventions, we have to figure out
 * what it does follow.  If you didn't get an error before this line, you can
 * ignore everything down to "#define HAVE_ANSI_DEFINITIONS".
 */

#ifndef INCLUDES_ARE_ANSI	/* skip these tests if INCLUDES_ARE_ANSI */

#include <stdio.h>		/* If you ain't got this, you ain't got C. */

/* jinclude.h will try to include <sys/types.h> if you don't set
 * INCLUDES_ARE_ANSI.  We need to test whether that include file is provided.
 * If you get an error here, undefine HAVE_TYPES_H.
 */

#define HAVE_TYPES_H

#ifdef HAVE_TYPES_H
#include <sys/types.h>
#endif

/* We have to see if your string functions are defined by
 * strings.h (BSD convention) or string.h (everybody else).
 * We try the non-BSD convention first; define BSD if the compiler
 * says it can't find string.h.
 */

#undef BSD

#ifdef BSD
#include <strings.h>
#else
#include <string.h>
#endif

/* Usually size_t is defined in stdio.h, sys/types.h, and/or string.h.
 * If not, you'll get an error on the "typedef size_t my_size_t;" line below.
 * In that case, you'll have to search through your system library to
 * figure out which include file defines "size_t".  Look for a line that
 * says "typedef something-or-other size_t;" (stddef.h and stdlib.h are
 * good places to look first).  Then, change the line below that says
 * "#include <someincludefile.h>" to instead include the file
 * you found size_t in, and define NEED_SPECIAL_INCLUDE.
 */

#undef NEED_SPECIAL_INCLUDE	/* assume we DON'T need it, for starters */

#ifdef NEED_SPECIAL_INCLUDE
#include <someincludefile.h>
#endif

typedef size_t my_size_t;	/* The payoff: do we have size_t now? */


#endif /* INCLUDES_ARE_ANSI */



/* The next question is whether your compiler supports ANSI-style function
 * definitions.  You need to know this in order to choose between using
 * makefile.ansi and using makefile.unix.
 * The #define line below is set to assume you have ANSI function definitions.
 * If you get an error in this group of lines, undefine HAVE_ANSI_DEFINITIONS.
 */

#define HAVE_ANSI_DEFINITIONS

#ifdef HAVE_ANSI_DEFINITIONS
int testfunction (int arg1, int * arg2); /* check prototypes */

struct methods_struct {		/* check method-pointer declarations */
  int (*error_exit) (char *msgtext);
  int (*trace_message) (char *msgtext);
  int (*another_method) (void);
};

int testfunction (int arg1, int * arg2) /* check definitions */
{
  return arg2[arg1];
}

int testfunction1 (void)	/* check void arg list */
{
  return 0;
}
#endif


/* Now we want to find out if your compiler knows what "unsigned char" means.
 * If you get an error on the "unsigned char un_char;" line,
 * then undefine HAVE_UNSIGNED_CHAR.
 */

#define HAVE_UNSIGNED_CHAR

#ifdef HAVE_UNSIGNED_CHAR
unsigned char un_char;
#endif


/* Now we want to find out if your compiler knows what "unsigned short" means.
 * If you get an error on the "unsigned short un_short;" line,
 * then undefine HAVE_UNSIGNED_SHORT.
 */

#define HAVE_UNSIGNED_SHORT

#ifdef HAVE_UNSIGNED_SHORT
unsigned short un_short;
#endif


/* Now we want to find out if your compiler understands type "void".
 * If you get an error anywhere in here, undefine HAVE_VOID.
 */

#define HAVE_VOID

#ifdef HAVE_VOID
typedef void * void_ptr;	/* check void * */
typedef void (*void_func) ();	/* check ptr to function returning void */

void testfunction2 (arg1, arg2)	/* check void function result */
     void_ptr arg1;
     void_func arg2;
{
  char * locptr = (char *) arg1; /* check casting to and from void * */
  arg1 = (void *) locptr;
  (*arg2) (1, 2);		/* check call of fcn returning void */
}
#endif


/* Now we want to find out if your compiler knows what "const" means.
 * If you get an error here, undefine HAVE_CONST.
 */

#define HAVE_CONST

#ifdef HAVE_CONST
static const int carray[3] = {1, 2, 3};

int testfunction3 (arg1)
     const int arg1;
{
  return carray[arg1];
}
#endif



/************************************************************************
 *  OK, that's it.  You should not have to change anything beyond this
 *  point in order to compile and execute this program.  (You might get
 *  some warnings, but you can ignore them.)
 *  When you run the program, it will make a couple more tests that it
 *  can do automatically, and then it will print out a summary of the changes
 *  that you need to make to the makefile and jconfig.h.
 ************************************************************************
 */


static int any_changes = 0;

int new_change ()
{
  if (! any_changes) {
    printf("\nMost of the changes recommended by this program can be made either\n");
    printf("by editing jconfig.h, or by adding -Dsymbol switches to the CFLAGS\n");
    printf("line in your Makefile.  (Some PC compilers expect /Dsymbol instead.)\n");
    printf("The CFLAGS method is simpler, but if your compiler doesn't support -D,\n");
    printf("then you must change jconfig.h.  Also, it's best to change jconfig.h\n");
    printf("if you plan to use the JPEG software as a library for other programs.\n");
    any_changes = 1;
  }
  printf("\n");			/* blank line before each problem report */
  return 0;
}


int test_char_sign (arg)
     int arg;
{
  if (arg == 189) {		/* expected result for unsigned char */
    new_change();
    printf("You should add -DCHAR_IS_UNSIGNED to CFLAGS,\n");
    printf("or else remove the /* */ comment marks from the line\n");
    printf("/* #define CHAR_IS_UNSIGNED */  in jconfig.h.\n");
    printf("(Be sure to delete the space before the # character too.)\n");
  }
  else if (arg != -67) {	/* expected result for signed char */
    new_change();
    printf("Hmm, it seems 'char' is less than eight bits wide on your machine.\n");
    printf("I fear the JPEG software will not work at all.\n");
  }
  return 0;
}


int test_shifting (arg)
     long arg;
/* See whether right-shift on a long is signed or not. */
{
  long res = arg >> 4;

  if (res == 0x80817F4L) {	/* expected result for unsigned */
    new_change();
    printf("You must add -DRIGHT_SHIFT_IS_UNSIGNED to CFLAGS,\n");
    printf("or else remove the /* */ comment marks from the line\n");
    printf("/* #define RIGHT_SHIFT_IS_UNSIGNED */  in jconfig.h.\n");
  }
  else if (res != -0x7F7E80CL) { /* expected result for signed */
    new_change();
    printf("Right shift isn't acting as I expect it to.\n");
    printf("I fear the JPEG software will not work at all.\n");
  }
  return 0;
}


int main (argc, argv)
     int argc;
     char ** argv;
{
  char signed_char_check = (char) (-67);

  printf("Results of configuration check for Independent JPEG Group's software:\n");
  printf("\nIf there's not a specific makefile provided for your compiler,\n");
#ifdef HAVE_ANSI_DEFINITIONS
  printf("you should use makefile.ansi as the starting point for your Makefile.\n");
#else
  printf("you should use makefile.unix as the starting point for your Makefile.\n");
#endif

  /* Check whether we have all the ANSI features, */
  /* and whether this agrees with __STDC__ being predefined. */
#ifdef __STDC__
#define HAVE_STDC	/* ANSI compilers won't allow redefining __STDC__ */
#endif

#ifdef HAVE_ANSI_DEFINITIONS
#ifdef HAVE_UNSIGNED_CHAR
#ifdef HAVE_UNSIGNED_SHORT
#ifdef HAVE_CONST
#define HAVE_ALL_ANSI_FEATURES
#endif
#endif
#endif
#endif

#ifdef HAVE_ALL_ANSI_FEATURES
#ifndef HAVE_STDC
  new_change();
  printf("Your compiler doesn't claim to be ANSI-compliant, but it is close enough\n");
  printf("for me.  Either add -DHAVE_STDC to CFLAGS, or add #define HAVE_STDC at the\n");
  printf("beginning of jconfig.h.\n");
#define HAVE_STDC
#endif
#else /* !HAVE_ALL_ANSI_FEATURES */
#ifdef HAVE_STDC
  new_change();
  printf("Your compiler claims to be ANSI-compliant, but it is lying!\n");
  printf("Delete the line  #define HAVE_STDC  near the beginning of jconfig.h.\n");
#undef HAVE_STDC
#endif
#endif /* HAVE_ALL_ANSI_FEATURES */

#ifndef HAVE_STDC

#ifdef HAVE_ANSI_DEFINITIONS
  new_change();
  printf("You should add -DPROTO to CFLAGS, or else take out the several\n");
  printf("#ifdef/#else/#endif lines surrounding #define PROTO in jconfig.h.\n");
  printf("(Leave only one #define PROTO line.)\n");
#endif

#ifdef HAVE_UNSIGNED_CHAR
#ifdef HAVE_UNSIGNED_SHORT
  new_change();
  printf("You should add -DHAVE_UNSIGNED_CHAR and -DHAVE_UNSIGNED_SHORT\n");
  printf("to CFLAGS, or else take out the #ifdef HAVE_STDC/#endif lines\n");
  printf("surrounding #define HAVE_UNSIGNED_CHAR and #define HAVE_UNSIGNED_SHORT\n");
  printf("in jconfig.h.\n");
#else /* only unsigned char */
  new_change();
  printf("You should add -DHAVE_UNSIGNED_CHAR to CFLAGS,\n");
  printf("or else move #define HAVE_UNSIGNED_CHAR outside the\n");
  printf("#ifdef HAVE_STDC/#endif lines surrounding it in jconfig.h.\n");
#endif
#else /* !HAVE_UNSIGNED_CHAR */
#ifdef HAVE_UNSIGNED_SHORT
  new_change();
  printf("You should add -DHAVE_UNSIGNED_SHORT to CFLAGS,\n");
  printf("or else move #define HAVE_UNSIGNED_SHORT outside the\n");
  printf("#ifdef HAVE_STDC/#endif lines surrounding it in jconfig.h.\n");
#endif
#endif /* HAVE_UNSIGNED_CHAR */

#ifdef HAVE_CONST
  new_change();
  printf("You should delete the  #define const  line from jconfig.h.\n");
#endif

#endif /* HAVE_STDC */

  test_char_sign((int) signed_char_check);

  test_shifting(-0x7F7E80B1L);

#ifndef HAVE_VOID
  new_change();
  printf("You should add -Dvoid=char to CFLAGS,\n");
  printf("or else remove the /* */ comment marks from the line\n");
  printf("/* #define void char */  in jconfig.h.\n");
  printf("(Be sure to delete the space before the # character too.)\n");
#endif

#ifdef INCLUDES_ARE_ANSI
#ifndef __STDC__
  new_change();
  printf("You should add -DINCLUDES_ARE_ANSI to CFLAGS, or else add\n");
  printf("#define INCLUDES_ARE_ANSI at the beginning of jinclude.h (NOT jconfig.h).\n");
#endif
#else /* !INCLUDES_ARE_ANSI */
#ifdef __STDC__
  new_change();
  printf("You should add -DNONANSI_INCLUDES to CFLAGS, or else add\n");
  printf("#define NONANSI_INCLUDES at the beginning of jinclude.h (NOT jconfig.h).\n");
#endif
#ifdef NEED_SPECIAL_INCLUDE
  new_change();
  printf("In jinclude.h, change the line reading #include <sys/types.h>\n");
  printf("to instead include the file you found size_t in.\n");
#else /* !NEED_SPECIAL_INCLUDE */
#ifndef HAVE_TYPES_H
  new_change();
  printf("In jinclude.h, delete the line reading #include <sys/types.h>.\n");
#endif
#endif /* NEED_SPECIAL_INCLUDE */
#ifdef BSD
  new_change();
  printf("You should add -DBSD to CFLAGS, or else add\n");
  printf("#define BSD at the beginning of jinclude.h (NOT jconfig.h).\n");
#endif
#endif /* INCLUDES_ARE_ANSI */

  if (any_changes) {
    printf("\nI think that's everything...\n");
  } else {
    printf("\nI think jconfig.h is OK as distributed.\n");
  }

  return any_changes;
}

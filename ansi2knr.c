/* Copyright (C) 1989, 1991, 1993 Aladdin Enterprises. All rights reserved. */

/* ansi2knr.c */
/* Convert ANSI function declarations to K&R syntax */

/*
ansi2knr is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY.  No author or distributor accepts responsibility
to anyone for the consequences of using it or for whether it serves any
particular purpose or works at all, unless he says so in writing.  Refer
to the GNU General Public License for full details.

Everyone is granted permission to copy, modify and redistribute
ansi2knr, but only under the conditions described in the GNU
General Public License.  A copy of this license is supposed to have been
given to you along with ansi2knr so you can know your rights and
responsibilities.  It should be in a file named COPYING.  Among other
things, the copyright notice and this notice must be preserved on all
copies.
*/

/*
---------- Here is the GNU GPL file COPYING, referred to above ----------
----- These terms do NOT apply to the JPEG software itself; see README ------

		    GHOSTSCRIPT GENERAL PUBLIC LICENSE
		    (Clarified 11 Feb 1988)

 Copyright (C) 1988 Richard M. Stallman
 Everyone is permitted to copy and distribute verbatim copies of this
 license, but changing it is not allowed.  You can also use this wording
 to make the terms for other programs.

  The license agreements of most software companies keep you at the
mercy of those companies.  By contrast, our general public license is
intended to give everyone the right to share Ghostscript.  To make sure
that you get the rights we want you to have, we need to make
restrictions that forbid anyone to deny you these rights or to ask you
to surrender the rights.  Hence this license agreement.

  Specifically, we want to make sure that you have the right to give
away copies of Ghostscript, that you receive source code or else can get
it if you want it, that you can change Ghostscript or use pieces of it
in new free programs, and that you know you can do these things.

  To make sure that everyone has such rights, we have to forbid you to
deprive anyone else of these rights.  For example, if you distribute
copies of Ghostscript, you must give the recipients all the rights that
you have.  You must make sure that they, too, receive or can get the
source code.  And you must tell them their rights.

  Also, for our own protection, we must make certain that everyone finds
out that there is no warranty for Ghostscript.  If Ghostscript is
modified by someone else and passed on, we want its recipients to know
that what they have is not what we distributed, so that any problems
introduced by others will not reflect on our reputation.

  Therefore we (Richard M. Stallman and the Free Software Foundation,
Inc.) make the following terms which say what you must do to be allowed
to distribute or change Ghostscript.


			COPYING POLICIES

  1. You may copy and distribute verbatim copies of Ghostscript source
code as you receive it, in any medium, provided that you conspicuously
and appropriately publish on each copy a valid copyright and license
notice "Copyright (C) 1989 Aladdin Enterprises.  All rights reserved.
Distributed by Free Software Foundation, Inc." (or with whatever year is
appropriate); keep intact the notices on all files that refer to this
License Agreement and to the absence of any warranty; and give any other
recipients of the Ghostscript program a copy of this License Agreement
along with the program.  You may charge a distribution fee for the
physical act of transferring a copy.

  2. You may modify your copy or copies of Ghostscript or any portion of
it, and copy and distribute such modifications under the terms of
Paragraph 1 above, provided that you also do the following:

    a) cause the modified files to carry prominent notices stating
    that you changed the files and the date of any change; and

    b) cause the whole of any work that you distribute or publish,
    that in whole or in part contains or is a derivative of Ghostscript
    or any part thereof, to be licensed at no charge to all third
    parties on terms identical to those contained in this License
    Agreement (except that you may choose to grant more extensive
    warranty protection to some or all third parties, at your option).

    c) You may charge a distribution fee for the physical act of
    transferring a copy, and you may at your option offer warranty
    protection in exchange for a fee.

Mere aggregation of another unrelated program with this program (or its
derivative) on a volume of a storage or distribution medium does not bring
the other program under the scope of these terms.

  3. You may copy and distribute Ghostscript (or a portion or derivative
of it, under Paragraph 2) in object code or executable form under the
terms of Paragraphs 1 and 2 above provided that you also do one of the
following:

    a) accompany it with the complete corresponding machine-readable
    source code, which must be distributed under the terms of
    Paragraphs 1 and 2 above; or,

    b) accompany it with a written offer, valid for at least three
    years, to give any third party free (except for a nominal
    shipping charge) a complete machine-readable copy of the
    corresponding source code, to be distributed under the terms of
    Paragraphs 1 and 2 above; or,

    c) accompany it with the information you received as to where the
    corresponding source code may be obtained.  (This alternative is
    allowed only for noncommercial distribution and only if you
    received the program in object code or executable form alone.)

For an executable file, complete source code means all the source code for
all modules it contains; but, as a special exception, it need not include
source code for modules which are standard libraries that accompany the
operating system on which the executable file runs.

  4. You may not copy, sublicense, distribute or transfer Ghostscript
except as expressly provided under this License Agreement.  Any attempt
otherwise to copy, sublicense, distribute or transfer Ghostscript is
void and your rights to use the program under this License agreement
shall be automatically terminated.  However, parties who have received
computer software programs from you with this License Agreement will not
have their licenses terminated so long as such parties remain in full
compliance.

  5. If you wish to incorporate parts of Ghostscript into other free
programs whose distribution conditions are different, write to the Free
Software Foundation at 675 Mass Ave, Cambridge, MA 02139.  We have not
yet worked out a simple rule that can be stated here, but we will often
permit this.  We will be guided by the two goals of preserving the free
status of all derivatives of our free software and of promoting the
sharing and reuse of software.

Your comments and suggestions about our licensing policies and our
software are welcome!  Please contact the Free Software Foundation,
Inc., 675 Mass Ave, Cambridge, MA 02139, or call (617) 876-3296.

		       NO WARRANTY

  BECAUSE GHOSTSCRIPT IS LICENSED FREE OF CHARGE, WE PROVIDE ABSOLUTELY
NO WARRANTY, TO THE EXTENT PERMITTED BY APPLICABLE STATE LAW.  EXCEPT
WHEN OTHERWISE STATED IN WRITING, FREE SOFTWARE FOUNDATION, INC, RICHARD
M. STALLMAN, ALADDIN ENTERPRISES, L. PETER DEUTSCH, AND/OR OTHER PARTIES
PROVIDE GHOSTSCRIPT "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER
EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE
ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF GHOSTSCRIPT IS WITH
YOU.  SHOULD GHOSTSCRIPT PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL
NECESSARY SERVICING, REPAIR OR CORRECTION.

  IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW WILL RICHARD M.
STALLMAN, THE FREE SOFTWARE FOUNDATION, INC., L. PETER DEUTSCH, ALADDIN
ENTERPRISES, AND/OR ANY OTHER PARTY WHO MAY MODIFY AND REDISTRIBUTE
GHOSTSCRIPT AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING
ANY LOST PROFITS, LOST MONIES, OR OTHER SPECIAL, INCIDENTAL OR
CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE
(INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
INACCURATE OR LOSSES SUSTAINED BY THIRD PARTIES OR A FAILURE OF THE
PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS) GHOSTSCRIPT, EVEN IF YOU
HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES, OR FOR ANY CLAIM
BY ANY OTHER PARTY.

-------------------- End of file COPYING ------------------------------
*/


#include <stdio.h>
#include <ctype.h>

#ifdef BSD
#include <strings.h>
#else
#ifdef VMS
	extern int strlen(), strncmp();
#else
#include <string.h>
#endif
#endif

/* malloc and free should be declared in stdlib.h, */
/* but if you've got a K&R compiler, they probably aren't. */
#ifdef MSDOS
#include <malloc.h>
#else
#ifdef VMS
     extern char *malloc();
     extern void free();
#else
     extern char *malloc();
     extern int free();
#endif
#endif

/* Usage:
	ansi2knr input_file [output_file]
 * If no output_file is supplied, output goes to stdout.
 * There are no error messages.
 *
 * ansi2knr recognizes functions by seeing a non-keyword identifier
 * at the left margin, followed by a left parenthesis,
 * with a right parenthesis as the last character on the line.
 * It will recognize a multi-line header provided that the last character
 * of the last line of the header is a right parenthesis,
 * and no intervening line ends with a left brace or a semicolon.
 * These algorithms ignore whitespace and comments, except that
 * the function name must be the first thing on the line.
 * The following constructs will confuse it:
 *	- Any other construct that starts at the left margin and
 *	    follows the above syntax (such as a macro or function call).
 *	- Macros that tinker with the syntax of the function header.
 */

/* Scanning macros */
#define isidchar(ch) (isalnum(ch) || (ch) == '_')
#define isidfirstchar(ch) (isalpha(ch) || (ch) == '_')

/* Forward references */
char *skipspace();
int writeblanks();
int test1();
int convert1();

/* The main program */
main(argc, argv)
    int argc;
    char *argv[];
{	FILE *in, *out;
#define bufsize 5000			/* arbitrary size */
	char *buf;
	char *line;
	switch ( argc )
	   {
	default:
		printf("Usage: ansi2knr input_file [output_file]\n");
		exit(0);
	case 2:
		out = stdout; break;
	case 3:
		out = fopen(argv[2], "w");
		if ( out == NULL )
		   {	fprintf(stderr, "Cannot open %s\n", argv[2]);
			exit(1);
		   }
	   }
	in = fopen(argv[1], "r");
	if ( in == NULL )
	   {	fprintf(stderr, "Cannot open %s\n", argv[1]);
		exit(1);
	   }
	fprintf(out, "#line 1 \"%s\"\n", argv[1]);
	buf = malloc(bufsize);
	line = buf;
	while ( fgets(line, (unsigned)(buf + bufsize - line), in) != NULL )
	   {	switch ( test1(buf) )
		   {
		case 1:			/* a function */
			convert1(buf, out);
			break;
		case -1:		/* maybe the start of a function */
			line = buf + strlen(buf);
			if ( line != buf + (bufsize - 1) ) /* overflow check */
				continue;
			/* falls through */
		default:		/* not a function */
			fputs(buf, out);
			break;
		   }
		line = buf;
	   }
	if ( line != buf ) fputs(buf, out);
	free(buf);
	fclose(out);
	fclose(in);
	return 0;
}

/* Skip over space and comments, in either direction. */
char *
skipspace(p, dir)
    register char *p;
    register int dir;			/* 1 for forward, -1 for backward */
{	for ( ; ; )
	   {	while ( isspace(*p) ) p += dir;
		if ( !(*p == '/' && p[dir] == '*') ) break;
		p += dir;  p += dir;
		while ( !(*p == '*' && p[dir] == '/') )
		   {	if ( *p == 0 ) return p;	/* multi-line comment?? */
			p += dir;
		   }
		p += dir;  p += dir;
	   }
	return p;
}

/*
 * Write blanks over part of a string.
 */
int
writeblanks(start, end)
    char *start;
    char *end;
{	char *p;
	for ( p = start; p < end; p++ ) *p = ' ';
	return 0;
}

/*
 * Test whether the string in buf is a function definition.
 * The string may contain and/or end with a newline.
 * Return as follows:
 *	0 - definitely not a function definition;
 *	1 - definitely a function definition;
 *	-1 - may be the beginning of a function definition,
 *		append another line and look again.
 */
int
test1(buf)
    char *buf;
{	register char *p = buf;
	char *bend;
	char *endfn;
	int contin;
	if ( !isidfirstchar(*p) )
		return 0;		/* no name at left margin */
	bend = skipspace(buf + strlen(buf) - 1, -1);
	switch ( *bend )
	   {
	case ')': contin = 1; break;
	case '{':
	case ';': return 0;		/* not a function */
	default: contin = -1;
	   }
	while ( isidchar(*p) ) p++;
	endfn = p;
	p = skipspace(p, 1);
	if ( *p++ != '(' )
		return 0;		/* not a function */
	p = skipspace(p, 1);
	if ( *p == ')' )
		return 0;		/* no parameters */
	/* Check that the apparent function name isn't a keyword. */
	/* We only need to check for keywords that could be followed */
	/* by a left parenthesis (which, unfortunately, is most of them). */
	   {	static char *words[] =
		   {	"asm", "auto", "case", "char", "const", "double",
			"extern", "float", "for", "if", "int", "long",
			"register", "return", "short", "signed", "sizeof",
			"static", "switch", "typedef", "unsigned",
			"void", "volatile", "while", 0
		   };
		char **key = words;
		char *kp;
		int len = endfn - buf;
		while ( (kp = *key) != 0 )
		   {	if ( strlen(kp) == len && !strncmp(kp, buf, len) )
				return 0;	/* name is a keyword */
			key++;
		   }
	   }
	return contin;
}

int
convert1(buf, out)
    char *buf;
    FILE *out;
{	char *endfn;
	register char *p;
	char **breaks;
	unsigned num_breaks = 2;	/* for testing */
	char **btop;
	char **bp;
	char **ap;
	/* Pre-ANSI implementations don't agree on whether strchr */
	/* is called strchr or index, so we open-code it here. */
	for ( endfn = buf; *(endfn++) != '('; ) ;
top:	p = endfn;
	breaks = (char **)malloc(sizeof(char *) * num_breaks * 2);
	if ( breaks == 0 )
	   {	/* Couldn't allocate break table, give up */
		fprintf(stderr, "Unable to allocate break table!\n");
		fputs(buf, out);
		return -1;
	   }
	btop = breaks + num_breaks * 2 - 2;
	bp = breaks;
	/* Parse the argument list */
	do
	   {	int level = 0;
		char *end = NULL;
		if ( bp >= btop )
		   {	/* Filled up break table. */
			/* Allocate a bigger one and start over. */
			free((char *)breaks);
			num_breaks <<= 1;
			goto top;
		   }
		*bp++ = p;
		/* Find the end of the argument */
		for ( ; end == NULL; p++ )
		   {	switch(*p)
			   {
			case ',': if ( !level ) end = p; break;
			case '(': level++; break;
			case ')': if ( --level < 0 ) end = p; break;
			case '/': p = skipspace(p, 1) - 1; break;
			default: ;
			   }
		   }
		p--;			/* back up over terminator */
		/* Find the name being declared. */
		/* This is complicated because of procedure and */
		/* array modifiers. */
		for ( ; ; )
		   {	p = skipspace(p - 1, -1);
			switch ( *p )
			   {
			case ']':	/* skip array dimension(s) */
			case ')':	/* skip procedure args OR name */
			   {	int level = 1;
				while ( level )
				 switch ( *--p )
				   {
				case ']': case ')': level++; break;
				case '[': case '(': level--; break;
				case '/': p = skipspace(p, -1) + 1; break;
				default: ;
				   }
			   }
				if ( *p == '(' && *skipspace(p + 1, 1) == '*' )
				   {	/* We found the name being declared */
					while ( !isidfirstchar(*p) )
						p = skipspace(p, 1) + 1;
					goto found;
				   }
				break;
			default: goto found;
			   }
		   }
found:		if ( *p == '.' && p[-1] == '.' && p[-2] == '.' )
		   {	p++;
			if ( bp == breaks + 1 )	/* sole argument */
				writeblanks(breaks[0], p);
			else
				writeblanks(bp[-1] - 1, p);
			bp--;
		   }
		else
		   {	while ( isidchar(*p) ) p--;
			*bp++ = p+1;
		   }
		p = end;
	   }
	while ( *p++ == ',' );
	*bp = p;
	/* Make a special check for 'void' arglist */
	if ( bp == breaks+2 )
	   {	p = skipspace(breaks[0], 1);
		if ( !strncmp(p, "void", 4) )
		   {	p = skipspace(p+4, 1);
			if ( p == breaks[2] - 1 )
			   {	bp = breaks;	/* yup, pretend arglist is empty */
				writeblanks(breaks[0], p + 1);
			   }
		   }
	   }
	/* Put out the function name */
	p = buf;
	while ( p != endfn ) putc(*p, out), p++;
	/* Put out the declaration */
	for ( ap = breaks+1; ap < bp; ap += 2 )
	   {	p = *ap;
		while ( isidchar(*p) ) putc(*p, out), p++;
		if ( ap < bp - 1 ) fputs(", ", out);
	   }
	fputs(")  ", out);
	/* Put out the argument declarations */
	for ( ap = breaks+2; ap <= bp; ap += 2 ) (*ap)[-1] = ';';
	fputs(breaks[0], out);
	free((char *)breaks);
	return 0;
}

# Makefile for Independent JPEG Group's software

# This makefile is for Amiga systems using SAS C 6.0 and up.
# Thanks to Ed Hanway, Mark Rinfret, and Jim Zepeda.

# Read SETUP instructions before saying "make" !!
# Use jmemname.c as the system-dependent memory manager.

# The name of your C compiler:
CC= sc

# Uncomment the following lines for generic 680x0 version
ARCHFLAGS= cpu=any
SUFFIX=

# Uncomment the following lines for 68030-only version
#ARCHFLAGS= cpu=68030
#SUFFIX=.030

# You may need to adjust these cc options:
CFLAGS= nostackcheck data=near parms=register optimize $(ARCHFLAGS) \
	ignore=104 ignore=304 ignore=306 \
	define HAVE_STDC define INCLUDES_ARE_ANSI \
	define AMIGA define TWO_FILE_COMMANDLINE \
	define NO_MKTEMP define NEED_SIGNAL_CATCHER define SHORTxSHORT_32
# ignore=104 disables warnings for mismatched const qualifiers
# ignore=304 disables warnings for variables being optimized out
# ignore=306 disables warnings for the inlining of functions

# Link-time cc options:
LDFLAGS= SC SD ND BATCH

# To link any special libraries, add the necessary commands here.
LDLIBS= LIB LIB:scm.lib LIB:sc.lib

# miscellaneous OS-dependent stuff
# linker
LN= slink
# file deletion command
RM= delete quiet
# library (.lib) file creation command
AR= oml


# source files (independently compilable files)
SOURCES= jbsmooth.c jcarith.c jccolor.c jcdeflts.c jcexpand.c jchuff.c \
        jcmain.c jcmaster.c jcmcu.c jcpipe.c jcsample.c jdarith.c jdcolor.c \
        jddeflts.c jdhuff.c jdmain.c jdmaster.c jdmcu.c jdpipe.c jdsample.c \
        jerror.c jquant1.c jquant2.c jfwddct.c jrevdct.c jutils.c jmemmgr.c \
        jrdjfif.c jrdgif.c jrdppm.c jrdrle.c jrdtarga.c jwrjfif.c jwrgif.c \
        jwrppm.c jwrrle.c jwrtarga.c
# virtual source files (not present in distribution file, see SETUP)
VIRTSOURCES= jmemsys.c
# system-dependent implementations of virtual source files
SYSDEPFILES= jmemansi.c jmemname.c jmemnobs.c jmemdos.c jmemdos.h \
        jmemdosa.asm
# files included by source files
INCLUDES= jinclude.h jconfig.h jpegdata.h jversion.h jmemsys.h
# documentation, test, and support files
DOCS= README SETUP USAGE CHANGELOG cjpeg.1 djpeg.1 architecture codingrules
MAKEFILES= makefile.ansi makefile.unix makefile.manx makefile.sas \
        makcjpeg.st makdjpeg.st makljpeg.st makefile.mc5 makefile.mc6 \
        makefile.bcc makefile.icc makljpeg.icc makefile.mms makefile.vms \
        makvms.opt
OTHERFILES= ansi2knr.c ckconfig.c example.c
TESTFILES= testorig.jpg testimg.ppm testimg.gif testimg.jpg
DISTFILES= $(DOCS) $(MAKEFILES) $(SOURCES) $(SYSDEPFILES) $(INCLUDES) \
        $(OTHERFILES) $(TESTFILES)
# objectfiles common to cjpeg and djpeg
COMOBJECTS= jutils.o jerror.o jmemmgr.o jmemsys.o
# compression objectfiles
CLIBOBJECTS= jcmaster.o jcdeflts.o jcarith.o jccolor.o jcexpand.o jchuff.o \
        jcmcu.o jcpipe.o jcsample.o jfwddct.o jwrjfif.o jrdgif.o jrdppm.o \
        jrdrle.o jrdtarga.o
COBJECTS= jcmain.o $(CLIBOBJECTS) $(COMOBJECTS)
# decompression objectfiles
DLIBOBJECTS= jdmaster.o jddeflts.o jbsmooth.o jdarith.o jdcolor.o jdhuff.o \
        jdmcu.o jdpipe.o jdsample.o jquant1.o jquant2.o jrevdct.o jrdjfif.o \
        jwrgif.o jwrppm.o jwrrle.o jwrtarga.o
DOBJECTS= jdmain.o $(DLIBOBJECTS) $(COMOBJECTS)
# These objectfiles are included in libjpeg.lib
LIBOBJECTS= $(CLIBOBJECTS) $(DLIBOBJECTS) $(COMOBJECTS)


all: cjpeg$(SUFFIX) djpeg$(SUFFIX)
# By default, libjpeg.lib is not built unless you explicitly request it.
# You can add libjpeg.lib to the line above if you want it built by default.


cjpeg$(SUFFIX): $(COBJECTS)
	$(LN) <WITH <
$(LDFLAGS)
TO cjpeg$(SUFFIX)
FROM LIB:c.o $(COBJECTS)
$(LDLIBS)
<

djpeg$(SUFFIX): $(DOBJECTS)
	$(LN) <WITH <
$(LDFLAGS)
TO djpeg$(SUFFIX)
FROM LIB:c.o $(DOBJECTS)
$(LDLIBS)
<

# libjpeg.lib is useful if you are including the JPEG software in a larger
# program; you'd include it in your link, rather than the individual modules.
libjpeg.lib: $(LIBOBJECTS)
	-$(RM) libjpeg.lib
	$(AR) libjpeg.lib r $(LIBOBJECTS)

jmemsys.c:
	echo You must select a system-dependent jmemsys.c file.
	echo Please read the SETUP directions.
	exit 1

clean:
	-$(RM) *.o cjpeg djpeg cjpeg.030 djpeg.030 libjpeg.lib core testout.*

distribute:
	-$(RM) jpegsrc.tar*
	tar cvf jpegsrc.tar $(DISTFILES)
	compress -v jpegsrc.tar

test: cjpeg djpeg
	-$(RM) testout.ppm testout.gif testout.jpg
	djpeg -outfile testout.ppm  testorig.jpg
	djpeg -gif -outfile testout.gif  testorig.jpg
	cjpeg -outfile testout.jpg  testimg.ppm
	cmp testimg.ppm testout.ppm
	cmp testimg.gif testout.gif
	cmp testimg.jpg testout.jpg


jbsmooth.o : jbsmooth.c jinclude.h jconfig.h jpegdata.h 
jcarith.o : jcarith.c jinclude.h jconfig.h jpegdata.h 
jccolor.o : jccolor.c jinclude.h jconfig.h jpegdata.h 
jcdeflts.o : jcdeflts.c jinclude.h jconfig.h jpegdata.h 
jcexpand.o : jcexpand.c jinclude.h jconfig.h jpegdata.h 
jchuff.o : jchuff.c jinclude.h jconfig.h jpegdata.h 
jcmain.o : jcmain.c jinclude.h jconfig.h jpegdata.h jversion.h 
jcmaster.o : jcmaster.c jinclude.h jconfig.h jpegdata.h 
jcmcu.o : jcmcu.c jinclude.h jconfig.h jpegdata.h 
jcpipe.o : jcpipe.c jinclude.h jconfig.h jpegdata.h 
jcsample.o : jcsample.c jinclude.h jconfig.h jpegdata.h 
jdarith.o : jdarith.c jinclude.h jconfig.h jpegdata.h 
jdcolor.o : jdcolor.c jinclude.h jconfig.h jpegdata.h 
jddeflts.o : jddeflts.c jinclude.h jconfig.h jpegdata.h 
jdhuff.o : jdhuff.c jinclude.h jconfig.h jpegdata.h 
jdmain.o : jdmain.c jinclude.h jconfig.h jpegdata.h jversion.h 
jdmaster.o : jdmaster.c jinclude.h jconfig.h jpegdata.h 
jdmcu.o : jdmcu.c jinclude.h jconfig.h jpegdata.h 
jdpipe.o : jdpipe.c jinclude.h jconfig.h jpegdata.h 
jdsample.o : jdsample.c jinclude.h jconfig.h jpegdata.h 
jerror.o : jerror.c jinclude.h jconfig.h jpegdata.h 
jquant1.o : jquant1.c jinclude.h jconfig.h jpegdata.h 
jquant2.o : jquant2.c jinclude.h jconfig.h jpegdata.h 
jfwddct.o : jfwddct.c jinclude.h jconfig.h jpegdata.h 
jrevdct.o : jrevdct.c jinclude.h jconfig.h jpegdata.h 
jutils.o : jutils.c jinclude.h jconfig.h jpegdata.h 
jmemmgr.o : jmemmgr.c jinclude.h jconfig.h jpegdata.h jmemsys.h 
jrdjfif.o : jrdjfif.c jinclude.h jconfig.h jpegdata.h 
jrdgif.o : jrdgif.c jinclude.h jconfig.h jpegdata.h 
jrdppm.o : jrdppm.c jinclude.h jconfig.h jpegdata.h 
jrdrle.o : jrdrle.c jinclude.h jconfig.h jpegdata.h 
jrdtarga.o : jrdtarga.c jinclude.h jconfig.h jpegdata.h 
jwrjfif.o : jwrjfif.c jinclude.h jconfig.h jpegdata.h 
jwrgif.o : jwrgif.c jinclude.h jconfig.h jpegdata.h 
jwrppm.o : jwrppm.c jinclude.h jconfig.h jpegdata.h 
jwrrle.o : jwrrle.c jinclude.h jconfig.h jpegdata.h 
jwrtarga.o : jwrtarga.c jinclude.h jconfig.h jpegdata.h 
jmemsys.o : jmemsys.c jinclude.h jconfig.h jpegdata.h jmemsys.h 

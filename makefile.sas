# Makefile for Independent JPEG Group's software

# This makefile is for Amiga systems using SAS C 6.0 and up.
# Thanks to Ed Hanway, Mark Rinfret, and Jim Zepeda.

# Read installation instructions before saying "make" !!

# The name of your C compiler:
CC= sc

# You may need to adjust these cc options:
# Uncomment the following lines for generic 680x0 version
ARCHFLAGS= cpu=any
SUFFIX=

# Uncomment the following lines for 68030-only version
#ARCHFLAGS= cpu=68030
#SUFFIX=.030

CFLAGS= nostackcheck data=near parms=register optimize $(ARCHFLAGS) \
	ignore=104 ignore=304 ignore=306
# ignore=104 disables warnings for mismatched const qualifiers
# ignore=304 disables warnings for variables being optimized out
# ignore=306 disables warnings for the inlining of functions
# Generally, we recommend defining any configuration symbols in jconfig.h,
# NOT via define switches here.

# Link-time cc options:
LDFLAGS= SC SD ND BATCH

# To link any special libraries, add the necessary commands here.
LDLIBS= LIB:scm.lib LIB:sc.lib

# Put here the object file name for the correct system-dependent memory
# manager file.  For Amiga we recommend jmemname.o.
SYSDEPMEM= jmemname.o

# miscellaneous OS-dependent stuff
# linker
LN= slink
# file deletion command
RM= delete quiet
# library (.lib) file creation command
AR= oml

# End of configurable options.


# source files: JPEG library proper
LIBSOURCES= jcapi.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c jcmainct.c \
        jcmarker.c jcmaster.c jcomapi.c jcparam.c jcprepct.c jcsample.c \
        jdapi.c jdatasrc.c jdatadst.c jdcoefct.c jdcolor.c jddctmgr.c \
        jdhuff.c jdmainct.c jdmarker.c jdmaster.c jdpostct.c jdsample.c \
        jerror.c jutils.c jfdctfst.c jfdctflt.c jfdctint.c jidctfst.c \
        jidctflt.c jidctint.c jidctred.c jquant1.c jquant2.c jdmerge.c \
        jmemmgr.c jmemansi.c jmemname.c jmemnobs.c jmemdos.c
# source files: cjpeg/djpeg applications, also rdjpgcom/wrjpgcom
APPSOURCES= cjpeg.c djpeg.c rdcolmap.c rdppm.c wrppm.c rdgif.c wrgif.c \
        rdtarga.c wrtarga.c rdbmp.c wrbmp.c rdrle.c wrrle.c rdjpgcom.c \
        wrjpgcom.c
SOURCES= $(LIBSOURCES) $(APPSOURCES)
# files included by source files
INCLUDES= jdct.h jerror.h jinclude.h jmemsys.h jmorecfg.h jpegint.h \
        jpeglib.h jversion.h cdjpeg.h cderror.h
# documentation, test, and support files
DOCS= README install.doc usage.doc cjpeg.1 djpeg.1 rdjpgcom.1 wrjpgcom.1 \
        example.c libjpeg.doc structure.doc coderules.doc filelist.doc \
        change.log
MKFILES= configure makefile.auto makefile.ansi makefile.unix makefile.manx \
        makefile.sas makcjpeg.st makdjpeg.st makljpeg.st makefile.bcc \
        makefile.mc6 makefile.dj makefile.mms makefile.vms makvms.opt
CONFIGFILES= jconfig.auto jconfig.manx jconfig.sas jconfig.st jconfig.bcc \
        jconfig.mc6 jconfig.dj jconfig.vms
OTHERFILES= jconfig.doc ckconfig.c ansi2knr.c ansi2knr.1 jmemdosa.asm
TESTFILES= testorig.jpg testimg.ppm testimg.gif testimg.jpg
DISTFILES= $(DOCS) $(MKFILES) $(CONFIGFILES) $(SOURCES) $(INCLUDES) \
        $(OTHERFILES) $(TESTFILES)
# library object files common to compression and decompression
COMOBJECTS= jcomapi.o jutils.o jerror.o jmemmgr.o $(SYSDEPMEM)
# compression library object files
CLIBOBJECTS= jcapi.o jcparam.o jdatadst.o jcmaster.o jcmarker.o jcmainct.o \
        jcprepct.o jccoefct.o jccolor.o jcsample.o jchuff.o jcdctmgr.o \
        jfdctfst.o jfdctflt.o jfdctint.o
# decompression library object files
DLIBOBJECTS= jdapi.o jdatasrc.o jdmaster.o jdmarker.o jdmainct.o jdcoefct.o \
        jdpostct.o jddctmgr.o jidctfst.o jidctflt.o jidctint.o jidctred.o \
        jdhuff.o jdsample.o jdcolor.o jquant1.o jquant2.o jdmerge.o
# These objectfiles are included in libjpeg.lib
LIBOBJECTS= $(CLIBOBJECTS) $(DLIBOBJECTS) $(COMOBJECTS)
# object files for cjpeg and djpeg applications (excluding library files)
COBJECTS= cjpeg.o rdppm.o rdgif.o rdtarga.o rdrle.o rdbmp.o
DOBJECTS= djpeg.o wrppm.o wrgif.o wrtarga.o wrrle.o wrbmp.o rdcolmap.o


all: libjpeg.lib cjpeg$(SUFFIX) djpeg$(SUFFIX) rdjpgcom$(SUFFIX) wrjpgcom$(SUFFIX)

libjpeg.lib: $(LIBOBJECTS)
	-$(RM) libjpeg.lib
	$(AR) libjpeg.lib r $(LIBOBJECTS)

cjpeg$(SUFFIX): $(COBJECTS) libjpeg.lib
	$(LN) <WITH <
$(LDFLAGS)
TO cjpeg$(SUFFIX)
FROM LIB:c.o $(COBJECTS)
LIB libjpeg.lib $(LDLIBS)
<

djpeg$(SUFFIX): $(DOBJECTS) libjpeg.lib
	$(LN) <WITH <
$(LDFLAGS)
TO djpeg$(SUFFIX)
FROM LIB:c.o $(DOBJECTS)
LIB libjpeg.lib $(LDLIBS)
<

rdjpgcom$(SUFFIX): rdjpgcom.o
	$(LN) <WITH <
$(LDFLAGS)
TO rdjpgcom$(SUFFIX)
FROM LIB:c.o rdjpgcom.o
LIB $(LDLIBS)
<

wrjpgcom$(SUFFIX): wrjpgcom.o
	$(LN) <WITH <
$(LDFLAGS)
TO wrjpgcom$(SUFFIX)
FROM LIB:c.o wrjpgcom.o
LIB $(LDLIBS)
<

jconfig.h: jconfig.doc
	echo You must prepare a system-dependent jconfig.h file.
	echo Please read the installation directions in install.doc.
	exit 1

clean:
	-$(RM) *.o cjpeg djpeg cjpeg.030 djpeg.030 libjpeg.lib core testout.*
	-$(RM) rdjpgcom wrjpgcom rdjpgcom.030 wrjpgcom.030

test: cjpeg djpeg
	-$(RM) testout.ppm testout.gif testout.jpg
	djpeg -dct int -ppm -outfile testout.ppm  testorig.jpg
	djpeg -dct int -gif -outfile testout.gif  testorig.jpg
	cjpeg -dct int -outfile testout.jpg  testimg.ppm
	cmp testimg.ppm testout.ppm
	cmp testimg.gif testout.gif
	cmp testimg.jpg testout.jpg


jcapi.o : jcapi.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jccoefct.o : jccoefct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jccolor.o : jccolor.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcdctmgr.o : jcdctmgr.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jchuff.o : jchuff.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcmainct.o : jcmainct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcmarker.o : jcmarker.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcmaster.o : jcmaster.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcomapi.o : jcomapi.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcparam.o : jcparam.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcprepct.o : jcprepct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcsample.o : jcsample.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdapi.o : jdapi.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdatasrc.o : jdatasrc.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h
jdatadst.o : jdatadst.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h
jdcoefct.o : jdcoefct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdcolor.o : jdcolor.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jddctmgr.o : jddctmgr.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jdhuff.o : jdhuff.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdmainct.o : jdmainct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdmarker.o : jdmarker.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdmaster.o : jdmaster.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdpostct.o : jdpostct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdsample.o : jdsample.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jerror.o : jerror.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jversion.h jerror.h
jutils.o : jutils.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jfdctfst.o : jfdctfst.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jfdctflt.o : jfdctflt.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jfdctint.o : jfdctint.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jidctfst.o : jidctfst.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jidctflt.o : jidctflt.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jidctint.o : jidctint.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jidctred.o : jidctred.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jquant1.o : jquant1.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jquant2.o : jquant2.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdmerge.o : jdmerge.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jmemmgr.o : jmemmgr.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
jmemansi.o : jmemansi.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
jmemname.o : jmemname.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
jmemnobs.o : jmemnobs.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
jmemdos.o : jmemdos.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
cjpeg.o : cjpeg.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h jversion.h
djpeg.o : djpeg.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h jversion.h
rdcolmap.o : rdcolmap.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdppm.o : rdppm.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
wrppm.o : wrppm.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdgif.o : rdgif.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
wrgif.o : wrgif.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdtarga.o : rdtarga.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
wrtarga.o : wrtarga.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdbmp.o : rdbmp.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
wrbmp.o : wrbmp.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdrle.o : rdrle.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
wrrle.o : wrrle.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdjpgcom.o : rdjpgcom.c jinclude.h jconfig.h
wrjpgcom.o : wrjpgcom.c jinclude.h jconfig.h

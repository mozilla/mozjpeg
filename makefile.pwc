# Makefile for Independent JPEG Group's software

# This makefile is for Mix Software's Power C, v2.1.1
# and Dan Grayson's pd make 2.14 under MS-DOS.
# This file assumes that you have Microsoft's MASM or a compatible assembler
# to handle the jmemdosa.asm file.  If not, you will need to use jmemname.c
# and go to a large-data memory model.
# Thanks to Bob Hardy for this version.

# Read SETUP instructions before saying "make" !!

# The name of your C compiler:
CC=pc

# You may need to adjust these cc options:
MODEL=m
CFLAGS= -dMSDOS -m$(MODEL)
# In particular:
#   Add -dMEM_STATS to enable gathering of memory usage statistics.

# Link-time cc options:
LDFLAGS=
LDLIBS=

# miscellaneous OS-dependent stuff
# linker
LN=pcl
# file deletion command
RM=del
# library (.mix) file creation command
AR=merge


# source files (independently compilable files)
SOURCES= jbsmooth.c jcarith.c jccolor.c jcdeflts.c jcexpand.c jchuff.c jcmain.c jcmaster.c jcmcu.c jcpipe.c jcsample.c jdarith.c jdcolor.c jddeflts.c jdhuff.c jdmain.c jdmaster.c jdmcu.c jdpipe.c jdsample.c jerror.c jquant1.c jquant2.c jfwddct.c jrevdct.c jutils.c jmemmgr.c jrdjfif.c jrdgif.c jrdppm.c jrdrle.c jrdtarga.c jwrjfif.c jwrgif.c jwrppm.c jwrrle.c jwrtarga.c
# virtual source files (not present in distribution file)
VIRTSOURCES= jmemsys.c
# system-dependent implementations of source files
SYSDEPFILES= jmemansi.c jmemname.c jmemnobs.c jmemdos.c jmemdos.h jmemdosa.asm
# files included by source files
INCLUDES= jinclude.h jconfig.h jpegdata.h jversion.h jmemsys.h egetopt.c
# documentation, test, and support files
DOCS= README SETUP USAGE CHANGELOG cjpeg.1 djpeg.1 architecture codingrules
MAKEFILES= makefile.ansi makefile.unix makefile.manx makefile.sas makefile.mc5 makefile.mc6 makcjpeg.lnk makdjpeg.lnk makefile.bcc makcjpeg.lst makdjpeg.lst makefile.pwc makcjpeg.cf makdjpeg.cf makljpeg.cf makefile.mms makefile.vms makvms.opt
OTHERFILES= ansi2knr.c ckconfig.c example.c
TESTFILES= testorig.jpg testimg.ppm testimg.gif testimg.jpg
DISTFILES= $(DOCS) $(MAKEFILES) $(SOURCES) $(SYSDEPFILES) $(INCLUDES) $(OTHERFILES) $(TESTFILES)
# objectfiles common to cjpeg and djpeg
COMOBJECTS= jutils.mix jerror.mix jmemmgr.mix jmemsys.mix jmemdosa.mix
# compression objectfiles
CLIBOBJECTS= jcmaster.mix jcdeflts.mix jcarith.mix jccolor.mix jcexpand.mix jchuff.mix jcmcu.mix jcpipe.mix jcsample.mix jfwddct.mix jwrjfif.mix jrdgif.mix jrdppm.mix jrdrle.mix jrdtarga.mix
COBJECTS= jcmain.mix $(CLIBOBJECTS) $(COMOBJECTS)
# decompression objectfiles
DLIBOBJECTS= jdmaster.mix jddeflts.mix jbsmooth.mix jdarith.mix jdcolor.mix jdhuff.mix jdmcu.mix jdpipe.mix jdsample.mix jquant1.mix jquant2.mix jrevdct.mix jrdjfif.mix jwrgif.mix jwrppm.mix jwrrle.mix jwrtarga.mix
DOBJECTS= jdmain.mix $(DLIBOBJECTS) $(COMOBJECTS)
# These objectfiles are included in libjpeg.mix
LIBOBJECTS= $(CLIBOBJECTS) $(DLIBOBJECTS) $(COMOBJECTS)


all: cjpeg.exe djpeg.exe
# By default, libjpeg.mix is not built unless you explicitly request it.
# You can add libjpeg.mix to the line above if you want it built by default.


cjpeg.exe: $(COBJECTS)
	$(LN) $(LDFLAGS) @makcjpeg.cf

djpeg.exe: $(DOBJECTS)
	$(LN) $(LDFLAGS) @makdjpeg.cf

# libjpeg.mix is useful if you are including the JPEG software in a larger
# program; you'd include it in your link, rather than the individual modules.
libjpeg.mix: $(LIBOBJECTS)
	@$(RM) libjpeg.mix
	$(AR) libjpeg.mix @makljpeg.cf

clean:
	$(RM) *.mix cjpeg.exe djpeg.exe testout.*

test:
	@$(RM) testout.*
	+djpeg testorig.jpg testout.ppm
	+djpeg -G testorig.jpg testout.gif
	+cjpeg testimg.ppm testout.jpg
	fc testimg.ppm testout.ppm
	fc testimg.gif testout.gif
	fc testimg.jpg testout.jpg


jbsmooth.mix : jbsmooth.c jinclude.h jconfig.h jpegdata.h
jcarith.mix : jcarith.c jinclude.h jconfig.h jpegdata.h
jccolor.mix : jccolor.c jinclude.h jconfig.h jpegdata.h
jcdeflts.mix : jcdeflts.c jinclude.h jconfig.h jpegdata.h
jcexpand.mix : jcexpand.c jinclude.h jconfig.h jpegdata.h
jchuff.mix : jchuff.c jinclude.h jconfig.h jpegdata.h
jcmain.mix : jcmain.c jinclude.h jconfig.h jpegdata.h jversion.h egetopt.c
jcmaster.mix : jcmaster.c jinclude.h jconfig.h jpegdata.h
jcmcu.mix : jcmcu.c jinclude.h jconfig.h jpegdata.h
jcpipe.mix : jcpipe.c jinclude.h jconfig.h jpegdata.h
jcsample.mix : jcsample.c jinclude.h jconfig.h jpegdata.h
jdarith.mix : jdarith.c jinclude.h jconfig.h jpegdata.h
jdcolor.mix : jdcolor.c jinclude.h jconfig.h jpegdata.h
jddeflts.mix : jddeflts.c jinclude.h jconfig.h jpegdata.h
jdhuff.mix : jdhuff.c jinclude.h jconfig.h jpegdata.h
jdmain.mix : jdmain.c jinclude.h jconfig.h jpegdata.h jversion.h egetopt.c
jdmaster.mix : jdmaster.c jinclude.h jconfig.h jpegdata.h
jdmcu.mix : jdmcu.c jinclude.h jconfig.h jpegdata.h
jdpipe.mix : jdpipe.c jinclude.h jconfig.h jpegdata.h
jdsample.mix : jdsample.c jinclude.h jconfig.h jpegdata.h
jerror.mix : jerror.c jinclude.h jconfig.h jpegdata.h
jquant1.mix : jquant1.c jinclude.h jconfig.h jpegdata.h
jquant2.mix : jquant2.c jinclude.h jconfig.h jpegdata.h
jfwddct.mix : jfwddct.c jinclude.h jconfig.h jpegdata.h
jrevdct.mix : jrevdct.c jinclude.h jconfig.h jpegdata.h
jutils.mix : jutils.c jinclude.h jconfig.h jpegdata.h
jmemmgr.mix : jmemmgr.c jinclude.h jconfig.h jpegdata.h jmemsys.h
jrdjfif.mix : jrdjfif.c jinclude.h jconfig.h jpegdata.h
jrdgif.mix : jrdgif.c jinclude.h jconfig.h jpegdata.h
jrdppm.mix : jrdppm.c jinclude.h jconfig.h jpegdata.h
jrdrle.mix : jrdrle.c jinclude.h jconfig.h jpegdata.h
jrdtarga.mix : jrdtarga.c jinclude.h jconfig.h jpegdata.h
jwrjfif.mix : jwrjfif.c jinclude.h jconfig.h jpegdata.h
jwrgif.mix : jwrgif.c jinclude.h jconfig.h jpegdata.h
jwrppm.mix : jwrppm.c jinclude.h jconfig.h jpegdata.h
jwrrle.mix : jwrrle.c jinclude.h jconfig.h jpegdata.h
jwrtarga.mix : jwrtarga.c jinclude.h jconfig.h jpegdata.h
jmemsys.mix : jmemsys.c jinclude.h jconfig.h jpegdata.h jmemsys.h
jmemdosa.mix : jmemdosa.asm
	masm /mx jmemdosa;
	mix jmemdosa.obj

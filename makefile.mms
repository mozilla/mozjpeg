# Makefile for Independent JPEG Group's software

# This makefile is for use with MMS on VAX/VMS systems.
# Thanks to Rick Dyson (dyson@iowasp.physics.uiowa.edu)
# and Tim Bell (tbell@netcom.com) for their help.

# Read SETUP instructions before saying "MMS" !!

CFLAGS= $(CFLAGS) /NoDebug /Optimize /Define = (TWO_FILE_COMMANDLINE,HAVE_STDC,INCLUDES_ARE_ANSI)
OPT= Sys$Disk:[]MAKVMS.OPT


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
COMOBJECTS= jutils.obj jerror.obj jmemmgr.obj jmemsys.obj
# compression objectfiles
CLIBOBJECTS= jcmaster.obj jcdeflts.obj jcarith.obj jccolor.obj jcexpand.obj \
        jchuff.obj jcmcu.obj jcpipe.obj jcsample.obj jfwddct.obj \
        jwrjfif.obj jrdgif.obj jrdppm.obj jrdrle.obj jrdtarga.obj
COBJECTS= jcmain.obj $(CLIBOBJECTS) $(COMOBJECTS)
# decompression objectfiles
DLIBOBJECTS= jdmaster.obj jddeflts.obj jbsmooth.obj jdarith.obj jdcolor.obj \
        jdhuff.obj jdmcu.obj jdpipe.obj jdsample.obj jquant1.obj \
        jquant2.obj jrevdct.obj jrdjfif.obj jwrgif.obj jwrppm.obj \
        jwrrle.obj jwrtarga.obj
DOBJECTS= jdmain.obj $(DLIBOBJECTS) $(COMOBJECTS)
# These objectfiles are included in libjpeg.olb
LIBOBJECTS= $(CLIBOBJECTS) $(DLIBOBJECTS) $(COMOBJECTS)
# objectfile lists with commas --- what a crock
COBJLIST= jcmain.obj,jcmaster.obj,jcdeflts.obj,jcarith.obj,jccolor.obj,\
          jcexpand.obj,jchuff.obj,jcmcu.obj,jcpipe.obj,jcsample.obj,\
          jfwddct.obj,jwrjfif.obj,jrdgif.obj,jrdppm.obj,jrdrle.obj,\
          jrdtarga.obj,jutils.obj,jerror.obj,jmemmgr.obj,jmemsys.obj
DOBJLIST= jdmain.obj,jdmaster.obj,jddeflts.obj,jbsmooth.obj,jdarith.obj,\
          jdcolor.obj,jdhuff.obj,jdmcu.obj,jdpipe.obj,jdsample.obj,\
          jquant1.obj,jquant2.obj,jrevdct.obj,jrdjfif.obj,jwrgif.obj,\
          jwrppm.obj,jwrrle.obj,jwrtarga.obj,jutils.obj,jerror.obj,\
          jmemmgr.obj,jmemsys.obj
LIBOBJLIST= jcmaster.obj,jcdeflts.obj,jcarith.obj,jccolor.obj,jcexpand.obj,\
          jchuff.obj,jcmcu.obj,jcpipe.obj,jcsample.obj,jfwddct.obj,\
          jwrjfif.obj,jrdgif.obj,jrdppm.obj,jrdrle.obj,jrdtarga.obj,\
          jdmaster.obj,jddeflts.obj,jbsmooth.obj,jdarith.obj,jdcolor.obj,\
          jdhuff.obj,jdmcu.obj,jdpipe.obj,jdsample.obj,jquant1.obj,\
          jquant2.obj,jrevdct.obj,jrdjfif.obj,jwrgif.obj,jwrppm.obj,\
          jwrrle.obj,jwrtarga.obj,jutils.obj,jerror.obj,jmemmgr.obj,\
          jmemsys.obj


.first
	@ Define Sys Sys$Library

# By default, libjpeg.olb is not built unless you explicitly request it.
# You can add libjpeg.olb to the next line if you want it built by default.
ALL : cjpeg.exe djpeg.exe
	@ Continue

cjpeg.exe : $(COBJECTS)
	$(LINK) $(LFLAGS) /Executable = cjpeg.exe $(COBJLIST),$(OPT)/Option

djpeg.exe : $(DOBJECTS)
	$(LINK) $(LFLAGS) /Executable = djpeg.exe $(DOBJLIST),$(OPT)/Option

# libjpeg.olb is useful if you are including the JPEG software in a larger
# program; you'd include it in your link, rather than the individual modules.
libjpeg.olb : $(LIBOBJECTS)
	Library /Create libjpeg.olb $(LIBOBJLIST)

clean :
	@- Set Protection = Owner:RWED *.*;-1
	@- Set Protection = Owner:RWED *.OBJ
	- Purge /NoLog /NoConfirm *.*
	- Delete /NoLog /NoConfirm *.OBJ;

test : cjpeg.exe djpeg.exe
	mcr sys$disk:[]djpeg      testorig.jpg testout.ppm
	mcr sys$disk:[]djpeg -gif testorig.jpg testout.gif
	mcr sys$disk:[]cjpeg      testimg.ppm testout.jpg
	- Backup /Compare/Log	  testimg.ppm testout.ppm
	- Backup /Compare/Log	  testimg.gif testout.gif
	- Backup /Compare/Log	  testimg.jpg testout.jpg


jbsmooth.obj : jbsmooth.c jinclude.h jconfig.h jpegdata.h
jcarith.obj : jcarith.c jinclude.h jconfig.h jpegdata.h
jccolor.obj : jccolor.c jinclude.h jconfig.h jpegdata.h
jcdeflts.obj : jcdeflts.c jinclude.h jconfig.h jpegdata.h
jcexpand.obj : jcexpand.c jinclude.h jconfig.h jpegdata.h
jchuff.obj : jchuff.c jinclude.h jconfig.h jpegdata.h
jcmain.obj : jcmain.c jinclude.h jconfig.h jpegdata.h jversion.h
jcmaster.obj : jcmaster.c jinclude.h jconfig.h jpegdata.h
jcmcu.obj : jcmcu.c jinclude.h jconfig.h jpegdata.h
jcpipe.obj : jcpipe.c jinclude.h jconfig.h jpegdata.h
jcsample.obj : jcsample.c jinclude.h jconfig.h jpegdata.h
jdarith.obj : jdarith.c jinclude.h jconfig.h jpegdata.h
jdcolor.obj : jdcolor.c jinclude.h jconfig.h jpegdata.h
jddeflts.obj : jddeflts.c jinclude.h jconfig.h jpegdata.h
jdhuff.obj : jdhuff.c jinclude.h jconfig.h jpegdata.h
jdmain.obj : jdmain.c jinclude.h jconfig.h jpegdata.h jversion.h
jdmaster.obj : jdmaster.c jinclude.h jconfig.h jpegdata.h
jdmcu.obj : jdmcu.c jinclude.h jconfig.h jpegdata.h
jdpipe.obj : jdpipe.c jinclude.h jconfig.h jpegdata.h
jdsample.obj : jdsample.c jinclude.h jconfig.h jpegdata.h
jerror.obj : jerror.c jinclude.h jconfig.h jpegdata.h
jquant1.obj : jquant1.c jinclude.h jconfig.h jpegdata.h
jquant2.obj : jquant2.c jinclude.h jconfig.h jpegdata.h
jfwddct.obj : jfwddct.c jinclude.h jconfig.h jpegdata.h
jrevdct.obj : jrevdct.c jinclude.h jconfig.h jpegdata.h
jutils.obj : jutils.c jinclude.h jconfig.h jpegdata.h
jmemmgr.obj : jmemmgr.c jinclude.h jconfig.h jpegdata.h jmemsys.h
jrdjfif.obj : jrdjfif.c jinclude.h jconfig.h jpegdata.h
jrdgif.obj : jrdgif.c jinclude.h jconfig.h jpegdata.h
jrdppm.obj : jrdppm.c jinclude.h jconfig.h jpegdata.h
jrdrle.obj : jrdrle.c jinclude.h jconfig.h jpegdata.h
jrdtarga.obj : jrdtarga.c jinclude.h jconfig.h jpegdata.h
jwrjfif.obj : jwrjfif.c jinclude.h jconfig.h jpegdata.h
jwrgif.obj : jwrgif.c jinclude.h jconfig.h jpegdata.h
jwrppm.obj : jwrppm.c jinclude.h jconfig.h jpegdata.h
jwrrle.obj : jwrrle.c jinclude.h jconfig.h jpegdata.h
jwrtarga.obj : jwrtarga.c jinclude.h jconfig.h jpegdata.h
jmemsys.obj : jmemsys.c jinclude.h jconfig.h jpegdata.h jmemsys.h

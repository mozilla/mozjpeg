; Project file for Independent JPEG Group's software
;
; This project file is for Atari ST/STE/TT systems using Pure C or Turbo C.
; Thanks to Frank Moehle (Frank.Moehle@arbi.informatik.uni-oldenburg.de)
; and to Dr. B. Setzepfandt (bernd@gina.uni-muenster.de).
;
; To use this file, rename it to LIB_JPEG.PRJ.
; Read SETUP instructions before trying to make the library!
;
;
;      * * * Output file * * *
jpeg.lib
;
;      * * * DEFINES * * * 
.C[-DHAVE_STDC]
.C[-DINCLUDES_ARE_ANSI]
;
; * * * COMPILER OPTIONS * * *  
.C[-P]        ; absolute calls
.C[-M]        ; and no string merging, folks
.C[-w-par]    ; no "parameter xxxx unused"
.C[-DINCOMPLETE_TYPES_BROKEN] ; suppress warnings about undefined structures
.C[-wsig]     ; warn if significant digits may be lost
.C[-w-cln]    ; no "constant is long" warnings
.L[-J]        ; link new Obj-format (so we get a library)
=
; * * * * List of modules * * * * 
jbsmooth.c		(jinclude.h,jconfig.h,jpegdata.h)
jcarith.c		(jinclude.h,jconfig.h,jpegdata.h)
jccolor.c		(jinclude.h,jconfig.h,jpegdata.h)
jcdeflts.c		(jinclude.h,jconfig.h,jpegdata.h)
jcexpand.c		(jinclude.h,jconfig.h,jpegdata.h)
jchuff.c		(jinclude.h,jconfig.h,jpegdata.h)
jcmaster.c		(jinclude.h,jconfig.h,jpegdata.h)
jcmcu.c		(jinclude.h,jconfig.h,jpegdata.h)
jcpipe.c		(jinclude.h,jconfig.h,jpegdata.h)
jcsample.c		(jinclude.h,jconfig.h,jpegdata.h)
jdarith.c		(jinclude.h,jconfig.h,jpegdata.h)
jdcolor.c		(jinclude.h,jconfig.h,jpegdata.h)
jddeflts.c		(jinclude.h,jconfig.h,jpegdata.h)
jdhuff.c		(jinclude.h,jconfig.h,jpegdata.h)
jdmaster.c		(jinclude.h,jconfig.h,jpegdata.h)
jdmcu.c		(jinclude.h,jconfig.h,jpegdata.h)
jdpipe.c		(jinclude.h,jconfig.h,jpegdata.h)
jdsample.c		(jinclude.h,jconfig.h,jpegdata.h)
jerror.c		(jinclude.h,jconfig.h,jpegdata.h)
jquant1.c		(jinclude.h,jconfig.h,jpegdata.h)
jquant2.c		(jinclude.h,jconfig.h,jpegdata.h)
jfwddct.c		(jinclude.h,jconfig.h,jpegdata.h)
jrevdct.c		(jinclude.h,jconfig.h,jpegdata.h)
jutils.c		(jinclude.h,jconfig.h,jpegdata.h)
jmemmgr.c		(jinclude.h,jconfig.h,jpegdata.h,jmemsys.h)
jrdjfif.c		(jinclude.h,jconfig.h,jpegdata.h)
jrdgif.c		(jinclude.h,jconfig.h,jpegdata.h)
jrdppm.c		(jinclude.h,jconfig.h,jpegdata.h)
jrdrle.c		(jinclude.h,jconfig.h,jpegdata.h)
jrdtarga.c		(jinclude.h,jconfig.h,jpegdata.h)
jwrjfif.c		(jinclude.h,jconfig.h,jpegdata.h)
jwrgif.c		(jinclude.h,jconfig.h,jpegdata.h)
jwrppm.c		(jinclude.h,jconfig.h,jpegdata.h)
jwrrle.c		(jinclude.h,jconfig.h,jpegdata.h)
jwrtarga.c		(jinclude.h,jconfig.h,jpegdata.h)
jmemsys.c		(jinclude.h,jconfig.h,jpegdata.h,jmemsys.h)

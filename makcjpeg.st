; Project file for Independent JPEG Group's software
;
; This project file is for Atari ST/STE/TT systems using Pure C or Turbo C.
; Thanks to Frank Moehle (Frank.Moehle@arbi.informatik.uni-oldenburg.de)
; and to Dr. B. Setzepfandt (bernd@gina.uni-muenster.de).
;
; To use this file, rename it to CJPEG.PRJ.
; If you are using Turbo C, change filenames beginning with "PC..." to "TC..."
; Read installation instructions before trying to make the program!
;
;
;      * * * Output file * * *
cjpeg.ttp
;
; * * * COMPILER OPTIONS * * *  
.C[-P]        ; absolute calls
.C[-M]        ; and no string merging, folks
.C[-w-cln]    ; no "constant is long" warnings
.C[-w-par]    ; no "parameter xxxx unused"
.C[-w-rch]    ; no "unreachable code"
.C[-wsig]     ; warn if significant digits may be lost
=
; * * * * List of modules * * * * 
PCSTART.O
cjpeg.c	(cdjpeg.h,jinclude.h,jconfig.h,jpeglib.h,jmorecfg.h,jerror.h,cderror.h,jversion.h)
rdppm.c	(cdjpeg.h,jinclude.h,jconfig.h,jpeglib.h,jmorecfg.h,jerror.h,cderror.h)
rdgif.c	(cdjpeg.h,jinclude.h,jconfig.h,jpeglib.h,jmorecfg.h,jerror.h,cderror.h)
rdtarga.c	(cdjpeg.h,jinclude.h,jconfig.h,jpeglib.h,jmorecfg.h,jerror.h,cderror.h)
rdbmp.c	(cdjpeg.h,jinclude.h,jconfig.h,jpeglib.h,jmorecfg.h,jerror.h,cderror.h)
rdrle.c	(cdjpeg.h,jinclude.h,jconfig.h,jpeglib.h,jmorecfg.h,jerror.h,cderror.h)
LIBJPEG.LIB        ; built by LIBJPEG.PRJ
PCFLTLIB.LIB       ; floating point library
; the float library can be omitted if you've turned off DCT_FLOAT_SUPPORTED
PCSTDLIB.LIB       ; standard library
PCEXTLIB.LIB       ; extended library

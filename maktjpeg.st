; Project file for Independent JPEG Group's software
;
; This project file is for Atari ST/STE/TT systems using Pure C or Turbo C.
; Thanks to Frank Moehle (Frank.Moehle@arbi.informatik.uni-oldenburg.de)
; and to Dr. B. Setzepfandt (bernd@gina.uni-muenster.de).
;
; To use this file, rename it to JPEGTRAN.PRJ.
; If you are using Turbo C, change filenames beginning with "PC..." to "TC..."
; Read installation instructions before trying to make the program!
;
;
;      * * * Output file * * *
jpegtran.ttp
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
jpegtran.c	(cdjpeg.h,jinclude.h,jconfig.h,jpeglib.h,jmorecfg.h,jerror.h,cderror.h,jversion.h)
cdjpeg.c	(cdjpeg.h,jinclude.h,jconfig.h,jpeglib.h,jmorecfg.h,jerror.h,cderror.h)
rdswitch.c	(cdjpeg.h,jinclude.h,jconfig.h,jpeglib.h,jmorecfg.h,jerror.h,cderror.h)
LIBJPEG.LIB        ; built by LIBJPEG.PRJ
PCSTDLIB.LIB       ; standard library
PCEXTLIB.LIB       ; extended library

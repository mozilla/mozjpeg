; Project file for Independent JPEG Group's software
;
; This project file is for Atari ST/STE/TT systems using Pure C or Turbo C.
; Thanks to Frank Moehle (Frank.Moehle@arbi.informatik.uni-oldenburg.de)
; and to Dr. B. Setzepfandt (bernd@gina.uni-muenster.de).
;
; To use this file, rename it to DJPEG.PRJ.
; If you are using Turbo C, change filenames beginning with "PC..." to "TC..."
; Read SETUP instructions before trying to make the program!
;
;
;      * * * Output file * * *
djpeg.ttp
;
;      * * * DEFINES * * * 
.C[-DHAVE_STDC]
.C[-DINCLUDES_ARE_ANSI]
.C[-DNEED_SIGNAL_CATCHER]  ; It's safer to have a signal catcher.
.C[-DTWO_FILE_COMMANDLINE] ; If you like the UNIX - style with
                           ; I/O redirection, comment this line out.
.C[-DPROGRESS_REPORT]      ; This enables the % progress indicator
;
; * * * COMPILER OPTIONS * * *  
.C[-P]        ; absolute calls
.C[-M]        ; and no string merging, folks
.C[-w-par]    ; no "parameter xxxx unused"
.C[-DINCOMPLETE_TYPES_BROKEN] ; suppress warnings about undefined structures
.C[-wsig]     ; warn if significant digits may be lost
.C[-w-cln]    ; no "constant is long" warnings
=
; * * * * List of modules * * * * 
PCSTART.O
jdmain.c		(jinclude.h,jconfig.h,jpegdata.h,jversion.h)
JPEG.LIB           ; built by LIB_JPEG.PRJ
;PCFLTLIB.LIB      ; floating point library -- you shouldn't need this
PCSTDLIB.LIB       ; standard library
PCEXTLIB.LIB       ; extended library

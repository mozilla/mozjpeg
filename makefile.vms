$! Makefile for Independent JPEG Group's software
$!
$! This is a command procedure for Digital VMS systems that do not have MMS.
$! It builds the JPEG software by brute force, recompiling everything whether
$! or not it is necessary.  It then runs the basic self-test.
$! Thanks to Rick Dyson (dyson@iowasp.physics.uiowa.edu)
$! and Tim Bell (tbell@netcom.com) for their help.
$!
$! Read installation instructions before running this!!
$!
$ If F$GetSyi ("HW_MODEL") .gt. 1023 
$   Then
$       OPT = ""
$   Else
$       OPT = ",Sys$Disk:[]makvms.opt/Option"
$ EndIf
$ 
$ DoCompile := CC /NoDebug /Optimize
$!
$ DoCompile jcapi.c
$ DoCompile jcparam.c
$ DoCompile jdatadst.c
$ DoCompile jcmaster.c
$ DoCompile jcmarker.c
$ DoCompile jcmainct.c
$ DoCompile jcprepct.c
$ DoCompile jccoefct.c
$ DoCompile jccolor.c
$ DoCompile jcsample.c
$ DoCompile jchuff.c
$ DoCompile jcdctmgr.c
$ DoCompile jfdctfst.c
$ DoCompile jfdctflt.c
$ DoCompile jfdctint.c
$ DoCompile jdapi.c
$ DoCompile jdatasrc.c
$ DoCompile jdmaster.c
$ DoCompile jdmarker.c
$ DoCompile jdmainct.c
$ DoCompile jdcoefct.c
$ DoCompile jdpostct.c
$ DoCompile jddctmgr.c
$ DoCompile jidctfst.c
$ DoCompile jidctflt.c
$ DoCompile jidctint.c
$ DoCompile jidctred.c
$ DoCompile jdhuff.c
$ DoCompile jdsample.c
$ DoCompile jdcolor.c
$ DoCompile jquant1.c
$ DoCompile jquant2.c
$ DoCompile jdmerge.c
$ DoCompile jcomapi.c
$ DoCompile jutils.c
$ DoCompile jerror.c
$ DoCompile jmemmgr.c
$ DoCompile jmemnobs.c
$!
$ Library /Create libjpeg.olb  jcapi.obj,jcparam.obj,jdatadst.obj, -
          jcmaster.obj,jcmarker.obj,jcmainct.obj,jcprepct.obj,jccoefct.obj, -
          jccolor.obj,jcsample.obj,jchuff.obj,jcdctmgr.obj,jfdctfst.obj, -
          jfdctflt.obj,jfdctint.obj,jdapi.obj,jdatasrc.obj,jdmaster.obj, -
          jdmarker.obj,jdmainct.obj,jdcoefct.obj,jdpostct.obj,jddctmgr.obj, -
          jidctfst.obj,jidctflt.obj,jidctint.obj,jidctred.obj,jdhuff.obj, -
          jdsample.obj,jdcolor.obj,jquant1.obj,jquant2.obj,jdmerge.obj, -
          jcomapi.obj,jutils.obj,jerror.obj,jmemmgr.obj,jmemnobs.obj
$!
$ DoCompile cjpeg.c
$ DoCompile rdppm.c
$ DoCompile rdgif.c
$ DoCompile rdtarga.c
$ DoCompile rdrle.c
$ DoCompile rdbmp.c
$!
$ Link /Executable = cjpeg.exe  cjpeg.obj,rdppm.obj,rdgif.obj,rdtarga.obj, -
          rdrle.obj,rdbmp.obj,libjpeg.olb/Library'OPT'
$!
$ DoCompile djpeg.c
$ DoCompile wrppm.c
$ DoCompile wrgif.c
$ DoCompile wrtarga.c
$ DoCompile wrrle.c
$ DoCompile wrbmp.c
$ DoCompile rdcolmap.c
$!
$ Link /Executable = djpeg.exe  djpeg.obj,wrppm.obj,wrgif.obj,wrtarga.obj, -
          wrrle.obj,wrbmp.obj,rdcolmap.obj,libjpeg.olb/Library'OPT'
$!
$ DoCompile rdjpgcom.c
$ Link /Executable = rdjpgcom.exe  rdjpgcom.obj'OPT'
$!
$ DoCompile wrjpgcom.c
$ Link /Executable = wrjpgcom.exe  wrjpgcom.obj'OPT'
$!
$! Run the self-test
$!
$ mcr sys$disk:[]djpeg -dct int -ppm -outfile testout.ppm testorig.jpg
$ mcr sys$disk:[]djpeg -dct int -gif -outfile testout.gif testorig.jpg
$ mcr sys$disk:[]cjpeg -dct int      -outfile testout.jpg testimg.ppm
$ Backup /Compare/Log testimg.ppm testout.ppm
$ Backup /Compare/Log testimg.gif testout.gif
$ Backup /Compare/Log testimg.jpg testout.jpg
$!
$ Exit

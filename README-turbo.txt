*******************************************************************************
**     Background
*******************************************************************************

libjpeg-turbo is a high-speed version of libjpeg for x86 and x86-64 processors
which uses SIMD instructions (MMX, SSE2, etc.) to accelerate baseline JPEG
compression and decompression.  libjpeg-turbo is generally 2-3x as fast
as the unmodified version of libjpeg, all else being equal.

libjpeg-turbo was originally based on libjpeg/SIMD by Miyasaka Masaru, but
the TigerVNC and VirtualGL projects made numerous enhancements to the codec,
including improved support for Mac OS X, 64-bit support, support for 32-bit
and big endian pixel formats, accelerated Huffman encoding/decoding, and
various bug fixes.  The goal was to produce a fully open source codec that
could replace the partially closed source TurboJPEG/IPP codec used by VirtualGL
and TurboVNC.  libjpeg-turbo generally performs in the range of 80-110% of
TurboJPEG/IPP.  It is faster in some areas but slower in others.

It was decided to split libjpeg-turbo into a separate SDK so that other
projects could take advantage of this technology.  The shared libraries
built from the libjpeg-turbo source can be used as drop-in replacements for
libjpeg on most systems.


*******************************************************************************
**     Building on Unix Platforms, Cygwin, and MinGW
*******************************************************************************

==================
Build Requirements
==================

-- autoconf 2.56 or later
   * If using MinGW, this can be obtained by installing the MSYS DTK

-- automake 1.7 or later
   * If using MinGW, this can be obtained by installing the MSYS DTK

-- libtool 1.4 or later
   * If using MinGW, this can be obtained by installing the MSYS DTK

-- NASM
   * 0.98 or later is required for a 32-bit build
   * NASM 2.05 or later is required for a 64-bit build
   * NASM 2.07 or later is required for a 64-bit build on OS/X (10.6 "Snow
     Leopard" or later.)  This can be obtained from MacPorts
     (http://www.macports.org/).

   The NASM 2.05 RPMs do not work on older Linux systems, such as Enterprise
   Linux 4.  On such systems, you can easily build and install NASM 2.05
   from the source RPM by executing the following as root:

     ARCH=`uname -m`
     wget http://www.nasm.us/pub/nasm/releasebuilds/2.05.01/nasm-2.05.01-1.src.rpm
     rpmbuild --rebuild nasm-2.05.01-1.src.rpm
     rpm -Uvh /usr/src/redhat/RPMS/$ARCH/nasm-2.05.01-1.$ARCH.rpm

-- GCC v4 or later recommended for best performance

======================
Building libjpeg-turbo
======================

The following procedure will build libjpeg-turbo on Linux, 32-bit OS X, and
Solaris/x86 systems (on Solaris, this generates a 32-bit library.  See below
for 64-bit build instructions.)

  cd libjpeg-turbo
  autoreconf -fiv
  sh ./configure CFLAGS='-O3' CXXFLAGS='-O3'
  make

NOTE: Running autoreconf is only necessary if building libjpeg-turbo from the
SVN repository.

This will generate the following files under .libs/

  libjpeg.a
      Static link library for libjpeg-turbo

  libjpeg.so.62.0.0 (Linux, Solaris)
  libjpeg.62.dylib (OS X)
  libjpeg-62.dll (MinGW)
  cygjpeg-62.dll (Cygwin)
      Shared library for libjpeg-turbo

  libjpeg.so (Linux, Solaris)
  libjpeg.dylib (OS X)
  libjpeg.dll.a (Cygwin, MinGW)
      Development stub for libjpeg-turbo shared library

  libturbojpeg.a
      Static link library for TurboJPEG/OSS

  libturbojpeg.so (Linux, Solaris)
  libturbojpeg.dylib (OS X)
      Shared library and development stub for TurboJPEG/OSS

  libturbojpeg.dll (MinGW)
  cygturbojpeg.dll (Cygwin)
      Shared library for TurboJPEG/OSS

  libturbojpeg.dll.a (Cygwin, MinGW)
      Development stub for TurboJPEG/OSS shared library

========================
Installing libjpeg-turbo
========================

If you intend to install these libraries and the associated header files, then
replace 'make' in the instructions above with

  make install prefix={base dir} libdir={library directory}

For example,

  make install prefix=/usr libdir=/usr/lib64

will overwrite the system version of libjpeg on a 64-bit RedHat-based Linux
machine, causing any 64-bit applications that use libjpeg to be instantly
accelerated.  BACK UP YOUR SYSTEM'S INSTALLATION OF LIBJPEG BEFORE OVERWRITING
IT. 

The same can be done for 32-bit applications by building libjpeg-turbo as a
32-bit library (see below) and installing with a libdir of /usr/lib.  On
Debian-based systems, 64-bit libraries are stored in /usr/lib and 32-bit
libraries in /usr/lib32.  On Solaris, 64-bit libraries are stored in
/usr/lib/amd64 and 32-bit libraries in /usr/lib.

Mac applications typically bundle their own copies of libjpeg.62.dylib, so it
is not possible to globally replace libjpeg on OS X systems.  However, libjpeg
can be replaced on an application-by-application basis, for those applications
which use a shared library version of it.  This would generally involve copying
libjpeg.62.dylib into the appropriate place in the application's Contents and
using install_name_tool to repoint the dylib to the new directory.  This
requires an advanced knowledge of OS X and is not recommended for most users.

=============
Build Recipes
=============

32-bit Library Build on 64-bit Linux
------------------------------------

Same instructions as above, but add

  --host i686-pc-linux-gnu

to the configure command line and replace CFLAGS and CXXFLAGS with '-O3 -m32'.


64-bit Library Build on 64-bit OS/X
-----------------------------------

Same instructions as above, but add

  --host x86_64-apple-darwin10.0.0 NASM=/opt/local/bin/nasm

to the configure command line.  NASM 2.07 from MacPorts must be installed.


32-bit Library Build on 64-bit OS/X
-----------------------------------

Same instructions as above, but add

  LDFLAGS='-m32'

to the configure command line and replace CFLAGS and CXXFLAGS with '-O3 -m32'.


64-bit Library Build on 64-bit Solaris
--------------------------------------

Same instructions as above, but add

  --host x86_64-pc-solaris LDFLAGS='-m64'

to the configure command line and replace CFLAGS and CXXFLAGS with '-O3 -m64'. 


MinGW Build on Cygwin
---------------------

Same instructions as above, but add

  --host mingw32

to the configure command line.  This will produce libraries which do not
depend on cygwin1.dll or other Cygwin DLL's.


*******************************************************************************
**     Windows (Visual C++)
*******************************************************************************

==================
Build Requirements
==================

-- GNU Make v3.7 or later
   * Can be found in MSYS (http://www.mingw.org/download.shtml) or
     Cygwin (http://www.cygwin.com/)

-- Microsoft Visual C++ 2003 or later
   * Tested with Microsoft Visual C++ 2008 Express Edition (free download):

     http://msdn.microsoft.com/vstudio/express/visualc/

   * Add the compiler binary directories (for instance,
     c:\Program Files\Microsoft Visual Studio 9.0\VC\BIN;
     c:\Program Files\Microsoft Visual Studio 9.0\Common7\IDE)
     to the system or user PATH environment variable prior to building
     libjpeg-turbo.
   * Add the compiler include directory (for instance,
     c:\Program Files\Microsoft Visual Studio 9.0\VC\INCLUDE)
     to the system or user INCLUDE environment variable prior to building
     libjpeg-turbo.
   * Add the compiler library directory (for instance,
     c:\Program Files\Microsoft Visual Studio 9.0\VC\LIB)
     to the system or user LIB environment variable prior to building
     libjpeg-turbo.

-- Microsoft Windows SDK
   * This is included with Microsoft Visual C++ 2008 Express Edition, but users
     of prior editions of Visual C++ can download the SDK from:

     http://msdn2.microsoft.com/en-us/windowsserver/bb980924.aspx

   * Add the SDK binary directory (for instance,
     c:\Program Files\Microsoft SDKs\Windows\v6.0A\bin)
     to the system or user PATH environment variable prior to building
     libjpeg-turbo.
   * Add the SDK include directory (for instance,
     c:\Program Files\Microsoft SDKs\Windows\v6.0A\include)
     to the system or user INCLUDE environment variable prior to building
     libjpeg-turbo.
   * Add the SDK library directory (for instance,
     c:\Program Files\Microsoft SDKs\Windows\v6.0A\lib)
     to the system or user LIB environment variable prior to building
     libjpeg-turbo.

-- NASM (http://www.nasm.us/) 0.98 or later

======================
Building libjpeg-turbo
======================

  cd libjpeg-turbo
  make -f win/Makefile

This will generate the following files:

  jpeg-static.lib        Static link library for libjpeg-turbo
  jpeg62.dll             Shared library for libjpeg-turbo
  jpeg.lib               Development stub for libjpeg-turbo shared library
  turbojpeg-static.lib   Static link library for TurboJPEG/OSS
  turbojpeg.dll          Shared library for TurboJPEG/OSS
  turbojpeg.lib          Development stub for TurboJPEG/OSS shared library

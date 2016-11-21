Un*x Platforms (including Mac and Cygwin)
=========================================


Build Requirements
------------------

- autoconf 2.56 or later
- automake 1.7 or later
- libtool 1.4 or later
  * If using Xcode 4.3 or later on OS X, autoconf and automake are no longer
    provided.  The easiest way to obtain them is from
    [MacPorts](http://www.MacPorts.org) or [Homebrew](http://brew.sh/).

- [NASM](http://www.nasm.us) or [YASM](http://yasm.tortall.net)
  (if building x86 or x86-64 SIMD extensions)
  * If using NASM, 0.98, or 2.01 or later is required for an x86 build (0.99
    and 2.00 do not work properly with libjpeg-turbo's x86 SIMD code.)
  * If using NASM, 2.00 or later is required for an x86-64 build.
  * If using NASM, 2.07 or later (except 2.11.08) is required for an x86-64
    Mac build (2.11.08 does not work properly with libjpeg-turbo's x86-64 SIMD
    code when building macho64 objects.)  NASM or YASM can be obtained from
    [MacPorts](http://www.macports.org/) or [Homebrew](http://brew.sh/).

  The binary RPMs released by the NASM project do not work on older Linux
  systems, such as Red Hat Enterprise Linux 5.  On such systems, you can easily
  build and install NASM from a source RPM by downloading one of the SRPMs from

  <http://www.nasm.us/pub/nasm/releasebuilds>

  and executing the following as root:

        ARCH=`uname -m`
        rpmbuild --rebuild nasm-{version}.src.rpm
        rpm -Uvh /usr/src/redhat/RPMS/$ARCH/nasm-{version}.$ARCH.rpm

  NOTE: the NASM build will fail if texinfo is not installed.

- GCC v4.1 (or later) or Clang recommended for best performance

- If building the TurboJPEG Java wrapper, JDK or OpenJDK 1.5 or later is
  required.  Most modern Linux distributions, as well as Solaris 10 and later,
  include JDK or OpenJDK.  On OS X 10.5 and 10.6, it will be necessary to
  install the Java Developer Package, which can be downloaded from
  <http://developer.apple.com/downloads> (Apple ID required.)  For other
  systems, you can obtain the Oracle Java Development Kit from
  <http://www.java.com>.


Out-of-Tree Builds
------------------

Binary objects, libraries, and executables are generated in the directory from
which `configure` is executed (the "binary directory"), and this directory need
not necessarily be the same as the libjpeg-turbo source directory.  You can
create multiple independent binary directories, in which different versions of
libjpeg-turbo can be built from the same source tree using different compilers
or settings.  In the sections below, *{build_directory}* refers to the binary
directory, whereas *{source_directory}* refers to the libjpeg-turbo source
directory.  For in-tree builds, these directories are the same.


Build Procedure
---------------

The following procedure will build libjpeg-turbo on Unix and Unix-like systems.
(On Solaris, this generates a 32-bit build.  See "Build Recipes" below for
64-bit build instructions.)

    cd {source_directory}
    autoreconf -fiv
    cd {build_directory}
    sh {source_directory}/configure [additional configure flags]
    make

NOTE: Running autoreconf in the source directory is not necessary if building
libjpeg-turbo from one of the official release tarballs.

This will generate the following files under **.libs/**:

**libjpeg.a**<br>
Static link library for the libjpeg API

**libjpeg.so.{version}** (Linux, Unix)<br>
**libjpeg.{version}.dylib** (Mac)<br>
**cygjpeg-{version}.dll** (Cygwin)<br>
Shared library for the libjpeg API

By default, *{version}* is 62.2.0, 7.2.0, or 8.1.2, depending on whether
libjpeg v6b (default), v7, or v8 emulation is enabled.  If using Cygwin,
*{version}* is 62, 7, or 8.

**libjpeg.so** (Linux, Unix)<br>
**libjpeg.dylib** (Mac)<br>
Development symlink for the libjpeg API

**libjpeg.dll.a** (Cygwin)<br>
Import library for the libjpeg API

**libturbojpeg.a**<br>
Static link library for the TurboJPEG API

**libturbojpeg.so.0.1.0** (Linux, Unix)<br>
**libturbojpeg.0.1.0.dylib** (Mac)<br>
**cygturbojpeg-0.dll** (Cygwin)<br>
Shared library for the TurboJPEG API

**libturbojpeg.so** (Linux, Unix)<br>
**libturbojpeg.dylib** (Mac)<br>
Development symlink for the TurboJPEG API

**libturbojpeg.dll.a** (Cygwin)<br>
Import library for the TurboJPEG API


### libjpeg v7 or v8 API/ABI Emulation

Add `--with-jpeg7` to the `configure` command line to build a version of
libjpeg-turbo that is API/ABI-compatible with libjpeg v7.  Add `--with-jpeg8`
to the `configure` command to build a version of libjpeg-turbo that is
API/ABI-compatible with libjpeg v8.  See [README.md](README.md) for more
information about libjpeg v7 and v8 emulation.


### In-Memory Source/Destination Managers

When using libjpeg v6b or v7 API/ABI emulation, add `--without-mem-srcdst` to
the `configure` command line to build a version of libjpeg-turbo that lacks the
`jpeg_mem_src()` and `jpeg_mem_dest()` functions.  These functions were not
part of the original libjpeg v6b and v7 APIs, so removing them ensures strict
conformance with those APIs.  See [README.md](README.md) for more information.


### Arithmetic Coding Support

Since the patent on arithmetic coding has expired, this functionality has been
included in this release of libjpeg-turbo.  libjpeg-turbo's implementation is
based on the implementation in libjpeg v8, but it works when emulating libjpeg
v7 or v6b as well.  The default is to enable both arithmetic encoding and
decoding, but those who have philosophical objections to arithmetic coding can
add `--without-arith-enc` or `--without-arith-dec` to the `configure` command
line to disable encoding or decoding (respectively.)


### TurboJPEG Java Wrapper

Add `--with-java` to the `configure` command line to incorporate an optional
Java Native Interface (JNI) wrapper into the TurboJPEG shared library and build
the Java front-end classes to support it.  This allows the TurboJPEG shared
library to be used directly from Java applications.  See
[java/README](java/README) for more details.

You can set the `JAVAC`, `JAR`, and `JAVA` configure variables to specify
alternate commands for javac, jar, and java (respectively.)  You can also
set the `JAVACFLAGS` configure variable to specify arguments that should be
passed to the Java compiler when building the TurboJPEG classes, and
`JNI_CFLAGS` to specify arguments that should be passed to the C compiler when
building the JNI wrapper.  Run `configure --help` for more details.


Build Recipes
-------------


### 32-bit Build on 64-bit Linux

Add

    --host i686-pc-linux-gnu CFLAGS='-O3 -m32' LDFLAGS=-m32

to the `configure` command line.


### 64-bit Build on 64-bit OS X

Add

    --host x86_64-apple-darwin NASM=/opt/local/bin/nasm

to the `configure` command line.  NASM 2.07 or later from MacPorts or Homebrew
must be installed.  If using Homebrew, then replace `/opt/local` with
`/usr/local`.


### 32-bit Build on 64-bit OS X

Add

    --host i686-apple-darwin CFLAGS='-O3 -m32' LDFLAGS=-m32

to the `configure` command line.


### 64-bit Backward-Compatible Build on 64-bit OS X

Add

    --host x86_64-apple-darwin NASM=/opt/local/bin/nasm \
      CFLAGS='-mmacosx-version-min=10.5 -O3' \
      LDFLAGS='-mmacosx-version-min=10.5'

to the `configure` command line.  NASM 2.07 or later from MacPorts or Homebrew
must be installed.  If using Homebrew, then replace `/opt/local` with
`/usr/local`.


### 32-bit Backward-Compatible Build on OS X

Add

    --host i686-apple-darwin \
      CFLAGS='-mmacosx-version-min=10.5 -O3 -m32' \
      LDFLAGS='-mmacosx-version-min=10.5 -m32'

to the `configure` command line.


### 64-bit Build on 64-bit Solaris

Add

    --host x86_64-pc-solaris CFLAGS='-O3 -m64' LDFLAGS=-m64

to the `configure` command line.


### 32-bit Build on 64-bit FreeBSD

Add

    --host i386-unknown-freebsd CFLAGS='-O3 -m32' LDFLAGS=-m32

to the `configure` command line.  NASM 2.07 or later from FreeBSD ports must be
installed.


### Oracle Solaris Studio

Add

    CC=cc

to the `configure` command line.  libjpeg-turbo will automatically be built
with the maximum optimization level (-xO5) unless you override `CFLAGS`.

To build a 64-bit version of libjpeg-turbo using Oracle Solaris Studio, add

    --host x86_64-pc-solaris CC=cc CFLAGS='-xO5 -m64' LDFLAGS=-m64

to the `configure` command line.


### MinGW Build on Cygwin

Use CMake (see recipes below)


Building libjpeg-turbo for iOS
------------------------------

iOS platforms, such as the iPhone and iPad, use ARM processors, and all
currently supported models include NEON instructions.  Thus, they can take
advantage of libjpeg-turbo's SIMD extensions to significantly accelerate JPEG
compression/decompression.  This section describes how to build libjpeg-turbo
for these platforms.


### Additional build requirements

- For configurations that require [gas-preprocessor.pl]
  (https://raw.githubusercontent.com/libjpeg-turbo/gas-preprocessor/master/gas-preprocessor.pl),
  it should be installed in your `PATH`.


### ARMv7 (32-bit)

**gas-preprocessor.pl required**

The following scripts demonstrate how to build libjpeg-turbo to run on the
iPhone 3GS-4S/iPad 1st-3rd Generation and newer:

#### Xcode 4.2 and earlier (LLVM-GCC)

    IOS_PLATFORMDIR=/Developer/Platforms/iPhoneOS.platform
    IOS_SYSROOT=($IOS_PLATFORMDIR/Developer/SDKs/iPhoneOS*.sdk)

    export host_alias=arm-apple-darwin10
    export CC=${IOS_PLATFORMDIR}/Developer/usr/bin/arm-apple-darwin10-llvm-gcc-4.2
    export CFLAGS="-mfloat-abi=softfp -isysroot ${IOS_SYSROOT[0]} -O3 -march=armv7 -mcpu=cortex-a8 -mtune=cortex-a8 -mfpu=neon -miphoneos-version-min=3.0"

    cd {build_directory}
    sh {source_directory}/configure [additional configure flags]
    make

#### Xcode 4.3-4.6 (LLVM-GCC)

Same as above, but replace the first line with:

    IOS_PLATFORMDIR=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform

#### Xcode 5 and later (Clang)

    IOS_PLATFORMDIR=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform
    IOS_SYSROOT=($IOS_PLATFORMDIR/Developer/SDKs/iPhoneOS*.sdk)

    export host_alias=arm-apple-darwin10
    export CC=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
    export CFLAGS="-mfloat-abi=softfp -isysroot ${IOS_SYSROOT[0]} -O3 -arch armv7 -miphoneos-version-min=3.0"
    export CCASFLAGS="$CFLAGS -no-integrated-as"

    cd {build_directory}
    sh {source_directory}/configure [additional configure flags]
    make


### ARMv7s (32-bit)

**gas-preprocessor.pl required**

The following scripts demonstrate how to build libjpeg-turbo to run on the
iPhone 5/iPad 4th Generation and newer:

#### Xcode 4.5-4.6 (LLVM-GCC)

    IOS_PLATFORMDIR=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform
    IOS_SYSROOT=($IOS_PLATFORMDIR/Developer/SDKs/iPhoneOS*.sdk)

    export host_alias=arm-apple-darwin10
    export CC=${IOS_PLATFORMDIR}/Developer/usr/bin/arm-apple-darwin10-llvm-gcc-4.2
    export CFLAGS="-mfloat-abi=softfp -isysroot ${IOS_SYSROOT[0]} -O3 -march=armv7s -mcpu=swift -mtune=swift -mfpu=neon -miphoneos-version-min=6.0"

    cd {build_directory}
    sh {source_directory}/configure [additional configure flags]
    make

#### Xcode 5 and later (Clang)

Same as the ARMv7 build procedure for Xcode 5 and later, except replace the
compiler flags as follows:

    export CFLAGS="-mfloat-abi=softfp -isysroot ${IOS_SYSROOT[0]} -O3 -arch armv7s -miphoneos-version-min=6.0"


### ARMv8 (64-bit)

**gas-preprocessor.pl required if using Xcode < 6**

The following script demonstrates how to build libjpeg-turbo to run on the
iPhone 5S/iPad Mini 2/iPad Air and newer.

    IOS_PLATFORMDIR=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform
    IOS_SYSROOT=($IOS_PLATFORMDIR/Developer/SDKs/iPhoneOS*.sdk)

    export host_alias=aarch64-apple-darwin
    export CC=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
    export CFLAGS="-isysroot ${IOS_SYSROOT[0]} -O3 -arch arm64 -miphoneos-version-min=7.0 -funwind-tables"

    cd {build_directory}
    sh {source_directory}/configure [additional configure flags]
    make

Once built, lipo can be used to combine the ARMv7, v7s, and/or v8 variants into
a universal library.


Building libjpeg-turbo for Android
----------------------------------

Building libjpeg-turbo for Android platforms requires the
[Android NDK](https://developer.android.com/tools/sdk/ndk) and autotools.


### ARMv7 (32-bit)

The following is a general recipe script that can be modified for your specific
needs.

    # Set these variables to suit your needs
    NDK_PATH={full path to the "ndk" directory-- for example, /opt/android/sdk/ndk-bundle}
    BUILD_PLATFORM={the platform name for the NDK package you installed--
      for example, "windows-x86" or "linux-x86_64" or "darwin-x86_64"}
    TOOLCHAIN_VERSION={"4.8", "4.9", "clang3.5", etc.  This corresponds to a
      toolchain directory under ${NDK_PATH}/toolchains/.}
    ANDROID_VERSION={The minimum version of Android to support-- for example,
      "16", "19", etc.}

    # It should not be necessary to modify the rest
    HOST=arm-linux-androideabi
    SYSROOT=${NDK_PATH}/platforms/android-${ANDROID_VERSION}/arch-arm
    ANDROID_CFLAGS="-march=armv7-a -mfloat-abi=softfp -fprefetch-loop-arrays \
      --sysroot=${SYSROOT}"

    TOOLCHAIN=${NDK_PATH}/toolchains/${HOST}-${TOOLCHAIN_VERSION}/prebuilt/${BUILD_PLATFORM}
    export CPP=${TOOLCHAIN}/bin/${HOST}-cpp
    export AR=${TOOLCHAIN}/bin/${HOST}-ar
    export NM=${TOOLCHAIN}/bin/${HOST}-nm
    export CC=${TOOLCHAIN}/bin/${HOST}-gcc
    export LD=${TOOLCHAIN}/bin/${HOST}-ld
    export RANLIB=${TOOLCHAIN}/bin/${HOST}-ranlib
    export OBJDUMP=${TOOLCHAIN}/bin/${HOST}-objdump
    export STRIP=${TOOLCHAIN}/bin/${HOST}-strip
    cd {build_directory}
    sh {source_directory}/configure --host=${HOST} \
      CFLAGS="${ANDROID_CFLAGS} -O3 -fPIE" \
      CPPFLAGS="${ANDROID_CFLAGS}" \
      LDFLAGS="${ANDROID_CFLAGS} -pie" --with-simd ${1+"$@"}
    make


### ARMv8 (64-bit)

The following is a general recipe script that can be modified for your specific
needs.

    # Set these variables to suit your needs
    NDK_PATH={full path to the "ndk" directory-- for example, /opt/android/sdk/ndk-bundle}
    BUILD_PLATFORM={the platform name for the NDK package you installed--
      for example, "windows-x86" or "linux-x86_64" or "darwin-x86_64"}
    TOOLCHAIN_VERSION={"4.8", "4.9", "clang3.5", etc.  This corresponds to a
      toolchain directory under ${NDK_PATH}/toolchains/.}
    ANDROID_VERSION={The minimum version of Android to support.  "21" or later
      is required for a 64-bit build.}

    # It should not be necessary to modify the rest
    HOST=aarch64-linux-android
    SYSROOT=${NDK_PATH}/platforms/android-${ANDROID_VERSION}/arch-arm64
    ANDROID_CFLAGS="--sysroot=${SYSROOT}"

    TOOLCHAIN=${NDK_PATH}/toolchains/${HOST}-${TOOLCHAIN_VERSION}/prebuilt/${BUILD_PLATFORM}
    export CPP=${TOOLCHAIN}/bin/${HOST}-cpp
    export AR=${TOOLCHAIN}/bin/${HOST}-ar
    export NM=${TOOLCHAIN}/bin/${HOST}-nm
    export CC=${TOOLCHAIN}/bin/${HOST}-gcc
    export LD=${TOOLCHAIN}/bin/${HOST}-ld
    export RANLIB=${TOOLCHAIN}/bin/${HOST}-ranlib
    export OBJDUMP=${TOOLCHAIN}/bin/${HOST}-objdump
    export STRIP=${TOOLCHAIN}/bin/${HOST}-strip
    cd {build_directory}
    sh {source_directory}/configure --host=${HOST} \
      CFLAGS="${ANDROID_CFLAGS} -O3 -fPIE" \
      CPPFLAGS="${ANDROID_CFLAGS}" \
      LDFLAGS="${ANDROID_CFLAGS} -pie" --with-simd ${1+"$@"}
    make

If building for Android 4.0.x (API level < 16) or earlier, remove `-fPIE` from
`CFLAGS` and `-pie` from `LDFLAGS`.


Installing libjpeg-turbo
------------------------

To install libjpeg-turbo after it is built, replace `make` in the build
instructions with `make install`.

The `--prefix` argument to configure (or the `prefix` configure variable) can
be used to specify an installation directory of your choosing.  If you don't
specify an installation directory, then the default is to install libjpeg-turbo
under **/opt/libjpeg-turbo** and to place the libraries in
**/opt/libjpeg-turbo/lib32** (32-bit) or **/opt/libjpeg-turbo/lib64** (64-bit.)

The `bindir`, `datadir`, `docdir`, `includedir`, `libdir`, and `mandir`
configure variables allow a finer degree of control over where specific files in
the libjpeg-turbo distribution should be installed.  These variables can either
be specified at configure time or passed as arguments to `make install`.


Windows (Visual C++ or MinGW)
=============================


Build Requirements
------------------

- [CMake](http://www.cmake.org) v2.8.11 or later

- [NASM](http://www.nasm.us) or [YASM](http://yasm.tortall.net)
  * If using NASM, 0.98 or later is required for an x86 build.
  * If using NASM, 2.05 or later is required for an x86-64 build.
  * **nasm.exe**/**yasm.exe** should be in your `PATH`.

- Microsoft Visual C++ 2005 or later

  If you don't already have Visual C++, then the easiest way to get it is by
  installing the
  [Windows SDK](http://msdn.microsoft.com/en-us/windows/bb980924.aspx).
  The Windows SDK includes both 32-bit and 64-bit Visual C++ compilers and
  everything necessary to build libjpeg-turbo.

  * You can also use Microsoft Visual Studio Express/Community Edition, which
    is a free download.  (NOTE: versions prior to 2012 can only be used to
    build 32-bit code.)
  * If you intend to build libjpeg-turbo from the command line, then add the
    appropriate compiler and SDK directories to the `INCLUDE`, `LIB`, and
    `PATH` environment variables.  This is generally accomplished by
    executing `vcvars32.bat` or `vcvars64.bat` and `SetEnv.cmd`.
    `vcvars32.bat` and `vcvars64.bat` are part of Visual C++ and are located in
    the same directory as the compiler.  `SetEnv.cmd` is part of the Windows
    SDK.  You can pass optional arguments to `SetEnv.cmd` to specify a 32-bit
    or 64-bit build environment.

   ... OR ...

- MinGW

  [MSYS2](http://msys2.github.io/) or [tdm-gcc](http://tdm-gcc.tdragon.net/)
  recommended if building on a Windows machine.  Both distributions install a
  Start Menu link that can be used to launch a command prompt with the
  appropriate compiler paths automatically set.

- If building the TurboJPEG Java wrapper, JDK 1.5 or later is required.  This
  can be downloaded from <http://www.java.com>.


Out-of-Tree Builds
------------------

Binary objects, libraries, and executables are generated in the directory from
which CMake is executed (the "binary directory"), and this directory need not
necessarily be the same as the libjpeg-turbo source directory.  You can create
multiple independent binary directories, in which different versions of
libjpeg-turbo can be built from the same source tree using different compilers
or settings.  In the sections below, *{build_directory}* refers to the binary
directory, whereas *{source_directory}* refers to the libjpeg-turbo source
directory.  For in-tree builds, these directories are the same.


Build Procedure
---------------

NOTE: The build procedures below assume that CMake is invoked from the command
line, but all of these procedures can be adapted to the CMake GUI as
well.


### Visual C++ (Command Line)

    cd {build_directory}
    cmake -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Release [additional CMake flags] {source_directory}
    nmake

This will build either a 32-bit or a 64-bit version of libjpeg-turbo, depending
on which version of **cl.exe** is in the `PATH`.

The following files will be generated under *{build_directory}*:

**jpeg-static.lib**<br>
Static link library for the libjpeg API

**sharedlib/jpeg{version}.dll**<br>
DLL for the libjpeg API

**sharedlib/jpeg.lib**<br>
Import library for the libjpeg API

**turbojpeg-static.lib**<br>
Static link library for the TurboJPEG API

**turbojpeg.dll**<br>
DLL for the TurboJPEG API

**turbojpeg.lib**<br>
Import library for the TurboJPEG API

*{version}* is 62, 7, or 8, depending on whether libjpeg v6b (default), v7, or
v8 emulation is enabled.


### Visual C++ (IDE)

Choose the appropriate CMake generator option for your version of Visual Studio
(run `cmake` with no arguments for a list of available generators.)  For
instance:

    cd {build_directory}
    cmake -G"Visual Studio 10" [additional CMake flags] {source_directory}

NOTE: Add "Win64" to the generator name (for example, "Visual Studio 10
Win64") to build a 64-bit version of libjpeg-turbo.  A separate build directory
must be used for 32-bit and 64-bit builds.

You can then open **ALL_BUILD.vcproj** in Visual Studio and build one of the
configurations in that project ("Debug", "Release", etc.) to generate a full
build of libjpeg-turbo.

This will generate the following files under *{build_directory}*:

**{configuration}/jpeg-static.lib**<br>
Static link library for the libjpeg API

**sharedlib/{configuration}/jpeg{version}.dll**<br>
DLL for the libjpeg API

**sharedlib/{configuration}/jpeg.lib**<br>
Import library for the libjpeg API

**{configuration}/turbojpeg-static.lib**<br>
Static link library for the TurboJPEG API

**{configuration}/turbojpeg.dll**<br>
DLL for the TurboJPEG API

**{configuration}/turbojpeg.lib**<br>
Import library for the TurboJPEG API

*{configuration}* is Debug, Release, RelWithDebInfo, or MinSizeRel, depending
on the configuration you built in the IDE, and *{version}* is 62, 7, or 8,
depending on whether libjpeg v6b (default), v7, or v8 emulation is enabled.


### MinGW

NOTE: This assumes that you are building on a Windows machine using the MSYS
environment.  If you are cross-compiling on a Un*x platform (including Mac and
Cygwin), then see "Build Recipes" below.

    cd {build_directory}
    cmake -G"MSYS Makefiles" [additional CMake flags] {source_directory}
    make

This will generate the following files under *{build_directory}*:

**libjpeg.a**<br>
Static link library for the libjpeg API

**sharedlib/libjpeg-{version}.dll**<br>
DLL for the libjpeg API

**sharedlib/libjpeg.dll.a**<br>
Import library for the libjpeg API

**libturbojpeg.a**<br>
Static link library for the TurboJPEG API

**libturbojpeg.dll**<br>
DLL for the TurboJPEG API

**libturbojpeg.dll.a**<br>
Import library for the TurboJPEG API

*{version}* is 62, 7, or 8, depending on whether libjpeg v6b (default), v7, or
v8 emulation is enabled.


### Debug Build

Add `-DCMAKE_BUILD_TYPE=Debug` to the CMake command line.  Or, if building
with NMake, remove `-DCMAKE_BUILD_TYPE=Release` (Debug builds are the default
with NMake.)


### libjpeg v7 or v8 API/ABI Emulation

Add `-DWITH_JPEG7=1` to the CMake command line to build a version of
libjpeg-turbo that is API/ABI-compatible with libjpeg v7.  Add `-DWITH_JPEG8=1`
to the CMake command line to build a version of libjpeg-turbo that is
API/ABI-compatible with libjpeg v8.  See [README.md](README.md) for more
information about libjpeg v7 and v8 emulation.


### In-Memory Source/Destination Managers

When using libjpeg v6b or v7 API/ABI emulation, add `-DWITH_MEM_SRCDST=0` to
the CMake command line to build a version of libjpeg-turbo that lacks the
`jpeg_mem_src()` and `jpeg_mem_dest()` functions.  These functions were not
part of the original libjpeg v6b and v7 APIs, so removing them ensures strict
conformance with those APIs.  See [README.md](README.md) for more information.


### Arithmetic Coding Support

Since the patent on arithmetic coding has expired, this functionality has been
included in this release of libjpeg-turbo.  libjpeg-turbo's implementation is
based on the implementation in libjpeg v8, but it works when emulating libjpeg
v7 or v6b as well.  The default is to enable both arithmetic encoding and
decoding, but those who have philosophical objections to arithmetic coding can
add `-DWITH_ARITH_ENC=0` or `-DWITH_ARITH_DEC=0` to the CMake command line to
disable encoding or decoding (respectively.)


### TurboJPEG Java Wrapper

Add `-DWITH_JAVA=1` to the CMake command line to incorporate an optional Java
Native Interface (JNI) wrapper into the TurboJPEG shared library and build the
Java front-end classes to support it.  This allows the TurboJPEG shared library
to be used directly from Java applications.  See [java/README](java/README) for
more details.

If Java is not in your `PATH`, or if you wish to use an alternate JDK to
build/test libjpeg-turbo, then (prior to running CMake) set the `JAVA_HOME`
environment variable to the location of the JDK that you wish to use.  The
`Java_JAVAC_EXECUTABLE`, `Java_JAVA_EXECUTABLE`, and `Java_JAR_EXECUTABLE`
CMake variables can also be used to specify alternate commands or locations for
javac, jar, and java (respectively.)  You can also set the `JAVACFLAGS` CMake
variable to specify arguments that should be passed to the Java compiler when
building the TurboJPEG classes.


Build Recipes
-------------


### 32-bit MinGW Build on Un*x (including Mac and Cygwin)

Create a file called **toolchain.cmake** under *{build_directory}*, with the
following contents:

    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_SYSTEM_PROCESSOR X86)
    set(CMAKE_C_COMPILER {mingw_binary_path}/i686-w64-mingw32-gcc)
    set(CMAKE_RC_COMPILER {mingw_binary_path}/i686-w64-mingw32-windres)

*{mingw\_binary\_path}* is the directory under which the MinGW binaries are
located (usually **/usr/bin**.)  Next, execute the following commands:

    cd {build_directory}
    cmake -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
      [additional CMake flags] {source_directory}
    make


### 64-bit MinGW Build on Un*x (including Mac and Cygwin)

Create a file called **toolchain.cmake** under *{build_directory}*, with the
following contents:

    set(CMAKE_SYSTEM_NAME Windows)
    set(CMAKE_SYSTEM_PROCESSOR AMD64)
    set(CMAKE_C_COMPILER {mingw_binary_path}/x86_64-w64-mingw32-gcc)
    set(CMAKE_RC_COMPILER {mingw_binary_path}/x86_64-w64-mingw32-windres)

*{mingw\_binary\_path}* is the directory under which the MinGW binaries are
located (usually **/usr/bin**.)  Next, execute the following commands:

    cd {build_directory}
    cmake -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake \
      [additional CMake flags] {source_directory}
    make


Installing libjpeg-turbo
------------------------

You can use the build system to install libjpeg-turbo (as opposed to creating
an installer package.)  To do this, run `make install` or `nmake install`
(or build the "install" target in the Visual Studio IDE.)  Running
`make uninstall` or `nmake uninstall` (or building the "uninstall" target in
the Visual Studio IDE) will uninstall libjpeg-turbo.

The `CMAKE_INSTALL_PREFIX` CMake variable can be modified in order to install
libjpeg-turbo into a directory of your choosing.  If you don't specify
`CMAKE_INSTALL_PREFIX`, then the default is:

**c:\libjpeg-turbo**<br>
Visual Studio 32-bit build

**c:\libjpeg-turbo64**<br>
Visual Studio 64-bit build

**c:\libjpeg-turbo-gcc**<br>
MinGW 32-bit build

**c:\libjpeg-turbo-gcc64**<br>
MinGW 64-bit build


Creating Distribution Packages
==============================

The following commands can be used to create various types of distribution
packages:


Linux
-----

    make rpm

Create Red Hat-style binary RPM package.  Requires RPM v4 or later.

    make srpm

This runs `make dist` to create a pristine source tarball, then creates a
Red Hat-style source RPM package from the tarball.  Requires RPM v4 or later.

    make deb

Create Debian-style binary package.  Requires dpkg.


Mac
---

    make dmg

Create Mac package/disk image.  This requires pkgbuild and productbuild, which
are installed by default on OS X 10.7 and later and which can be obtained by
installing Xcode 3.2.6 (with the "Unix Development" option) on OS X 10.6.
Packages built in this manner can be installed on OS X 10.5 and later, but they
must be built on OS X 10.6 or later.

    make udmg [BUILDDIR32={32-bit build directory}]

On 64-bit OS X systems, this creates a Mac package/disk image that contains
universal i386/x86-64 binaries.  You should first configure a 32-bit
out-of-tree build of libjpeg-turbo, then configure a 64-bit out-of-tree build,
then run `make udmg` from the 64-bit build directory.  The build system will
look for the 32-bit build under *{source_directory}*/osxx86 by default, but you
can override this by setting the `BUILDDIR32` variable on the make command line
as shown above.

    make iosdmg [BUILDDIR32={32-bit build directory}] \
      [BUILDDIRARMV7={ARMv7 build directory}] \
      [BUILDDIRARMV7S={ARMv7s build directory}] \
      [BUILDDIRARMV8={ARMv8 build directory}]

This creates a Mac package/disk image in which the libjpeg-turbo libraries
contain ARM architectures necessary to build iOS applications.  If building on
an x86-64 system, the binaries will also contain the i386 architecture, as with
`make udmg` above.  You should first configure ARMv7, ARMv7s, and/or ARMv8
out-of-tree builds of libjpeg-turbo (see "Building libjpeg-turbo for iOS"
above.)  If you are building an x86-64 version of libjpeg-turbo, you should
configure a 32-bit out-of-tree build as well.  Next, build libjpeg-turbo as you
would normally, using an out-of-tree build.  When it is built, run `make
iosdmg` from the build directory.  The build system will look for the ARMv7
build under *{source_directory}*/iosarmv7 by default, the ARMv7s build under
*{source_directory}*/iosarmv7s by default, the ARMv8 build under
*{source_directory}*/iosarmv8 by default, and (if applicable) the 32-bit build
under *{source_directory}*/osxx86 by default, but you can override this by
setting the `BUILDDIR32`, `BUILDDIRARMV7`, `BUILDDIRARMV7S`, and/or
`BUILDDIRARMV8` variables on the `make` command line as shown above.

NOTE: If including an ARMv8 build in the package, then you may need to use
Xcode's version of lipo instead of the operating system's.  To do this, pass
an argument of `LIPO="xcrun lipo"` on the make command line.

    make cygwinpkg

Build a Cygwin binary package.


Windows
-------

If using NMake:

    cd {build_directory}
    nmake installer

If using MinGW:

    cd {build_directory}
    make installer

If using the Visual Studio IDE, build the "installer" target.

The installer package (libjpeg-turbo-*{version}*[-gcc|-vc][64].exe) will be
located under *{build_directory}*.  If building using the Visual Studio IDE,
then the installer package will be located in a subdirectory with the same name
as the configuration you built (such as *{build_directory}*\Debug\ or
*{build_directory}*\Release\).

Building a Windows installer requires the
[Nullsoft Install System](http://nsis.sourceforge.net/).  makensis.exe should
be in your `PATH`.


Regression testing
==================

The most common way to test libjpeg-turbo is by invoking `make test` (Un*x) or
`nmake test` (Windows command line) or by building the "RUN_TESTS" target
(Visual Studio IDE), once the build has completed.  This runs a series of tests
to ensure that mathematical compatibility has been maintained between
libjpeg-turbo and libjpeg v6b.  This also invokes the TurboJPEG unit tests,
which ensure that the colorspace extensions, YUV encoding, decompression
scaling, and other features of the TurboJPEG C and Java APIs are working
properly (and, by extension, that the equivalent features of the underlying
libjpeg API are also working.)

Invoking `make testclean` (Un*x) or `nmake testclean` (Windows command line) or
building the "testclean" target (Visual Studio IDE) will clean up the output
images generated by the tests.

On Un*x platforms, more extensive tests of the TurboJPEG C and Java wrappers
can be run by invoking `make tjtest`.  These extended TurboJPEG tests
essentially iterate through all of the available features of the TurboJPEG APIs
that are not covered by the TurboJPEG unit tests (including the lossless
transform options) and compare the images generated by each feature to images
generated using the equivalent feature in the libjpeg API.  The extended
TurboJPEG tests are meant to test for regressions in the TurboJPEG wrappers,
not in the underlying libjpeg API library.

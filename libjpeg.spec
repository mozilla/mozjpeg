%define LIBVER 62.1.0
Summary: A library for manipulating JPEG image format files (with SIMD support)
Summary(ja): JPEG 形式画像ファイルを扱う為のライブラリ (x86 SIMD 対応版)
Name: libjpeg
Version: 6bx1.02
Release: 1
License: distributable
Group: System Environment/Libraries
Source0: http://cetus.sakura.ne.jp/softlab/jpeg-x86simd/sources/jpegsrc-6b-x86simd-1.02.tar.bz2
Buildroot: %{_tmppath}/%{name}-%{version}-root
ExclusiveArch: %{ix86}
BuildPrereq: nasm >= 0.98.25

%package devel
Summary: Development tools for programs which will use the libjpeg library.
Summary(ja): libjpeg ライブラリを使うプログラム向け開発ツール
Group: Development/Libraries
Requires: libjpeg = %{version}-%{release}

%description
The libjpeg package contains a library of functions for manipulating
JPEG images, as well as simple client programs for accessing the
libjpeg functions.  Libjpeg client programs include cjpeg, djpeg,
jpegtran, rdjpgcom and wrjpgcom.  Cjpeg compresses an image file into
JPEG format.  Djpeg decompresses a JPEG file into a regular image
file.  Jpegtran can perform various useful transformations on JPEG
files.  Rdjpgcom displays any text comments included in a JPEG file.
Wrjpgcom inserts text comments into a JPEG file.

The libjpeg library in this package uses SIMD instructions if available.
On a processor that supports SIMD instructions (MMX, SSE, etc),
it runs 2-3 times faster than the original version of libjpeg.

%description -l ja
libjpeg パッケージには JPEG 画像を扱う為に必要なライブラリと，
libjpeg 関数にアクセスする為の簡単なクライアントプログラムが
収められています．libjpeg クライアントプログラムには cjpeg, djpeg,
jpegtran, rdjpgcom, wrjpgcom があります．cjpeg は画像ファイルを
JPEG 形式に圧縮します．djpeg は JPEG ファイルを通常の画像ファイルに
展開します．jpegtran は JPEG ファイルに様々な変換を施すことが出来ます．
rdjpgcom は JPEG ファイルに含まれているテキスト形式のコメントを表示し，
wrjpgcom は JPEG ファイルにテキスト形式のコメントを追加します．

このパッケージに収められている libjpeg ライブラリは、x86 SIMD 対応版です。
MMX や SSE などの SIMD 演算機能を装備しているプロセッサ上で動作させると、
オリジナル版の libjpeg ライブラリと比較して 2〜3倍程度の速度で動作します。

%description devel
The libjpeg-devel package includes the header files and static libraries
necessary for developing programs which will manipulate JPEG files using
the libjpeg library.

If you are going to develop programs which will manipulate JPEG images,
you should install libjpeg-devel.  You'll also need to have the libjpeg
package installed.

%description devel -l ja
libjpeg-devel パッケージには，libjpeg ライブラリを使って JPEG ファイルを
扱うプログラムを開発するのに必要なヘッダファイルとスタティックライブラリが
収められています．

JPEG 画像を扱うプログラムを開発する際には，libjpeg-devel を
インストールして下さい．同時に libjpeg パッケージもインストールする
必要があります．

%prep
%setup -q -n jpeg-6bx
# suppress "libtoolize --copy --force"
mv configure.in configure.in_

%build
%configure --enable-shared --enable-static

make libdir=%{_libdir} %{?_smp_mflags}
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$PWD make test

%install
rm -rf $RPM_BUILD_ROOT

%makeinstall
#strip -R .comment $RPM_BUILD_ROOT/usr/bin/* || :
#/sbin/ldconfig -n $RPM_BUILD_ROOT/%{_libdir}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc usage.doc wizard.doc README
%{_libdir}/libjpeg.so.*
%{_bindir}/*
%{_mandir}/*/*

%files devel
%defattr(-,root,root)
%doc libjpeg.doc coderules.doc structure.doc example.c
%doc simd_*.txt
%{_libdir}/*.a
%{_libdir}/*.la
%{_libdir}/*.so
/usr/include/*.h

%changelog
* Sat Feb 04 2006 MIYASAKA Masaru <alkaid@coral.ocn.ne.jp> - 6bx1.02-1
- upgraded to 6bx1.02

* Thu Jan 26 2006 MIYASAKA Masaru <alkaid@coral.ocn.ne.jp> - 6bx1.01-1
- upgraded to 6bx1.01

* Thu Mar 24 2005 MIYASAKA Masaru <alkaid@coral.ocn.ne.jp> - 6bx1.0-1
- based on 6b-33 from Fedora Core 3 and modified for SIMD-extended libjpeg
- added Japanese summary and description, which is delivered from Vine Linux
- moved wizard.doc to main package

* Thu Oct  7 2004 Matthias Clasen <mclasen@redhat.com> - 6b-33
- Add URL.  (#134791)

* Tue Jun 15 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Tue Mar 02 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Fri Feb 13 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Thu Sep 25 2003 Jeremy Katz <katzj@redhat.com> 6b-30
- rebuild to fix gzipped file md5sums (#91211)

* Tue Sep 23 2003 Florian La Roche <Florian.LaRoche@redhat.de>
- do not set rpath

* Wed Jun 04 2003 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Thu Feb 13 2003 Elliot Lee <sopwith@redhat.com> 6b-27
- Add libjpeg-shared.patch to fix shlibs on powerpc

* Tue Feb 04 2003 Florian La Roche <Florian.LaRoche@redhat.de>
- add symlink to shared lib

* Wed Jan 22 2003 Tim Powers <timp@redhat.com>
- rebuilt

* Mon Jan  6 2003 Jonathan Blandford <jrb@redhat.com>
- add docs, #76508

* Fri Dec 13 2002 Elliot Lee <sopwith@redhat.com> 6b-23
- Merge in multilib changes
- _smp_mflags

* Tue Sep 10 2002 Than Ngo <than@redhat.com> 6b-22
- use %%_libdir

* Fri Jun 21 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Thu May 23 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Thu Jan 31 2002 Bernhard Rosenkraenzer <bero@redhat.com> 6b-19
- Fix bug #59011

* Mon Jan 28 2002 Bernhard Rosenkraenzer <bero@redhat.com> 6b-18
- Fix bug #58982

* Wed Jan 09 2002 Tim Powers <timp@redhat.com>
- automated rebuild

* Tue Jul 24 2001 Bill Nottingham <notting@redhat.com>
- require libjpeg = %%{version}

* Sun Jun 24 2001 Elliot Lee <sopwith@redhat.com>
- Bump release + rebuild.

* Mon Dec 11 2000 Than Ngo <than@redhat.com>
- rebuilt with the fixed fileutils
- use %%{_tmppath}

* Wed Nov  8 2000 Bernhard Rosenkraenzer <bero@redhat.com>
- fix a typo (strip -R .comment, not .comments)

* Thu Jul 13 2000 Prospector <bugzilla@redhat.com>
- automatic rebuild

* Sat Jun 17 2000 Bernhard Rosenkraenzer <bero@redhat.com>
- FHSify
- add some C++ tweaks to the headers as suggested by bug #9822)

* Wed May  5 2000 Bill Nottingham <notting@redhat.com>
- configure tweaks for ia64; remove alpha patch (it's pointless)

* Sat Feb  5 2000 Bernhard Rosenkr粮zer <bero@redhat.com>
- rebuild to get compressed man pages
- fix description
- some minor tweaks to the spec file
- add docs
- fix build on alpha (alphaev6 stuff)

* Sun Mar 21 1999 Cristian Gafton <gafton@redhat.com> 
- auto rebuild in the new build environment (release 9)

* Wed Jan 13 1999 Cristian Gafton <gafton@redhat.com>
- patch to build on arm
- build for glibc 2.1

* Mon Oct 12 1998 Cristian Gafton <gafton@redhat.com>
- strip binaries

* Mon Aug  3 1998 Jeff Johnson <jbj@redhat.com>
- fix buildroot problem.

* Tue Jun 09 1998 Prospector System <bugs@redhat.com>
- translations modified for de

* Thu Jun 04 1998 Marc Ewing <marc@redhat.com>
- up to release 4
- remove patch that set (improper) soname - libjpeg now does it itself

* Thu May 07 1998 Prospector System <bugs@redhat.com>
- translations modified for de, fr, tr

* Fri May 01 1998 Cristian Gafton <gafton@redhat.com>
- fixed build on manhattan

* Wed Apr 08 1998 Cristian Gafton <gafton@redhat.com>
- upgraded to version 6b

* Wed Oct 08 1997 Donnie Barnes <djb@redhat.com>
- new package to remove jpeg stuff from libgr and put in it's own package

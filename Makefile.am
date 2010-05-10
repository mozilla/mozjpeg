lib_LTLIBRARIES = libjpeg.la libturbojpeg.la
libjpeg_la_LDFLAGS = -version-number 62:0:0 -no-undefined
libturbojpeg_la_LDFLAGS = -avoid-version -no-undefined
include_HEADERS = jerror.h jmorecfg.h jpeglib.h turbojpeg.h
nodist_include_HEADERS = jconfig.h

HDRS = jchuff.h jdct.h jdhuff.h jerror.h jinclude.h jmemsys.h jmorecfg.h \
	jpegint.h jpeglib.h jversion.h jsimd.h jsimddct.h

libjpeg_la_SOURCES = $(HDRS) jcapimin.c jcapistd.c jccoefct.c jccolor.c \
	jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c \
	jcomapi.c jcparam.c jcphuff.c jcprepct.c jcsample.c jctrans.c \
	jdapimin.c jdapistd.c jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c \
	jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c jdmaster.c \
	jdmerge.c jdphuff.c jdpostct.c jdsample.c jdtrans.c jerror.c \
	jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c \
	jidctred.c jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c

libturbojpeg_la_SOURCES = $(libjpeg_la_SOURCES) turbojpegl.c turbojpeg.h \
	turbojpeg-mapfile

if VERSION_SCRIPT

libturbojpeg_la_LDFLAGS += $(VERSION_SCRIPT_FLAG)$(srcdir)/turbojpeg-mapfile

endif

if WITH_SIMD

SUBDIRS = simd
libjpeg_la_LIBADD = simd/libsimd.la
libturbojpeg_la_LIBADD = simd/libsimd.la

else

libjpeg_la_SOURCES += jsimd_none.c

endif

TSTHDRS = rrutil.h rrtimer.h

noinst_PROGRAMS = jpgtest jpegut cjpeg djpeg jpegtran rdjpgcom wrjpgcom

jpgtest_SOURCES = $(TSTHDRS) jpgtest.cxx bmp.h bmp.c

jpgtest_LDADD = libturbojpeg.la

jpegut_SOURCES = $(TSTHDRS) jpegut.c bmp.h bmp.c

jpegut_LDADD = libturbojpeg.la

cjpeg_SOURCES = cdjpeg.h cderror.h cdjpeg.c cjpeg.c rdbmp.c rdgif.c \
	rdppm.c rdswitch.c rdtarga.c 

cjpeg_LDADD = libjpeg.la

cjpeg_CFLAGS = -DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED \
	-DTARGA_SUPPORTED

djpeg_SOURCES = cdjpeg.h cderror.h cdjpeg.c djpeg.c rdcolmap.c rdswitch.c \
	wrbmp.c wrgif.c wrppm.c wrtarga.c

djpeg_LDADD = libjpeg.la

djpeg_CFLAGS = -DBMP_SUPPORTED -DGIF_SUPPORTED -DPPM_SUPPORTED \
	-DTARGA_SUPPORTED

jpegtran_SOURCES = jpegtran.c rdswitch.c cdjpeg.c transupp.c transupp.h

jpegtran_LDADD = libjpeg.la

rdjpgcom_SOURCES = rdjpgcom.c

rdjpgcom_LDADD = libjpeg.la

wrjpgcom_SOURCES = wrjpgcom.c

wrjpgcom_LDADD = libjpeg.la


DISTMANS= cjpeg.1 djpeg.1 jpegtran.1 rdjpgcom.1 wrjpgcom.1

DOCS= README install.doc usage.doc wizard.doc example.c libjpeg.doc \
	structure.doc coderules.doc filelist.doc jconfig.doc change.log \
	README-turbo.txt rdrle.c wrrle.c LICENSE.txt LGPL.txt BUILDING.txt \
	ChangeLog.txt

TESTFILES= testorig.jpg testorig.ppm testimg.bmp testimgflt.jpg \
	testimgfst.jpg testimgint.jpg testimgp.jpg testimgfst.ppm testimgint.ppm

EXTRA_DIST = win release $(DOCS) $(DISTMANS) $(TESTFILES)

dist-hook:
	rm -rf `find $(distdir) -name .svn`


test: testclean cjpeg djpeg jpegtran jpegut
	./jpegut
	./cjpeg -dct int -outfile testoutint.jpg $(srcdir)/testorig.ppm
	./cjpeg -dct fast -opt -outfile testoutfst.jpg $(srcdir)/testorig.ppm
	./cjpeg -dct float -outfile testoutflt.jpg $(srcdir)/testorig.ppm
	cmp $(srcdir)/testimgint.jpg testoutint.jpg
	cmp $(srcdir)/testimgfst.jpg testoutfst.jpg
	cmp $(srcdir)/testimgflt.jpg testoutflt.jpg
	./djpeg -dct int -fast -ppm -outfile testoutint.ppm $(srcdir)/testorig.jpg
	./djpeg -dct fast -ppm -outfile testoutfst.ppm $(srcdir)/testorig.jpg
	./djpeg -dct float -ppm -outfile testoutflt.ppm $(srcdir)/testorig.jpg
	cmp $(srcdir)/testimgint.ppm testoutint.ppm
	cmp $(srcdir)/testimgfst.ppm testoutfst.ppm
	cmp $(srcdir)/testorig.ppm testoutflt.ppm
	./djpeg -dct int -bmp -colors 256 -outfile testout.bmp  $(srcdir)/testorig.jpg
	cmp $(srcdir)/testimg.bmp testout.bmp
	./cjpeg -dct int -progressive -outfile testoutp.jpg $(srcdir)/testorig.ppm
	cmp $(srcdir)/testimgp.jpg testoutp.jpg
	./jpegtran -outfile testoutt.jpg testoutp.jpg
	cmp $(srcdir)/testimgint.jpg testoutt.jpg

testclean:
	$(RM) testout*
	$(RM) *_GRAYQ[0-9]*.bmp
	$(RM) *_GRAYQ[0-9]*.ppm
	$(RM) *_GRAYQ[0-9]*.jpg
	$(RM) *_420Q[0-9]*.bmp
	$(RM) *_420Q[0-9]*.ppm
	$(RM) *_420Q[0-9]*.jpg
	$(RM) *_422Q[0-9]*.bmp
	$(RM) *_422Q[0-9]*.ppm
	$(RM) *_422Q[0-9]*.jpg
	$(RM) *_444Q[0-9]*.bmp
	$(RM) *_444Q[0-9]*.ppm
	$(RM) *_444Q[0-9]*.jpg

rpm: all
	sh $(srcdir)/release/makerpm ${PACKAGE_NAME} ${VERSION} ${BUILD} \
		${RPMARCH} ${srcdir}

srpm: dist-gzip
	sh $(srcdir)/release/makesrpm ${PACKAGE_NAME} ${VERSION} ${BUILD} ${srcdir}

deb: all
	sh $(srcdir)/release/makedpkg ${PACKAGE_NAME} ${VERSION} ${BUILD} \
		${DEBARCH} ${srcdir}

if X86_64

udmg: all
	sh $(srcdir)/release/makemacpkg ${PACKAGE_NAME} ${VERSION} ${BUILD} \
		${srcdir} universal

endif

dmg: all
	sh $(srcdir)/release/makemacpkg ${PACKAGE_NAME} ${VERSION} ${BUILD} ${srcdir}

if X86_64

sunpkg: all
	sh $(srcdir)/release/makesunpkg ${PACKAGE_NAME} ${VERSION} ${BUILD} \
		${DEBARCH} ${srcdir} $(CC) $(CXX) combined

nsi: all
	makensis -nocd -DVERSION=$(VERSION) -DAPPNAME=libjpeg-turbo-gcc64 \
		-DWLIBDIR=.libs -DWSRCDIR=$(srcdir) -DWBLDDIR=. -DWHDRDIR=. -DWIN64 \
		-DPLATFORM="GCC 64-bit" -DGCC $(srcdir)/release/libjpeg-turbo.nsi

else

sunpkg: all
	sh $(srcdir)/release/makesunpkg ${PACKAGE_NAME} ${VERSION} ${BUILD} \
		${DEBARCH} ${srcdir} $(CC) $(CXX)

nsi: all
	makensis -nocd -DVERSION=$(VERSION) -DAPPNAME=libjpeg-turbo-gcc \
		-DWLIBDIR=.libs -DWSRCDIR=$(srcdir) -DWBLDDIR=. -DWHDRDIR=. \
		-DPLATFORM="GCC" -DGCC $(srcdir)/release/libjpeg-turbo.nsi

endif

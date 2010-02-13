lib_LTLIBRARIES = libjpeg.la libturbojpeg.la
libjpeg_la_LDFLAGS = -version-number 62:0:0 -no-undefined
libturbojpeg_la_LDFLAGS = -avoid-version -no-undefined
include_HEADERS = jconfig.h jerror.h jmorecfg.h jpeglib.h turbojpeg.h

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

libturbojpeg_la_SOURCES = $(libjpeg_la_SOURCES) turbojpegl.c turbojpeg.h

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

jpegtran_SOURCES = jpegtran.c rdswitch.c cdjpeg.c transupp.c

jpegtran_LDADD = libjpeg.la

rdjpgcom_SOURCES = wrjpgcom.c

rdjpgcom_LDADD = libjpeg.la

wrjpgcom_SOURCES = wrjpgcom.c

wrjpgcom_LDADD = libjpeg.la

test: testclean cjpeg djpeg jpegtran
	./jpegut
	./djpeg -dct int -ppm -outfile testout.ppm  $(srcdir)/testorig.jpg
	./djpeg -dct int -bmp -colors 256 -outfile testout.bmp  $(srcdir)/testorig.jpg
	./cjpeg -dct int -outfile testout.jpg  $(srcdir)/testimg.ppm
	./djpeg -dct int -ppm -outfile testoutp.ppm $(srcdir)/testprog.jpg
	./cjpeg -dct int -progressive -opt -outfile testoutp.jpg $(srcdir)/testimg.ppm
	./jpegtran -outfile testoutt.jpg $(srcdir)/testprog.jpg
	cmp $(top_srcdir)/testimg.ppm testout.ppm
	cmp $(top_srcdir)/testimg.bmp testout.bmp
	cmp $(top_srcdir)/testimg.jpg testout.jpg
	cmp $(top_srcdir)/testimg.ppm testoutp.ppm
	cmp $(top_srcdir)/testimgp.jpg testoutp.jpg
	cmp $(top_srcdir)/testorig.jpg testoutt.jpg

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

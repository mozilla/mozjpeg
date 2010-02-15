vpath %.c simd
vpath %.asm simd

include win/Makerules

TARGETS = jpeg62.dll \
          jpeg.lib \
          jpeg-static.lib \
          cjpeg.exe \
          djpeg.exe \
          jpegtran.exe \
          rdjpgcom.exe \
          wrjpgcom.exe \
          turbojpeg.dll \
          turbojpeg.lib \
          turbojpeg-static.lib \
          jpgtest.exe \
          jpegut.exe

LOBJS = jcapimin.obj \
        jcapistd.obj \
        jccoefct.obj \
        jccolor.obj \
        jcdctmgr.obj \
        jchuff.obj \
        jcinit.obj \
        jcmainct.obj \
        jcmarker.obj \
        jcmaster.obj \
        jcomapi.obj \
        jcparam.obj \
        jcphuff.obj \
        jcprepct.obj \
        jcsample.obj \
        jctrans.obj \
        jdapimin.obj \
        jdapistd.obj \
        jdatadst.obj \
        jdatasrc.obj \
        jdcoefct.obj \
        jdcolor.obj \
        jddctmgr.obj \
        jdhuff.obj \
        jdinput.obj \
        jdmainct.obj \
        jdmarker.obj \
        jdmaster.obj \
        jdmerge.obj \
        jdphuff.obj \
        jdpostct.obj \
        jdsample.obj \
        jdtrans.obj \
        jerror.obj \
        jfdctflt.obj \
        jfdctfst.obj \
        jfdctint.obj \
        jidctflt.obj \
        jidctfst.obj \
        jidctint.obj \
        jidctred.obj \
        jquant1.obj \
        jquant2.obj \
        jutils.obj \
        jmemmgr.obj \
        jmemnobs.obj

ifeq ($(WITH_SIMD), yes)

 SIMD_OBJS = jsimd_i386.obj \
             jsimdcpu.obj \
             jccolmmx.obj \
             jdcolmmx.obj \
             jcsammmx.obj \
             jdsammmx.obj \
             jdmermmx.obj \
             jcqntmmx.obj \
             jfmmxfst.obj \
             jfmmxint.obj \
             jimmxred.obj \
             jimmxint.obj \
             jimmxfst.obj \
             jcqnt3dn.obj \
             jf3dnflt.obj \
             ji3dnflt.obj \
             jcqntsse.obj \
             jfsseflt.obj \
             jisseflt.obj \
             jccolss2.obj \
             jdcolss2.obj \
             jcsamss2.obj \
             jdsamss2.obj \
             jdmerss2.obj \
             jcqnts2i.obj \
             jfss2fst.obj \
             jfss2int.obj \
             jiss2red.obj \
             jiss2int.obj \
             jiss2fst.obj \
             jcqnts2f.obj \
             jiss2flt.obj

 LOBJS := $(LOBJS) $(SIMD_OBJS)

else

 LOBJS := $(LOBJS) jsimd_none.obj

endif

OBJS := $(LOBJS) \
        cdjpeg.obj \
        cjpeg.obj \
        djpeg.obj \
        jpegtran.obj \
        rdbmp.obj \
        rdcolmap.obj \
        rdgif.obj \
        rdjpgcom.obj \
        rdppm.obj \
        rdtarga.obj \
        rdswitch.obj \
        rdtarga.obj \
        transupp.obj \
        wrbmp.obj \
        wrgif.obj \
        wrppm.obj \
        wrtarga.obj \
        wrjpgcom.obj \
        turbojpegl.obj \
        jpgtest.obj \
        jpegut.obj \
        bmp.obj

all: $(TARGETS)

clean:
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard *.h)
$(OBJS): $(HDRS)

ifeq ($(WITH_SIMD), yes)
 SIMD_HDRS := $(wildcard simd/*.inc)
 $(SIMD_OBJS): $(SIMD_HDRS)
endif



jpeg-static.lib: $(LOBJS)
	$(AR) -out:$@ $^

jpeg62.dll jpeg.lib: $(LOBJS) win/jpeg.def
	$(LINK) $(LDFLAGS) -dll -out:jpeg62.dll -implib:jpeg.lib -def:win/jpeg.def \
		$(LOBJS)

cjpeg.exe: cdjpeg.obj cjpeg.obj rdbmp.obj rdgif.obj rdppm.obj rdswitch.obj \
	rdtarga.obj jpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

djpeg.exe: cdjpeg.obj djpeg.obj rdcolmap.obj rdswitch.obj wrbmp.obj wrgif.obj \
	wrppm.obj wrtarga.obj jpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

jpegtran.exe: cdjpeg.obj jpegtran.obj rdswitch.obj transupp.obj jpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

rdjpgcom.exe: rdjpgcom.obj jpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

wrjpgcom.exe: wrjpgcom.obj jpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^


turbojpeg-static.lib: turbojpegl.obj $(LOBJS)
	$(AR) -out:$@ $^

turbojpeg-dll.obj: turbojpegl.c
	$(CC) $(CFLAGS) -DDLLDEFINE -c $< -Fo$@

turbojpeg.dll turbojpeg.lib: turbojpeg-dll.obj $(LOBJS)
	$(LINK) $(LDFLAGS) -dll -out:turbojpeg.dll -implib:turbojpeg.lib $^


jpgtest.exe: jpgtest.obj bmp.obj turbojpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

jpegut.exe: jpegut.obj turbojpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^


dist: all
	$(RM) libjpeg-turbo.exe
	makensis /nocd //DVERSION=$(VERSION) release/libjpeg-turbo.nsi || \
	makensis /nocd /DVERSION=$(VERSION) release/libjpeg-turbo.nsi  # Cygwin doesn't like the //


test: testclean cjpeg.exe djpeg.exe jpegtran.exe
	jpegut
	djpeg -dct int -ppm -outfile testout.ppm  testorig.jpg
	djpeg -dct int -bmp -colors 256 -outfile testout.bmp  testorig.jpg
	cjpeg -dct int -outfile testout.jpg  testimg.ppm
	djpeg -dct int -ppm -outfile testoutp.ppm testprog.jpg
	cjpeg -dct int -progressive -opt -outfile testoutp.jpg testimg.ppm
	jpegtran -outfile testoutt.jpg testprog.jpg
	cmp testimg.ppm testout.ppm
	cmp testimg.bmp testout.bmp
	cmp testimg.jpg testout.jpg
	cmp testimg.ppm testoutp.ppm
	cmp testimgp.jpg testoutp.jpg
	cmp testorig.jpg testoutt.jpg

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

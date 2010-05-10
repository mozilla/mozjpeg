vpath %.c simd
vpath %.asm simd

include win/Makerules

TARGETS = $(ODIR)/jpeg62.dll \
          $(ODIR)/jpeg.lib \
          $(ODIR)/jpeg-static.lib \
          $(ODIR)/cjpeg.exe \
          $(ODIR)/djpeg.exe \
          $(ODIR)/jpegtran.exe \
          $(ODIR)/rdjpgcom.exe \
          $(ODIR)/wrjpgcom.exe \
          $(ODIR)/turbojpeg.dll \
          $(ODIR)/turbojpeg.lib \
          $(ODIR)/turbojpeg-static.lib \
          $(ODIR)/jpgtest.exe \
          $(ODIR)/jpegut.exe

LOBJS = $(ODIR)/jcapimin.obj \
        $(ODIR)/jcapistd.obj \
        $(ODIR)/jccoefct.obj \
        $(ODIR)/jccolor.obj \
        $(ODIR)/jcdctmgr.obj \
        $(ODIR)/jchuff.obj \
        $(ODIR)/jcinit.obj \
        $(ODIR)/jcmainct.obj \
        $(ODIR)/jcmarker.obj \
        $(ODIR)/jcmaster.obj \
        $(ODIR)/jcomapi.obj \
        $(ODIR)/jcparam.obj \
        $(ODIR)/jcphuff.obj \
        $(ODIR)/jcprepct.obj \
        $(ODIR)/jcsample.obj \
        $(ODIR)/jctrans.obj \
        $(ODIR)/jdapimin.obj \
        $(ODIR)/jdapistd.obj \
        $(ODIR)/jdatadst.obj \
        $(ODIR)/jdatasrc.obj \
        $(ODIR)/jdcoefct.obj \
        $(ODIR)/jdcolor.obj \
        $(ODIR)/jddctmgr.obj \
        $(ODIR)/jdhuff.obj \
        $(ODIR)/jdinput.obj \
        $(ODIR)/jdmainct.obj \
        $(ODIR)/jdmarker.obj \
        $(ODIR)/jdmaster.obj \
        $(ODIR)/jdmerge.obj \
        $(ODIR)/jdphuff.obj \
        $(ODIR)/jdpostct.obj \
        $(ODIR)/jdsample.obj \
        $(ODIR)/jdtrans.obj \
        $(ODIR)/jerror.obj \
        $(ODIR)/jfdctflt.obj \
        $(ODIR)/jfdctfst.obj \
        $(ODIR)/jfdctint.obj \
        $(ODIR)/jidctflt.obj \
        $(ODIR)/jidctfst.obj \
        $(ODIR)/jidctint.obj \
        $(ODIR)/jidctred.obj \
        $(ODIR)/jquant1.obj \
        $(ODIR)/jquant2.obj \
        $(ODIR)/jutils.obj \
        $(ODIR)/jmemmgr.obj \
        $(ODIR)/jmemnobs.obj

ifeq ($(WITH_SIMD), yes)

 ifeq ($(WIN64), yes)
  SIMD_OBJS = $(ODIR)/jfsseflt-64.obj \
              $(ODIR)/jccolss2-64.obj \
              $(ODIR)/jdcolss2-64.obj \
              $(ODIR)/jcsamss2-64.obj \
              $(ODIR)/jdsamss2-64.obj \
              $(ODIR)/jdmerss2-64.obj \
              $(ODIR)/jcqnts2i-64.obj \
              $(ODIR)/jfss2fst-64.obj \
              $(ODIR)/jfss2int-64.obj \
              $(ODIR)/jiss2red-64.obj \
              $(ODIR)/jiss2int-64.obj \
              $(ODIR)/jiss2fst-64.obj \
              $(ODIR)/jcqnts2f-64.obj \
              $(ODIR)/jiss2flt-64.obj
  LOBJS := $(LOBJS) $(ODIR)/jsimd_x86_64.obj
 else
  SIMD_OBJS = $(ODIR)/jsimdcpu.obj \
              $(ODIR)/jccolmmx.obj \
              $(ODIR)/jdcolmmx.obj \
              $(ODIR)/jcsammmx.obj \
              $(ODIR)/jdsammmx.obj \
              $(ODIR)/jdmermmx.obj \
              $(ODIR)/jcqntmmx.obj \
              $(ODIR)/jfmmxfst.obj \
              $(ODIR)/jfmmxint.obj \
              $(ODIR)/jimmxred.obj \
              $(ODIR)/jimmxint.obj \
              $(ODIR)/jimmxfst.obj \
              $(ODIR)/jcqnt3dn.obj \
              $(ODIR)/jf3dnflt.obj \
              $(ODIR)/ji3dnflt.obj \
              $(ODIR)/jcqntsse.obj \
              $(ODIR)/jfsseflt.obj \
              $(ODIR)/jisseflt.obj \
              $(ODIR)/jccolss2.obj \
              $(ODIR)/jdcolss2.obj \
              $(ODIR)/jcsamss2.obj \
              $(ODIR)/jdsamss2.obj \
              $(ODIR)/jdmerss2.obj \
              $(ODIR)/jcqnts2i.obj \
              $(ODIR)/jfss2fst.obj \
              $(ODIR)/jfss2int.obj \
              $(ODIR)/jiss2red.obj \
              $(ODIR)/jiss2int.obj \
              $(ODIR)/jiss2fst.obj \
              $(ODIR)/jcqnts2f.obj \
              $(ODIR)/jiss2flt.obj
  LOBJS := $(LOBJS) $(ODIR)/jsimd_i386.obj
 endif

else

 LOBJS := $(LOBJS) $(ODIR)/jsimd_none.obj

endif

LOBJSSTATIC := $(subst .obj,-static.obj,$(LOBJS))
LOBJS := $(LOBJS) $(SIMD_OBJS)
LOBJSSTATIC := $(LOBJSSTATIC) $(SIMD_OBJS)

OBJS := $(LOBJS) $(LOBJSSTATIC) \
        $(ODIR)/cdjpeg.obj \
        $(ODIR)/cjpeg.obj \
        $(ODIR)/djpeg.obj \
        $(ODIR)/jpegtran.obj \
        $(ODIR)/rdbmp.obj \
        $(ODIR)/rdcolmap.obj \
        $(ODIR)/rdgif.obj \
        $(ODIR)/rdjpgcom.obj \
        $(ODIR)/rdppm.obj \
        $(ODIR)/rdtarga.obj \
        $(ODIR)/rdswitch.obj \
        $(ODIR)/rdtarga.obj \
        $(ODIR)/transupp.obj \
        $(ODIR)/wrbmp.obj \
        $(ODIR)/wrgif.obj \
        $(ODIR)/wrppm.obj \
        $(ODIR)/wrtarga.obj \
        $(ODIR)/wrjpgcom.obj \
        $(ODIR)/turbojpegl-static.obj \
        $(ODIR)/turbojpegl-dll.obj \
        $(ODIR)/jpgtest.obj \
        $(ODIR)/jpegut.obj \
        $(ODIR)/bmp-static.obj

all: $(TARGETS)

clean: testclean
	-$(RM) $(TARGETS) $(OBJS)

HDRS := $(wildcard *.h)
$(OBJS): $(HDRS)

ifeq ($(WITH_SIMD), yes)
SIMD_HDRS := $(wildcard simd/*.inc)
$(SIMD_OBJS): $(SIMD_HDRS)

ifeq ($(WIN64), yes)
$(ODIR)/jccolss2-64.obj: simd/jcclrss2-64.asm
$(ODIR)/jdcolss2-64.obj: simd/jdclrss2-64.asm
$(ODIR)/jdmerss2-64.obj: simd/jdmrgss2-64.asm
else
$(ODIR)/jccolmmx.obj: simd/jcclrmmx.asm
$(ODIR)/jccolss2.obj: simd/jcclrss2.asm
$(ODIR)/jdcolmmx.obj: simd/jdclrmmx.asm
$(ODIR)/jdcolss2.obj: simd/jdclrss2.asm
$(ODIR)/jdmermmx.obj: simd/jdmrgmmx.asm
$(ODIR)/jdmerss2.obj: simd/jdmrgss2.asm
endif

endif

$(ODIR)/jpeg-static.lib: $(LOBJSSTATIC)
	$(AR) -out:$@ $^

$(ODIR)/jpeg62.dll $(ODIR)/jpeg.lib: $(LOBJS) win/jpeg.def
	$(LINK) $(LDFLAGS) -dll -out:$(ODIR)/jpeg62.dll -implib:$(ODIR)/jpeg.lib \
		-def:win/jpeg.def $(LOBJS)

$(ODIR)/cjpeg.exe: $(ODIR)/cdjpeg.obj $(ODIR)/cjpeg.obj $(ODIR)/rdbmp.obj \
	$(ODIR)/rdgif.obj $(ODIR)/rdppm.obj $(ODIR)/rdswitch.obj \
	$(ODIR)/rdtarga.obj $(ODIR)/jpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^


$(ODIR)/djpeg.exe: $(ODIR)/cdjpeg.obj $(ODIR)/djpeg.obj $(ODIR)/rdcolmap.obj \
	$(ODIR)/rdswitch.obj $(ODIR)/wrbmp.obj $(ODIR)/wrgif.obj $(ODIR)/wrppm.obj \
	$(ODIR)/wrtarga.obj $(ODIR)/jpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^


$(ODIR)/jpegtran.exe: $(ODIR)/cdjpeg.obj $(ODIR)/jpegtran.obj \
	$(ODIR)/rdswitch.obj $(ODIR)/transupp.obj $(ODIR)/jpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

$(ODIR)/rdjpgcom.exe: $(ODIR)/rdjpgcom.obj $(ODIR)/jpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

$(ODIR)/wrjpgcom.exe: $(ODIR)/wrjpgcom.obj $(ODIR)/jpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^


$(ODIR)/turbojpeg-static.lib: $(ODIR)/turbojpegl-static.obj $(LOBJSSTATIC)
	$(AR) -out:$@ $^

$(ODIR)/turbojpegl-dll.obj: turbojpegl.c
	$(CC) $(CFLAGS) $(LIBCSTATIC) -DDLLDEFINE -c $< -Fo$@

$(ODIR)/turbojpeg.dll $(ODIR)/turbojpeg.lib: $(ODIR)/turbojpegl-dll.obj \
	$(LOBJSSTATIC)
	$(LINK) $(LDFLAGS) -dll -out:$(ODIR)/turbojpeg.dll \
		-implib:$(ODIR)/turbojpeg.lib $^


$(ODIR)/jpgtest.exe: $(ODIR)/jpgtest.obj $(ODIR)/bmp-static.obj \
	$(ODIR)/turbojpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^

$(ODIR)/jpegut.exe: $(ODIR)/jpegut.obj $(ODIR)/turbojpeg.lib
	$(LINK) $(LDFLAGS) -out:$@ $^


ifeq ($(WIN64), yes)
nsi: all
	$(RM) $(ODIR)/libjpeg-turbo64.exe
	makensis -nocd -DVERSION=$(VERSION) -DAPPNAME=libjpeg-turbo64 \
		-DWLIBDIR=windows64 -DWSRCDIR=. -DWBLDDIR=windows64 -DWHDRDIR=win \
		-DPLATFORM="Visual C++ 64-bit" -DWIN64 release/libjpeg-turbo.nsi
else
nsi: all
	$(RM) $(ODIR)/libjpeg-turbo.exe
	makensis -nocd -DVERSION=$(VERSION) -DAPPNAME=libjpeg-turbo \
		-DWLIBDIR=windows -DWSRCDIR=. -DWBLDDIR=windows -DWHDRDIR=win \
		-DPLATFORM="Visual C++" release/libjpeg-turbo.nsi
endif


test: testclean $(ODIR)/cjpeg.exe $(ODIR)/djpeg.exe $(ODIR)/jpegtran.exe \
	$(ODIR)/jpegut.exe
	cd $(ODIR);	./jpegut
	$(ODIR)/cjpeg -dct int -outfile $(ODIR)/testoutint.jpg testorig.ppm
	$(ODIR)/cjpeg -dct fast -opt -outfile $(ODIR)/testoutfst.jpg testorig.ppm
	$(ODIR)/cjpeg -dct float -outfile $(ODIR)/testoutflt.jpg testorig.ppm
	cmp testimgint.jpg $(ODIR)/testoutint.jpg
	cmp testimgfst.jpg $(ODIR)/testoutfst.jpg
	cmp testimgflt.jpg $(ODIR)/testoutflt.jpg
	$(ODIR)/djpeg -dct int -fast -ppm -outfile $(ODIR)/testoutint.ppm testorig.jpg
	$(ODIR)/djpeg -dct fast -ppm -outfile $(ODIR)/testoutfst.ppm testorig.jpg
	$(ODIR)/djpeg -dct float -ppm -outfile $(ODIR)/testoutflt.ppm testorig.jpg
	cmp testimgint.ppm $(ODIR)/testoutint.ppm
	cmp testimgfst.ppm $(ODIR)/testoutfst.ppm
	cmp testorig.ppm $(ODIR)/testoutflt.ppm
	$(ODIR)/djpeg -dct int -bmp -colors 256 -outfile $(ODIR)/testout.bmp  testorig.jpg
	cmp testimg.bmp $(ODIR)/testout.bmp
	$(ODIR)/cjpeg -dct int -progressive -outfile $(ODIR)/testoutp.jpg testorig.ppm
	cmp testimgp.jpg $(ODIR)/testoutp.jpg
	$(ODIR)/jpegtran -outfile $(ODIR)/testoutt.jpg $(ODIR)/testoutp.jpg
	cmp testimgint.jpg $(ODIR)/testoutt.jpg

testclean:
	$(RM) $(ODIR)/testout* \
	$(ODIR)/*_GRAYQ[0-9]*.bmp \
	$(ODIR)/*_GRAYQ[0-9]*.ppm \
	$(ODIR)/*_GRAYQ[0-9]*.jpg \
	$(ODIR)/*_420Q[0-9]*.bmp \
	$(ODIR)/*_420Q[0-9]*.ppm \
	$(ODIR)/*_420Q[0-9]*.jpg \
	$(ODIR)/*_422Q[0-9]*.bmp \
	$(ODIR)/*_422Q[0-9]*.ppm \
	$(ODIR)/*_422Q[0-9]*.jpg \
	$(ODIR)/*_444Q[0-9]*.bmp \
	$(ODIR)/*_444Q[0-9]*.ppm \
	$(ODIR)/*_444Q[0-9]*.jpg

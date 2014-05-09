noinst_LTLIBRARIES = libsimd.la

BUILT_SOURCES = jsimdcfg.inc

EXTRA_DIST = nasm_lt.sh CMakeLists.txt \
	i386-mmx/jccolext.asm i386-mmx/jcgryext.asm i386-mmx/jdcolext.asm \
	i386-mmx/jdmrgext.asm \
	i386-sse2/jccolext.asm i386-sse2/jcgryext.asm i386-sse2/jdcolext.asm \
	i386-sse2/jdmrgext.asm \
	x86_64-sse2/jccolext.asm x86_64-sse2/jcgryext.asm x86_64-sse2/jdcolext.asm \
	x86_64-sse2/jdmrgext.asm

if SIMD_X86_64

libsimd_la_SOURCES = jsimd_x86_64.c \
	jsimd.h jsimdcfg.inc.h \
	jsimdext.inc jcolsamp.inc jdct.inc \
	x86_64-sse/jfdctflt.asm  x86_64-sse2/jcgray.asm \
	x86_64-sse2/jccolor.asm  x86_64-sse2/jdcolor.asm \
	x86_64-sse2/jcsample.asm x86_64-sse2/jdsample.asm x86_64-sse2/jdmerge.asm \
	x86_64-sse2/jquanti.asm  x86_64-sse2/jfdctfst.asm x86_64-sse2/jfdctint.asm \
	x86_64-sse2/jidctred.asm x86_64-sse2/jidctint.asm x86_64-sse2/jidctfst.asm \
	x86_64-sse2/jquantf.asm  x86_64-sse2/jidctflt.asm

x86_64-sse2/jccolor.lo: x86_64-sse2/jccolext.asm
x86_64-sse2/jcgray.lo:  x86_64-sse2/jcgryext.asm
x86_64-sse2/jdcolor.lo: x86_64-sse2/jdcolext.asm
x86_64-sse2/jdmerge.lo: x86_64-sse2/jdmrgext.asm

endif

if SIMD_I386

libsimd_la_SOURCES = jsimd_i386.c \
	jsimd.h jsimdcfg.inc.h \
	jsimdext.inc jcolsamp.inc jdct.inc \
	jsimdcpu.asm \
	i386-mmx/jccolor.asm   i386-mmx/jdcolor.asm    i386-mmx/jcgray.asm \
	i386-mmx/jcsample.asm  i386-mmx/jdsample.asm   i386-mmx/jdmerge.asm \
	i386-mmx/jquant.asm    i386-mmx/jfdctfst.asm   i386-mmx/jfdctint.asm \
	i386-mmx/jidctred.asm  i386-mmx/jidctint.asm   i386-mmx/jidctfst.asm \
	i386-3dnow/jquant.asm  i386-3dnow/jfdctflt.asm i386-3dnow/jidctflt.asm \
	i386-sse/jquant.asm    i386-sse/jfdctflt.asm   i386-sse/jidctflt.asm \
	i386-sse2/jccolor.asm  i386-sse2/jdcolor.asm   i386-sse2/jcgray.asm \
	i386-sse2/jcsample.asm i386-sse2/jdsample.asm  i386-sse2/jdmerge.asm \
	i386-sse2/jquanti.asm  i386-sse2/jfdctfst.asm  i386-sse2/jfdctint.asm \
	i386-sse2/jidctred.asm i386-sse2/jidctint.asm  i386-sse2/jidctfst.asm \
	i386-sse2/jquantf.asm  i386-sse2/jidctflt.asm

i386-mmx/jccolor.lo:  i386-mmx/jccolext.asm
i386-mmx/jcgray.lo:   i386-mmx/jcgryext.asm
i386-mmx/jdcolor.lo:  i386-mmx/jdcolext.asm
i386-mmx/jdmerge.lo:  i386-mmx/jdmrgext.asm
i386-sse2/jccolor.lo: i386-sse2/jccolext.asm
i386-sse2/jcgray.lo:  i386-sse2/jcgryext.asm
i386-sse2/jdcolor.lo: i386-sse2/jdcolext.asm
i386-sse2/jdmerge.lo: i386-sse2/jdmrgext.asm

endif

if SIMD_ARM

libsimd_la_SOURCES = jsimd_arm.c jsimd_arm_neon.S

endif

if SIMD_ARM_64

libsimd_la_SOURCES = jsimd_arm64.c jsimd_arm_neon_64.S

endif

if SIMD_MIPSEL

libsimd_la_SOURCES = jsimd_mips.c jsimd_mips_dspr2_asm.h jsimd_mips_dspr2.S

endif

AM_CPPFLAGS = -I$(top_srcdir)

.asm.lo:
	$(LIBTOOL) --mode=compile --tag NASM $(srcdir)/nasm_lt.sh $(NASM) $(NAFLAGS) -I$(srcdir) -I. $< -o $@

jsimdcfg.inc: $(srcdir)/jsimdcfg.inc.h ../jpeglib.h ../jconfig.h ../jmorecfg.h
	$(CPP) -I$(top_builddir) -I$(top_builddir)/simd $(srcdir)/jsimdcfg.inc.h | $(EGREP) "^[\;%]|^\ %" | sed 's%_cpp_protection_%%' | sed 's@% define@%define@g' > $@

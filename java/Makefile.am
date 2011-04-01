JAVAROOT = .

dist_noinst_JAVA = org/libjpegturbo/turbojpeg/TJ.java \
	org/libjpegturbo/turbojpeg/TJCompressor.java \
	org/libjpegturbo/turbojpeg/TJDecompressor.java \
	org/libjpegturbo/turbojpeg/TJTransform.java \
	org/libjpegturbo/turbojpeg/TJTransformer.java \
	TJExample.java \
	TJUnitTest.java

JAVA_CLASSES = org/libjpegturbo/turbojpeg/TJ.class \
	org/libjpegturbo/turbojpeg/TJCompressor.class \
	org/libjpegturbo/turbojpeg/TJDecompressor.class \
	org/libjpegturbo/turbojpeg/TJTransform.class \
	org/libjpegturbo/turbojpeg/TJTransformer.class \
	TJExample.class \
	TJUnitTest.class

all: all-am turbojpeg.jar

turbojpeg.jar: $(JAVA_CLASSES) ${srcdir}/MANIFEST.MF
	$(JAR) cfm turbojpeg.jar ${srcdir}/MANIFEST.MF $(JAVA_CLASSES) \
		org/libjpegturbo/turbojpeg/TJ\$$ScalingFactor.class

clean-local:
	rm -f turbojpeg.jar

install-exec-local: turbojpeg.jar
	mkdir -p $(DESTDIR)/$(prefix)/classes
	$(INSTALL) -m 644 turbojpeg.jar $(DESTDIR)/$(prefix)/classes/

uninstall-local:
	rm -f $(DESTDIR)/$(prefix)/classes/turbojpeg.jar
	if [ -d $(DESTDIR)/$(prefix)/classes ]; then rmdir $(DESTDIR)/$(prefix)/classes; fi

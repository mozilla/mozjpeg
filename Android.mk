# Android makefile for libjpeg-turbo

ifneq ($(TARGET_SIMULATOR), true)

LOCAL_PATH := $(my-dir)
VERSION = 1.3.80
BUILD := $(shell date +%Y%m%d)

DEFINES = -DPACKAGE_NAME=\"libjpeg-turbo\" -DVERSION=\"$(VERSION)\" \
  -DBUILD=\"$(BUILD)\"

# Default configuration (all of these can be overridden on the command line)
CFLAGS = -O3
ifeq ($(strip $(TARGET_ARCH)), arm)
ifeq ($(TARGET_ARCH_VARIANT), armv7-a-neon)
WITH_SIMD = 1
SIMD_ARCH = arm
endif
ifneq ($(findstring armv7-a, $(TARGET_ARCH_VARIANT)),)
CFLAGS += -fstrict-aliasing
endif
endif
WITH_TURBOJPEG = 1
WITH_ARITH_ENC = 1
WITH_ARITH_DEC = 1
JPEG_LIB_VERSION = 62
WITH_JPEG7 = 0
WITH_JPEG8 = 0
WITH_MEM_SRCDST = 1

# Define C macros based on configuration
ifeq ($(WITH_SIMD), 1)
DEFINES += -DWITH_SIMD
endif

ifeq ($(WITH_ARITH_ENC), 1)
WITH_ARITH = 1
DEFINES += -DC_ARITH_CODING_SUPPORTED
endif
ifeq ($(WITH_ARITH_DEC), 1)
WITH_ARITH = 1
DEFINES += -DD_ARITH_CODING_SUPPORTED
endif

ifeq ($(WITH_JPEG7), 1)
JPEG_LIB_VERSION = 70
endif
ifeq ($(WITH_JPEG8), 1)
JPEG_LIB_VERSION = 80
endif
DEFINES += -DJPEG_LIB_VERSION=$(JPEG_LIB_VERSION)

ifeq ($(WITH_MEM_SRCDST), 1)
DEFINES += -DMEM_SRCDST_SUPPORTED
endif

##################################################
###           SIMD                             ###
##################################################

include $(CLEAR_VARS)

ifeq ($(WITH_SIMD), 1)

ifeq ($(SIMD_ARCH), arm)
LOCAL_SRC_FILES = simd/jsimd_arm_neon.S simd/jsimd_arm.c
LOCAL_ARM_NEON := true
endif

LOCAL_CFLAGS := $(CFLAGS) $(DEFINES)

LOCAL_C_INCLUDES := $(LOCAL_PATH)/simd $(LOCAL_PATH)/android

LOCAL_MODULE = simd

include $(BUILD_STATIC_LIBRARY)

endif # WITH_SIMD

##################################################
###           libjpeg                          ###
##################################################

include $(CLEAR_VARS)

LIBJPEG_SRC_FILES = jcapimin.c jcapistd.c jccoefct.c jccolor.c jcdctmgr.c \
  jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c jcparam.c \
  jcphuff.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c jdatadst.c \
  jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdinput.c jdmainct.c \
  jdmarker.c jdmaster.c jdmerge.c jdphuff.c jdpostct.c jdsample.c jdtrans.c \
  jerror.c jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c \
  jidctred.c jquant1.c jquant2.c jutils.c jmemmgr.c jmemnobs.c

ifneq ($(WITH_SIMD), 1)
LIBJPEG_SRC_FILES += jsimd_none.c
endif

ifeq ($(WITH_ARITH), 1)
LIBJPEG_SRC_FILES += jaricom.c
endif
ifeq ($(WITH_ARITH_ENC), 1)
LIBJPEG_SRC_FILES += jcarith.c
endif
ifeq ($(WITH_ARITH_DEC), 1)
LIBJPEG_SRC_FILES += jdarith.c
endif

LOCAL_SRC_FILES := ${LIBJPEG_SRC_FILES}

LOCAL_CFLAGS := $(CFLAGS) $(DEFINES)

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/android

ifeq ($(WITH_SIMD), 1)
LOCAL_STATIC_LIBRARIES = libsimd
endif
LOCAL_MODULE = jpeg
include $(BUILD_SHARED_LIBRARY)

##################################################
###           libturbojpeg                     ###
##################################################

ifeq ($(WITH_TURBOJPEG), 1)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := ${LIBJPEG_SRC_FILES} turbojpeg.c transupp.c jdatadst-tj.c \
  jdatasrc-tj.c turbojpeg-jni.c

LOCAL_CFLAGS := $(CFLAGS) $(DEFINES)

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/android

ifeq ($(WITH_SIMD), 1)
LOCAL_STATIC_LIBRARIES = libsimd
endif
LOCAL_MODULE = turbojpeg
include $(BUILD_SHARED_LIBRARY)

endif # WITH_TURBOJPEG

######################################################
###           cjpeg                                ###
######################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES = cdjpeg.c cjpeg.c rdbmp.c rdgif.c rdppm.c rdswitch.c \
  rdtarga.c

LOCAL_CFLAGS := $(CFLAGS) $(DEFINES) -DBMP_SUPPORTED -DGIF_SUPPORTED \
  -DPPM_SUPPORTED -DTARGA_SUPPORTED

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/android

LOCAL_SHARED_LIBRARIES = libjpeg
LOCAL_MODULE = cjpeg
include $(BUILD_EXECUTABLE)

######################################################
###           djpeg                                ###
######################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES = cdjpeg.c djpeg.c rdcolmap.c rdswitch.c wrbmp.c wrgif.c \
  wrppm.c wrtarga.c

LOCAL_CFLAGS := $(CFLAGS) $(DEFINES) -DBMP_SUPPORTED -DGIF_SUPPORTED \
  -DPPM_SUPPORTED -DTARGA_SUPPORTED

LOCAL_SHARED_LIBRARIES = libjpeg

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/android

LOCAL_SHARED_LIBRARIES = libjpeg
LOCAL_MODULE = djpeg
include $(BUILD_EXECUTABLE)

######################################################
###           jpegtran                             ###
######################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES = jpegtran.c rdswitch.c cdjpeg.c transupp.c

LOCAL_CFLAGS := $(CFLAGS) $(DEFINES)

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/android

LOCAL_SHARED_LIBRARIES = libjpeg
LOCAL_MODULE = jpegtran
include $(BUILD_EXECUTABLE)

######################################################
###           rdjpgcom                             ###
######################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES = rdjpgcom.c

LOCAL_CFLAGS := $(CFLAGS) $(DEFINES)

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/android

LOCAL_SHARED_LIBRARIES = libjpeg
LOCAL_MODULE = rdjpgcom
include $(BUILD_EXECUTABLE)

######################################################
###           wrjpgcom                            ###
######################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES = wrjpgcom.c

LOCAL_CFLAGS := $(CFLAGS) $(DEFINES)

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/android

LOCAL_SHARED_LIBRARIES = libjpeg
LOCAL_MODULE = wrjpgcom
include $(BUILD_EXECUTABLE)

######################################################
###           tjunittest                           ###
######################################################

ifeq ($(WITH_TURBOJPEG), 1)

include $(CLEAR_VARS)

LOCAL_SRC_FILES = tjunittest.c tjutil.c

LOCAL_CFLAGS := $(CFLAGS) $(DEFINES)

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/android

LOCAL_SHARED_LIBRARIES = libturbojpeg
LOCAL_MODULE = tjunittest
include $(BUILD_EXECUTABLE)

######################################################
###           tjbench                              ###
######################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES = tjbench.c bmp.c tjutil.c rdbmp.c rdppm.c wrbmp.c wrppm.c

LOCAL_CFLAGS = $(DEFINES) -DBMP_SUPPORTED -DPPM_SUPPORTED

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/android

LOCAL_SHARED_LIBRARIES = libturbojpeg
LOCAL_MODULE = tjbench
include $(BUILD_EXECUTABLE)

endif # WITH_TURBOJPEG

######################################################
###           md5cmp                               ###
######################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES = md5/md5cmp.c md5/md5.c md5/md5hl.c

LOCAL_MODULE = md5cmp
include $(BUILD_EXECUTABLE)

endif # TARGET_SIMULATOR != true

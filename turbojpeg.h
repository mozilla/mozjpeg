/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009-2011 D. R. Commander
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3.1 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#if (defined(_MSC_VER) || defined(__CYGWIN__) || defined(__MINGW32__)) && defined(_WIN32) && defined(DLLDEFINE)
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#define DLLCALL

/* Subsampling */
#define NUMSUBOPT 5

enum {TJ_444=0, TJ_422, TJ_420, TJ_GRAYSCALE, TJ_440};
#define TJ_411 TJ_420  /* for backward compatibility with VirtualGL <= 2.1.x,
                          TurboVNC <= 0.6, and TurboJPEG/IPP */

/* MCU block sizes:
   8x8 for no subsampling or grayscale
   16x8 for 4:2:2
   8x16 for 4:4:0
   16x16 for 4:2:0 */
static const int tjmcuw[NUMSUBOPT]={8, 16, 16, 8, 8};
static const int tjmcuh[NUMSUBOPT]={8, 8, 16, 8, 16};

/* Flags */
#define TJ_BGR             1
  /* The components of each pixel in the uncompressed source/destination image
     are stored in B,G,R order, not R,G,B */
#define TJ_BOTTOMUP        2
  /* The uncompressed source/destination image is stored in bottom-up (Windows,
     OpenGL) order, not top-down (X11) order */
#define TJ_FORCEMMX        8
  /* Turn off CPU auto-detection and force TurboJPEG to use MMX code
     (IPP and 32-bit libjpeg-turbo versions only) */
#define TJ_FORCESSE       16
  /* Turn off CPU auto-detection and force TurboJPEG to use SSE code
     (32-bit IPP and 32-bit libjpeg-turbo versions only) */
#define TJ_FORCESSE2      32
  /* Turn off CPU auto-detection and force TurboJPEG to use SSE2 code
     (32-bit IPP and 32-bit libjpeg-turbo versions only) */
#define TJ_ALPHAFIRST     64
  /* If the uncompressed source/destination image has 32 bits per pixel,
     assume that each pixel is ARGB/XRGB (or ABGR/XBGR if TJ_BGR is also
     specified) */
#define TJ_FORCESSE3     128
  /* Turn off CPU auto-detection and force TurboJPEG to use SSE3 code
     (64-bit IPP version only) */
#define TJ_FASTUPSAMPLE  256
  /* Use fast, inaccurate chrominance upsampling routines in the JPEG
     decompressor (libjpeg and libjpeg-turbo versions only) */
#define TJ_YUV           512
  /* Nothing to see here.  Pay no attention to the man behind the curtain. */

/* Scaling factor structure */
typedef struct
{
	int num, denom;
} tjscalingfactor;

/* Transform operations for tjTransform() */
#define NUMXFORMOPT 8

enum {
TJXFORM_NONE=0,     /* Do not transform the position of the image pixels */
TJXFORM_HFLIP,      /* Flip (mirror) image horizontally.  This transform is
                       imperfect if there are any partial MCU blocks on the
                       right edge (see below for explanation.) */
TJXFORM_VFLIP,      /* Flip (mirror) image vertically.  This transform is
                       imperfect if there are any partial MCU blocks on the
                       bottom edge. */
TJXFORM_TRANSPOSE,  /* Transpose image (flip/mirror along upper left to lower
                       right axis.)  This transform is always perfect. */
TJXFORM_TRANSVERSE, /* Transverse transpose image (flip/mirror along upper
                       right to lower left axis.)  This transform is imperfect
                       if there are any partial MCU blocks in the image. */
TJXFORM_ROT90,      /* Rotate image clockwise by 90 degrees.  This transform
                       is imperfect if there are any partial MCU blocks on the
                       bottom edge. */
TJXFORM_ROT180,     /* Rotate image 180 degrees.  This transform is imperfect
                       if there are any partial MCU blocks in the image. */
TJXFORM_ROT270      /* Rotate image counter-clockwise by 90 degrees.  This
                       transform is imperfect if there are any partial MCU
                       blocks on the right edge. */
};

/* Transform options (these can be OR'ed together) */
#define TJXFORM_PERFECT  1
  /* This will cause the tjTransform() function to return an error if the
     transform is not perfect.  Lossless transforms operate on MCU blocks,
     whose size depends on the level of chrominance subsampling used (see
     "MCU block sizes" above).  If the image's width or height is not evenly
     divisible by the MCU block size, then there will be partial MCU blocks on
     the right and/or bottom edges.  It is not possible to move these partial
     MCU blocks to the top or left of the image, so any transform that would
     require that is "imperfect."  If this option is not specified, then any
     partial MCU blocks that cannot be transformed will be left in place, which
     will create odd-looking strips on the right or bottom edge of the image.
     */
#define TJXFORM_TRIM     2
  /* This option will cause tjTransform() to discard any partial MCU blocks
     that cannot be transformed. */
#define TJXFORM_CROP     4
  /* This option will enable lossless cropping.  See the description of
     tjTransform() below for more information. */
#define TJXFORM_GRAY     8
  /* This option will discard the color data in the input image and produce
     a grayscale output image. */

typedef struct
{
	int x, y, w, h;
} tjregion;

typedef struct
{
	tjregion r;
	int op, options;
} tjtransform;

typedef void* tjhandle;

#define TJPAD(p) (((p)+3)&(~3))

#ifdef __cplusplus
extern "C" {
#endif

/* API follows */


/*
  tjhandle tjInitCompress(void)

  Creates a new JPEG compressor instance, allocates memory for the structures,
  and returns a handle to the instance.  Most applications will only
  need to call this once at the beginning of the program or once for each
  concurrent thread.  Don't try to create a new instance every time you
  compress an image, because this may cause performance to suffer in some
  TurboJPEG implementations.

  RETURNS: NULL on error
*/
DLLEXPORT tjhandle DLLCALL tjInitCompress(void);


/*
  int tjCompress(tjhandle hnd,
     unsigned char *srcbuf, int width, int pitch, int height, int pixelsize,
     unsigned char *dstbuf, unsigned long *size,
     int jpegsubsamp, int jpegqual, int flags)

  [INPUT] hnd = instance handle previously returned from a call to
     tjInitCompress() or tjInitTransform()
  [INPUT] srcbuf = pointer to user-allocated image buffer containing RGB or
     grayscale pixels to be compressed
  [INPUT] width = width (in pixels) of the source image
  [INPUT] pitch = bytes per line of the source image (width*pixelsize if the
     image is unpadded, else TJPAD(width*pixelsize) if each line of the image
     is padded to the nearest 32-bit boundary, such as is the case for Windows
     bitmaps.  You can also be clever and use this parameter to skip lines,
     etc.  Setting this parameter to 0 is the equivalent of setting it to
     width*pixelsize.
  [INPUT] height = height (in pixels) of the source image
  [INPUT] pixelsize = size (in bytes) of each pixel in the source image
     RGBX/BGRX/XRGB/XBGR: 4, RGB/BGR: 3, Grayscale: 1
  [INPUT] dstbuf = pointer to user-allocated image buffer which will receive
     the JPEG image.  Use the TJBUFSIZE(width, height) function to determine
     the maximum size for this buffer based on the image width and height.
  [OUTPUT] size = pointer to unsigned long which receives the actual size (in
     bytes) of the JPEG image
  [INPUT] jpegsubsamp = Specifies the level of chrominance subsampling.  When
     the image is converted from the RGB to YCbCr colorspace as part of the
     JPEG compression process, some of the Cb and Cr (chrominance) components
     can be discarded or averaged together to produce a smaller image with
     little perceptible loss of image clarity (the human eye is more sensitive
     to small changes in brightness than small changes in color.)

     TJ_420: 4:2:0 subsampling.  The JPEG image will contain one chrominance
        component for every 2x2 block of pixels in the source image.
     TJ_422: 4:2:2 subsampling.  The JPEG image will contain one chrominance
        component for every 2x1 block of pixels in the source image.
     TJ_440: 4:4:0 subsampling.  The JPEG image will contain one chrominance
        component for every 1x2 block of pixels in the source image.
     TJ_444: no subsampling.  The JPEG image will contain one chrominance
        component for every pixel in the source image.
     TJ_GRAYSCALE: Generate grayscale JPEG image.  The JPEG image will contain
        no chrominance components.

  [INPUT] jpegqual = JPEG quality (an integer between 0 and 100 inclusive)
  [INPUT] flags = the bitwise OR of one or more of the flags described in the
     "Flags" section above

  RETURNS: 0 on success, -1 on error
*/
DLLEXPORT int DLLCALL tjCompress(tjhandle hnd,
	unsigned char *srcbuf, int width, int pitch, int height, int pixelsize,
	unsigned char *dstbuf, unsigned long *size,
	int jpegsubsamp, int jpegqual, int flags);


/*
  unsigned long TJBUFSIZE(int width, int height)

  Convenience function which returns the maximum size of the buffer required to
  hold a JPEG image with the given width and height

  RETURNS: -1 if arguments are out of bounds
*/
DLLEXPORT unsigned long DLLCALL TJBUFSIZE(int width, int height);


/*
  unsigned long TJBUFSIZEYUV(int width, int height, int subsamp)

  Convenience function which returns the size of the buffer required to
  hold a YUV planar image with the given width, height, and level of
  chrominance subsampling

  RETURNS: -1 if arguments are out of bounds
*/
DLLEXPORT unsigned long DLLCALL TJBUFSIZEYUV(int width, int height,
  int subsamp);


/*
  int tjEncodeYUV(tjhandle hnd,
     unsigned char *srcbuf, int width, int pitch, int height, int pixelsize,
     unsigned char *dstbuf, int subsamp, int flags)

  This function uses the accelerated color conversion routines in TurboJPEG's
  underlying codec to produce a planar YUV image that is suitable for X Video.
  Specifically, if the chrominance components are subsampled along the
  horizontal dimension, then the width of the luminance plane is padded to 2 in
  the output image (same goes for the height of the luminance plane, if the
  chrominance components are subsampled along the vertical dimension.)  Also,
  each line of each plane in the output image is padded to 4 bytes.  Although
  this will work with any subsampling option, it is really only useful in
  combination with TJ_420, which produces an image compatible with the I420
  (AKA "YUV420P") format.

  [INPUT] hnd = instance handle previously returned from a call to
     tjInitCompress() or tjInitTransform()
  [INPUT] srcbuf = pointer to user-allocated image buffer containing RGB or
     grayscale pixels to be encoded
  [INPUT] width = width (in pixels) of the source image
  [INPUT] pitch = bytes per line of the source image (width*pixelsize if the
     image is unpadded, else TJPAD(width*pixelsize) if each line of the image
     is padded to the nearest 32-bit boundary, such as is the case for Windows
     bitmaps.  You can also be clever and use this parameter to skip lines,
     etc.  Setting this parameter to 0 is the equivalent of setting it to
     width*pixelsize.
  [INPUT] height = height (in pixels) of the source image
  [INPUT] pixelsize = size (in bytes) of each pixel in the source image
     RGBX/BGRX/XRGB/XBGR: 4, RGB/BGR: 3, Grayscale: 1
  [INPUT] dstbuf = pointer to user-allocated image buffer which will receive
     the YUV image.  Use the TJBUFSIZEYUV(width, height, subsamp) function to
     determine the appropriate size for this buffer based on the image width,
     height, and level of subsampling.
  [INPUT] subsamp = specifies the level of chrominance subsampling for the
     YUV image.  See description under tjCompress())
  [INPUT] flags = the bitwise OR of one or more of the flags described in the
     "Flags" section above

  RETURNS: 0 on success, -1 on error
*/
DLLEXPORT int DLLCALL tjEncodeYUV(tjhandle hnd,
	unsigned char *srcbuf, int width, int pitch, int height, int pixelsize,
	unsigned char *dstbuf, int subsamp, int flags);


/*
  tjhandle tjInitDecompress(void)

  Creates a new JPEG decompressor instance, allocates memory for the
  structures, and returns a handle to the instance.  Most applications will
  only need to call this once at the beginning of the program or once for each
  concurrent thread.  Don't try to create a new instance every time you
  decompress an image, because this may cause performance to suffer in some
  TurboJPEG implementations.

  RETURNS: NULL on error
*/
DLLEXPORT tjhandle DLLCALL tjInitDecompress(void);


/*
  int tjDecompressHeader2(tjhandle hnd,
     unsigned char *srcbuf, unsigned long size,
     int *width, int *height, int *jpegsubsamp)

  [INPUT] hnd = instance handle previously returned from a call to
     tjInitDecompress() or tjInitTransform()
  [INPUT] srcbuf = pointer to a user-allocated buffer containing a JPEG image
  [INPUT] size = size of the JPEG image buffer (in bytes)
  [OUTPUT] width = width (in pixels) of the JPEG image
  [OUTPUT] height = height (in pixels) of the JPEG image
  [OUTPUT] jpegsubsamp = type of chrominance subsampling used when compressing
     the JPEG image

  RETURNS: 0 on success, -1 on error
*/
DLLEXPORT int DLLCALL tjDecompressHeader2(tjhandle hnd,
	unsigned char *srcbuf, unsigned long size,
	int *width, int *height, int *jpegsubsamp);

/*
  Legacy version of the above function
*/
DLLEXPORT int DLLCALL tjDecompressHeader(tjhandle hnd,
	unsigned char *srcbuf, unsigned long size,
	int *width, int *height);


/*
  tjscalingfactor *tjGetScalingFactors(int *numscalingfactors)

  Returns a list of fractional scaling factors that the JPEG decompressor in
  this implementation of TurboJPEG supports.

  [OUTPUT] numscalingfactors = the size of the list

  RETURNS: NULL on error
*/
DLLEXPORT tjscalingfactor* DLLCALL tjGetScalingFactors(int *numscalingfactors);


/*
  int tjDecompress(tjhandle hnd,
     unsigned char *srcbuf, unsigned long size,
     unsigned char *dstbuf, int width, int pitch, int height, int pixelsize,
     int flags)

  [INPUT] hnd = instance handle previously returned from a call to
     tjInitDecompress() or tjInitTransform()
  [INPUT] srcbuf = pointer to a user-allocated buffer containing the JPEG image
     to decompress
  [INPUT] size = size of the JPEG image buffer (in bytes)
  [INPUT] dstbuf = pointer to user-allocated image buffer which will receive
     the decompressed image.  This buffer should normally be
     pitch*scaled_height bytes in size, where scaled_height is
     ceil(jpeg_height*scaling_factor), and the supported scaling factors can be
     determined by calling tjGetScalingFactors().  The dstbuf pointer may also
     be used to decompress into a specific region of a larger buffer.
  [INPUT] width = desired width (in pixels) of the destination image.  If this
     is smaller than the width of the JPEG image being decompressed, then
     TurboJPEG will use scaling in the JPEG decompressor to generate the
     largest possible image that will fit within the desired width.  If width
     is set to 0, then only the height will be considered when determining the
     scaled image size.
  [INPUT] pitch = bytes per line of the destination image.  Normally, this is
     scaled_width*pixelsize if the decompressed image is unpadded, else
     TJPAD(scaled_width*pixelsize) if each line of the decompressed image is
     padded to the nearest 32-bit boundary, such as is the case for Windows
     bitmaps.  (NOTE: scaled_width = ceil(jpeg_width*scaling_factor).)  You can
     also be clever and use this parameter to skip lines, etc.  Setting this
     parameter to 0 is the equivalent of setting it to scaled_width*pixelsize.
  [INPUT] height = desired height (in pixels) of the destination image.  If
     this is smaller than the height of the JPEG image being decompressed, then
     TurboJPEG will use scaling in the JPEG decompressor to generate the
     largest possible image that will fit within the desired height.  If
     height is set to 0, then only the width will be considered when
     determining the scaled image size.
  [INPUT] pixelsize = size (in bytes) of each pixel in the destination image
     RGBX/BGRX/XRGB/XBGR: 4, RGB/BGR: 3, Grayscale: 1
  [INPUT] flags = the bitwise OR of one or more of the flags described in the
     "Flags" section above.

  RETURNS: 0 on success, -1 on error
*/
DLLEXPORT int DLLCALL tjDecompress(tjhandle hnd,
	unsigned char *srcbuf, unsigned long size,
	unsigned char *dstbuf, int width, int pitch, int height, int pixelsize,
	int flags);


/*
  int tjDecompressToYUV(tjhandle hnd,
     unsigned char *srcbuf, unsigned long size,
     unsigned char *dstbuf, int flags)

  This function performs JPEG decompression but leaves out the color conversion
  step, so a planar YUV image is generated instead of an RGB image.  The
  padding of the planes in this image is the same as the images generated
  by tjEncodeYUV().  Note that, if the width or height of the image is not an
  even multiple of the MCU block size (see "MCU block sizes" above), then an
  intermediate buffer copy will be performed within TurboJPEG.

  [INPUT] hnd = instance handle previously returned from a call to
     tjInitDecompress() or tjInitTransform()
  [INPUT] srcbuf = pointer to a user-allocated buffer containing the JPEG image
     to decompress
  [INPUT] size = size of the JPEG image buffer (in bytes)
  [INPUT] dstbuf = pointer to user-allocated image buffer which will receive
     the YUV image.  Use the TJBUFSIZEYUV(width, height, subsamp) function to
     determine the appropriate size for this buffer based on the image width,
     height, and level of subsampling.
  [INPUT] flags = the bitwise OR of one or more of the flags described in the
     "Flags" section above.

  RETURNS: 0 on success, -1 on error
*/
DLLEXPORT int DLLCALL tjDecompressToYUV(tjhandle hnd,
	unsigned char *srcbuf, unsigned long size,
	unsigned char *dstbuf, int flags);


/*
  tjhandle tjInitTransform(void)

  Creates a new JPEG transformer instance, allocates memory for the structures,
  and returns a handle to the instance.  Most applications will only need to
  call this once at the beginning of the program or once for each concurrent
  thread.  Don't try to create a new instance every time you transform an
  image, because this may cause performance to suffer in some TurboJPEG
  implementations.

  RETURNS: NULL on error
*/
DLLEXPORT tjhandle DLLCALL tjInitTransform(void);


/*
  int tjTransform(tjhandle hnd,
     unsigned char *srcbuf, unsigned long srcsize,
     int n, unsigned char **dstbufs, unsigned long *dstsizes,
     tjtransform *transforms, int flags);

  This function can losslessly transform a JPEG image into another JPEG image.
  Lossless transforms work by moving the raw coefficients from one JPEG image
  structure to another without altering the values of the coefficients.  While
  this is typically faster than decompressing the image, transforming it, and
  re-compressing it, lossless transforms are not free.  Each lossless transform
  requires reading and Huffman decoding all of the coefficients in the source
  image, regardless of the size of the destination image.  Thus, this function
  provides a means of generating multiple transformed images from the same
  source or of applying multiple transformations simultaneously, in order to
  eliminate the need to read the source coefficients multiple times.

  [INPUT] hnd = instance handle previously returned from a call to
     tjInitTransform()
  [INPUT] srcbuf = pointer to a user-allocated buffer containing the JPEG image
     to transform
  [INPUT] srcsize = size of the source JPEG image buffer (in bytes)
  [INPUT] n = the number of transformed JPEG images to generate
  [INPUT] dstbufs = pointer to an array of n user-allocated image buffers.
     dstbufs[i] will receive a JPEG image that has been transformed using the
     parameters in transforms[i].  Use the TJBUFSIZE(width, height) function to
     determine the maximum size for each buffer based on the cropped width and
     height.
  [OUTPUT] dstsizes = pointer to an array of n unsigned longs which will
     receive the actual sizes (in bytes) of each transformed JPEG image
  [INPUT] transforms = pointer to an array of n tjtransform structures, each of
     which specifies the transform parameters and/or cropping region for the
     corresponding transformed output image.  The structure members are as
     follows:

     r.x = the left boundary of the cropping region.  This must be evenly
        divisible by tjmcuw[subsamp] (the MCU block width corresponding to the
        level of chrominance subsampling used in the source image)
     r.y = the upper boundary of the cropping region.  This must be evenly
        divisible by tjmcuh[subsamp] (the MCU block height corresponding to the
        level of chrominance subsampling used in the source image)
     r.w = the width of the cropping region.  Setting this to 0 is the
        equivalent of setting it to the width of the source JPEG image.
     r.h = the height of the cropping region.  Setting this to 0 is the
        equivalent of setting it to the height of the source JPEG image.
     op = one of the transform operations described in the
        "Transform operations" section above
     options = the bitwise OR of one or more of the transform options described
        in the "Transform options" section above.

  [INPUT] flags = the bitwise OR of one or more of the flags described in the
     "Flags" section above.

  RETURNS: 0 on success, -1 on error
*/
DLLEXPORT int DLLCALL tjTransform(tjhandle hnd,
	unsigned char *srcbuf, unsigned long srcsize,
	int n, unsigned char **dstbufs, unsigned long *dstsizes,
	tjtransform *transforms, int flags);


/*
  int tjDestroy(tjhandle h)

  Frees structures associated with a compression or decompression instance
  
  [INPUT] h = instance handle (returned from a previous call to
     tjInitCompress(), tjInitDecompress(), or tjInitTransform()

  RETURNS: 0 on success, -1 on error
*/
DLLEXPORT int DLLCALL tjDestroy(tjhandle h);


/*
  char *tjGetErrorStr(void)
  
  Returns a descriptive error message explaining why the last command failed
*/
DLLEXPORT char* DLLCALL tjGetErrorStr(void);

#ifdef __cplusplus
}
#endif

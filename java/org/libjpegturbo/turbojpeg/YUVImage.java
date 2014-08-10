/*
 * Copyright (C)2014 D. R. Commander.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the libjpeg-turbo Project nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS",
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package org.libjpegturbo.turbojpeg;

/**
 * This class encapsulates a YUV planar image and the metadata
 * associated with it.  The TurboJPEG API allows both the JPEG compression and
 * decompression pipelines to be split into stages:  YUV encode, compress from
 * YUV, decompress to YUV, and YUV decode.  A <code>YUVImage</code> instance
 * serves as the destination image for YUV encode and decompress-to-YUV
 * operations and as the source image for compress-from-YUV and YUV decode
 * operations.
 * <p>
 * Technically, the JPEG format uses the YCbCr colorspace (which technically is
 * not a "colorspace" but rather a "color transform"), but per the convention
 * of the digital video community, the TurboJPEG API uses "YUV" to refer to an
 * image format consisting of Y, Cb, and Cr image planes.
 * <p>
 * Each plane is simply a 2D array of bytes, each byte representing the value
 * of one of the components at a particular location in the image.  The
 * "component width" and "component height" of each plane are determined by the
 * image width, height, and level of chrominance subsampling.  For the
 * luminance plane, the component width is the image width padded to the
 * nearest multiple of the horizontal subsampling factor (2 in the case of
 * 4:2:0 and 4:2:2, 4 in the case of 4:1:1, 1 in the case of 4:4:4 or
 * grayscale.)  Similarly, the component height of the luminance plane is the
 * image height padded to the nearest multiple of the vertical subsampling
 * factor (2 in the case of 4:2:0 or 4:4:0, 1 in the case of 4:4:4 or
 * grayscale.)  The component width of the chrominance planes is equal to the
 * component width of the luminance plane divided by the horizontal subsampling
 * factor, and the component height of the chrominance planes is equal to the
 * component height of the luminance plane divided by the vertical subsampling
 * factor.
 * <p>
 * For example, if the source image is 35 x 35 pixels and 4:2:2 subsampling is
 * used, then the luminance plane would be 36 x 35 bytes, and each of the
 * chrominance planes would be 18 x 35 bytes.  If you specify a line padding of
 * 4 bytes on top of this, then the luminance plane would be 36 x 35 bytes, and
 * each of the chrominance planes would be 20 x 35 bytes.
 */
public class YUVImage {

  private static final String NO_ASSOC_ERROR =
    "No YUV buffer is associated with this instance";

  /**
   * Create a <code>YUVImage</code> instance with a new image buffer.
   *
   * @param width width (in pixels) of the YUV image
   *
   * @param pad Each line of each plane in the YUV image buffer will be padded
   * to this number of bytes (must be a power of 2.)
   *
   * @param height height (in pixels) of the YUV image
   *
   * @param subsamp the level of chrominance subsampling to be used in the YUV
   * image (one of {@link TJ#SAMP_444 TJ.SAMP_*})
   */
  public YUVImage(int width, int pad, int height, int subsamp)
                    throws Exception {
    setBuf(new byte[TJ.bufSizeYUV(width, pad, height, subsamp)], width, pad,
           height, subsamp);
  }

  /**
   * Create a <code>YUVImage</code> instance from an existing YUV planar image
   * buffer.
   *
   * @param yuvImage image buffer that contains or will contain YUV planar
   * image data.  Use {@link TJ#bufSizeYUV} to determine the minimum size for
   * this buffer.  The Y, U (Cb), and V (Cr) image planes are stored
   * sequentially in the buffer (see {@link YUVImage above} for a description
   * of the image format.)
   *
   * @param width width (in pixels) of the YUV image
   *
   * @param pad the line padding used in the YUV image buffer.  For
   * instance, if each line in each plane of the buffer is padded to the
   * nearest multiple of 4 bytes, then <code>pad</code> should be set to 4.
   *
   * @param height height (in pixels) of the YUV image
   *
   * @param subsamp the level of chrominance subsampling used in the YUV
   * image (one of {@link TJ#SAMP_444 TJ.SAMP_*})
   */
  public YUVImage(byte[] yuvImage, int width, int pad, int height,
                  int subsamp) throws Exception {
    setBuf(yuvImage, width, pad, height, subsamp);
  }

  /**
   * Assign an existing YUV planar image buffer to this <code>YUVImage</code>
   * instance.
   *
   * @param yuvImage image buffer that contains or will contain YUV planar
   * image data.  Use {@link TJ#bufSizeYUV} to determine the minimum size for
   * this buffer.  The Y, U (Cb), and V (Cr) image planes are stored
   * sequentially in the buffer (see {@link YUVImage above} for a description
   * of the image format.)
   *
   * @param width width (in pixels) of the YUV image
   *
   * @param pad the line padding used in the YUV image buffer.  For
   * instance, if each line in each plane of the buffer is padded to the
   * nearest multiple of 4 bytes, then <code>pad</code> should be set to 4.
   *
   * @param height height (in pixels) of the YUV image
   *
   * @param subsamp the level of chrominance subsampling used in the YUV
   * image (one of {@link TJ#SAMP_444 TJ.SAMP_*})
   */
  public void setBuf(byte[] yuvImage, int width, int pad, int height,
                     int subsamp) throws Exception {
    if (yuvImage == null || width < 1 || pad < 1 || ((pad & (pad - 1)) != 0) ||
        height < 1 || subsamp < 0 || subsamp >= TJ.NUMSAMP)
      throw new Exception("Invalid argument in YUVImage()");
    if (yuvImage.length < TJ.bufSizeYUV(width, pad, height, subsamp))
      throw new Exception("YUV image buffer is not large enough");
    yuvBuf = yuvImage;
    yuvWidth = width;
    yuvPad = pad;
    yuvHeight = height;
    yuvSubsamp = subsamp;
  }

  /**
   * Returns the width of the YUV image.
   *
   * @return the width of the YUV image
   */
  public int getWidth() throws Exception {
    if (yuvWidth < 1)
      throw new Exception(NO_ASSOC_ERROR);
    return yuvWidth;
  }

  /**
   * Returns the height of the YUV image.
   *
   * @return the height of the YUV image
   */
  public int getHeight() throws Exception {
    if (yuvHeight < 1)
      throw new Exception(NO_ASSOC_ERROR);
    return yuvHeight;
  }

  /**
   * Returns the line padding used in the YUV image buffer.
   *
   * @return the line padding used in the YUV image buffer
   */
  public int getPad() throws Exception {
    if (yuvPad < 1 || ((yuvPad & (yuvPad - 1)) != 0))
      throw new Exception(NO_ASSOC_ERROR);
    return yuvPad;
  }

  /**
   * Returns the level of chrominance subsampling used in the YUV image.  See
   * {@link TJ#SAMP_444 TJ.SAMP_*}.
   *
   * @return the level of chrominance subsampling used in the YUV image
   */
  public int getSubsamp() throws Exception {
    if (yuvSubsamp < 0 || yuvSubsamp >= TJ.NUMSAMP)
      throw new Exception(NO_ASSOC_ERROR);
    return yuvSubsamp;
  }

  /**
   * Returns the YUV image buffer
   *
   * @return the YUV image buffer
   */
  public byte[] getBuf() throws Exception {
    if (yuvBuf == null)
      throw new Exception(NO_ASSOC_ERROR);
    return yuvBuf;
  }

  /**
   * Returns the size (in bytes) of the YUV image buffer
   *
   * @return the size (in bytes) of the YUV image buffer
   */
   public int getSize() throws Exception {
     if (yuvBuf == null)
       throw new Exception(NO_ASSOC_ERROR);
     return TJ.bufSizeYUV(yuvWidth, yuvPad, yuvHeight, yuvSubsamp);
   }

  protected long handle = 0;
  protected byte[] yuvBuf = null;
  protected int yuvPad = 0;
  protected int yuvWidth = 0;
  protected int yuvHeight = 0;
  protected int yuvSubsamp = -1;
};

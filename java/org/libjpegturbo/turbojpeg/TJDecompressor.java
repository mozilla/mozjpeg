/*
 * Copyright (C)2011 D. R. Commander.  All Rights Reserved.
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

import java.awt.image.*;

public class TJDecompressor {

  public TJDecompressor() throws Exception {
    init();
  }

  public TJDecompressor(byte [] buf) throws Exception {
    setJPEGBuffer(buf, buf.length);
  }

  public TJDecompressor(byte [] buf, int bufSize) throws Exception {
    setJPEGBuffer(buf, bufSize);
  }

  public void setJPEGBuffer(byte [] buf, int bufSize) throws Exception {
    if(handle == 0) init();
    if(buf == null || bufSize < 1)
      throw new Exception("Invalid argument in setJPEGBuffer()");
    jpegBuf = buf;
    jpegBufSize = bufSize;
    decompressHeader(jpegBuf, jpegBufSize);
  }

  public int getWidth() throws Exception {
    if(jpegWidth < 1) throw new Exception("JPEG buffer not initialized");
    return jpegWidth;
  }

  public int getHeight() throws Exception {
    if(jpegHeight < 1) throw new Exception("JPEG buffer not initialized");
    return jpegHeight;
  }

  public int getSubsamp() throws Exception {
    if(jpegSubsamp < 0) throw new Exception("JPEG buffer not initialized");
    return jpegSubsamp;
  }

  public int getScaledWidth(int desiredWidth, int desiredHeight)
    throws Exception {
    if(jpegWidth < 1 || jpegHeight < 1)
      throw new Exception("JPEG buffer not initialized");
    return getScaledWidth(jpegWidth, jpegHeight, desiredWidth,
      desiredHeight);
  }

  public int getScaledHeight(int desiredWidth, int desiredHeight)
    throws Exception {
    if(jpegWidth < 1 || jpegHeight < 1)
      throw new Exception("JPEG buffer not initialized");
    return getScaledHeight(jpegWidth, jpegHeight, desiredWidth,
      desiredHeight);
  }

  public void decompress(byte [] dstBuf, int desiredWidth, int pitch,
    int desiredHeight, int pixelFormat, int flags) throws Exception {
    if(jpegBuf == null) throw new Exception("JPEG buffer not initialized");
    decompress(jpegBuf, jpegBufSize, dstBuf, desiredWidth, pitch,
      desiredHeight, TJ.getPixelSize(pixelFormat),
      flags | TJ.getFlags(pixelFormat));
  }

  public byte [] decompress(int desiredWidth, int pitch, int desiredHeight,
    int pixelFormat, int flags) throws Exception {
    if(desiredWidth < 0 || desiredHeight < 0 || pitch < 0 || pixelFormat < 0
      || pixelFormat >= TJ.NUMPFOPT || flags < 0)
      throw new Exception("Invalid argument in decompress()");
    int pixelSize = TJ.getPixelSize(pixelFormat);
    int scaledWidth = getScaledWidth(desiredWidth, desiredHeight);
    int scaledHeight = getScaledHeight(desiredWidth, desiredHeight);
    if(pitch == 0) pitch = scaledWidth * pixelSize;
    int bufSize;
    if((flags&TJ.YUV)!=0)
      bufSize = TJ.bufSizeYUV(jpegWidth, jpegHeight, jpegSubsamp);
    else bufSize = pitch * scaledHeight;
    byte [] buf = new byte[bufSize];
    if(jpegBuf == null) throw new Exception("JPEG buffer not initialized");
    decompress(jpegBuf, jpegBufSize, buf, desiredWidth, pitch, desiredHeight,
      TJ.getPixelSize(pixelFormat), flags | TJ.getFlags(pixelFormat));
    return buf;
  }

  public void decompress(BufferedImage dstImage, int flags) throws Exception {
    if(dstImage == null || flags < 0)
      throw new Exception("Invalid argument in decompress()");
    flags &= ~(TJ.YUV);
    int desiredWidth = dstImage.getWidth();
    int desiredHeight = dstImage.getHeight();
    int scaledWidth = getScaledWidth(desiredWidth, desiredHeight);
    int scaledHeight = getScaledHeight(desiredWidth, desiredHeight);
    if(scaledWidth != desiredWidth || scaledHeight != desiredHeight)
      throw new Exception("BufferedImage dimensions do not match a scaled image size that TurboJPEG is capable of generating.");
    int pixelFormat;  boolean intPixels=false;
    switch(dstImage.getType()) {
      case BufferedImage.TYPE_3BYTE_BGR:
        pixelFormat=TJ.PF_BGR;  break;
      case BufferedImage.TYPE_BYTE_GRAY:
        pixelFormat=TJ.PF_GRAY;  break;
      case BufferedImage.TYPE_INT_BGR:
        pixelFormat=TJ.PF_RGBX;  intPixels=true;  break;
      case BufferedImage.TYPE_INT_RGB:
        pixelFormat=TJ.PF_BGRX;  intPixels=true;  break;
      default:
        throw new Exception("Unsupported BufferedImage format");
    }
    WritableRaster wr = dstImage.getRaster();
    if(intPixels) {
      SinglePixelPackedSampleModel sm =
        (SinglePixelPackedSampleModel)dstImage.getSampleModel();
      int pitch = sm.getScanlineStride();
      DataBufferInt db = (DataBufferInt)wr.getDataBuffer();
      int [] buf = db.getData();
      decompress(jpegBuf, jpegBufSize, buf, scaledWidth, pitch, scaledHeight,
        flags | TJ.getFlags(pixelFormat));
    }
    else {
      ComponentSampleModel sm =
        (ComponentSampleModel)dstImage.getSampleModel();
      int pixelSize = sm.getPixelStride();
      if(pixelSize != TJ.pixelSize[pixelFormat])
        throw new Exception("Inconsistency between pixel format and pixel size in BufferedImage");
      int pitch = sm.getScanlineStride();
      DataBufferByte db = (DataBufferByte)wr.getDataBuffer();
      byte [] buf = db.getData();
      decompress(buf, scaledWidth, pitch, scaledHeight, pixelFormat, flags);
    }
  }

  public void close() throws Exception {
    destroy();
  }

  protected void finalize() throws Throwable {
    try {
      close();
    } catch(Exception e) {
    }
    finally {
      super.finalize();
    }
  };

  private native void init() throws Exception;

  private native void destroy() throws Exception;

  private native void decompressHeader(byte [] srcBuf, int size)
    throws Exception;

  private native void decompress(byte [] srcBuf, int size, byte [] dstBuf,
    int desiredWidth, int pitch, int desiredHeight, int pixelSize, int flags)
    throws Exception;

  private native void decompress(byte [] srcBuf, int size, int [] dstBuf,
    int desiredWidth, int pitch, int desiredHeight, int flags)
    throws Exception;

  private native int getScaledWidth(int jpegWidth, int jpegHeight,
    int desiredWidth, int desiredHeight) throws Exception;

  private native int getScaledHeight(int jpegWidth, int jpegHeight,
    int desiredWidth, int desiredHeight) throws Exception;

  static {
    System.loadLibrary("turbojpeg");
  }

  private long handle = 0;
  private byte [] jpegBuf = null;
  private int jpegBufSize = 0;
  private int jpegWidth = 0;
  private int jpegHeight = 0;
  private int jpegSubsamp = -1;
};

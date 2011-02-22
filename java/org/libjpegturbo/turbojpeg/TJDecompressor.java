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

public class TJDecompressor {

  public TJDecompressor() throws Exception {
    init();
  }

  public TJDecompressor(byte [] buf) throws Exception {
    setJPEGBuffer(buf);
  }

  public void setJPEGBuffer(byte [] buf) throws Exception {
    if(handle == 0) init();
    if(buf == null) throw new Exception("Invalid argument in setJPEGBuffer()");
    jpegBuf = buf;
    decompressHeader();
  }

  public int getWidth() throws Exception {
    if(header.width < 1) throw new Exception("JPEG buffer not initialized");
    return header.width;
  }

  public int getHeight() throws Exception {
    if(header.height < 1) throw new Exception("JPEG buffer not initialized");
    return header.height;
  }

  public int getSubsamp() throws Exception {
    if(header.subsamp < 0) throw new Exception("JPEG buffer not initialized");
    return header.subsamp;
  }

  public int getScaledWidth(int desired_width, int desired_height)
    throws Exception {
    if(header.width < 1 || header.height < 1)
      throw new Exception("JPEG buffer not initialized");
    return getScaledWidth(header.width, header.height, desired_width,
      desired_height);
  }

  public int getScaledHeight(int output_width, int output_height)
    throws Exception {
    if(header.width < 1 || header.height < 1)
      throw new Exception("JPEG buffer not initialized");
    return getScaledHeight(header.width, header.height, output_width,
      output_height);
  }

  public void decompress(byte [] dstBuf, int width, int pitch,
    int height, int pixelFormat, int flags) throws Exception {
    if(jpegBuf == null) throw new Exception("JPEG buffer not initialized");
    decompress(jpegBuf, jpegBuf.length, dstBuf, width, pitch, height,
      TJ.getPixelSize(pixelFormat), flags | TJ.getFlags(pixelFormat));
  }

  public byte [] decompress(int width, int pitch, int height,
    int pixelFormat, int flags) throws Exception {
    if(width < 0 || height < 0 || pitch < 0 || pixelFormat < 0
      || pixelFormat >= TJ.NUMPIXFORMATS)
      throw new Exception("Invalid argument in decompress()");
    int pixelSize = TJ.getPixelSize(pixelFormat);
    int scaledWidth = getScaledWidth(width, height);
    int scaledHeight = getScaledHeight(width, height);
    if(pitch == 0) pitch = scaledWidth * pixelSize;
    long bufSize;
    if(pixelFormat == TJ.YUV)
      bufSize = TJ.bufSizeYUV(width, height, header.subsamp);
    else bufSize = pitch * scaledHeight;
    byte [] buf = new byte[(int)bufSize];
    if(jpegBuf == null) throw new Exception("JPEG buffer not initialized");
    decompress(jpegBuf, jpegBuf.length, buf, width, pitch, height,
      TJ.getPixelSize(pixelFormat), flags | TJ.getFlags(pixelFormat));
    return buf;
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

  private native TJHeaderInfo decompressHeader(byte [] srcBuf, long size)
    throws Exception;

  private void decompressHeader() throws Exception {
    header = decompressHeader(jpegBuf, jpegBuf.length);
  }

  private native void decompress(byte [] srcBuf, long size, byte [] dstBuf,
    int width, int pitch, int height, int pixelSize, int flags)
    throws Exception;

  private native int getScaledWidth(int input_width, int input_height,
    int output_width, int output_height) throws Exception;

  private native int getScaledHeight(int input_width, int input_height,
    int output_width, int output_height) throws Exception;

  static {
    System.loadLibrary("turbojpeg");
  }

  private long handle = 0;
  private byte [] jpegBuf = null;
  TJHeaderInfo header = null;
};

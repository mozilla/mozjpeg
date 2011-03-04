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

public class TJTransformer extends TJDecompressor {

  public TJTransformer() throws Exception {
    init();
  }

  public TJTransformer(byte[] buf) throws Exception {
    init();
    setJPEGBuffer(buf, buf.length);
  }

  public TJTransformer(byte[] buf, int bufSize) throws Exception {
    init();
    setJPEGBuffer(buf, bufSize);
  }

  public void transform(byte[][] dstBufs, TJTransform[] transforms,
    int flags) throws Exception {
    if(jpegBuf == null) throw new Exception("JPEG buffer not initialized");
    transformedSizes = transform(jpegBuf, jpegBufSize, dstBufs, transforms,
      flags);
  }

  public TJDecompressor[] transform(TJTransform[] transforms, int flags)
    throws Exception {
    byte[][] dstBufs = new byte[transforms.length][];
    if(jpegWidth < 1 || jpegHeight < 1)
      throw new Exception("JPEG buffer not initialized");
    for(int i = 0; i < transforms.length; i++) {
      int w = jpegWidth, h = jpegHeight;
      if((transforms[i].options & TJ.XFORM_CROP) != 0) {
        if(transforms[i].width != 0) w = transforms[i].width;
        if(transforms[i].height != 0) h = transforms[i].height;
      }
      dstBufs[i] = new byte[TJ.bufSize(w, h)];
    }
    TJDecompressor[] tjd = new TJDecompressor[transforms.length];
    transform(dstBufs, transforms, flags);
    for(int i = 0; i < transforms.length; i++)
      tjd[i] = new TJDecompressor(dstBufs[i], transformedSizes[i]);
	  return tjd;
  }

  public int[] getTransformedSizes() throws Exception {
    if(transformedSizes == null)
      throw new Exception("No image has been transformed yet");
    return transformedSizes;
  }

  private native void init() throws Exception;

  private native int[] transform(byte[] srcBuf, int srcSize, byte[][] dstBufs,
    TJTransform[] transforms, int flags) throws Exception;

  static {
    System.loadLibrary("turbojpeg");
  }

  private int[] transformedSizes = null;
};

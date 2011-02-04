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

final class TJ {

  // Subsampling options
  final static int
    SAMP444    = 0,
    SAMP422    = 1,
    SAMP420    = 2,
    GRAYSCALE  = 3;

  // Flags
  final static int
    BGR          = 1,
    BOTTOMUP     = 2,
    FORCEMMX     = 8,
    FORCESSE     = 16,
    FORCESSE2    = 32,
    ALPHAFIRST   = 64,
    FORCESSE3    = 128,
    FASTUPSAMPLE = 256,
    YUV          = 512;

  public native final static long BUFSIZE(int width, int height);
};

class tjCompressor {

  tjCompressor() throws Exception {Init();}

  public void close() throws Exception {Destroy();}

  protected void finalize() throws Throwable {
    try {
      close();
    } catch(Exception e) {
    }
    finally {
      super.finalize();
    }
  };

  private native void Init() throws Exception;

  private native void Destroy() throws Exception;

  // JPEG size in bytes is returned
  public native long Compress(byte [] srcbuf, int width, int pitch,
    int height, int pixelsize, byte [] dstbuf, int jpegsubsamp, int jpegqual,
    int flags) throws Exception;

  static {
    System.loadLibrary("turbojpeg");
  }

  private long handle=0;
};

class tjHeaderInfo {
  int subsamp=-1;
  int width=-1;
  int height=-1;
};

class tjDecompressor {

  tjDecompressor() throws Exception {Init();}

  public void close() throws Exception {Destroy();}

  protected void finalize() throws Throwable {
    try {
      close();
    } catch(Exception e) {
    }
    finally {
      super.finalize();
    }
  };

  private native void Init() throws Exception;

  private native void Destroy() throws Exception;

  public native tjHeaderInfo DecompressHeader(byte [] srcbuf, long size)
    throws Exception;

  public native void Decompress(byte [] srcbuf, long size, byte [] dstbuf,
    int width, int pitch, int height, int pixelsize, int flags)
    throws Exception;

  static {
    System.loadLibrary("turbojpeg");
  }

  private long handle=0;
};

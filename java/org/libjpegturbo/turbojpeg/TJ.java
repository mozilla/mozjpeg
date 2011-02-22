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

final public class TJ {

  // Subsampling options
  final public static int
    NUMSUBOPT  = 4,
    SAMP444    = 0,
    SAMP422    = 1,
    SAMP420    = 2,
    GRAYSCALE  = 3;

  // Pixel formats
  final public static int
    NUMPIXFORMATS = 7,
    RGB           = 0,
    BGR           = 1,
    RGBX          = 2,
    BGRX          = 3,
    XBGR          = 4,
    XRGB          = 5,
    YUV           = 6;

  final public static int pixelSize[] = {
    3, 3, 4, 4, 4, 4, 3
  };

  public static int getPixelSize(int pixelFormat) throws Exception {
    if(pixelFormat < 0 || pixelFormat >= NUMPIXFORMATS)
      throw new Exception("Invalid pixel format");
    return pixelSize[pixelFormat];
  }

  // Flags
  final public static int
    BOTTOMUP     = 2,
    FORCEMMX     = 8,
    FORCESSE     = 16,
    FORCESSE2    = 32,
    FORCESSE3    = 128,
    FASTUPSAMPLE = 256;

  final private static int
    TJ_BGR        = 1,
    TJ_ALPHAFIRST = 64,
    TJ_YUV        = 512;

  final private static int flags[] = {
    0, TJ_BGR, 0, TJ_BGR, TJ_BGR|TJ_ALPHAFIRST, TJ_ALPHAFIRST, TJ_YUV
  };

  public static int getFlags(int pixelFormat) throws Exception {
    if(pixelFormat < 0 || pixelFormat >= NUMPIXFORMATS)
      throw new Exception("Invalid pixel format");
    return flags[pixelFormat];
  }

  public native final static long bufSize(int width, int height)
    throws Exception;

  public native final static long bufSizeYUV(int width, int height,
    int subsamp)
    throws Exception;
};

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

  final public class ScalingFactor {
    public int num = 1;
    public int denom = 1;
  };

  // Chrominance subsampling options
  final public static int
    NUMSAMPOPT = 4,
    SAMP_444   = 0,
    SAMP_422   = 1,
    SAMP_420   = 2,
    SAMP_GRAY  = 3;

  // Bitmap pixel formats
  final public static int
    NUMPFOPT = 7,
    PF_RGB   = 0,
    PF_BGR   = 1,
    PF_RGBX  = 2,
    PF_BGRX  = 3,
    PF_XBGR  = 4,
    PF_XRGB  = 5,
    PF_GRAY  = 6;

  final private static int pixelSize[] = {
    3, 3, 4, 4, 4, 4, 1
  };

  public static int getPixelSize(int pixelFormat) throws Exception {
    if(pixelFormat < 0 || pixelFormat >= NUMPFOPT)
      throw new Exception("Invalid pixel format");
    return pixelSize[pixelFormat];
  }

  final private static int redShift[] = {
    0, 16, 0, 16, 24, 8, 0
  };

  public static int getRedShift(int pixelFormat) throws Exception {
    if(pixelFormat < 0 || pixelFormat >= NUMPFOPT)
      throw new Exception("Invalid pixel format");
    return redShift[pixelFormat];
  }

  final private static int greenShift[] = {
    8, 8, 8, 8, 16, 16, 0
  };

  public static int getGreenShift(int pixelFormat) throws Exception {
    if(pixelFormat < 0 || pixelFormat >= NUMPFOPT)
      throw new Exception("Invalid pixel format");
    return greenShift[pixelFormat];
  }

  final private static int blueShift[] = {
    16, 0, 16, 0, 8, 24, 0
  };

  final public static int getBlueShift(int pixelFormat) throws Exception {
    if(pixelFormat < 0 || pixelFormat >= NUMPFOPT)
      throw new Exception("Invalid pixel format");
    return blueShift[pixelFormat];
  }

  // Flags
  final public static int
    BOTTOMUP     = 2,
    FORCEMMX     = 8,
    FORCESSE     = 16,
    FORCESSE2    = 32,
    FORCESSE3    = 128,
    FASTUPSAMPLE = 256;

  public native static int bufSize(int width, int height)
    throws Exception;

  public native static int bufSizeYUV(int width, int height,
    int subsamp)
    throws Exception;

  public native static ScalingFactor[] getScalingFactors()
    throws Exception;

  static {
    System.loadLibrary("turbojpeg");
  }
};

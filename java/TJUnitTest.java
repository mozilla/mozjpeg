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

/*
 * This program tests the various code paths in the TurboJPEG JNI Wrapper
 */

import java.io.*;
import java.util.*;
import java.awt.image.*;
import javax.imageio.*;
import org.libjpegturbo.turbojpeg.*;

public class TJUnitTest {

  private static final String classname =
    new TJUnitTest().getClass().getName();

  private static void usage() {
    System.out.println("\nUSAGE: java " + classname + " [options]\n");
    System.out.println("Options:\n");
    System.out.println("-yuv = test YUV encoding/decoding support\n");
    System.out.println("-bi = test BufferedImage support\n");
    System.exit(1);
  }

  private final static String subNameLong[] = {
    "4:4:4", "4:2:2", "4:2:0", "GRAY"
  };
  private final static String subName[] = {
    "444", "422", "420", "GRAY"
  };
  private final static int horizSampFactor[] = {
    1, 2, 2, 1
  };
  private final static int vertSampFactor[] = {
    1, 1, 2, 1
  };

  private final static String pixFormatStr[] = {
    "RGB", "BGR", "RGBX", "BGRX", "XBGR", "XRGB", "Grayscale"
  };
  private final static int biType[] = {
    0, BufferedImage.TYPE_3BYTE_BGR, BufferedImage.TYPE_INT_BGR,
    BufferedImage.TYPE_INT_RGB, 0, 0, BufferedImage.TYPE_BYTE_GRAY
  };

  private final static int _3byteFormats[] = {
    TJ.PF_RGB, TJ.PF_BGR
  };
  private final static int _3byteFormatsBI[] = {
    TJ.PF_BGR
  };
  private final static int _4byteFormats[] = {
    TJ.PF_RGBX, TJ.PF_BGRX, TJ.PF_XBGR, TJ.PF_XRGB
  };
  private final static int _4byteFormatsBI[] = {
    TJ.PF_RGBX, TJ.PF_BGRX
  };
  private final static int onlyGray[] = {
    TJ.PF_GRAY
  };
  private final static int onlyRGB[] = {
    TJ.PF_RGB
  };

  private final static int YUVENCODE = 1;
  private final static int YUVDECODE = 2;
  private static int yuv = 0;
  private static boolean bi = false;

  private static int exitStatus = 0;

  private static double getTime() {
    return (double)System.nanoTime() / 1.0e9;
  }

  private final static byte pixels[][] = {
    {0, (byte)255, 0},
    {(byte)255, 0, (byte)255},
    {(byte)255, (byte)255, 0},
    {0, 0, (byte)255},
    {0, (byte)255, (byte)255},
    {(byte)255, 0, 0},
    {(byte)255, (byte)255, (byte)255},
    {0, 0, 0},
    {(byte)255, 0, 0}
  };

  private static void initBuf(byte[] buf, int w, int pitch, int h, int pf,
    int flags) throws Exception {
    int roffset = TJ.getRedShift(pf) / 8;
    int goffset = TJ.getGreenShift(pf) / 8;
    int boffset = TJ.getBlueShift(pf) / 8;
    int ps = TJ.getPixelSize(pf);
    int i, _i, j;

    Arrays.fill(buf, (byte)0);
    if(pf == TJ.PF_GRAY) {
      for(_i = 0; _i < 16; _i++) {
        if((flags & TJ.BOTTOMUP) != 0) i = h - _i - 1;
        else i = _i;
        for(j = 0; j < w; j++) {
          if(((_i / 8) + (j / 8)) % 2 == 0) buf[pitch * i + j] = (byte)255;
          else buf[pitch * i + j] = 76;
        }
      }
      for(_i = 16; _i < h; _i++) {
        if((flags & TJ.BOTTOMUP) != 0) i = h - _i - 1;
        else i = _i;
        for(j = 0; j < w; j++) {
          if(((_i / 8) + (j / 8)) % 2 == 0) buf[pitch * i + j] = 0;
          else buf[pitch * i + j] = (byte)226;
        }
      }
      return;
    }
    for(_i = 0; _i < 16; _i++) {
      if((flags & TJ.BOTTOMUP) != 0) i = h - _i - 1;
      else i = _i;
      for(j = 0; j < w; j++) {
        buf[pitch * i + j * ps + roffset] = (byte)255;
        if(((_i / 8) + (j / 8)) % 2 == 0) {
          buf[pitch * i + j * ps + goffset] = (byte)255;
          buf[pitch * i + j * ps + boffset] = (byte)255;
        }
      }
    }
    for(_i = 16; _i < h; _i++) {
      if((flags & TJ.BOTTOMUP) != 0) i = h - _i - 1;
      else i = _i;
      for(j = 0; j < w; j++) {
        if(((_i / 8) + (j / 8)) % 2 != 0) {
          buf[pitch * i + j * ps + roffset] = (byte)255;
          buf[pitch * i + j * ps + goffset] = (byte)255;
        }
      }
    }
  }

  private static void initIntBuf(int[] buf, int w, int pitch, int h, int pf,
    int flags) throws Exception {
    int rshift = TJ.getRedShift(pf);
    int gshift = TJ.getGreenShift(pf);
    int bshift = TJ.getBlueShift(pf);
    int i, _i, j;

    Arrays.fill(buf, 0);
    for(_i = 0; _i < 16; _i++) {
      if((flags & TJ.BOTTOMUP) != 0) i = h - _i - 1;
      else i = _i;
      for(j = 0; j < w; j++) {
        buf[pitch * i + j] = (255 << rshift);
        if(((_i / 8) + (j / 8)) % 2 == 0) {
          buf[pitch * i + j] |= (255 << gshift);
          buf[pitch * i + j] |= (255 << bshift);
        }
      }
    }
    for(_i = 16; _i < h; _i++) {
      if((flags & TJ.BOTTOMUP) != 0) i = h - _i - 1;
      else i = _i;
      for(j = 0; j < w; j++) {
        if(((_i / 8) + (j / 8)) % 2 != 0) {
          buf[pitch * i + j] = (255 << rshift);
          buf[pitch * i + j] |= (255 << gshift);
        }
      }
    }
  }

  private static void initImg(BufferedImage img, int pf, int flags)
    throws Exception {
    WritableRaster wr = img.getRaster();
    int imgtype = img.getType();
    if(imgtype == BufferedImage.TYPE_INT_RGB
      || imgtype == BufferedImage.TYPE_INT_BGR) {
      SinglePixelPackedSampleModel sm =
        (SinglePixelPackedSampleModel)img.getSampleModel();
      int pitch = sm.getScanlineStride();
      DataBufferInt db = (DataBufferInt)wr.getDataBuffer();
      int[] buf = db.getData();
      initIntBuf(buf, img.getWidth(), pitch, img.getHeight(), pf, flags);
    }
    else {
      ComponentSampleModel sm = (ComponentSampleModel)img.getSampleModel();
      int pitch = sm.getScanlineStride();
      DataBufferByte db = (DataBufferByte)wr.getDataBuffer();
      byte[] buf = db.getData();
      initBuf(buf, img.getWidth(), pitch, img.getHeight(), pf, flags);
    }
  }

  private static void checkVal(int i, int j, int v, String vname, int cv)
    throws Exception {
    v = (v < 0) ? v + 256 : v;
    if(v < cv - 1 || v > cv + 1) {
      throw new Exception("\nComp. " + vname + " at " + i + "," + j
        + " should be " + cv + ", not " + v + "\n");
    }
  }

  private static void checkVal0(int i, int j, int v, String vname)
    throws Exception {
    v = (v < 0) ? v + 256 : v;
    if(v > 1) {
      throw new Exception("\nComp. " + vname + " at " + i + "," + j
        + " should be 0, not " + v + "\n");
    }
  }

  private static void checkVal255(int i, int j, int v, String vname)
    throws Exception {
    v = (v < 0) ? v + 256 : v;
    if(v < 254) {
      throw new Exception("\nComp. " + vname + " at " + i + "," + j
        + " should be 255, not " + v + "\n");
    }
  }

  private static int checkBuf(byte[] buf, int w, int pitch, int h, int pf,
    int subsamp, int scaleNum, int scaleDenom, int flags) throws Exception {
    int roffset = TJ.getRedShift(pf) / 8;
    int goffset = TJ.getGreenShift(pf) / 8;
    int boffset = TJ.getBlueShift(pf) / 8;
    int ps = TJ.getPixelSize(pf);
    int i, _i, j, retval = 1;
    int halfway = 16 * scaleNum / scaleDenom;
    int blockSize = 8 * scaleNum / scaleDenom;

    try {
      for(_i = 0; _i < halfway; _i++) {
        if((flags & TJ.BOTTOMUP) != 0) i = h - _i - 1;
        else i = _i;
        for(j = 0; j < w; j++) {
          byte r = buf[pitch * i + j * ps + roffset];
          byte g = buf[pitch * i + j * ps + goffset];
          byte b = buf[pitch * i + j * ps + boffset];
          if(((_i / blockSize) + (j / blockSize)) % 2 == 0) {
            checkVal255(_i, j, r, "R");
            checkVal255(_i, j, g, "G");
            checkVal255(_i, j, b, "B");
          }
          else {
            if(subsamp == TJ.SAMP_GRAY) {
              checkVal(_i, j, r, "R", 76);
              checkVal(_i, j, g, "G", 76);
              checkVal(_i, j, b, "B", 76);
            }
            else {
              checkVal255(_i, j, r, "R");
              checkVal0(_i, j, g, "G");
              checkVal0(_i, j, b, "B");
            }
          }
        }
      }
      for(_i = halfway; _i < h; _i++) {
        if((flags & TJ.BOTTOMUP) != 0) i = h - _i - 1;
        else i = _i;
        for(j = 0; j < w; j++) {
          byte r = buf[pitch * i + j * ps + roffset];
          byte g = buf[pitch * i + j * ps + goffset];
          byte b = buf[pitch * i + j * ps + boffset];
          if(((_i / blockSize) + (j / blockSize)) % 2 == 0) {
            checkVal0(_i, j, r, "R");
            checkVal0(_i, j, g, "G");
          }
          else {
            if(subsamp == TJ.SAMP_GRAY) {
              checkVal(_i, j, r, "R", 226);
              checkVal(_i, j, g, "G", 226);
              checkVal(_i, j, b, "B", 226);
            }
            else {
              checkVal255(_i, j, r, "R");
              checkVal255(_i, j, g, "G");
              checkVal0(_i, j, b, "B");							
            }
          }
        }
      }
    }
    catch(Exception e) {
      System.out.println(e);
      retval = 0;
    }

    if(retval == 0) {
      System.out.print("\n");
      for(i = 0; i < h; i++) {
        for(j = 0; j < w; j++) {
          int r = buf[pitch * i + j * ps + roffset];
          int g = buf[pitch * i + j * ps + goffset];
          int b = buf[pitch * i + j * ps + boffset];
          if(r < 0) r += 256;  if(g < 0) g += 256;  if(b < 0) b += 256;
          System.out.format("%3d/%3d/%3d ", r, g, b);
        }
        System.out.print("\n");
      }
    }
    return retval;
  }

  private static int checkIntBuf(int[] buf, int w, int pitch, int h, int pf,
    int subsamp, int scaleNum, int scaleDenom, int flags) throws Exception {
    int rshift = TJ.getRedShift(pf);
    int gshift = TJ.getGreenShift(pf);
    int bshift = TJ.getBlueShift(pf);
    int i, _i, j, retval = 1;
    int halfway = 16 * scaleNum / scaleDenom;
    int blockSize = 8 * scaleNum / scaleDenom;

    try {
      for(_i = 0; _i < halfway; _i++) {
        if((flags & TJ.BOTTOMUP) != 0) i = h - _i - 1;
        else i = _i;
        for(j = 0; j < w; j++) {
          int r = (buf[pitch * i + j] >> rshift) & 0xFF;
          int g = (buf[pitch * i + j] >> gshift) & 0xFF;
          int b = (buf[pitch * i + j] >> bshift) & 0xFF;
          if(((_i / blockSize) + (j / blockSize)) % 2 == 0) {
            checkVal255(_i, j, r, "R");
            checkVal255(_i, j, g, "G");
            checkVal255(_i, j, b, "B");
          }
          else {
            if(subsamp == TJ.SAMP_GRAY) {
              checkVal(_i, j, r, "R", 76);
              checkVal(_i, j, g, "G", 76);
              checkVal(_i, j, b, "B", 76);
            }
            else {
              checkVal255(_i, j, r, "R");
              checkVal0(_i, j, g, "G");
              checkVal0(_i, j, b, "B");
            }
          }
        }
      }
      for(_i = halfway; _i < h; _i++) {
        if((flags & TJ.BOTTOMUP) != 0) i = h - _i - 1;
        else i = _i;
        for(j = 0; j < w; j++) {
          int r = (buf[pitch * i + j] >> rshift) & 0xFF;
          int g = (buf[pitch * i + j] >> gshift) & 0xFF;
          int b = (buf[pitch * i + j] >> bshift) & 0xFF;
          if(((_i / blockSize) + (j / blockSize)) % 2 == 0) {
            checkVal0(_i, j, r, "R");
            checkVal0(_i, j, g, "G");
          }
          else {
            if(subsamp == TJ.SAMP_GRAY) {
              checkVal(_i, j, r, "R", 226);
              checkVal(_i, j, g, "G", 226);
              checkVal(_i, j, b, "B", 226);
            }
            else {
              checkVal255(_i, j, r, "R");
              checkVal255(_i, j, g, "G");
              checkVal0(_i, j, b, "B");
            }
          }
        }
      }
    }
    catch(Exception e) {
      System.out.println(e);
      retval = 0;
    }

    if(retval == 0) {
      System.out.print("\n");
      for(i = 0; i < h; i++) {
        for(j = 0; j < w; j++) {
          int r = (buf[pitch * i + j] >> rshift) & 0xFF;
          int g = (buf[pitch * i + j] >> gshift) & 0xFF;
          int b = (buf[pitch * i + j] >> bshift) & 0xFF;
          if(r < 0) r += 256;  if(g < 0) g += 256;  if(b < 0) b += 256;
          System.out.format("%3d/%3d/%3d ", r, g, b);
        }
        System.out.print("\n");
      }
    }
    return retval;
  }

  private static int checkImg(BufferedImage img, int pf,
    int subsamp, int scaleNum, int scaleDenom, int flags) throws Exception {
    WritableRaster wr = img.getRaster();
    int imgtype = img.getType();
    if(imgtype == BufferedImage.TYPE_INT_RGB
      || imgtype == BufferedImage.TYPE_INT_BGR) {
      SinglePixelPackedSampleModel sm =
        (SinglePixelPackedSampleModel)img.getSampleModel();
      int pitch = sm.getScanlineStride();
      DataBufferInt db = (DataBufferInt)wr.getDataBuffer();
      int[] buf = db.getData();
      return checkIntBuf(buf, img.getWidth(), pitch, img.getHeight(), pf,
        subsamp, scaleNum, scaleDenom, flags);
    }
    else {
      ComponentSampleModel sm = (ComponentSampleModel)img.getSampleModel();
      int pitch = sm.getScanlineStride();
      DataBufferByte db = (DataBufferByte)wr.getDataBuffer();
      byte[] buf = db.getData();
      return checkBuf(buf, img.getWidth(), pitch, img.getHeight(), pf, subsamp,
        scaleNum, scaleDenom, flags);
    }
  }

  private static int PAD(int v, int p) {
    return ((v + (p) - 1) & (~((p) - 1)));
  }

  private static int checkBufYUV(byte[] buf, int size, int w, int h,
    int subsamp) {
    int i, j;
    int hsf = horizSampFactor[subsamp], vsf = vertSampFactor[subsamp];
    int pw = PAD(w, hsf), ph = PAD(h, vsf);
    int cw = pw / hsf, ch = ph / vsf;
    int ypitch = PAD(pw, 4), uvpitch = PAD(cw, 4);
    int retval = 1;
    int correctsize = ypitch * ph
      + (subsamp == TJ.SAMP_GRAY ? 0 : uvpitch * ch * 2);

    try {
      if(size != correctsize)
        throw new Exception("\nIncorrect size " + size + ".  Should be "
          + correctsize);

      for(i = 0; i < 16; i++) {
        for(j = 0; j < pw; j++) {
          byte y = buf[ypitch * i + j];
          if(((i / 8) + (j / 8)) % 2 == 0) checkVal255(i, j, y, "Y");
          else checkVal(i, j, y, "Y", 76);
        }
      }
      for(i = 16; i < ph; i++) {
        for(j = 0; j < pw; j++) {
          byte y = buf[ypitch * i + j];
          if(((i / 8) + (j / 8)) % 2 == 0) checkVal0(i, j, y, "Y");
          else checkVal(i, j, y, "Y", 226);
        }
      }
      if(subsamp != TJ.SAMP_GRAY) {
        for(i = 0; i < 16 / vsf; i++) {
          for(j = 0; j < cw; j++) {
            byte u = buf[ypitch * ph + (uvpitch * i + j)],
              v = buf[ypitch * ph + uvpitch * ch + (uvpitch * i + j)];
            if(((i * vsf / 8) + (j * hsf / 8)) % 2 == 0) {
              checkVal(i, j, u, "U", 128);  checkVal(i, j, v, "V", 128);
            }
            else {
              checkVal(i, j, u, "U", 85);  checkVal255(i, j, v, "V");
            }
          }
        }
        for(i = 16 / vsf; i < ch; i++) {
          for(j = 0; j < cw; j++) {
            byte u = buf[ypitch * ph + (uvpitch * i + j)],
              v = buf[ypitch * ph + uvpitch * ch + (uvpitch * i + j)];
            if(((i * vsf / 8) + (j * hsf / 8)) % 2 == 0) {
              checkVal(i, j, u, "U", 128);  checkVal(i, j, v, "V", 128);
            }
            else {
              checkVal0(i, j, u, "U");  checkVal(i, j, v, "V", 149);
            }
          }
        }
      }
    }
    catch(Exception e) {
      System.out.println(e);
      retval = 0;
    }

    if(retval == 0) {
      for(i = 0; i < ph; i++) {
        for(j = 0; j < pw; j++) {
          int y = buf[ypitch * i + j];
          if(y < 0) y += 256;
          System.out.format("%3d ", y);
        }
        System.out.print("\n");
      }
      System.out.print("\n");
      for(i = 0; i < ch; i++) {
        for(j = 0; j < cw; j++) {
          int u = buf[ypitch * ph + (uvpitch * i + j)];
          if(u < 0) u += 256;
          System.out.format("%3d ", u);
        }
        System.out.print("\n");
      }
      System.out.print("\n");
      for(i = 0; i < ch; i++) {
        for(j = 0; j < cw; j++) {
          int v = buf[ypitch * ph + uvpitch * ch + (uvpitch * i + j)];
          if(v < 0) v += 256;
          System.out.format("%3d ", v);
        }
        System.out.print("\n");
      }
      System.out.print("\n");
    }

    return retval;
  }

  private static void writeJPEG(byte[] jpegBuf, int jpegBufSize,
    String filename) throws Exception {
    File file = new File(filename);
    FileOutputStream fos = new FileOutputStream(file);
    fos.write(jpegBuf, 0, jpegBufSize);
    fos.close();
  }

  private static int genTestJPEG(TJCompressor tjc, byte[] jpegBuf, int w,
    int h, int pf, String baseFilename, int subsamp, int qual,
    int flags) throws Exception {
    String tempstr;
    byte[] bmpBuf = null;
    BufferedImage img = null;
    String pfStr;
    double t;
    int size = 0, ps = TJ.getPixelSize(pf);

    pfStr = pixFormatStr[pf];

    System.out.print(pfStr + " ");
    if((flags & TJ.BOTTOMUP) != 0) System.out.print("Bottom-Up");
    else System.out.print("Top-Down ");
    System.out.print(" -> " + subNameLong[subsamp] + " ");
    if(yuv == YUVENCODE) System.out.print("YUV ... ");
    else System.out.print("Q" + qual + " ... ");

    if(bi) {
      img = new BufferedImage(w, h, biType[pf]);
      initImg(img, pf, flags);
      tempstr = baseFilename + "_enc_" + pfStr + "_"
        + (((flags & TJ.BOTTOMUP) != 0) ? "BU" : "TD") + "_"
        + subName[subsamp] + "_Q" + qual + ".png";
      File file = new File(tempstr);
      ImageIO.write(img, "png", file);
    }
    else {
      bmpBuf = new byte[w * h * ps + 1];
      initBuf(bmpBuf, w, w * ps, h, pf, flags);
    }
    Arrays.fill(jpegBuf, (byte)0);

    t = getTime();
    tjc.setSubsamp(subsamp);
    tjc.setJPEGQuality(qual);
    if(bi) {
      if(yuv == YUVENCODE) tjc.encodeYUV(img, jpegBuf, flags);
      else tjc.compress(img, jpegBuf, flags);
    }
    else {
      tjc.setBitmapBuffer(bmpBuf, w, 0, h, pf);
      if(yuv == YUVENCODE) tjc.encodeYUV(jpegBuf, flags);
      else tjc.compress(jpegBuf, flags);
    }
    size = tjc.getCompressedSize();
    t = getTime() - t;

    if(yuv == YUVENCODE)
      tempstr = baseFilename + "_enc_" + pfStr + "_"
        + (((flags & TJ.BOTTOMUP) != 0) ? "BU" : "TD") + "_"
        + subName[subsamp] + ".yuv";
    else
      tempstr = baseFilename + "_enc_" + pfStr + "_"
        + (((flags & TJ.BOTTOMUP) != 0) ? "BU" : "TD") + "_"
        + subName[subsamp] + "_Q" + qual + ".jpg";
    writeJPEG(jpegBuf, size, tempstr);

    if(yuv == YUVENCODE) {
      if(checkBufYUV(jpegBuf, size, w, h, subsamp) == 1)
        System.out.print("Passed.");
      else {
        System.out.print("FAILED!");  exitStatus = -1;
      }
    }
    else System.out.print("Done.");
    System.out.format("  %.6f ms\n", t * 1000.);
    System.out.println("  Result in " + tempstr);

    return size;
  }

  private static void genTestBMP(TJDecompressor tjd, byte[] jpegBuf,
    int jpegsize, int w, int h, int pf, String baseFilename, int subsamp,
    int flags, int scaleNum, int scaleDenom) throws Exception {
    String pfStr, tempstr;
    double t;
    int scaledWidth = (w * scaleNum + scaleDenom - 1) / scaleDenom;
    int scaledHeight = (h * scaleNum + scaleDenom - 1) / scaleDenom;
    int temp1, temp2;
    BufferedImage img = null;
    byte[] bmpBuf = null;

    if(yuv == YUVENCODE) return;

    pfStr = pixFormatStr[pf];
    System.out.print("JPEG -> ");
    if(yuv == YUVDECODE)
      System.out.print("YUV " + subName[subsamp] + " ... ");
    else {
      System.out.print(pfStr + " ");
      if((flags & TJ.BOTTOMUP) != 0) System.out.print("Bottom-Up ");
      else System.out.print("Top-Down  ");
      if(scaleNum != 1 || scaleDenom != 1)
        System.out.print(scaleNum + "/" + scaleDenom + " ... ");
      else System.out.print("... ");
    }

    t = getTime();
    tjd.setJPEGBuffer(jpegBuf, jpegsize);
    if(tjd.getWidth() != w || tjd.getHeight() != h
      || tjd.getSubsamp() != subsamp)
      throw new Exception("Incorrect JPEG header");

    temp1 = scaledWidth;
    temp2 = scaledHeight;
    temp1 = tjd.getScaledWidth(temp1, temp2);
    temp2 = tjd.getScaledHeight(temp1, temp2);
    if(temp1 != scaledWidth || temp2 != scaledHeight)
      throw new Exception("Scaled size mismatch");

    if(yuv == YUVDECODE) bmpBuf = tjd.decompressToYUV(flags);
    else {
      if(bi)
        img = tjd.decompress(scaledWidth, scaledHeight, biType[pf], flags);
      else bmpBuf = tjd.decompress(scaledWidth, 0, scaledHeight, pf, flags);
    }
    t = getTime() - t;

    if(bi) {
      tempstr = baseFilename + "_dec_" + pfStr + "_"
        + (((flags & TJ.BOTTOMUP) != 0) ? "BU" : "TD") + "_"
        + subName[subsamp] + "_" + (double)scaleNum / (double)scaleDenom
        + "x" + ".png";
      File file = new File(tempstr);
      ImageIO.write(img, "png", file);
    }

    if(yuv == YUVDECODE) {
      if(checkBufYUV(bmpBuf, bmpBuf.length, w, h, subsamp) == 1)
        System.out.print("Passed.");
      else {
        System.out.print("FAILED!");  exitStatus = -1;
      }
    }
    else {
      if((bi && checkImg(img, pf, subsamp, scaleNum, scaleDenom, flags) == 1)
        || (!bi && checkBuf(bmpBuf, scaledWidth, scaledWidth
          * TJ.getPixelSize(pf), scaledHeight, pf, subsamp, scaleNum,
          scaleDenom, flags) == 1))
        System.out.print("Passed.");
      else {
        System.out.print("FAILED!");  exitStatus = -1;
      }
    }
    System.out.format("  %.6f ms\n", t * 1000.);
  }

  private static void genTestBMP(TJDecompressor tjd, byte[] jpegBuf,
    int jpegsize, int w, int h, int pf, String baseFilename, int subsamp,
    int flags) throws Exception {
    int i;
    if((subsamp == TJ.SAMP_444 || subsamp == TJ.SAMP_GRAY) && yuv == 0) {
      TJ.ScalingFactor sf[] = TJ.getScalingFactors();
      for(i = 0; i < sf.length; i++)
        genTestBMP(tjd, jpegBuf, jpegsize, w, h, pf, baseFilename, subsamp,
          flags, sf[i].num, sf[i].denom);
    }
    else
      genTestBMP(tjd, jpegBuf, jpegsize, w, h, pf, baseFilename, subsamp,
        flags, 1, 1);
    System.out.print("\n");
  }

  private static void doTest(int w, int h, int[] formats, int subsamp,
    String baseFilename) throws Exception {
    TJCompressor tjc = null;
    TJDecompressor tjd = null;
    int size, pfstart, pfend;
    byte[] jpegBuf;

    if(yuv == YUVENCODE) jpegBuf = new byte[TJ.bufSizeYUV(w, h, subsamp)];
    else jpegBuf = new byte[TJ.bufSize(w, h)];

    try {
      tjc = new TJCompressor();
      tjd = new TJDecompressor();  

      for(int pf : formats) {
        for(int i = 0; i < 2; i++) {
          int flags = 0;
          if(i == 1) {
            if(yuv == YUVDECODE) {
              tjc.close();  tjd.close();  return;
            }
            else flags |= TJ.BOTTOMUP;
          }
          size = genTestJPEG(tjc, jpegBuf, w, h, pf, baseFilename, subsamp,
            100, flags);
          genTestBMP(tjd, jpegBuf, size, w, h, pf, baseFilename, subsamp,
            flags);
        }
      }
    }
    catch(Exception e) {
      if(tjc != null) tjc.close();
      if(tjd != null) tjd.close();
      throw e;
    }
    if(tjc != null) tjc.close();
    if(tjd != null) tjd.close();
  }

  private final static int MAXLENGTH = 2048;

  private static void doTest1() throws Exception {
    int i, j, i2;
    byte[] bmpBuf, jpegBuf;
    TJCompressor tjc = null;

    try {
      tjc = new TJCompressor();
      System.out.println("Buffer size regression test");
      for(j = 1; j < 48; j++) {
        for(i = 1; i < (j == 1 ? MAXLENGTH : 48); i++) {
          if(i % 100 == 0)
            System.out.format("%04d x %04d\b\b\b\b\b\b\b\b\b\b\b", i, j);
          bmpBuf = new byte[i * j * 4];
          jpegBuf = new byte[TJ.bufSize(i, j)];
          Arrays.fill(bmpBuf, (byte)0);
          for(i2 = 0; i2 < i * j; i2++) {
            bmpBuf[i2 * 4] = pixels[i2 % 9][2];
            bmpBuf[i2 * 4 + 1] = pixels[i2 % 9][1];
            bmpBuf[i2 * 4 + 2] = pixels[i2 % 9][0];
          }
          tjc.setBitmapBuffer(bmpBuf, i, 0, j, TJ.PF_BGRX);
          tjc.setSubsamp(TJ.SAMP_444);
          tjc.setJPEGQuality(100);
          tjc.compress(jpegBuf, 0);

          bmpBuf = new byte[j * i * 4];
          jpegBuf = new byte[TJ.bufSize(j, i)];
          for(i2 = 0; i2 < j * i; i2++) {
            if(i2 % 2 == 0) bmpBuf[i2 * 4] =
                bmpBuf[i2 * 4 + 1] = bmpBuf[i2 * 4 + 2] = (byte)0xFF;
            else bmpBuf[i2 * 4] = bmpBuf[i2 * 4 + 1] = bmpBuf[i2 * 4 + 2] = 0;
          }
          tjc.setBitmapBuffer(bmpBuf, j, 0, i, TJ.PF_BGRX);
          tjc.compress(jpegBuf, 0);
        }
      }
      System.out.println("Done.      ");
    }
    catch(Exception e) {
      if(tjc != null) tjc.close();
      throw e;
    }
    if(tjc != null) tjc.close();
  }

  public static void main(String argv[]) {
    try {
      boolean doyuv = false;
      for(int i = 0; i < argv.length; i++) {
        if(argv[i].equalsIgnoreCase("-yuv")) doyuv = true;
        if(argv[i].substring(0, 1).equalsIgnoreCase("-h")
          || argv[i].equalsIgnoreCase("-?"))
          usage();
        if(argv[i].equalsIgnoreCase("-bi")) bi = true;
      }
      if(doyuv) yuv = YUVENCODE;
      doTest(35, 39, bi ? _3byteFormatsBI : _3byteFormats, TJ.SAMP_444, "test");
      doTest(39, 41, bi ? _4byteFormatsBI : _4byteFormats, TJ.SAMP_444, "test");
      if(doyuv) {
        doTest(41, 35, bi ? _3byteFormatsBI : _3byteFormats, TJ.SAMP_422,
          "test");
        doTest(35, 39, bi ? _4byteFormatsBI : _4byteFormats, TJ.SAMP_422,
          "test");
        doTest(39, 41, bi ? _3byteFormatsBI : _3byteFormats, TJ.SAMP_420,
          "test");
        doTest(41, 35, bi ? _4byteFormatsBI : _4byteFormats, TJ.SAMP_420,
          "test");
      }
      doTest(35, 39, onlyGray, TJ.SAMP_GRAY, "test");
      doTest(39, 41, bi ? _3byteFormatsBI : _3byteFormats, TJ.SAMP_GRAY,
        "test");
      doTest(41, 35, bi ? _4byteFormatsBI : _4byteFormats, TJ.SAMP_GRAY,
        "test");
      if(!doyuv && !bi) doTest1();
      if(doyuv && !bi) {
        yuv = YUVDECODE;
        doTest(48, 48, onlyRGB, TJ.SAMP_444, "test");
        doTest(35, 39, onlyRGB, TJ.SAMP_444, "test");
        doTest(48, 48, onlyRGB, TJ.SAMP_422, "test");
        doTest(39, 41, onlyRGB, TJ.SAMP_422, "test");
        doTest(48, 48, onlyRGB, TJ.SAMP_420, "test");
        doTest(41, 35, onlyRGB, TJ.SAMP_420, "test");
        doTest(48, 48, onlyRGB, TJ.SAMP_GRAY, "test");
        doTest(35, 39, onlyRGB, TJ.SAMP_GRAY, "test");
        doTest(48, 48, onlyGray, TJ.SAMP_GRAY, "test");
        doTest(39, 41, onlyGray, TJ.SAMP_GRAY, "test");
      }
    }
    catch(Exception e) {
      System.out.println(e);
      exitStatus = -1;
    }
    System.exit(exitStatus);
  }
}

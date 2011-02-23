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
 * This program demonstrates how to compress and decompress JPEG files using
 * the TurboJPEG JNI wrapper
 */

import java.io.*;
import java.awt.image.*;
import javax.imageio.*;
import org.libjpegturbo.turbojpeg.*;

public class TJExample {

  public static final String classname = new TJExample().getClass().getName();

  private static void usage() {
    System.out.println("\nUSAGE: java " + classname + " <Input file> <Output file> [options]\n");
    System.out.println("Input and output files can be any image format that the Java Image I/O");
    System.out.println("extensions understand.  If either filename ends in a .jpg extension, then");
    System.out.println("TurboJPEG will be used to compress or decompress the file.\n");
    System.out.println("Options:\n");
    System.out.println("-scale 1/N = if the input image is a JPEG file, scale the width/height of the");
    System.out.println("             output image by a factor of 1/N (N = 1, 2, 4, or 8}\n");
    System.out.println("-samp <444|422|420|gray> = If the output image is a JPEG file, this specifies");
    System.out.println("                           the level of chrominance subsampling to use when");
    System.out.println("                           recompressing it.  Default is to use the same level");
    System.out.println("                           of subsampling as the input, if the input is a JPEG");
    System.out.println("                           file, or 4:4:4 otherwise.\n");
    System.out.println("-q <1-100> = If the output image is a JPEG file, this specifies the JPEG");
    System.out.println("             quality to use when recompressing it (default = 95).\n");
    System.exit(1);
  }

  private final static String sampName[] = {
    "4:4:4", "4:2:2", "4:2:0", "Grayscale"
  };

  public static void main(String argv[]) {

    BufferedImage img = null;  byte [] bmpBuf = null;

    try {

      if(argv.length < 2) {
        usage();
      }

      int scaleFactor = 1;
      String inFormat = "jpg", outFormat = "jpg";
      int outSubsamp = -1, outQual = 95;

      if(argv.length > 2) {
        for(int i = 2; i < argv.length; i++) {
          if(argv[i].length() < 2) continue;
          if(argv[i].length() > 2
            && argv[i].substring(0, 3).equalsIgnoreCase("-sc")) {
            if(i < argv.length - 1) {
              String [] scaleArg = argv[++i].split("/");
              if(scaleArg.length != 2 || Integer.parseInt(scaleArg[0]) != 1
                || (scaleFactor = Integer.parseInt(scaleArg[1])) < 1
                || scaleFactor > 8 || (scaleFactor & (scaleFactor - 1)) != 0)
                usage();
            }
            else usage();
          }
          if(argv[i].substring(0, 2).equalsIgnoreCase("-h")
            || argv[i].equalsIgnoreCase("-?"))
            usage();
          if(argv[i].length() > 2
            && argv[i].substring(0, 3).equalsIgnoreCase("-sa")) {
            if(i < argv.length - 1) {
              i++;
              if(argv[i].substring(0, 1).equalsIgnoreCase("g"))
                outSubsamp = TJ.SAMP_GRAY;
              else if(argv[i].equals("444")) outSubsamp = TJ.SAMP_444;
              else if(argv[i].equals("422")) outSubsamp = TJ.SAMP_422;
              else if(argv[i].equals("420")) outSubsamp = TJ.SAMP_420;
              else usage();
            }
            else usage();
          }
          if(argv[i].substring(0, 2).equalsIgnoreCase("-q")) {
            if(i < argv.length - 1) {
              int qual = Integer.parseInt(argv[++i]);
              if(qual >= 1 && qual <= 100) outQual = qual;
              else usage();
            }
            else usage();
          }
        }
      }
      String [] inFileTokens = argv[0].split("\\.");
      if(inFileTokens.length > 1)
        inFormat = inFileTokens[inFileTokens.length - 1];
      String [] outFileTokens = argv[1].split("\\.");
      if(outFileTokens.length > 1)
        outFormat = outFileTokens[outFileTokens.length - 1];

      File file = new File(argv[0]);
      int width, height, subsamp = TJ.SAMP_444;

      if(inFormat.equalsIgnoreCase("jpg")) {
        FileInputStream fis = new FileInputStream(file);
        int inputSize = fis.available();
        if(inputSize < 1) {
          System.out.println("Input file contains no data");
          System.exit(1);
        }
        byte [] inputBuf = new byte[inputSize];
        fis.read(inputBuf);
        fis.close();

        TJDecompressor tjd = new TJDecompressor(inputBuf);
        width = tjd.getWidth();
        height = tjd.getHeight();
        int inSubsamp = tjd.getSubsamp();
        System.out.println("Source Image: " + width + " x " + height
          + " pixels, " + sampName[inSubsamp] + " subsampling");
        if(outSubsamp < 0) outSubsamp = inSubsamp;

        if(scaleFactor != 1) {
          width = (width + scaleFactor - 1)/scaleFactor;
          height = (height + scaleFactor - 1)/scaleFactor;
        }

        if(!outFormat.equalsIgnoreCase("jpg")) {
          img = new BufferedImage(width, height, BufferedImage.TYPE_INT_RGB);
          tjd.decompress(img, 0);
        }
        else bmpBuf = tjd.decompress(width, 0, height, TJ.PF_BGRX, 0);
        tjd.close();
      }
      else {
        img = ImageIO.read(file);
        width = img.getWidth();
        height = img.getHeight();
        if(outSubsamp < 0) {
          if(img.getType() == BufferedImage.TYPE_BYTE_GRAY)
            outSubsamp = TJ.SAMP_GRAY;
          else outSubsamp = TJ.SAMP_444;
        }
      }
      System.out.print("Dest. Image (" + outFormat + "):  " + width + " x "
        + height + " pixels");

      if(outFormat.equalsIgnoreCase("jpg")) {
        System.out.println(", " + sampName[outSubsamp]
          + " subsampling, quality = " + outQual);
        TJCompressor tjc = new TJCompressor();
        int jpegSize;
        byte [] jpegBuf = new byte[TJ.bufSize(width, height)];

        if(img != null)
          jpegSize = tjc.compress(img, jpegBuf, outSubsamp, outQual, 0);
        else {
          tjc.setBitmapBuffer(bmpBuf, width, 0, height, TJ.PF_BGRX);
          jpegSize = tjc.compress(jpegBuf, outSubsamp, outQual, 0);
        }
        tjc.close();

        file = new File(argv[1]);
        FileOutputStream fos = new FileOutputStream(file);
        fos.write(jpegBuf, 0, jpegSize);
        fos.close();
      }
      else {
        System.out.print("\n");
        file = new File(argv[1]);
        ImageIO.write(img, outFormat, file);
      }

    } catch(Exception e) {
      System.out.println(e);
    }
  }

};

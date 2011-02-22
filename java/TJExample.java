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
import org.libjpegturbo.turbojpeg.*;

public class TJExample {

  public static final String classname=new TJExample().getClass().getName();

  private static void usage() {
    System.out.println("\nUSAGE: java "+classname+" <Input file> <Output file> [options]\n");
    System.out.println("Options:\n");
    System.out.println("-scale 1/N = scale the width/height of the output image by a factor of 1/N");
    System.out.println("             (N = 1, 2, 4, or 8}\n");
    System.exit(1);
  }

  public static void main(String argv[]) {

    try {

      if(argv.length<2) {
        usage();
      }

      int scalefactor=1;
      if(argv.length>2) {
        for(int i=2; i<argv.length; i++) {
          if(argv[i].equalsIgnoreCase("-scale") && i<argv.length-1) {
            String [] scalearg=argv[++i].split("/");      
            if(scalearg.length!=2 || Integer.parseInt(scalearg[0])!=1
              || (scalefactor=Integer.parseInt(scalearg[1]))<1
              || scalefactor>8 || (scalefactor&(scalefactor-1))!=0)
              usage();
          }
        }
      }

      File file=new File(argv[0]);
      FileInputStream fis=new FileInputStream(file);
      int inputsize=fis.available();
      if(inputsize<1) {
        System.out.println("Input file contains no data");
        System.exit(1);
      }
      byte [] inputbuf=new byte[inputsize];
      fis.read(inputbuf);
      fis.close();

      TJDecompressor tjd=new TJDecompressor(inputbuf);
      int width=tjd.getWidth();
      int height=tjd.getHeight();
      int subsamp=tjd.getSubsamp();
      System.out.print("Source Image: "+width+" x "+height+" pixels, ");
      switch(subsamp) {
        case TJ.SAMP444:  System.out.println("4:4:4 subsampling");  break;
        case TJ.SAMP422:  System.out.println("4:2:2 subsampling");  break;
        case TJ.SAMP420:  System.out.println("4:2:0 subsampling");  break;
        case TJ.GRAYSCALE:  System.out.println("Grayscale");  break;
        default:  System.out.println("Unknown subsampling");  break;
      }

      if(scalefactor!=1) {
        width=(width+scalefactor-1)/scalefactor;
        height=(height+scalefactor-1)/scalefactor;
        System.out.println("Dest. Image:  "+width+" x "+height
          +" pixels");
      }

      byte [] tmpbuf=tjd.decompress(width, 0, height, TJ.BGR, TJ.BOTTOMUP);
      tjd.close();

      TJCompressor tjc=new TJCompressor(tmpbuf, width, 0, height, TJ.BGR);
      byte [] outputbuf=new byte[(int)TJ.bufSize(width, height)];
      long outputsize=tjc.compress(outputbuf, subsamp, 95, TJ.BOTTOMUP);
      tjc.close();

      file=new File(argv[1]);
      FileOutputStream fos=new FileOutputStream(file);
      fos.write(outputbuf, 0, (int)outputsize);
      fos.close();

    } catch(Exception e) {
      System.out.println(e);
    }
  }

};

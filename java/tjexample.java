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

public class tjexample {

  public static final String classname=new tjexample().getClass().getName();

  public static void main(String argv[]) {

    try {

      if(argv.length<2) {
        System.out.println("USAGE: java "+classname+" <Input file> <Output file>");
        System.exit(1);
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

      tjDecompressor tjd=new tjDecompressor();
      tjHeaderInfo tji=tjd.DecompressHeader(inputbuf, inputsize);
      System.out.print("Source Image: "+tji.width+" x "+tji.height+ " pixels, ");
      switch(tji.subsamp) {
        case TJ.SAMP444:  System.out.println("4:4:4 subsampling");  break;
        case TJ.SAMP422:  System.out.println("4:2:2 subsampling");  break;
        case TJ.SAMP420:  System.out.println("4:2:0 subsampling");  break;
        case TJ.GRAYSCALE:  System.out.println("Grayscale");  break;
        default:  System.out.println("Unknown subsampling");  break;
      }
      byte [] tmpbuf=new byte[tji.width*tji.height*3];
      tjd.Decompress(inputbuf, inputsize, tmpbuf, tji.width, tji.width*3,
        tji.height, 3, TJ.BOTTOMUP);
      tjd.close();

      tjCompressor tjc=new tjCompressor();
      byte [] outputbuf=new byte[(int)TJ.BUFSIZE(tji.width, tji.height)];
      long outputsize=tjc.Compress(tmpbuf, tji.width, tji.width*3, tji.height,
        3, outputbuf, tji.subsamp, 95, TJ.BOTTOMUP);
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

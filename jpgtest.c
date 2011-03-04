/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005, 2006 Sun Microsystems, Inc.
 * Copyright (C)2009-2011 D. R. Commander
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3.1 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include "./bmp.h"
#include "./rrutil.h"
#include "./rrtimer.h"
#include "./turbojpeg.h"

#define _throw(op, err) {  \
	printf("ERROR in line %d while %s:\n%s\n", __LINE__, op, err);  \
  retval=-1;  goto bailout;}
#define _throwunix(m) _throw(m, strerror(errno))
#define _throwtj(m) _throw(m, tjGetErrorStr())
#define _throwbmp(m) _throw(m, bmpgeterr())

enum {YUVENCODE=1, YUVDECODE};
int forcemmx=0, forcesse=0, forcesse2=0, forcesse3=0, fastupsample=0,
	decomponly=0, yuv=0, quiet=0, dotile=0, pf=BMP_BGR, bu=0, useppm=0,
	scale_num=1, scale_denom=1;
const int _ps[BMPPIXELFORMATS]={3, 4, 3, 4, 4, 4};
const int _flags[BMPPIXELFORMATS]={0, 0, TJ_BGR, TJ_BGR,
	TJ_BGR|TJ_ALPHAFIRST, TJ_ALPHAFIRST};
const int _rindex[BMPPIXELFORMATS]={0, 0, 2, 2, 3, 1};
const int _gindex[BMPPIXELFORMATS]={1, 1, 1, 1, 2, 2};
const int _bindex[BMPPIXELFORMATS]={2, 2, 0, 0, 1, 3};
const char *_pfname[]={"RGB", "RGBX", "BGR", "BGRX", "XBGR", "XRGB"};
const char *_subnamel[NUMSUBOPT]={"4:4:4", "4:2:2", "4:2:0", "GRAY", "4:4:0"};
const char *_subnames[NUMSUBOPT]={"444", "422", "420", "GRAY", "440"};
tjscalingfactor *sf=NULL;  int nsf=0;
int xformop=TJXFORM_NONE, xformopt=0;
double benchtime=5.0;

void printsigfig(double val, int figs)
{
	char format[80];
	double _l=log10(val);  int l;
	if(_l<0.)
	{
		l=(int)fabs(_l);
		snprintf(format, 80, "%%%d.%df", figs+l+2, figs+l);
	}
	else
	{
		l=(int)_l+1;
		if(figs<=l) snprintf(format, 80, "%%.0f");
		else snprintf(format, 80, "%%%d.%df", figs+1, figs-l);
	}	
	printf(format, val);
}

// Decompression test
int decomptest(unsigned char *srcbuf, unsigned char **jpegbuf,
	unsigned long *comptilesize, unsigned char *rgbbuf, int w, int h,
	int jpegsub, int qual, char *filename, int tilesizex, int tilesizey)
{
	char tempstr[1024], sizestr[20]="\0", qualstr[5]="\0", *ptr;
	FILE *outfile=NULL;  tjhandle hnd=NULL;
	int flags=(forcemmx?TJ_FORCEMMX:0)|(forcesse?TJ_FORCESSE:0)
		|(forcesse2?TJ_FORCESSE2:0)|(forcesse3?TJ_FORCESSE3:0)
		|(fastupsample?TJ_FASTUPSAMPLE:0);
	int i, j, ITER, rgbbufalloc=0, retval=0;
	double start, elapsed;
	int ps=_ps[pf];
	int yuvsize=TJBUFSIZEYUV(w, h, jpegsub), bufsize;
	int scaledw=(yuv==YUVDECODE)? w : (w*scale_num+scale_denom-1)/scale_denom;
	int scaledh=(yuv==YUVDECODE)? h : (h*scale_num+scale_denom-1)/scale_denom;
	int pitch=scaledw*ps;

	if(qual>0)
	{
		snprintf(qualstr, 5, "Q%d", qual);
		qualstr[4]=0;
	}

	flags |= _flags[pf];
	if(bu) flags |= TJ_BOTTOMUP;
	if((hnd=tjInitDecompress())==NULL)
		_throwtj("executing tjInitDecompress()");

	bufsize=(yuv==YUVDECODE? yuvsize:pitch*h);
	if(rgbbuf==NULL)
	{
		if((rgbbuf=(unsigned char *)malloc(bufsize)) == NULL)
			_throwunix("allocating image buffer");
		rgbbufalloc=1;
	}
	// Grey image means decompressor did nothing
	memset(rgbbuf, 127, bufsize);

	if(yuv==YUVDECODE)
	{
		if(tjDecompressToYUV(hnd, jpegbuf[0], comptilesize[0], rgbbuf, flags)==-1)
			_throwtj("executing tjDecompressToYUV()");
	}
	else if(tjDecompress(hnd, jpegbuf[0], comptilesize[0], rgbbuf, scaledw,
		pitch, scaledh, ps, flags)==-1)
		_throwtj("executing tjDecompress()");
	ITER=0;
	start=rrtime();
	do
	{
		int tilen=0;
		for(i=0; i<h; i+=tilesizey)
		{
			for(j=0; j<w; j+=tilesizex)
			{
				int tempw=dotile? min(tilesizex, w-j):scaledw;
				int temph=dotile? min(tilesizey, h-i):scaledh;
				if(yuv==YUVDECODE)
				{
					if(tjDecompressToYUV(hnd, jpegbuf[tilen], comptilesize[tilen],
						&rgbbuf[pitch*i+ps*j], flags)==-1)
						_throwtj("executing tjDecompressToYUV()");
				}
				else if(tjDecompress(hnd, jpegbuf[tilen], comptilesize[tilen],
					&rgbbuf[pitch*i+ps*j], tempw, pitch, temph, ps, flags)==-1)
					_throwtj("executing tjDecompress()");
				tilen++;
			}
		}
		ITER++;
	}	while((elapsed=rrtime()-start)<benchtime);
	if(tjDestroy(hnd)==-1) _throwtj("executing tjDestroy()");
	hnd=NULL;
	if(quiet)
	{
		printsigfig((double)(w*h)/1000000.*(double)ITER/elapsed, 4);
		printf("\n");
	}
	else
	{
		printf("D--> Frame rate:           %f fps\n", (double)ITER/elapsed);
		printf("     Dest. throughput:     %f Megapixels/sec\n",
			(double)(w*h)/1000000.*(double)ITER/elapsed);
	}
	if(yuv==YUVDECODE)
	{
		snprintf(tempstr, 1024, "%s_%s%s.yuv", filename, _subnames[jpegsub],
			qualstr);
		if((outfile=fopen(tempstr, "wb"))==NULL)
			_throwunix("opening YUV image for output");
		if(fwrite(rgbbuf, yuvsize, 1, outfile)!=1)
			_throwunix("writing YUV image");
		fclose(outfile);  outfile=NULL;
	}
	else
	{
		if(scale_num!=1 || scale_denom!=1)
			snprintf(sizestr, 20, "%d_%d", scale_num, scale_denom);
		else if(tilesizex!=w || tilesizey!=h)
			snprintf(sizestr, 20, "%dx%d", tilesizex, tilesizey);
		else snprintf(sizestr, 20, "full");
		if(decomponly)
			snprintf(tempstr, 1024, "%s_%s.%s", filename, sizestr,
				useppm?"ppm":"bmp");
		else
			snprintf(tempstr, 1024, "%s_%s%s_%s.%s", filename,
				_subnames[jpegsub], qualstr, sizestr, useppm?"ppm":"bmp");
		if(savebmp(tempstr, rgbbuf, scaledw, scaledh, pf, pitch, bu)==-1)
			_throwbmp("saving bitmap");
		ptr=strrchr(tempstr, '.');
		snprintf(ptr, 1024-(ptr-tempstr), "-err.%s", useppm?"ppm":"bmp");
		if(srcbuf && scale_num==1 && scale_denom==1)
		{
			if(!quiet)
				printf("Computing compression error and saving to %s.\n", tempstr);
			if(jpegsub==TJ_GRAYSCALE)
			{
				for(j=0; j<h; j++)
				{
					for(i=0; i<w*ps; i+=ps)
					{
						int y=(int)((double)srcbuf[w*ps*j+i+_rindex[pf]]*0.299
							+ (double)srcbuf[w*ps*j+i+_gindex[pf]]*0.587
							+ (double)srcbuf[w*ps*j+i+_bindex[pf]]*0.114 + 0.5);
						if(y>255) y=255;  if(y<0) y=0;
						rgbbuf[pitch*j+i+_rindex[pf]]=abs(rgbbuf[pitch*j+i+_rindex[pf]]-y);
						rgbbuf[pitch*j+i+_gindex[pf]]=abs(rgbbuf[pitch*j+i+_gindex[pf]]-y);
						rgbbuf[pitch*j+i+_bindex[pf]]=abs(rgbbuf[pitch*j+i+_bindex[pf]]-y);
					}
				}
			}		
			else
			{
				for(j=0; j<h; j++) for(i=0; i<w*ps; i++)
					rgbbuf[pitch*j+i]=abs(rgbbuf[pitch*j+i]-srcbuf[w*ps*j+i]);
			}
			if(savebmp(tempstr, rgbbuf, w, h, pf, pitch, bu)==-1)
				_throwbmp("saving bitmap");
		}
	}

	bailout:
	if(outfile) {fclose(outfile);  outfile=NULL;}
	if(hnd) {tjDestroy(hnd);  hnd=NULL;}
	if(rgbbuf && rgbbufalloc) {free(rgbbuf);  rgbbuf=NULL;}
	return retval;
}

void dotest(unsigned char *srcbuf, int w, int h, int jpegsub, int qual,
	char *filename)
{
	char tempstr[1024];
	FILE *outfile=NULL;  tjhandle hnd=NULL;
	unsigned char **jpegbuf=NULL, *rgbbuf=NULL;
	double start, elapsed;
	int jpgbufsize=0, i, j, tilesizex=w, tilesizey=h, numtilesx=1, numtilesy=1,
		ITER, retval=0;
	unsigned long *comptilesize=NULL;
	int flags=(forcemmx?TJ_FORCEMMX:0)|(forcesse?TJ_FORCESSE:0)
		|(forcesse2?TJ_FORCESSE2:0)|(forcesse3?TJ_FORCESSE3:0)
		|(fastupsample?TJ_FASTUPSAMPLE:0);
	int ps=_ps[pf], tilen;
	int pitch=w*ps, yuvsize=0;

	flags |= _flags[pf];
	if(bu) flags |= TJ_BOTTOMUP;

	if(yuv==YUVENCODE) yuvsize=TJBUFSIZEYUV(w, h, jpegsub);
	if((rgbbuf=(unsigned char *)malloc(max(yuvsize, pitch*h+1))) == NULL)
		_throwunix("allocating image buffer");

	if(!quiet)
	{
		if(yuv==YUVENCODE)
			printf(">>>>>  %s (%s) <--> YUV %s  <<<<<\n", _pfname[pf],
				bu?"Bottom-up":"Top-down", _subnamel[jpegsub]);
		else
			printf(">>>>>  %s (%s) <--> JPEG %s Q%d  <<<<<\n", _pfname[pf],
				bu?"Bottom-up":"Top-down", _subnamel[jpegsub], qual);
	}
	if(yuv) dotile=0;
	if(dotile) {tilesizex=tilesizey=4;}  else {tilesizex=w;  tilesizey=h;}

	do
	{
		tilesizex*=2;  if(tilesizex>w) tilesizex=w;
		tilesizey*=2;  if(tilesizey>h) tilesizey=h;
		numtilesx=(w+tilesizex-1)/tilesizex;
		numtilesy=(h+tilesizey-1)/tilesizey;

		if((jpegbuf=(unsigned char **)malloc(sizeof(unsigned char *)
			*numtilesx*numtilesy))==NULL)
			_throwunix("allocating image buffer array");
		if((comptilesize=(unsigned long *)malloc(sizeof(unsigned long)
			*numtilesx*numtilesy))==NULL)
			_throwunix("allocating image size array");
		memset(jpegbuf, 0, sizeof(unsigned char *)*numtilesx*numtilesy);

		for(i=0; i<numtilesx*numtilesy; i++)
		{
			if((jpegbuf[i]=(unsigned char *)malloc(
				yuv==YUVENCODE? TJBUFSIZEYUV(tilesizex, tilesizey, jpegsub)
					: TJBUFSIZE(tilesizex, tilesizey))) == NULL)
				_throwunix("allocating image buffers");
		}

		// Compression test
		if(quiet==1) printf("%s\t%s\t%s\t%d\t",  _pfname[pf], bu?"BU":"TD",
			_subnamel[jpegsub], qual);
		for(i=0; i<h; i++) memcpy(&rgbbuf[pitch*i], &srcbuf[w*ps*i], w*ps);
		if((hnd=tjInitCompress())==NULL)
			_throwtj("executing tjInitCompress()");
		if(yuv==YUVENCODE)
		{
			if(tjEncodeYUV(hnd, rgbbuf, tilesizex, pitch, tilesizey, ps,
				jpegbuf[0], jpegsub, flags)==-1)
				_throwtj("executing tjEncodeYUV()");
			comptilesize[0]=TJBUFSIZEYUV(tilesizex, tilesizey, jpegsub);
		}
		else if(tjCompress(hnd, rgbbuf, tilesizex, pitch, tilesizey, ps,
			jpegbuf[0], &comptilesize[0], jpegsub, qual, flags)==-1)
			_throwtj("executing tjCompress()");

		ITER=0;
		start=rrtime();
		do
		{
			jpgbufsize=0;  tilen=0;
			for(i=0; i<h; i+=tilesizey)
			{
				for(j=0; j<w; j+=tilesizex)
				{
					int tempw=min(tilesizex, w-j), temph=min(tilesizey, h-i);
					if(yuv==YUVENCODE)
					{
						if(tjEncodeYUV(hnd, &rgbbuf[pitch*i+j*ps], tempw, pitch,
							temph, ps, jpegbuf[tilen], jpegsub, flags)==-1)
							_throwtj("executing tjEncodeYUV()");
						comptilesize[tilen]=TJBUFSIZEYUV(tempw, temph, jpegsub);
					}
					else if(tjCompress(hnd, &rgbbuf[pitch*i+j*ps], tempw, pitch,
						temph, ps, jpegbuf[tilen], &comptilesize[tilen], jpegsub, qual,
						flags)==-1)
						_throwtj("executing tjCompress()");
					jpgbufsize+=comptilesize[tilen];
					tilen++;
				}
			}
			ITER++;
		} while((elapsed=rrtime()-start)<benchtime);
		if(tjDestroy(hnd)==-1) _throwtj("executing tjDestroy()");
		hnd=NULL;
		if(quiet==1) printf("%-4d  %-4d\t", tilesizex, tilesizey);
		if(quiet)
		{
			printsigfig((double)(w*h)/1000000.*(double)ITER/elapsed, 4);
			printf("%c", quiet==2? '\n':'\t');
			printsigfig((double)(w*h*ps)/(double)jpgbufsize, 4);
			printf("%c", quiet==2? '\n':'\t');
		}
		else
		{
			printf("\n%s size: %d x %d\n", dotile? "Tile":"Image", tilesizex,
				tilesizey);
			printf("C--> Frame rate:           %f fps\n", (double)ITER/elapsed);
			printf("     Output image size:    %d bytes\n", jpgbufsize);
			printf("     Compression ratio:    %f:1\n",
				(double)(w*h*ps)/(double)jpgbufsize);
			printf("     Source throughput:    %f Megapixels/sec\n",
				(double)(w*h)/1000000.*(double)ITER/elapsed);
			printf("     Output bit stream:    %f Megabits/sec\n",
				(double)jpgbufsize*8./1000000.*(double)ITER/elapsed);
		}
		if(tilesizex==w && tilesizey==h)
		{
			if(yuv==YUVENCODE)
				snprintf(tempstr, 1024, "%s_%s.yuv", filename, _subnames[jpegsub]);
			else
				snprintf(tempstr, 1024, "%s_%sQ%d.jpg", filename, _subnames[jpegsub],
					qual);
			if((outfile=fopen(tempstr, "wb"))==NULL)
				_throwunix("opening reference image");
			if(fwrite(jpegbuf[0], jpgbufsize, 1, outfile)!=1)
				_throwunix("writing reference image");
			fclose(outfile);  outfile=NULL;
			if(!quiet) printf("Reference image written to %s\n", tempstr);
		}
		if(yuv==YUVENCODE)
		{
			if(quiet==1) printf("\n");  goto bailout;
		}

		// Decompression test
		if(decomptest(srcbuf, jpegbuf, comptilesize, rgbbuf, w, h, jpegsub, qual,
			filename, tilesizex, tilesizey)==-1)
			goto bailout;

		// Cleanup
		for(i=0; i<numtilesx*numtilesy; i++)
			{if(jpegbuf[i]) free(jpegbuf[i]);  jpegbuf[i]=NULL;}
		free(jpegbuf);  jpegbuf=NULL;
		free(comptilesize);  comptilesize=NULL;
	} while(tilesizex<w || tilesizey<h);

	bailout:
	if(outfile) {fclose(outfile);  outfile=NULL;}
	if(jpegbuf)
	{
		for(i=0; i<numtilesx*numtilesy; i++)
			{if(jpegbuf[i]) free(jpegbuf[i]);  jpegbuf[i]=NULL;}
		free(jpegbuf);  jpegbuf=NULL;
	}
	if(comptilesize) {free(comptilesize);  comptilesize=NULL;}
	if(rgbbuf) {free(rgbbuf);  rgbbuf=NULL;}
	if(hnd) {tjDestroy(hnd);  hnd=NULL;}
	return;
}


void dodecomptest(char *filename)
{
	FILE *file=NULL;  tjhandle hnd=NULL;
	unsigned char **jpegbuf=NULL, *srcbuf=NULL;
	unsigned long *comptilesize=NULL, srcbufsize, jpgbufsize;
	tjtransform *t=NULL;
	int w=0, h=0, jpegsub=-1, _w, _h, _tilesizex, _tilesizey,
		_numtilesx, _numtilesy, _jpegsub;
	char *temp=NULL;
	int i, j, tilesizex, tilesizey, numtilesx, numtilesy, retval=0;
	double start, elapsed;
	int flags=(forcemmx?TJ_FORCEMMX:0)|(forcesse?TJ_FORCESSE:0)
		|(forcesse2?TJ_FORCESSE2:0)|(forcesse3?TJ_FORCESSE3:0);
	int ps=_ps[pf], tilen;

	useppm=1;

	if((file=fopen(filename, "rb"))==NULL)
		_throwunix("opening file");
	if(fseek(file, 0, SEEK_END)<0 || (srcbufsize=ftell(file))<0)
		_throwunix("determining file size");
	if((srcbuf=(unsigned char *)malloc(srcbufsize))==NULL)
		_throwunix("allocating memory");
	if(fseek(file, 0, SEEK_SET)<0)
		_throwunix("setting file position");
	if(fread(srcbuf, srcbufsize, 1, file)<1)
		_throwunix("reading JPEG data");
	fclose(file);  file=NULL;

	temp=strrchr(filename, '.');
	if(temp!=NULL) *temp='\0';

	if((hnd=tjInitTransform())==NULL) _throwtj("executing tjInitTransform()");
	if(tjDecompressHeader2(hnd, srcbuf, srcbufsize, &w, &h, &jpegsub)==-1)
		_throwtj("executing tjDecompressHeader2()");

	if(yuv) dotile=0;

	if(dotile) {tilesizex=tilesizey=8;}  else {tilesizex=w;  tilesizey=h;}

	if(quiet==1)
	{
		printf("All performance values in Mpixels/sec\n\n");
		printf("Bitmap\tBitmap\tJPEG\t%s %s \tXform\tCompr\tDecomp\n",
			dotile? "Tile ":"Image", dotile? "Tile ":"Image");
		printf("Format\tOrder\tFormat\tWidth Height\tPerf \tRatio\tPerf\n\n");
	}
	else if(!quiet)
	{
		printf(">>>>>  JPEG %s --> %s (%s)  <<<<<\n", _subnamel[jpegsub],
			_pfname[pf], bu?"Bottom-up":"Top-down");
	}

	do
	{
		tilesizex*=2;  if(tilesizex>w) tilesizex=w;
		tilesizey*=2;  if(tilesizey>h) tilesizey=h;
		numtilesx=(w+tilesizex-1)/tilesizex;
		numtilesy=(h+tilesizey-1)/tilesizey;

		if((jpegbuf=(unsigned char **)malloc(sizeof(unsigned char *)
			*numtilesx*numtilesy))==NULL)
			_throwunix("allocating image buffer array");
		if((comptilesize=(unsigned long *)malloc(sizeof(unsigned long)
			*numtilesx*numtilesy))==NULL)
			_throwunix("allocating image size array");
		memset(jpegbuf, 0, sizeof(unsigned char *)*numtilesx*numtilesy);

		for(i=0; i<numtilesx*numtilesy; i++)
		{
			if((jpegbuf[i]=(unsigned char *)malloc(
				TJBUFSIZE(tilesizex, tilesizey))) == NULL)
				_throwunix("allocating image buffers");
		}

		_w=w;  _h=h;  _tilesizex=tilesizex;  _tilesizey=tilesizey;
		if(!quiet)
		{
			printf("\n%s size: %d x %d\n", dotile? "Tile":"Image", _tilesizex,
				_tilesizey);
			if(scale_num!=1 || scale_denom!=1)
				printf(" --> %d x %d", (_w*scale_num+scale_denom-1)/scale_denom,
					(_h*scale_num+scale_denom-1)/scale_denom);
		}
		else if(quiet==1)
		{
			printf("%s\t%s\t%s\t",  _pfname[pf], bu?"BU":"TD", _subnamel[jpegsub]);
			printf("%-4d  %-4d\t", tilesizex, tilesizey);
		}

		_jpegsub=jpegsub;
		if(dotile || xformop!=TJXFORM_NONE || xformopt!=0)
		{
			if((t=(tjtransform *)malloc(sizeof(tjtransform)*numtilesx*numtilesy))
				==NULL)
				_throwunix("allocating image transform array");

			if(xformop==TJXFORM_TRANSPOSE || xformop==TJXFORM_TRANSVERSE
				|| xformop==TJXFORM_ROT90 || xformop==TJXFORM_ROT270)
			{
				_w=h;  _h=w;  _tilesizex=tilesizey;  _tilesizey=tilesizex;
			}

			if(xformopt&TJXFORM_GRAY) _jpegsub=TJ_GRAYSCALE;
			if(xformop==TJXFORM_HFLIP || xformop==TJXFORM_ROT180)
				_w=_w-(_w%tjmcuw[_jpegsub]);
			if(xformop==TJXFORM_VFLIP || xformop==TJXFORM_ROT180)
				_h=_h-(_h%tjmcuh[_jpegsub]);
			if(xformop==TJXFORM_TRANSVERSE || xformop==TJXFORM_ROT90)
				_w=_w-(_w%tjmcuh[_jpegsub]);
			if(xformop==TJXFORM_TRANSVERSE || xformop==TJXFORM_ROT270)
				_h=_h-(_h%tjmcuw[_jpegsub]);
			_numtilesx=(_w+_tilesizex-1)/_tilesizex;
			_numtilesy=(_h+_tilesizey-1)/_tilesizey;

			for(i=0, tilen=0; i<_h; i+=_tilesizey)
			{
				for(j=0; j<_w; j+=_tilesizex, tilen++)
				{
					t[tilen].r.w=min(_tilesizex, _w-j);
					t[tilen].r.h=min(_tilesizey, _h-i);
					t[tilen].r.x=j;
					t[tilen].r.y=i;
					t[tilen].op=xformop;
					t[tilen].options=xformopt|TJXFORM_TRIM;
				}
			}

			start=rrtime();
			if(tjTransform(hnd, srcbuf, srcbufsize, _numtilesx*_numtilesy, jpegbuf,
				comptilesize, t, flags)==-1)
				_throwtj("executing tjTransform()");
			elapsed=rrtime()-start;

			for(tilen=0, jpgbufsize=0; tilen<_numtilesx*_numtilesy; tilen++)
				jpgbufsize+=comptilesize[tilen];

			if(quiet)
			{
				printsigfig((double)(w*h)/1000000./elapsed, 4);
				printf("%c", quiet==2? '\n':'\t');
				printsigfig((double)(w*h*ps)/(double)jpgbufsize, 4);
				printf("%c", quiet==2? '\n':'\t');
			}
			else if(!quiet)
			{
				printf("X--> Frame rate:           %f fps\n", 1.0/elapsed);
				printf("     Output image size:    %lu bytes\n", jpgbufsize);
				printf("     Compression ratio:    %f:1\n",
					(double)(w*h*ps)/(double)jpgbufsize);
				printf("     Source throughput:    %f Megapixels/sec\n",
					(double)(w*h)/1000000./elapsed);
				printf("     Output bit stream:    %f Megabits/sec\n",
					(double)jpgbufsize*8./1000000./elapsed);
			}
		}
		else
		{
			if(quiet==1) printf("N/A\tN/A\t");
			comptilesize[0]=srcbufsize;
			memcpy(jpegbuf[0], srcbuf, srcbufsize);
		}

		if(w==tilesizex) _tilesizex=_w;
		if(h==tilesizey) _tilesizey=_h;
		if(decomptest(NULL, jpegbuf, comptilesize, NULL, _w, _h, _jpegsub, 0,
			filename, _tilesizex, _tilesizey)==-1)
			goto bailout;

		// Cleanup
		for(i=0; i<numtilesx*numtilesy; i++)
			{free(jpegbuf[i]);  jpegbuf[i]=NULL;}
		free(jpegbuf);  jpegbuf=NULL;
		if(comptilesize) {free(comptilesize);  comptilesize=NULL;}
	} while(tilesizex<w || tilesizey<h);

	bailout:
	if(file) {fclose(file);  file=NULL;}
	if(jpegbuf)
	{
		for(i=0; i<numtilesx*numtilesy; i++)
			{if(jpegbuf[i]) free(jpegbuf[i]);  jpegbuf[i]=NULL;}
		free(jpegbuf);  jpegbuf=NULL;
	}
	if(comptilesize) {free(comptilesize);  comptilesize=NULL;}
	if(srcbuf) {free(srcbuf);  srcbuf=NULL;}
	if(hnd) {tjDestroy(hnd);  hnd=NULL;}
	return;
}


void usage(char *progname)
{
	int i;
	printf("USAGE: %s\n", progname);
	printf("       <Inputfile (BMP|PPM)> <%% Quality> [options]\n\n");
	printf("       %s\n", progname);
	printf("       <Inputfile (JPG)> [options]\n\n");
	printf("Options:\n\n");
	printf("-tile = Test performance of the codec when the image is encoded as separate\n");
	printf("     tiles of varying sizes.\n");
	printf("-forcemmx, -forcesse, -forcesse2, -forcesse3 =\n");
	printf("     Force MMX, SSE, SSE2, or SSE3 code paths in the underlying codec\n");
	printf("-rgb, -bgr, -rgbx, -bgrx, -xbgr, -xrgb =\n");
	printf("     Test the specified color conversion path in the codec (default: BGR)\n");
	printf("-fastupsample = Use fast, inaccurate upsampling code to perform 4:2:2 and 4:2:0\n");
	printf("     YUV decoding in libjpeg decompressor\n");
	printf("-quiet = Output results in tabular rather than verbose format\n");
	printf("-yuvencode = Encode RGB input as planar YUV rather than compressing as JPEG\n");
	printf("-yuvdecode = Decode JPEG image to planar YUV rather than RGB\n");
	printf("-scale M/N = scale down the width/height of the decompressed JPEG image by a\n");
	printf("     factor of M/N (M/N = ");
	for(i=0; i<nsf; i++)
	{
		printf("%d/%d", sf[i].num, sf[i].denom);
		if(nsf==2 && i!=nsf-1) printf(" or ");
		else if(nsf>2)
		{
			if(i!=nsf-1) printf(", ");
			if(i==nsf-2) printf("or ");
		}
	}
	printf(")\n");
	printf("-hflip, -vflip, -transpose, -transverse, -rot90, -rot180, -rot270 =\n");
	printf("     Perform the corresponding lossless transform prior to\n");
	printf("     decompression (these options are mutually exclusive)\n");
	printf("-grayscale = Perform lossless grayscale conversion prior to decompression\n");
	printf("     test (can be combined with the other transforms above)\n");
	printf("-benchtime <t> = Run each benchmark for at least <t> seconds (default = 5.0)\n\n");
	printf("NOTE:  If the quality is specified as a range (e.g. 90-100), a separate\n");
	printf("test will be performed for all quality values in the range.\n\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	unsigned char *bmpbuf=NULL;  int w, h, i, j;
	int qual=-1, hiqual=-1;  char *temp;
	int minarg=2;  int retval=0;

	if((sf=tjGetScalingFactors(&nsf))==NULL || nsf==0)
		_throwtj("executing tjGetScalingFactors()");

	if(argc<minarg) usage(argv[0]);

	temp=strrchr(argv[1], '.');
	if(temp!=NULL)
	{
		if(!stricmp(temp, ".ppm")) useppm=1;
		if(!stricmp(temp, ".jpg") || !stricmp(temp, ".jpeg")) decomponly=1;
	}

	printf("\n");

	if(argc>minarg)
	{
		for(i=minarg; i<argc; i++)
		{
			if(!stricmp(argv[i], "-yuvencode"))
			{
				printf("Testing YUV planar encoding\n\n");
				yuv=YUVENCODE;  hiqual=qual=100;
			}
			if(!stricmp(argv[i], "-yuvdecode"))
			{
				printf("Testing YUV planar decoding\n\n");
				yuv=YUVDECODE;
			}
		}
	}

	if(!decomponly && yuv!=YUVENCODE)
	{
		minarg=3;
		if(argc<minarg) usage(argv[0]);
		if((qual=atoi(argv[2]))<1 || qual>100)
		{
			puts("ERROR: Quality must be between 1 and 100.");
			exit(1);
		}
		if((temp=strchr(argv[2], '-'))!=NULL && strlen(temp)>1
			&& sscanf(&temp[1], "%d", &hiqual)==1 && hiqual>qual && hiqual>=1
			&& hiqual<=100) {}
		else hiqual=qual;
	}

	if(argc>minarg)
	{
		for(i=minarg; i<argc; i++)
		{
			if(!stricmp(argv[i], "-tile"))
			{
				dotile=1;  xformopt|=TJXFORM_CROP;
			}
			if(!stricmp(argv[i], "-forcesse3"))
			{
				printf("Using SSE3 code\n\n");
				forcesse3=1;
			}
			if(!stricmp(argv[i], "-forcesse2"))
			{
				printf("Using SSE2 code\n\n");
				forcesse2=1;
			}
			if(!stricmp(argv[i], "-forcesse"))
			{
				printf("Using SSE code\n\n");
				forcesse=1;
			}
			if(!stricmp(argv[i], "-forcemmx"))
			{
				printf("Using MMX code\n\n");
				forcemmx=1;
			}
			if(!stricmp(argv[i], "-fastupsample"))
			{
				printf("Using fast upsampling code\n\n");
				fastupsample=1;
			}
			if(!stricmp(argv[i], "-rgb")) pf=BMP_RGB;
			if(!stricmp(argv[i], "-rgbx")) pf=BMP_RGBX;
			if(!stricmp(argv[i], "-bgr")) pf=BMP_BGR;
			if(!stricmp(argv[i], "-bgrx")) pf=BMP_BGRX;
			if(!stricmp(argv[i], "-xbgr")) pf=BMP_XBGR;
			if(!stricmp(argv[i], "-xrgb")) pf=BMP_XRGB;
			if(!stricmp(argv[i], "-bottomup")) bu=1;
			if(!stricmp(argv[i], "-quiet")) quiet=1;
			if(!stricmp(argv[i], "-qq")) quiet=2;
			if(!stricmp(argv[i], "-scale") && i<argc-1)
			{
				int temp1=0, temp2=0, match=0;
				if(sscanf(argv[++i], "%d/%d", &temp1, &temp2)==2)
				{
					for(j=0; j<nsf; j++)
					{
						if(temp1==sf[j].num && temp2==sf[j].denom)
						{
							scale_num=temp1;  scale_denom=temp2;
							match=1;  break;
						}
					}
					if(!match) usage(argv[0]);
				}
				else usage(argv[0]);
			}
			if(!stricmp(argv[i], "-hflip")) xformop=TJXFORM_HFLIP;
			if(!stricmp(argv[i], "-vflip")) xformop=TJXFORM_VFLIP;
			if(!stricmp(argv[i], "-transpose")) xformop=TJXFORM_TRANSPOSE;
			if(!stricmp(argv[i], "-transverse")) xformop=TJXFORM_TRANSVERSE;
			if(!stricmp(argv[i], "-rot90")) xformop=TJXFORM_ROT90;
			if(!stricmp(argv[i], "-rot180")) xformop=TJXFORM_ROT180;
			if(!stricmp(argv[i], "-rot270")) xformop=TJXFORM_ROT270;
			if(!stricmp(argv[i], "-grayscale")) xformopt|=TJXFORM_GRAY;
			if(!stricmp(argv[i], "-benchtime") && i<argc-1)
			{
				double temp=atof(argv[++i]);
				if(temp>0.0) benchtime=temp;
				else usage(argv[0]);
			}
		}
	}

	if((scale_num!=1 || scale_denom!=1) && dotile)
	{
		printf("Disabling tiled compression/decompression tests, because these tests do not\n");
		printf("work when scaled decompression is enabled.\n");
		dotile=0;
	}

	if(!decomponly)
	{
		if(loadbmp(argv[1], &bmpbuf, &w, &h, pf, 1, bu)==-1)
			_throwbmp("loading bitmap");
		temp=strrchr(argv[1], '.');
		if(temp!=NULL) *temp='\0';
	}

	if(quiet==1 && !decomponly)
	{
		printf("All performance values in Mpixels/sec\n\n");
		printf("Bitmap\tBitmap\tJPEG\tJPEG\t%s %s \tCompr\tCompr\tDecomp\n",
			dotile? "Tile ":"Image", dotile? "Tile ":"Image");
		printf("Format\tOrder\tFormat\tQual\tWidth Height\tPerf \tRatio\tPerf\n\n");
	}

	if(decomponly)
	{
		dodecomptest(argv[1]);
		printf("\n");
		goto bailout;
	}
	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, TJ_GRAYSCALE, i, argv[1]);
	printf("\n");
	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, TJ_420, i, argv[1]);
	printf("\n");
	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, TJ_422, i, argv[1]);
	printf("\n");
	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, TJ_444, i, argv[1]);
	printf("\n");

	bailout:
	if(bmpbuf) free(bmpbuf);
	return retval;
}

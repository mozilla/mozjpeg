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
	printf("ERROR in line %d while %s:\n%s\n", __LINE__, op, err);  goto bailout;}
#define _throwunix(m) _throw(m, strerror(errno))
#define _throwtj(m) _throw(m, tjGetErrorStr())
#define _throwbmp(m) _throw(m, bmpgeterr())

#define PAD(v, p) ((v+(p)-1)&(~((p)-1)))

enum {YUVENCODE=1, YUVDECODE};
int forcemmx=0, forcesse=0, forcesse2=0, forcesse3=0, fastupsample=0,
	decomponly=0, yuv=0, quiet=0, dotile=0, pf=BMP_BGR, bu=0, useppm=0,
	scalefactor=1;
const int _ps[BMPPIXELFORMATS]={3, 4, 3, 4, 4, 4};
const int _flags[BMPPIXELFORMATS]={0, 0, TJ_BGR, TJ_BGR,
	TJ_BGR|TJ_ALPHAFIRST, TJ_ALPHAFIRST};
const int _rindex[BMPPIXELFORMATS]={0, 0, 2, 2, 3, 1};
const int _gindex[BMPPIXELFORMATS]={1, 1, 1, 1, 2, 2};
const int _bindex[BMPPIXELFORMATS]={2, 2, 0, 0, 1, 3};
const char *_pfname[]={"RGB", "RGBA", "BGR", "BGRA", "ABGR", "ARGB"};
const char *_subnamel[NUMSUBOPT]={"4:4:4", "4:2:2", "4:2:0", "GRAY"};
const char *_subnames[NUMSUBOPT]={"444", "422", "420", "GRAY"};
const int _hsf[NUMSUBOPT]={1, 2, 2, 1};
const int _vsf[NUMSUBOPT]={1, 1, 2, 1};

void printsigfig(double val, int figs)
{
	char format[80];
	double _l=log10(val);  int l;
	if(_l<0.)
	{
		l=(int)fabs(_l);
		sprintf(format, "%%%d.%df", figs+l+2, figs+l);
	}
	else
	{
		l=(int)_l+1;
		if(figs<=l) sprintf(format, "%%.0f");
		else sprintf(format, "%%%d.%df", figs+1, figs-l);
	}	
	printf(format, val);
}

// Decompression test
int decomptest(unsigned char *srcbuf, unsigned char **jpegbuf,
	unsigned long *comptilesize, unsigned char *rgbbuf, int w, int h,
	int jpegsub, int qual, char *filename, int tilesizex, int tilesizey)
{
	char tempstr[1024], qualstr[5]="\0";
	FILE *outfile=NULL;  tjhandle hnd=NULL;
	int flags=(forcemmx?TJ_FORCEMMX:0)|(forcesse?TJ_FORCESSE:0)
		|(forcesse2?TJ_FORCESSE2:0)|(forcesse3?TJ_FORCESSE3:0)
		|(fastupsample?TJ_FASTUPSAMPLE:0);
	int i, j, ITER, rgbbufalloc=0;
	double start, elapsed;
	int ps=_ps[pf];
	int hsf=_hsf[jpegsub], vsf=_vsf[jpegsub];
	int pw=PAD(w, hsf), ph=PAD(h, vsf);
	int cw=pw/hsf, ch=ph/vsf;
	int ypitch=PAD(pw, 4), uvpitch=PAD(cw, 4);
	int yuvsize=ypitch*ph + (jpegsub==TJ_GRAYSCALE? 0:uvpitch*ch*2);
	int _w=(w+scalefactor-1)/scalefactor, _h=(h+scalefactor-1)/scalefactor;
	int pitch=_w*ps;

	if(qual>0)
	{
		snprintf(qualstr, 5, "Q%d", qual);
		qualstr[4]=0;
	}

	flags |= _flags[pf];
	if(bu) flags |= TJ_BOTTOMUP;
	if(yuv==YUVDECODE) flags |= TJ_YUV;
	if((hnd=tjInitDecompress())==NULL)
		_throwtj("executing tjInitDecompress()");

	if(rgbbuf==NULL)
	{
		if((rgbbuf=(unsigned char *)malloc(max(yuvsize, pitch*_h))) == NULL)
			_throwunix("allocating image buffer");
		rgbbufalloc=1;
	}
	memset(rgbbuf, 127, max(yuvsize, pitch*_h));  // Grey image means decompressor did nothing

	if(tjDecompress2(hnd, jpegbuf[0], comptilesize[0], rgbbuf, pitch, ps, 1,
		(flags&TJ_YUV)? 1:scalefactor, flags)==-1)
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
				int tempw=min(tilesizex, w-j), temph=min(tilesizey, h-i);
				if(tjDecompress2(hnd, jpegbuf[tilen], comptilesize[tilen],
					&rgbbuf[pitch*i+ps*j], pitch, ps, 1, (flags&TJ_YUV)? 1:scalefactor,
					flags)==-1)
					_throwtj("executing tjDecompress2()");
				tilen++;
			}
		}
		ITER++;
	}	while((elapsed=rrtime()-start)<5.);
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
		sprintf(tempstr, "%s_%s%s.yuv", filename, _subnames[jpegsub], qualstr);
		if((outfile=fopen(tempstr, "wb"))==NULL)
			_throwunix("opening YUV image for output");
		if(fwrite(rgbbuf, yuvsize, 1, outfile)!=1)
			_throwunix("writing YUV image");
		fclose(outfile);  outfile=NULL;
	}
	else
	{
		if(tilesizex==w && tilesizey==h)
		{
			if(decomponly)
				sprintf(tempstr, "%s_full.%s", filename, useppm?"ppm":"bmp");
			else
				sprintf(tempstr, "%s_%s%s_full.%s", filename, _subnames[jpegsub],
					qualstr, useppm?"ppm":"bmp");
		}
		else sprintf(tempstr, "%s_%s%s_%dx%d.%s", filename, _subnames[jpegsub],
			qualstr, tilesizex, tilesizey, useppm?"ppm":"bmp");
		if(savebmp(tempstr, rgbbuf, _w, _h, pf, pitch, bu)==-1)
			_throwbmp("saving bitmap");
		sprintf(strrchr(tempstr, '.'), "-err.%s", useppm?"ppm":"bmp");
		if(srcbuf && scalefactor==1)
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

	if(hnd) {tjDestroy(hnd);  hnd=NULL;}
	if(rgbbuf && rgbbufalloc) {free(rgbbuf);  rgbbuf=NULL;}
	return 0;

	bailout:
	if(outfile) {fclose(outfile);  outfile=NULL;}
	if(hnd) {tjDestroy(hnd);  hnd=NULL;}
	if(rgbbuf && rgbbufalloc) {free(rgbbuf);  rgbbuf=NULL;}
	return -1;
}

void dotest(unsigned char *srcbuf, int w, int h, int jpegsub, int qual,
	char *filename)
{
	char tempstr[1024];
	FILE *outfile=NULL;  tjhandle hnd;
	unsigned char **jpegbuf=NULL, *rgbbuf=NULL;
	double start, elapsed;
	int jpgbufsize=0, i, j, tilesizex, tilesizey, numtilesx, numtilesy, ITER;
	unsigned long *comptilesize=NULL;
	int flags=(forcemmx?TJ_FORCEMMX:0)|(forcesse?TJ_FORCESSE:0)
		|(forcesse2?TJ_FORCESSE2:0)|(forcesse3?TJ_FORCESSE3:0)
		|(fastupsample?TJ_FASTUPSAMPLE:0);
	int ps=_ps[pf], tilen;
	int pitch=w*ps, yuvsize;
	int hsf=_hsf[jpegsub], vsf=_vsf[jpegsub];
	int pw=PAD(w, hsf), ph=PAD(h, vsf);
	int cw=pw/hsf, ch=ph/vsf;
	int ypitch=PAD(pw, 4), uvpitch=PAD(cw, 4);

	flags |= _flags[pf];
	if(bu) flags |= TJ_BOTTOMUP;
	if(yuv==YUVENCODE) flags |= TJ_YUV;

	yuvsize=ypitch*ph + (jpegsub==TJ_GRAYSCALE? 0:uvpitch*ch*2);
	if((rgbbuf=(unsigned char *)malloc(max(yuvsize, pitch*h))) == NULL)
		_throwunix("allocating image buffer");

	if(!quiet)
	{
		if(yuv==YUVENCODE)
			printf("\n>>>>>  %s (%s) <--> YUV %s  <<<<<\n", _pfname[pf],
				bu?"Bottom-up":"Top-down", _subnamel[jpegsub]);
		else
			printf("\n>>>>>  %s (%s) <--> JPEG %s Q%d  <<<<<\n", _pfname[pf],
				bu?"Bottom-up":"Top-down", _subnamel[jpegsub], qual);
	}
	if(yuv==YUVDECODE) dotile=0;
	if(dotile) {tilesizex=tilesizey=4;}  else {tilesizex=w;  tilesizey=h;}

	do
	{
		tilesizex*=2;  if(tilesizex>w) tilesizex=w;
		tilesizey*=2;  if(tilesizey>h) tilesizey=h;
		numtilesx=(w+tilesizex-1)/tilesizex;
		numtilesy=(h+tilesizey-1)/tilesizey;
		if((comptilesize=(unsigned long *)malloc(sizeof(unsigned long)*numtilesx*numtilesy)) == NULL
		|| (jpegbuf=(unsigned char **)malloc(sizeof(unsigned char *)*numtilesx*numtilesy)) == NULL)
			_throwunix("allocating image buffers");
		memset(jpegbuf, 0, sizeof(unsigned char *)*numtilesx*numtilesy);
		for(i=0; i<numtilesx*numtilesy; i++)
		{
			if((jpegbuf[i]=(unsigned char *)malloc(TJBUFSIZE(tilesizex, tilesizey))) == NULL)
				_throwunix("allocating image buffers");
		}

		// Compression test
		if(quiet==1) printf("%s\t%s\t%s\t%d\t",  _pfname[pf], bu?"BU":"TD",
			_subnamel[jpegsub], qual);
		for(i=0; i<h; i++) memcpy(&rgbbuf[pitch*i], &srcbuf[w*ps*i], w*ps);
		if((hnd=tjInitCompress())==NULL)
			_throwtj("executing tjInitCompress()");
		if(tjCompress(hnd, rgbbuf, tilesizex, pitch, tilesizey, ps,
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
					if(tjCompress(hnd, &rgbbuf[pitch*i+j*ps], tempw, pitch,
						temph, ps, jpegbuf[tilen], &comptilesize[tilen], jpegsub, qual,
						flags)==-1)
						_throwtj("executing tjCompress()");
					jpgbufsize+=comptilesize[tilen];
					tilen++;
				}
			}
			ITER++;
		} while((elapsed=rrtime()-start)<5.);
		if(tjDestroy(hnd)==-1) _throwtj("executing tjDestroy()");
		hnd=NULL;
		if(quiet==1)
		{
			if(tilesizex==w && tilesizey==h) printf("Full     \t");
			else printf("%-4d %-4d\t", tilesizex, tilesizey);
		}
		if(quiet)
		{
			printsigfig((double)(w*h)/1000000.*(double)ITER/elapsed, 4);
			printf("%c", quiet==2? '\n':'\t');
			printsigfig((double)(w*h*ps)/(double)jpgbufsize, 4);
			printf("%c", quiet==2? '\n':'\t');
		}
		else
		{
			if(tilesizex==w && tilesizey==h) printf("\nFull image\n");
			else printf("\nTile size: %d x %d\n", tilesizex, tilesizey);
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
				sprintf(tempstr, "%s_%s.yuv", filename, _subnames[jpegsub]);
			else
				sprintf(tempstr, "%s_%sQ%d.jpg", filename, _subnames[jpegsub], qual);
			if((outfile=fopen(tempstr, "wb"))==NULL)
				_throwunix("opening reference image");
			if(fwrite(jpegbuf[0], jpgbufsize, 1, outfile)!=1)
				_throwunix("writing reference image");
			fclose(outfile);  outfile=NULL;
			if(!quiet) printf("Reference image written to %s\n", tempstr);
		}
		if(yuv==YUVENCODE) goto bailout;

		// Decompression test
		if(decomptest(srcbuf, jpegbuf, comptilesize, rgbbuf, w, h, jpegsub, qual,
			filename, tilesizex, tilesizey)==-1)
			goto bailout;

		// Cleanup
		if(outfile) {fclose(outfile);  outfile=NULL;}
		if(jpegbuf)
		{
			for(i=0; i<numtilesx*numtilesy; i++)
				{if(jpegbuf[i]) free(jpegbuf[i]);  jpegbuf[i]=NULL;}
			free(jpegbuf);  jpegbuf=NULL;
		}
		if(comptilesize) {free(comptilesize);  comptilesize=NULL;}
	} while(tilesizex<w || tilesizey<h);

	if(rgbbuf) {free(rgbbuf);  rgbbuf=NULL;}
	return;

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
	unsigned char *jpegbuf=NULL;
	int w=0, h=0, jpegsub=-1;
	unsigned long jpgbufsize=0;
	char *temp=NULL;

	useppm=1;

	if((file=fopen(filename, "rb"))==NULL)
		_throwunix("opening file");
	if(fseek(file, 0, SEEK_END)<0 || (jpgbufsize=ftell(file))<0)
		_throwunix("determining file size");
	if((jpegbuf=(unsigned char *)malloc(jpgbufsize))==NULL)
		_throwunix("allocating memory");
	if(fseek(file, 0, SEEK_SET)<0)
		_throwunix("setting file position");
	if(fread(jpegbuf, jpgbufsize, 1, file)<1)
		_throwunix("reading JPEG data");
	fclose(file);  file=NULL;

	temp=strrchr(filename, '.');
	if(temp!=NULL) *temp='\0';

	if((hnd=tjInitDecompress())==NULL) _throwtj("executing tjInitDecompress()");
	if(tjDecompressHeader2(hnd, jpegbuf, jpgbufsize, &w, &h, &jpegsub)==-1)
		_throwtj("executing tjDecompressHeader2()");
	if(tjDestroy(hnd)==-1) _throwtj("executing tjDestroy()");
	hnd=NULL;

	if(quiet==1)
	{
		printf("\nAll performance values in Mpixels/sec\n\n");
		printf("Bitmap\tBitmap\tJPEG\tImage Size\tDecomp\n"),
		printf("Format\tOrder\tFormat\t  X    Y  \tPerf\n\n");
		printf("%s\t%s\t%s\t%-4d  %-4d\t", _pfname[pf], bu?"BU":"TD",
			_subnamel[jpegsub], w, h);
	}
	else
	{
		if(yuv==YUVDECODE)
			printf("\n>>>>>  JPEG --> YUV %s  <<<<<\n", _subnamel[jpegsub]);
		else
			printf("\n>>>>>  JPEG --> %s (%s)  <<<<<\n", _pfname[pf],
				bu?"Bottom-up":"Top-down");
		printf("\nImage size: %d x %d", w, h);
		if(scalefactor!=1) printf(" --> %d x %d", (w+scalefactor-1)/scalefactor,
			(h+scalefactor-1)/scalefactor);
		printf("\n");
	}

	decomptest(NULL, &jpegbuf, &jpgbufsize, NULL, w, h, jpegsub, 0, filename, w,
		h);

	bailout:
	if(file) {fclose(file);  file=NULL;}
	if(jpegbuf) {free(jpegbuf);  jpegbuf=NULL;}
	if(hnd) {tjDestroy(hnd);  hnd=NULL;}
	return;
}


void usage(char *progname)
{
	printf("USAGE: %s\n", progname);
	printf("       <Inputfile (BMP|PPM)> <%% Quality> [options]\n\n");
	printf("       %s\n", progname);
	printf("       <Inputfile (JPG)> [options]\n\n");
	printf("Options:\n\n");
	printf("-tile = Test performance of the codec when the image is encoded as separate\n");
	printf("     tiles of varying sizes.\n");
	printf("-forcemmx, -forcesse, -forcesse2, -forcesse3 =\n");
	printf("     Force MMX, SSE, or SSE2 code paths in TurboJPEG codec\n");
	printf("-rgb, -bgr, -rgba, -bgra, -abgr, -argb =\n");
	printf("     Test the specified color conversion path in the codec (default: BGR)\n");
	printf("-fastupsample = Use fast, inaccurate upsampling code to perform 4:2:2 and 4:2:0\n");
	printf("     YUV decoding in libjpeg decompressor\n");
	printf("-quiet = Output results in tabular rather than verbose format\n");
	printf("-yuvencode = Encode RGB input as planar YUV rather than compressing as JPEG\n");
	printf("-yuvdecode = Decode JPEG image to planar YUV rather than RGB\n");
	printf("-scale 1/N = scale down the width/height of the decompressed JPEG image by a\n");
	printf("     factor of N (N = 1, 2, 4, or 8}\n\n");
	printf("NOTE:  If the quality is specified as a range (e.g. 90-100), a separate\n");
	printf("test will be performed for all quality values in the range.\n\n");
	exit(1);
}


int main(int argc, char *argv[])
{
	unsigned char *bmpbuf=NULL;  int w, h, i;
	int qual, hiqual=-1;  char *temp;
	int minarg=2;

	if(argc<minarg) usage(argv[0]);

	temp=strrchr(argv[1], '.');
	if(temp!=NULL)
	{
		if(!stricmp(temp, ".ppm")) useppm=1;
		if(!stricmp(temp, ".jpg") || !stricmp(temp, ".jpeg")) decomponly=1;
	}

	if(argc>minarg)
	{
		for(i=minarg; i<argc; i++)
		{
			if(!stricmp(argv[i], "-yuvencode"))
			{
				printf("Testing YUV planar encoding\n");
				yuv=YUVENCODE;  hiqual=qual=100;
			}
			if(!stricmp(argv[i], "-yuvdecode"))
			{
				printf("Testing YUV planar decoding\n");
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
			if(!stricmp(argv[i], "-tile")) dotile=1;
			if(!stricmp(argv[i], "-forcesse3"))
			{
				printf("Using SSE3 code\n");
				forcesse3=1;
			}
			if(!stricmp(argv[i], "-forcesse2"))
			{
				printf("Using SSE2 code\n");
				forcesse2=1;
			}
			if(!stricmp(argv[i], "-forcesse"))
			{
				printf("Using SSE code\n");
				forcesse=1;
			}
			if(!stricmp(argv[i], "-forcemmx"))
			{
				printf("Using MMX code\n");
				forcemmx=1;
			}
			if(!stricmp(argv[i], "-fastupsample"))
			{
				printf("Using fast upsampling code\n");
				fastupsample=1;
			}
			if(!stricmp(argv[i], "-rgb")) pf=BMP_RGB;
			if(!stricmp(argv[i], "-rgba")) pf=BMP_RGBA;
			if(!stricmp(argv[i], "-bgr")) pf=BMP_BGR;
			if(!stricmp(argv[i], "-bgra")) pf=BMP_BGRA;
			if(!stricmp(argv[i], "-abgr")) pf=BMP_ABGR;
			if(!stricmp(argv[i], "-argb")) pf=BMP_ARGB;
			if(!stricmp(argv[i], "-bottomup")) bu=1;
			if(!stricmp(argv[i], "-quiet")) quiet=1;
			if(!stricmp(argv[i], "-qq")) quiet=2;
			if(!stricmp(argv[i], "-scale") && i<argc-1)
			{
				int temp1=0, temp2=0;
				if(sscanf(argv[++i], "%d/%d", &temp1, &temp2)!=2
					|| temp1!=1 || temp2<1 || temp2>8 || (temp2&(temp2-1))!=0)
					usage(argv[0]);
				scalefactor=temp2;
			}
		}
	}

	if(scalefactor!=1 && dotile)
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
		printf("\nAll performance values in Mpixels/sec\n\n");
		printf("Bitmap\tBitmap\tJPEG\tJPEG\tTile Size\tCompr\tCompr\tDecomp\n");
		printf("Format\tOrder\tFormat\tQual\t X    Y  \tPerf \tRatio\tPerf\n\n");
	}

	if(decomponly)
	{
		dodecomptest(argv[1]);
		goto bailout;
	}
	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, TJ_GRAYSCALE, i, argv[1]);
	if(quiet) printf("\n");
	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, TJ_420, i, argv[1]);
	if(quiet) printf("\n");
	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, TJ_422, i, argv[1]);
	if(quiet) printf("\n");
	for(i=hiqual; i>=qual; i--)
		dotest(bmpbuf, w, h, TJ_444, i, argv[1]);

	bailout:
	if(bmpbuf) free(bmpbuf);
	return 0;
}

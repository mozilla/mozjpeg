/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
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
#include "./rrtimer.h"
#include "./rrutil.h"
#include "./turbojpeg.h"

#define _catch(f) {if((f)==-1) {printf("TJPEG: %s\n", tjGetErrorStr());  bailout();}}

const char *_subnamel[NUMSUBOPT]={"4:4:4", "4:2:2", "4:2:0", "GRAY", "4:4:0"};
const char *_subnames[NUMSUBOPT]={"444", "422", "420", "GRAY", "440"};

#define NUMPF 7
enum {RGB=0, BGR, RGBX, BGRX, XBGR, XRGB, GRAY};
const int _ps[NUMPF]={3, 3, 4, 4, 4, 4, 1};
const int _roffset[NUMPF]={0, 2, 0, 2, 3, 1, 0};
const int _goffset[NUMPF]={1, 1, 1, 1, 2, 2, 0};
const int _boffset[NUMPF]={2, 0, 2, 0, 1, 3, 0};
const int _flags[NUMPF]={0, TJ_BGR, 0, TJ_BGR, TJ_BGR|TJ_ALPHAFIRST,
	TJ_ALPHAFIRST, 0};
const char *_pfstr[NUMPF]={"RGB", "BGR", "RGBX", "BGRX", "XBGR", "XRGB",
	"Grayscale"};

const int _3byteformats[]={RGB, BGR};
const int _4byteformats[]={RGBX, BGRX, XBGR, XRGB};
const int _onlygray[]={GRAY};
const int _onlyrgb[]={RGB};

enum {YUVENCODE=1, YUVDECODE};
int yuv=0;

int exitstatus=0;
#define bailout() {exitstatus=-1;  goto bailout;}

int pixels[9][3]=
{
	{0, 255, 0},
	{255, 0, 255},
	{255, 255, 0},
	{0, 0, 255},
	{0, 255, 255},
	{255, 0, 0},
	{255, 255, 255},
	{0, 0, 0},
	{255, 0, 0}
};

void initbuf(unsigned char *buf, int w, int h, int pf, int flags)
{
	int ps=_ps[pf], i, _i, j;
	int roffset=_roffset[pf], goffset=_goffset[pf], boffset=_boffset[pf];

	memset(buf, 0, w*h*ps);
	if(pf==GRAY)
	{
		for(_i=0; _i<16; _i++)
		{
			if(flags&TJ_BOTTOMUP) i=h-_i-1;  else i=_i;
			for(j=0; j<w; j++)
			{
				if(((_i/8)+(j/8))%2==0) buf[w*i+j]=255;
				else buf[w*i+j]=76;
			}
		}
		for(_i=16; _i<h; _i++)
		{
			if(flags&TJ_BOTTOMUP) i=h-_i-1;  else i=_i;
			for(j=0; j<w; j++)
			{
				if(((_i/8)+(j/8))%2==0) buf[w*i+j]=0;
				else buf[w*i+j]=226;
			}
		}
		return;
	}
	for(_i=0; _i<16; _i++)
	{
		if(flags&TJ_BOTTOMUP) i=h-_i-1;  else i=_i;
		for(j=0; j<w; j++)
		{
			buf[(w*i+j)*ps+roffset]=255;
			if(((_i/8)+(j/8))%2==0)
			{
				buf[(w*i+j)*ps+goffset]=255;
				buf[(w*i+j)*ps+boffset]=255;
			}
		}
	}
	for(_i=16; _i<h; _i++)
	{
		if(flags&TJ_BOTTOMUP) i=h-_i-1;  else i=_i;
		for(j=0; j<w; j++)
		{
			if(((_i/8)+(j/8))%2!=0)
			{
				buf[(w*i+j)*ps+roffset]=255;
				buf[(w*i+j)*ps+goffset]=255;
			}
		}
	}
}

#define checkval(v, cv) { \
	if(v<cv-1 || v>cv+1) { \
		printf("\nComp. %s at %d,%d should be %d, not %d\n", #v, i, j, cv, v); \
		retval=0;  goto bailout; \
	}}

#define checkval0(v) { \
	if(v>1) { \
		printf("\nComp. %s at %d,%d should be 0, not %d\n", #v, i, j, v); \
		retval=0;  goto bailout; \
	}}

#define checkval255(v) { \
	if(v<254) { \
		printf("\nComp. %s at %d,%d should be 255, not %d\n", #v, i, j, v); \
		retval=0;  goto bailout; \
	}}

int checkbuf(unsigned char *buf, int w, int h, int pf, int subsamp,
	int scale_num, int scale_denom, int flags)
{
	int ps=_ps[pf];
	int roffset=_roffset[pf], goffset=_goffset[pf], boffset=_boffset[pf];
	int i, _i, j, retval=1;
	int halfway=16*scale_num/scale_denom, blocksize=8*scale_num/scale_denom;

	for(_i=0; _i<halfway; _i++)
	{
		if(flags&TJ_BOTTOMUP) i=h-_i-1;  else i=_i;
		for(j=0; j<w; j++)
		{
			unsigned char r=buf[(w*i+j)*ps+roffset],
				g=buf[(w*i+j)*ps+goffset],
				b=buf[(w*i+j)*ps+boffset];
			if(((_i/blocksize)+(j/blocksize))%2==0)
			{
				checkval255(r);  checkval255(g);  checkval255(b);
			}
			else
			{
				if(subsamp==TJ_GRAYSCALE)
				{
					checkval(r, 76);  checkval(g, 76);  checkval(b, 76);
				}
				else
				{
					checkval255(r);  checkval0(g);  checkval0(b);
				}
			}
		}
	}
	for(_i=halfway; _i<h; _i++)
	{
		if(flags&TJ_BOTTOMUP) i=h-_i-1;  else i=_i;
		for(j=0; j<w; j++)
		{
			unsigned char r=buf[(w*i+j)*ps+roffset],
				g=buf[(w*i+j)*ps+goffset],
				b=buf[(w*i+j)*ps+boffset];
			if(((_i/blocksize)+(j/blocksize))%2==0)
			{
				checkval0(r);  checkval0(g);  checkval0(b);
			}
			else
			{
				if(subsamp==TJ_GRAYSCALE)
				{
					checkval(r, 226);  checkval(g, 226);  checkval(b, 226);
				}
				else
				{
					checkval255(r);  checkval255(g);  checkval0(b);
				}
			}
		}
	}

	bailout:
	if(retval==0)
	{
		printf("\n");
		for(i=0; i<h; i++)
		{
			for(j=0; j<w; j++)
			{
				printf("%.3d/%.3d/%.3d ", buf[(w*i+j)*ps+roffset],
					buf[(w*i+j)*ps+goffset], buf[(w*i+j)*ps+boffset]);
			}
			printf("\n");
		}
	}
	return retval;
}

#define PAD(v, p) ((v+(p)-1)&(~((p)-1)))

int checkbufyuv(unsigned char *buf, int w, int h, int subsamp)
{
	int i, j;
	int hsf=tjMCUWidth[subsamp]/8, vsf=tjMCUHeight[subsamp]/8;
	int pw=PAD(w, hsf), ph=PAD(h, vsf);
	int cw=pw/hsf, ch=ph/vsf;
	int ypitch=PAD(pw, 4), uvpitch=PAD(cw, 4);
	int retval=1;

	for(i=0; i<16; i++)
	{
		for(j=0; j<pw; j++)
		{
			unsigned char y=buf[ypitch*i+j];
			if(((i/8)+(j/8))%2==0) checkval255(y)
			else checkval(y, 76)
		}
	}
	for(i=16; i<ph; i++)
	{
		for(j=0; j<pw; j++)
		{
			unsigned char y=buf[ypitch*i+j];
			if(((i/8)+(j/8))%2==0) checkval0(y)
			else checkval(y, 226)
		}
	}
	if(subsamp!=TJ_GRAYSCALE)
	{
		for(i=0; i<16/vsf; i++)
		{
			for(j=0; j<cw; j++)
			{
				unsigned char u=buf[ypitch*ph + (uvpitch*i+j)],
					v=buf[ypitch*ph + uvpitch*ch + (uvpitch*i+j)];
				if(((i*vsf/8)+(j*hsf/8))%2==0)
				{
					checkval(u, 128);  checkval(v, 128);
				}
				else
				{
					checkval(u, 85);  checkval255(v);
				}
			}
		}
		for(i=16/vsf; i<ch; i++)
		{
			for(j=0; j<cw; j++)
			{
				unsigned char u=buf[ypitch*ph + (uvpitch*i+j)],
					v=buf[ypitch*ph + uvpitch*ch + (uvpitch*i+j)];
				if(((i*vsf/8)+(j*hsf/8))%2==0)
				{
					checkval(u, 128);  checkval(v, 128);
				}
				else
				{
					checkval0(u);  checkval(v, 149);
				}
			}
		}
	}

	bailout:
	if(retval==0)
	{
		for(i=0; i<ph; i++)
		{
			for(j=0; j<pw; j++)
				printf("%.3d ", buf[ypitch*i+j]);
			printf("\n");
		}
		printf("\n");
		for(i=0; i<ch; i++)
		{
			for(j=0; j<cw; j++)
				printf("%.3d ", buf[ypitch*ph + (uvpitch*i+j)]);
			printf("\n");
		}
		printf("\n");
		for(i=0; i<ch; i++)
		{
			for(j=0; j<cw; j++)
				printf("%.3d ", buf[ypitch*ph + uvpitch*ch + (uvpitch*i+j)]);
			printf("\n");
		}
		printf("\n");
	}

	return retval;
}

void writejpeg(unsigned char *jpegbuf, unsigned long jpgbufsize,
	char *filename)
{
	FILE *outfile=NULL;
	if((outfile=fopen(filename, "wb"))==NULL)
	{
		printf("ERROR: Could not open %s for writing.\n", filename);
		bailout();
	}
	if(fwrite(jpegbuf, jpgbufsize, 1, outfile)!=1)
	{
		printf("ERROR: Could not write to %s.\n", filename);
		bailout();
	}

	bailout:
	if(outfile) fclose(outfile);
}

void gentestjpeg(tjhandle hnd, unsigned char *jpegbuf, unsigned long *size,
	int w, int h, int pf, char *basefilename, int subsamp, int qual, int flags)
{
	char tempstr[1024];  unsigned char *bmpbuf=NULL;
	int ps=_ps[pf];  double t;

	if(yuv==YUVENCODE)
		printf("%s %s -> %s YUV ... ", _pfstr[pf],
			(flags&TJ_BOTTOMUP)?"Bottom-Up":"Top-Down ", _subnamel[subsamp]);
	else
		printf("%s %s -> %s Q%d ... ", _pfstr[pf],
			(flags&TJ_BOTTOMUP)?"Bottom-Up":"Top-Down ", _subnamel[subsamp], qual);

	if((bmpbuf=(unsigned char *)malloc(w*h*ps+1))==NULL)
	{
		printf("ERROR: Could not allocate buffer\n");  bailout();
	}
	initbuf(bmpbuf, w, h, pf, flags);
	memset(jpegbuf, 0,
		yuv==YUVENCODE? TJBUFSIZEYUV(w, h, subsamp):TJBUFSIZE(w, h));

	t=rrtime();
	if(yuv==YUVENCODE)
	{
		_catch(tjEncodeYUV(hnd, bmpbuf, w, 0, h, ps, jpegbuf, subsamp,
			flags|_flags[pf]));
		*size=TJBUFSIZEYUV(w, h, subsamp);
	}
	else
	{
		_catch(tjCompress(hnd, bmpbuf, w, 0, h, ps, jpegbuf, size, subsamp, qual,
			flags|_flags[pf]));
	}
	t=rrtime()-t;

	if(yuv==YUVENCODE)
		snprintf(tempstr, 1024, "%s_enc_%s_%s_%s.yuv", basefilename, _pfstr[pf],
			(flags&TJ_BOTTOMUP)? "BU":"TD", _subnames[subsamp]);
	else
		snprintf(tempstr, 1024, "%s_enc_%s_%s_%s_Q%d.jpg", basefilename, _pfstr[pf],
			(flags&TJ_BOTTOMUP)? "BU":"TD", _subnames[subsamp], qual);
	writejpeg(jpegbuf, *size, tempstr);
	if(yuv==YUVENCODE)
	{
		if(checkbufyuv(jpegbuf, w, h, subsamp)) printf("Passed.");
		else {printf("FAILED!");  exitstatus=-1;}
	}
	else printf("Done.");
	printf("  %f ms\n  Result in %s\n", t*1000., tempstr);

	bailout:
	if(bmpbuf) free(bmpbuf);
}

void _gentestbmp(tjhandle hnd, unsigned char *jpegbuf, unsigned long jpegsize,
	int w, int h, int pf, char *basefilename, int subsamp, int flags,
	int scale_num, int scale_denom)
{
	unsigned char *bmpbuf=NULL;
	int _hdrw=0, _hdrh=0, _hdrsubsamp=-1;  double t;
	int scaledw=(w*scale_num+scale_denom-1)/scale_denom;
	int scaledh=(h*scale_num+scale_denom-1)/scale_denom;
	int ps=_ps[pf];
	unsigned long size=0;

	if(yuv==YUVENCODE) return;

	if(yuv==YUVDECODE)
		printf("JPEG -> YUV %s ... ", _subnames[subsamp]);
	else
	{
		printf("JPEG -> %s %s ", _pfstr[pf],
			(flags&TJ_BOTTOMUP)?"Bottom-Up":"Top-Down ");
		if(scale_num!=1 || scale_denom!=1)
			printf("%d/%d ... ", scale_num, scale_denom);
		else printf("... ");
	}

	_catch(tjDecompressHeader2(hnd, jpegbuf, jpegsize, &_hdrw, &_hdrh,
		&_hdrsubsamp));
	if(_hdrw!=w || _hdrh!=h || _hdrsubsamp!=subsamp)
	{
		printf("Incorrect JPEG header\n");  bailout();
	}

	if(yuv==YUVDECODE) size=TJBUFSIZEYUV(w, h, subsamp);
	else size=scaledw*scaledh*ps+1;
	if((bmpbuf=(unsigned char *)malloc(size))==NULL)
	{
		printf("ERROR: Could not allocate buffer\n");  bailout();
	}
	memset(bmpbuf, 0, size);

	t=rrtime();
	if(yuv==YUVDECODE)
	{
		_catch(tjDecompressToYUV(hnd, jpegbuf, jpegsize, bmpbuf,
			flags|_flags[pf]));
	}
	else
	{
		_catch(tjDecompress(hnd, jpegbuf, jpegsize, bmpbuf, scaledw, 0, scaledh,
			ps, flags|_flags[pf]));
	}
	t=rrtime()-t;

	if(yuv==YUVDECODE)
	{
		if(checkbufyuv(bmpbuf, w, h, subsamp)) printf("Passed.");
		else {printf("FAILED!");  exitstatus=-1;}
	}
	else
	{
		if(checkbuf(bmpbuf, scaledw, scaledh, pf, subsamp, scale_num, scale_denom,
			flags)) printf("Passed.");
		else {printf("FAILED!");  exitstatus=-1;}
	}
	printf("  %f ms\n", t*1000.);

	bailout:
	if(bmpbuf) free(bmpbuf);
}

void gentestbmp(tjhandle hnd, unsigned char *jpegbuf, unsigned long jpegsize,
	int w, int h, int pf, char *basefilename, int subsamp, int flags)
{
	int i, n=0;
	tjscalingfactor *sf=tjGetScalingFactors(&n);
	if(!sf || !n) 
	{
		printf("Error in tjGetScalingFactors():\n%s\n", tjGetErrorStr());
		bailout();
	}

	if((subsamp==TJ_444 || subsamp==TJ_GRAYSCALE) && !yuv)
	{
		for(i=0; i<n; i++)
			_gentestbmp(hnd, jpegbuf, jpegsize, w, h, pf, basefilename, subsamp,
				flags, sf[i].num, sf[i].denom);
	}
	else
		_gentestbmp(hnd, jpegbuf, jpegsize, w, h, pf, basefilename, subsamp,
			flags, 1, 1);

	bailout:
	printf("\n");
}

void dotest(int w, int h, const int *formats, int nformats, int subsamp,
	char *basefilename)
{
	tjhandle hnd=NULL, dhnd=NULL;  unsigned char *jpegbuf=NULL;
	unsigned long size;  int pfi, pf, i;

	size=(yuv==YUVENCODE? TJBUFSIZEYUV(w, h, subsamp):TJBUFSIZE(w, h));
	if((jpegbuf=(unsigned char *)malloc(size)) == NULL)
	{
		puts("ERROR: Could not allocate buffer.");  bailout();
	}

	if((hnd=tjInitCompress())==NULL)
		{printf("Error in tjInitCompress():\n%s\n", tjGetErrorStr());  bailout();}
	if((dhnd=tjInitDecompress())==NULL)
		{printf("Error in tjInitDecompress():\n%s\n", tjGetErrorStr());  bailout();}

	for(pfi=0; pfi<nformats; pfi++)
	{
		for(i=0; i<2; i++)
		{
			int flags=0;
			if(i==1)
			{
				if(yuv==YUVDECODE) goto bailout;
				else flags|=TJ_BOTTOMUP;
			}
			pf=formats[pfi];
			gentestjpeg(hnd, jpegbuf, &size, w, h, pf, basefilename, subsamp, 100,
				flags);
			gentestbmp(dhnd, jpegbuf, size, w, h, pf, basefilename, subsamp, flags);
		}
	}

	bailout:
	if(hnd) tjDestroy(hnd);
	if(dhnd) tjDestroy(dhnd);

	if(jpegbuf) free(jpegbuf);
}

#define MAXLENGTH 2048

void dotest1(void)
{
	int i, j, i2;  unsigned char *bmpbuf=NULL, *jpgbuf=NULL;
	tjhandle hnd=NULL;  unsigned long size;
	if((hnd=tjInitCompress())==NULL)
		{printf("Error in tjInitCompress():\n%s\n", tjGetErrorStr());  bailout();}
	printf("Buffer size regression test\n");
	for(j=1; j<48; j++)
	{
		for(i=1; i<(j==1?MAXLENGTH:48); i++)
		{
			if(i%100==0) printf("%.4d x %.4d\b\b\b\b\b\b\b\b\b\b\b", i, j);
			if((bmpbuf=(unsigned char *)malloc(i*j*4))==NULL
			|| (jpgbuf=(unsigned char *)malloc(TJBUFSIZE(i, j)))==NULL)
			{
				printf("Memory allocation failure\n");  bailout();
			}
			memset(bmpbuf, 0, i*j*4);
			for(i2=0; i2<i*j; i2++)
			{
				bmpbuf[i2*4]=pixels[i2%9][2];
				bmpbuf[i2*4+1]=pixels[i2%9][1];
				bmpbuf[i2*4+2]=pixels[i2%9][0];
			}
			_catch(tjCompress(hnd, bmpbuf, i, 0, j, 4,
				jpgbuf, &size, TJ_444, 100, TJ_BGR));
			free(bmpbuf);  bmpbuf=NULL;  free(jpgbuf);  jpgbuf=NULL;

			if((bmpbuf=(unsigned char *)malloc(j*i*4))==NULL
			|| (jpgbuf=(unsigned char *)malloc(TJBUFSIZE(j, i)))==NULL)
			{
				printf("Memory allocation failure\n");  bailout();
			}
			for(i2=0; i2<j*i; i2++)
			{
				if(i2%2==0) bmpbuf[i2*4]=bmpbuf[i2*4+1]=bmpbuf[i2*4+2]=0xFF;
				else bmpbuf[i2*4]=bmpbuf[i2*4+1]=bmpbuf[i2*4+2]=0;
			}
			_catch(tjCompress(hnd, bmpbuf, j, 0, i, 4,
				jpgbuf, &size, TJ_444, 100, TJ_BGR));
			free(bmpbuf);  bmpbuf=NULL;  free(jpgbuf);  jpgbuf=NULL;
		}
	}
	printf("Done.      \n");

	bailout:
	if(bmpbuf) free(bmpbuf);  if(jpgbuf) free(jpgbuf);
	if(hnd) tjDestroy(hnd);
}

int main(int argc, char *argv[])
{
	int doyuv=0;
	if(argc>1 && !stricmp(argv[1], "-yuv")) doyuv=1;
	if(doyuv) yuv=YUVENCODE;
	dotest(35, 39, _3byteformats, 2, TJ_444, "test");
	dotest(39, 41, _4byteformats, 4, TJ_444, "test");
	if(doyuv)
	{
		dotest(41, 35, _3byteformats, 2, TJ_422, "test");
		dotest(35, 39, _4byteformats, 4, TJ_422, "test");
		dotest(39, 41, _3byteformats, 2, TJ_420, "test");
		dotest(41, 35, _4byteformats, 4, TJ_420, "test");
		dotest(35, 39, _3byteformats, 2, TJ_440, "test");
		dotest(39, 41, _4byteformats, 4, TJ_440, "test");
	}
	dotest(35, 39, _onlygray, 1, TJ_GRAYSCALE, "test");
	dotest(39, 41, _3byteformats, 2, TJ_GRAYSCALE, "test");
	dotest(41, 35, _4byteformats, 4, TJ_GRAYSCALE, "test");
	if(!doyuv) dotest1();
	if(doyuv)
	{
		yuv=YUVDECODE;
		dotest(48, 48, _onlyrgb, 1, TJ_444, "test_yuv0");
		dotest(35, 39, _onlyrgb, 1, TJ_444, "test_yuv1");
		dotest(48, 48, _onlyrgb, 1, TJ_422, "test_yuv0");
		dotest(39, 41, _onlyrgb, 1, TJ_422, "test_yuv1");
		dotest(48, 48, _onlyrgb, 1, TJ_420, "test_yuv0");
		dotest(41, 35, _onlyrgb, 1, TJ_420, "test_yuv1");
		dotest(48, 48, _onlyrgb, 1, TJ_440, "test_yuv0");
		dotest(35, 39, _onlyrgb, 1, TJ_440, "test_yuv1");
		dotest(48, 48, _onlyrgb, 1, TJ_GRAYSCALE, "test_yuv0");
		dotest(35, 39, _onlyrgb, 1, TJ_GRAYSCALE, "test_yuv1");
		dotest(48, 48, _onlygray, 1, TJ_GRAYSCALE, "test_yuv0");
		dotest(39, 41, _onlygray, 1, TJ_GRAYSCALE, "test_yuv1");
	}

	return exitstatus;
}

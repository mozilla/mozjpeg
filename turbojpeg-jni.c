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

#include <stdlib.h>
#include <string.h>
#include "turbojpeg.h"
#include <jni.h>
#include "java/org_libjpegturbo_turbojpeg_TJCompressor.h"
#include "java/org_libjpegturbo_turbojpeg_TJDecompressor.h"
#include "java/org_libjpegturbo_turbojpeg_TJ.h"

static const int _pixelsize[org_libjpegturbo_turbojpeg_TJ_NUMPFOPT]=
	{3, 3, 4, 4, 4, 4, 1};

static const int _flags[org_libjpegturbo_turbojpeg_TJ_NUMPFOPT]=
	{0, TJ_BGR, 0, TJ_BGR, TJ_BGR|TJ_ALPHAFIRST, TJ_ALPHAFIRST, 0};

#define _throw(msg) {  \
	jclass _exccls=(*env)->FindClass(env, "java/lang/Exception");  \
	if(!_exccls) goto bailout;  \
	(*env)->ThrowNew(env, _exccls, msg);  \
	goto bailout;  \
}

#define bailif0(f) {if(!(f)) goto bailout;}

#define gethandle()  \
	jclass _cls=(*env)->GetObjectClass(env, obj);  \
	jfieldID _fid;  \
	if(!_cls) goto bailout;  \
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "handle", "J"));  \
	handle=(tjhandle)(jlong)(*env)->GetLongField(env, obj, _fid);  \

JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJ_bufSize
	(JNIEnv *env, jclass cls, jint width, jint height)
{
	jint retval=(jint)TJBUFSIZE(width, height);
	if(retval==-1) _throw(tjGetErrorStr());

	bailout:
	return retval;
}

JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJ_bufSizeYUV
	(JNIEnv *env, jclass cls, jint width, jint height, jint subsamp)
{
	jint retval=(jint)TJBUFSIZEYUV(width, height, subsamp);
	if(retval==-1) _throw(tjGetErrorStr());

	bailout:
	return retval;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_init
	(JNIEnv *env, jobject obj)
{
	jclass cls;
	jfieldID fid;
	tjhandle handle;

	if((handle=tjInitCompress())==NULL)
		_throw(tjGetErrorStr());

	bailif0(cls=(*env)->GetObjectClass(env, obj));
	bailif0(fid=(*env)->GetFieldID(env, cls, "handle", "J"));
	(*env)->SetLongField(env, obj, fid, (jlong)handle);

	bailout:
	return;
}

JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compress___3BIIII_3BIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint width, jint pitch,
		jint height, jint pf, jbyteArray dst, jint jpegsubsamp, jint jpegqual,
		jint flags)
{
	tjhandle handle=0;  int pixelsize;
	unsigned long size=0;  jsize arraysize=0;
	unsigned char *srcbuf=NULL, *dstbuf=NULL;

	gethandle();

	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPFOPT || width<1 || height<1
		|| pitch<0)
		_throw("Invalid argument in compress()");
	flags|=_flags[pf];
	pixelsize=_pixelsize[pf];

	arraysize=(pitch==0)? width*pixelsize*height:pitch*height;
	if((*env)->GetArrayLength(env, src)<arraysize)
		_throw("Source buffer is not large enough");
	if((*env)->GetArrayLength(env, dst)<(jsize)TJBUFSIZE(width, height))
		_throw("Destination buffer is not large enough");

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstbuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjCompress(handle, srcbuf, width, pitch, height, pixelsize, dstbuf,
		&size, jpegsubsamp, jpegqual, flags)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		dstbuf=srcbuf=NULL;
		_throw(tjGetErrorStr());
	}

	bailout:
	if(dstbuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
	if(srcbuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
	return (jint)size;
}

JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compress___3IIIII_3BIII
	(JNIEnv *env, jobject obj, jintArray src, jint width, jint pitch,
		jint height, jint pf, jbyteArray dst, jint jpegsubsamp, jint jpegqual,
		jint flags)
{
	tjhandle handle=0;
	unsigned long size=0;  jsize arraysize=0;
	unsigned char *srcbuf=NULL, *dstbuf=NULL;

	gethandle();

	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPFOPT || width<1 || height<1
		|| pitch<0)
		_throw("Invalid argument in compress()");
	if(_pixelsize[pf]!=sizeof(jint))
		_throw("Pixel format must be 32-bit when compressing from an integer buffer.");
	flags|=_flags[pf];

	arraysize=(pitch==0)? width*height:pitch*height;
	if((*env)->GetArrayLength(env, src)<arraysize)
		_throw("Source buffer is not large enough");
	if((*env)->GetArrayLength(env, dst)<(jsize)TJBUFSIZE(width, height))
		_throw("Destination buffer is not large enough");

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstbuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjCompress(handle, srcbuf, width, pitch*sizeof(jint), height,
		sizeof(jint),	dstbuf, &size, jpegsubsamp, jpegqual, flags)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		dstbuf=srcbuf=NULL;
		_throw(tjGetErrorStr());
	}

	bailout:
	if(dstbuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
	if(srcbuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
	return (jint)size;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_encodeYUV___3BIIII_3BII
	(JNIEnv *env, jobject obj, jbyteArray src, jint width, jint pitch,
		jint height, jint pf, jbyteArray dst, jint subsamp, jint flags)
{
	tjhandle handle=0;  int pixelsize;
	jsize arraysize=0;
	unsigned char *srcbuf=NULL, *dstbuf=NULL;

	gethandle();

	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPFOPT || width<1 || height<1
		|| pitch<0)
		_throw("Invalid argument in encodeYUV()");
	flags|=_flags[pf];
	pixelsize=_pixelsize[pf];

	arraysize=(pitch==0)? width*pixelsize*height:pitch*height;
	if((*env)->GetArrayLength(env, src)<arraysize)
		_throw("Source buffer is not large enough");
	if((*env)->GetArrayLength(env, dst)
		<(jsize)TJBUFSIZEYUV(width, height, subsamp))
		_throw("Destination buffer is not large enough");

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstbuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjEncodeYUV(handle, srcbuf, width, pitch, height, pixelsize, dstbuf,
		subsamp, flags)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		dstbuf=srcbuf=NULL;
		_throw(tjGetErrorStr());
	}

	bailout:
	if(dstbuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
	if(srcbuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
	return;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_encodeYUV___3IIIII_3BII
	(JNIEnv *env, jobject obj, jintArray src, jint width, jint pitch,
		jint height, jint pf, jbyteArray dst, jint subsamp, jint flags)
{
	tjhandle handle=0;
	jsize arraysize=0;
	unsigned char *srcbuf=NULL, *dstbuf=NULL;

	gethandle();

	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPFOPT || width<1 || height<1
		|| pitch<0)
		_throw("Invalid argument in compress()");
	if(_pixelsize[pf]!=sizeof(jint))
		_throw("Pixel format must be 32-bit when encoding from an integer buffer.");
	flags|=_flags[pf];

	arraysize=(pitch==0)? width*height:pitch*height;
	if((*env)->GetArrayLength(env, src)<arraysize)
		_throw("Source buffer is not large enough");
	if((*env)->GetArrayLength(env, dst)
		<(jsize)TJBUFSIZEYUV(width, height, subsamp))
		_throw("Destination buffer is not large enough");

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstbuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjEncodeYUV(handle, srcbuf, width, pitch*sizeof(jint), height,
		sizeof(jint),	dstbuf, subsamp, flags)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		dstbuf=srcbuf=NULL;
		_throw(tjGetErrorStr());
	}

	bailout:
	if(dstbuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
	if(srcbuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
	return;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_destroy
	(JNIEnv *env, jobject obj)
{
	tjhandle handle=0;

	gethandle();

	if(tjDestroy(handle)==-1) _throw(tjGetErrorStr());
	(*env)->SetLongField(env, obj, _fid, 0);

	bailout:
	return;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_init
	(JNIEnv *env, jobject obj)
{
	jclass cls;
	jfieldID fid;
	tjhandle handle;

	if((handle=tjInitDecompress())==NULL) _throw(tjGetErrorStr());

	bailif0(cls=(*env)->GetObjectClass(env, obj));
	bailif0(fid=(*env)->GetFieldID(env, cls, "handle", "J"));
	(*env)->SetLongField(env, obj, fid, (jlong)handle);

	bailout:
	return;
}

JNIEXPORT jobjectArray JNICALL Java_org_libjpegturbo_turbojpeg_TJ_getScalingFactors
	(JNIEnv *env, jclass cls)
{
  jclass sfcls=NULL;  jfieldID fid=0;
	tjscalingfactor *sf=NULL;  int n=0, i;
	jobject sfobj=NULL;
	jobjectArray sfjava=NULL;

	if((sf=tjGetScalingFactors(&n))==NULL || n==0)
		_throw(tjGetErrorStr());

	bailif0(sfcls=(*env)->FindClass(env, "org/libjpegturbo/turbojpeg/TJ$ScalingFactor"));
	bailif0(sfjava=(jobjectArray)(*env)->NewObjectArray(env, n, sfcls, 0));

	for(i=0; i<n; i++)
	{
		bailif0(sfobj=(*env)->AllocObject(env, sfcls));
		bailif0(fid=(*env)->GetFieldID(env, sfcls, "num", "I"));
		(*env)->SetIntField(env, sfobj, fid, sf[i].num);
		bailif0(fid=(*env)->GetFieldID(env, sfcls, "denom", "I"));
		(*env)->SetIntField(env, sfobj, fid, sf[i].denom);
		(*env)->SetObjectArrayElement(env, sfjava, i, sfobj);
	}

	bailout:
	return sfjava;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompressHeader
	(JNIEnv *env, jobject obj, jbyteArray src, jint size)
{
	tjhandle handle=0;
	unsigned char *srcbuf=NULL;
	int width=0, height=0, jpegsubsamp=-1;

	gethandle();

	if((*env)->GetArrayLength(env, src)<size)
		_throw("Source buffer is not large enough");

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));

	if(tjDecompressHeader2(handle, srcbuf, (unsigned long)size, 
		&width, &height, &jpegsubsamp)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		_throw(tjGetErrorStr());
	}
	(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);  srcbuf=NULL;

	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegSubsamp", "I"));
	(*env)->SetIntField(env, obj, _fid, jpegsubsamp);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegWidth", "I"));
	(*env)->SetIntField(env, obj, _fid, width);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegHeight", "I"));
	(*env)->SetIntField(env, obj, _fid, height);

	bailout:
	return;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompress___3BI_3BIIIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint size, jbyteArray dst,
		jint width, jint pitch, jint height, jint pf, jint flags)
{
	tjhandle handle=0;  int pixelsize;
	jsize arraysize=0;
	unsigned char *srcbuf=NULL, *dstbuf=NULL;

	gethandle();

	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPFOPT)
		_throw("Invalid argument in decompress()");
	flags|=_flags[pf];
	pixelsize=_pixelsize[pf];

	if((*env)->GetArrayLength(env, src)<size)
		_throw("Source buffer is not large enough");
	arraysize=(pitch==0)? width*pixelsize*height:pitch*height;
	if((*env)->GetArrayLength(env, dst)<arraysize)
		_throw("Destination buffer is not large enough");

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstbuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjDecompress(handle, srcbuf, (unsigned long)size, dstbuf, width, pitch,
		height, pixelsize, flags)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		dstbuf=srcbuf=NULL;
		_throw(tjGetErrorStr());
	}

	bailout:
	if(dstbuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
	if(srcbuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
	return;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompress___3BI_3IIIIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint size, jintArray dst,
		jint width, jint pitch, jint height, jint pf, jint flags)
{
	tjhandle handle=0;
	jsize arraysize=0;
	unsigned char *srcbuf=NULL, *dstbuf=NULL;

	gethandle();

	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPFOPT)
		_throw("Invalid argument in decompress()");
	if(_pixelsize[pf]!=sizeof(jint))
		_throw("Pixel format must be 32-bit when decompressing to an integer buffer.");
	flags|=_flags[pf];

	if((*env)->GetArrayLength(env, src)<size)
		_throw("Source buffer is not large enough");
	arraysize=(pitch==0)? width*height:pitch*height;
	if((*env)->GetArrayLength(env, dst)<arraysize)
		_throw("Destination buffer is not large enough");

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstbuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjDecompress(handle, srcbuf, (unsigned long)size, dstbuf, width,
		pitch*sizeof(jint), height, sizeof(jint), flags)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		dstbuf=srcbuf=NULL;
		_throw(tjGetErrorStr());
	}

	bailout:
	if(dstbuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
	if(srcbuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
	return;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompressToYUV
	(JNIEnv *env, jobject obj, jbyteArray src, jint size, jbyteArray dst,
		jint flags)
{
	tjhandle handle=0;
	unsigned char *srcbuf=NULL, *dstbuf=NULL;
	int jpegSubsamp=-1, jpegWidth=0, jpegHeight=0;

	gethandle();

	if((*env)->GetArrayLength(env, src)<size)
		_throw("Source buffer is not large enough");
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegSubsamp", "I"));
	jpegSubsamp=(int)(*env)->GetIntField(env, obj, _fid);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegWidth", "I"));
	jpegWidth=(int)(*env)->GetIntField(env, obj, _fid);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegHeight", "I"));
	jpegHeight=(int)(*env)->GetIntField(env, obj, _fid);
	if((*env)->GetArrayLength(env, dst)
		<(jsize)TJBUFSIZEYUV(jpegWidth, jpegHeight, jpegSubsamp))
		_throw("Destination buffer is not large enough");

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstbuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjDecompressToYUV(handle, srcbuf, (unsigned long)size, dstbuf, flags)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		dstbuf=srcbuf=NULL;
		_throw(tjGetErrorStr());
	}

	bailout:
	if(dstbuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
	if(srcbuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
	return;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJTransformer_init
	(JNIEnv *env, jobject obj)
{
	jclass cls;
	jfieldID fid;
	tjhandle handle;

	if((handle=tjInitTransform())==NULL) _throw(tjGetErrorStr());

	bailif0(cls=(*env)->GetObjectClass(env, obj));
	bailif0(fid=(*env)->GetFieldID(env, cls, "handle", "J"));
	(*env)->SetLongField(env, obj, fid, (jlong)handle);

	bailout:
	return;
}

JNIEXPORT jintArray JNICALL Java_org_libjpegturbo_turbojpeg_TJTransformer_transform
	(JNIEnv *env, jobject obj, jbyteArray jsrcbuf, jint srcsize,
		jobjectArray dstobjs, jobjectArray tobjs, jint flags)
{
	tjhandle handle=0;  int i;
	unsigned char *srcbuf=NULL, **dstbufs=NULL;  jsize n=0;
	unsigned long *dstsizes=NULL;  tjtransform *t=NULL;
	jbyteArray *jdstbufs=NULL;
	int jpegWidth=0, jpegHeight=0;
	jintArray jdstsizes=0;  jint *dstsizesi=NULL;

	gethandle();

	if((*env)->GetArrayLength(env, jsrcbuf)<srcsize)
		_throw("Source buffer is not large enough");
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegWidth", "I"));
	jpegWidth=(int)(*env)->GetIntField(env, obj, _fid);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegHeight", "I"));
	jpegHeight=(int)(*env)->GetIntField(env, obj, _fid);

	n=(*env)->GetArrayLength(env, dstobjs);
	if(n!=(*env)->GetArrayLength(env, tobjs))
		_throw("Mismatch between size of transforms array and destination buffers array");

	if((dstbufs=(unsigned char **)malloc(sizeof(unsigned char *)*n))==NULL)
		_throw("Memory allocation failure");
	if((jdstbufs=(jbyteArray *)malloc(sizeof(jbyteArray)*n))==NULL)
		_throw("Memory allocation failure");
	if((dstsizes=(unsigned long *)malloc(sizeof(unsigned long)*n))==NULL)
		_throw("Memory allocation failure");
	if((t=(tjtransform *)malloc(sizeof(tjtransform)*n))==NULL)
		_throw("Memory allocation failure");
	for(i=0; i<n; i++)
	{
		dstbufs[i]=NULL;  jdstbufs[i]=NULL;  dstsizes[i]=0;
		memset(&t[i], 0, sizeof(tjtransform));
	}

	for(i=0; i<n; i++)
	{
		jobject tobj;

		bailif0(tobj=(*env)->GetObjectArrayElement(env, tobjs, i));
		bailif0(_cls=(*env)->GetObjectClass(env, tobj));
		bailif0(_fid=(*env)->GetFieldID(env, _cls, "op", "I"));
		t[i].op=(*env)->GetIntField(env, tobj, _fid);
		bailif0(_fid=(*env)->GetFieldID(env, _cls, "options", "I"));
		t[i].options=(*env)->GetIntField(env, tobj, _fid);
		bailif0(_fid=(*env)->GetFieldID(env, _cls, "x", "I"));
		t[i].r.x=(*env)->GetIntField(env, tobj, _fid);
		bailif0(_fid=(*env)->GetFieldID(env, _cls, "y", "I"));
		t[i].r.y=(*env)->GetIntField(env, tobj, _fid);
		bailif0(_fid=(*env)->GetFieldID(env, _cls, "width", "I"));
		t[i].r.w=(*env)->GetIntField(env, tobj, _fid);
		bailif0(_fid=(*env)->GetFieldID(env, _cls, "height", "I"));
		t[i].r.h=(*env)->GetIntField(env, tobj, _fid);
	}

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, jsrcbuf, 0));
	for(i=0; i<n; i++)
	{
		int w=jpegWidth, h=jpegHeight;
		if(t[i].r.w!=0) w=t[i].r.w;
		if(t[i].r.h!=0) h=t[i].r.h;
		bailif0(jdstbufs[i]=(*env)->GetObjectArrayElement(env, dstobjs, i));
		if((*env)->GetArrayLength(env, jdstbufs[i])<TJBUFSIZE(w, h))
			_throw("Destination buffer is not large enough");
		bailif0(dstbufs[i]=(*env)->GetPrimitiveArrayCritical(env, jdstbufs[i], 0));
	}

	if(tjTransform(handle, srcbuf, srcsize, n, dstbufs, dstsizes, t, flags)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, jsrcbuf, srcbuf, 0);
		srcbuf=NULL;
		for(i=0; i<n; i++)
		{
			(*env)->ReleasePrimitiveArrayCritical(env, jdstbufs[i], dstbufs[i], 0);
			dstbufs[i]=NULL;
		}
		_throw(tjGetErrorStr());
	}

	jdstsizes=(*env)->NewIntArray(env, n);
	bailif0(dstsizesi=(*env)->GetIntArrayElements(env, jdstsizes, 0));
	for(i=0; i<n; i++) dstsizesi[i]=(int)dstsizes[i];

	bailout:
	if(srcbuf) (*env)->ReleasePrimitiveArrayCritical(env, jsrcbuf, srcbuf, 0);
	if(dstbufs)
	{
		for(i=0; i<n; i++)
		{
			if(dstbufs[i] && jdstbufs && jdstbufs[i])
				(*env)->ReleasePrimitiveArrayCritical(env, jdstbufs[i], dstbufs[i], 0);
		}
		free(dstbufs);
	}
	if(jdstbufs) free(jdstbufs);
	if(dstsizes) free(dstsizes);
	if(dstsizesi) (*env)->ReleaseIntArrayElements(env, jdstsizes, dstsizesi, 0);
	if(t) free(t);
	return jdstsizes;
}

JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_destroy
	(JNIEnv *env, jobject obj)
{
	Java_org_libjpegturbo_turbojpeg_TJCompressor_destroy(env, obj);
}

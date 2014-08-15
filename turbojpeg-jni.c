/*
 * Copyright (C)2011-2014 D. R. Commander.  All Rights Reserved.
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
#ifdef WIN32
#include "tjutil.h"
#endif
#include <jni.h>
#include "java/org_libjpegturbo_turbojpeg_TJCompressor.h"
#include "java/org_libjpegturbo_turbojpeg_TJDecompressor.h"
#include "java/org_libjpegturbo_turbojpeg_TJ.h"

#define _throw(msg) {  \
	jclass _exccls=(*env)->FindClass(env, "java/lang/Exception");  \
	if(!_exccls) goto bailout;  \
	(*env)->ThrowNew(env, _exccls, msg);  \
	goto bailout;  \
}

#define bailif0(f) {if(!(f)) {  \
	char temps[80];  \
	snprintf(temps, 80, "Unexpected NULL condition in line %d", __LINE__);  \
	_throw(temps);  \
}}

#define gethandle()  \
	jclass _cls=(*env)->GetObjectClass(env, obj);  \
	jfieldID _fid;  \
	if(!_cls) goto bailout;  \
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "handle", "J"));  \
	handle=(tjhandle)(size_t)(*env)->GetLongField(env, obj, _fid);  \

/* TurboJPEG 1.2.x: TJ::bufSize() */
JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJ_bufSize
	(JNIEnv *env, jclass cls, jint width, jint height, jint jpegSubsamp)
{
	jint retval=(jint)tjBufSize(width, height, jpegSubsamp);
	if(retval==-1) _throw(tjGetErrorStr());

	bailout:
	return retval;
}

/* TurboJPEG 1.4.x: TJ::bufSizeYUV() */
JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJ_bufSizeYUV__IIII
	(JNIEnv *env, jclass cls, jint width, jint pad, jint height, jint subsamp)
{
	jint retval=(jint)tjBufSizeYUV2(width, pad, height, subsamp);
	if(retval==-1) _throw(tjGetErrorStr());

	bailout:
	return retval;
}

/* TurboJPEG 1.2.x: TJ::bufSizeYUV() */
JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJ_bufSizeYUV__III
	(JNIEnv *env, jclass cls, jint width, jint height, jint subsamp)
{
	return Java_org_libjpegturbo_turbojpeg_TJ_bufSizeYUV__IIII(env, cls, width,
		4, height, subsamp);
}

/* TurboJPEG 1.2.x: TJCompressor::init() */
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
	(*env)->SetLongField(env, obj, fid, (size_t)handle);

	bailout:
	return;
}

JNIEXPORT jint JNICALL TJCompressor_compress
	(JNIEnv *env, jobject obj, jarray src, jint srcElementSize, jint x, jint y,
		jint width, jint pitch, jint height, jint pf, jbyteArray dst,
		jint jpegSubsamp, jint jpegQual, jint flags)
{
	tjhandle handle=0;
	unsigned long jpegSize=0;
	jsize arraySize=0, actualPitch;
	unsigned char *srcBuf=NULL, *jpegBuf=NULL;

	gethandle();

	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF || width<1 || height<1
		|| pitch<0)
		_throw("Invalid argument in compress()");
	if(org_libjpegturbo_turbojpeg_TJ_NUMPF!=TJ_NUMPF)
		_throw("Mismatch between Java and C API");

	actualPitch=(pitch==0)? width*tjPixelSize[pf]:pitch;
	arraySize=(y+height-1)*actualPitch + (x+width)*tjPixelSize[pf];
	if((*env)->GetArrayLength(env, src)*srcElementSize<arraySize)
		_throw("Source buffer is not large enough");
	jpegSize=tjBufSize(width, height, jpegSubsamp);
	if((*env)->GetArrayLength(env, dst)<(jsize)jpegSize)
		_throw("Destination buffer is not large enough");

	bailif0(srcBuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(jpegBuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjCompress2(handle, &srcBuf[y*actualPitch + x*tjPixelSize[pf]], width,
		pitch, height, pf, &jpegBuf, &jpegSize, jpegSubsamp, jpegQual,
		flags|TJFLAG_NOREALLOC)==-1)
		_throw(tjGetErrorStr());

	bailout:
	if(jpegBuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, jpegBuf, 0);
	if(srcBuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcBuf, 0);
	return (jint)jpegSize;
}

/* TurboJPEG 1.3.x: TJCompressor::compress() byte source */
JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compress___3BIIIIII_3BIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint x, jint y, jint width,
		jint pitch, jint height, jint pf, jbyteArray dst, jint jpegSubsamp,
		jint jpegQual, jint flags)
{
	return TJCompressor_compress(env, obj, src, 1, x, y, width, pitch, height,
		pf, dst, jpegSubsamp, jpegQual, flags);
}

/* TurboJPEG 1.2.x: TJCompressor::compress() byte source */
JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compress___3BIIII_3BIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint width, jint pitch,
		jint height, jint pf, jbyteArray dst, jint jpegSubsamp, jint jpegQual,
		jint flags)
{
	return TJCompressor_compress(env, obj, src, 1, 0, 0, width, pitch, height,
		pf, dst, jpegSubsamp, jpegQual, flags);
}

/* TurboJPEG 1.3.x: TJCompressor::compress() int source */
JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compress___3IIIIIII_3BIII
	(JNIEnv *env, jobject obj, jintArray src, jint x, jint y, jint width,
		jint stride, jint height, jint pf, jbyteArray dst, jint jpegSubsamp,
		jint jpegQual, jint flags)
{
	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF)
		_throw("Invalid argument in compress()");
	if(tjPixelSize[pf]!=sizeof(jint))
		_throw("Pixel format must be 32-bit when compressing from an integer buffer.");

	return TJCompressor_compress(env, obj, src, sizeof(jint), x, y, width,
		stride*sizeof(jint), height, pf, dst, jpegSubsamp, jpegQual, flags);

	bailout:
	return 0;
}

/* TurboJPEG 1.2.x: TJCompressor::compress() int source */
JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compress___3IIIII_3BIII
	(JNIEnv *env, jobject obj, jintArray src, jint width, jint stride,
		jint height, jint pf, jbyteArray dst, jint jpegSubsamp, jint jpegQual,
		jint flags)
{
	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF)
		_throw("Invalid argument in compress()");
	if(tjPixelSize[pf]!=sizeof(jint))
		_throw("Pixel format must be 32-bit when compressing from an integer buffer.");

	return TJCompressor_compress(env, obj, src, sizeof(jint), 0, 0, width,
		stride*sizeof(jint), height, pf, dst, jpegSubsamp, jpegQual, flags);

	bailout:
	return 0;
}

/* TurboJPEG 1.4.x: TJCompressor::compressFromYUV() */
JNIEXPORT jint JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_compressFromYUV___3BIIII_3BII
	(JNIEnv *env, jobject obj, jbyteArray src, jint width, jint pad, jint height,
		jint subsamp, jbyteArray dst, jint jpegQual, jint flags)
{
	tjhandle handle=0;
	unsigned long jpegSize=0;
	jsize arraySize=0;
	unsigned char *srcBuf=NULL, *jpegBuf=NULL;

	gethandle();

	arraySize=tjBufSizeYUV2(width, pad, height, subsamp);
	if((*env)->GetArrayLength(env, src)<arraySize)
		_throw("Source buffer is not large enough");
	jpegSize=tjBufSize(width, height, subsamp);
	if((*env)->GetArrayLength(env, dst)<(jsize)jpegSize)
		_throw("Destination buffer is not large enough");

	bailif0(srcBuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(jpegBuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjCompressFromYUV(handle, srcBuf, width, pad, height, subsamp, &jpegBuf,
		&jpegSize, jpegQual, flags|TJFLAG_NOREALLOC)==-1)
		_throw(tjGetErrorStr());

	bailout:
	if(jpegBuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, jpegBuf, 0);
	if(srcBuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcBuf, 0);
	return (jint)jpegSize;
}

JNIEXPORT void JNICALL TJCompressor_encodeYUV
	(JNIEnv *env, jobject obj, jarray src, jint srcElementSize, jint x, jint y,
		jint width, jint pitch, jint height, jint pf, jbyteArray dst, jint pad,
		jint subsamp, jint flags)
{
	tjhandle handle=0;
	jsize arraySize=0, actualPitch, yuvSize;
	unsigned char *srcBuf=NULL, *dstBuf=NULL;

	gethandle();

	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF || width<1 || height<1
		|| pitch<0)
		_throw("Invalid argument in encodeYUV()");
	if(org_libjpegturbo_turbojpeg_TJ_NUMPF!=TJ_NUMPF)
		_throw("Mismatch between Java and C API");

	actualPitch=(pitch==0)? width*tjPixelSize[pf]:pitch;
	arraySize=(y+height-1)*actualPitch + (x+width)*tjPixelSize[pf];
	if((*env)->GetArrayLength(env, src)*srcElementSize<arraySize)
		_throw("Source buffer is not large enough");
	yuvSize=(jsize)tjBufSizeYUV2(width, pad, height, subsamp);
	if(yuvSize==(unsigned long)-1)
		_throw(tjGetErrorStr());
	if((*env)->GetArrayLength(env, dst)<yuvSize)
		_throw("Destination buffer is not large enough");

	bailif0(srcBuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstBuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjEncodeYUV3(handle, &srcBuf[y*actualPitch + x*tjPixelSize[pf]], width,
		pitch, height, pf, dstBuf, pad, subsamp, flags)==-1)
		_throw(tjGetErrorStr());

	bailout:
	if(dstBuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstBuf, 0);
	if(srcBuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcBuf, 0);
	return;
}

/* TurboJPEG 1.4.x: TJCompressor::encodeYUV() byte source */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_encodeYUV___3BIIIIII_3BIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint x, jint y, jint width,
		jint pitch, jint height, jint pf, jbyteArray dst, jint pad, jint subsamp,
		jint flags)
{
	TJCompressor_encodeYUV(env, obj, src, 1, x, y, width, pitch, height, pf, dst,
		pad, subsamp, flags);
}

/* TurboJPEG 1.2.x: TJCompressor::encodeYUV() byte source */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_encodeYUV___3BIIII_3BII
	(JNIEnv *env, jobject obj, jbyteArray src, jint width, jint pitch,
		jint height, jint pf, jbyteArray dst, jint subsamp, jint flags)
{
	TJCompressor_encodeYUV(env, obj, src, 1, 0, 0, width, pitch, height, pf, dst,
		4, subsamp, flags);
}

/* TurboJPEG 1.4.x: TJCompressor::encodeYUV() int source */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_encodeYUV___3IIIIIII_3BIII
	(JNIEnv *env, jobject obj, jintArray src, jint x, jint y, jint width,
		jint stride, jint height, jint pf, jbyteArray dst, jint pad, jint subsamp,
		jint flags)
{
	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF)
		_throw("Invalid argument in encodeYUV()");
	if(tjPixelSize[pf]!=sizeof(jint))
		_throw("Pixel format must be 32-bit when encoding from an integer buffer.");

	TJCompressor_encodeYUV(env, obj, src, sizeof(jint), x, y, width,
		stride*sizeof(jint), height, pf, dst, pad, subsamp, flags);

	bailout:
	return;
}

/* TurboJPEG 1.2.x: TJCompressor::encodeYUV() int source */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJCompressor_encodeYUV___3IIIII_3BII
	(JNIEnv *env, jobject obj, jintArray src, jint width, jint stride,
		jint height, jint pf, jbyteArray dst, jint subsamp, jint flags)
{
	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF)
		_throw("Invalid argument in encodeYUV()");
	if(tjPixelSize[pf]!=sizeof(jint))
		_throw("Pixel format must be 32-bit when encoding from an integer buffer.");

	TJCompressor_encodeYUV(env, obj, src, sizeof(jint), 0, 0, width,
		stride*sizeof(jint), height, pf, dst, 4, subsamp, flags);

	bailout:
	return;
}

/* TurboJPEG 1.2.x: TJCompressor::destroy() */
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

/* TurboJPEG 1.2.x: TJDecompressor::init() */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_init
	(JNIEnv *env, jobject obj)
{
	jclass cls;
	jfieldID fid;
	tjhandle handle;

	if((handle=tjInitDecompress())==NULL) _throw(tjGetErrorStr());

	bailif0(cls=(*env)->GetObjectClass(env, obj));
	bailif0(fid=(*env)->GetFieldID(env, cls, "handle", "J"));
	(*env)->SetLongField(env, obj, fid, (size_t)handle);

	bailout:
	return;
}

/* TurboJPEG 1.2.x: TJDecompressor::getScalingFactors() */
JNIEXPORT jobjectArray JNICALL Java_org_libjpegturbo_turbojpeg_TJ_getScalingFactors
	(JNIEnv *env, jclass cls)
{
  jclass sfcls=NULL;  jfieldID fid=0;
	tjscalingfactor *sf=NULL;  int n=0, i;
	jobject sfobj=NULL;
	jobjectArray sfjava=NULL;

	if((sf=tjGetScalingFactors(&n))==NULL || n==0)
		_throw(tjGetErrorStr());

	bailif0(sfcls=(*env)->FindClass(env, "org/libjpegturbo/turbojpeg/TJScalingFactor"));
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

/* TurboJPEG 1.2.x: TJDecompressor::decompressHeader() */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompressHeader
	(JNIEnv *env, jobject obj, jbyteArray src, jint jpegSize)
{
	tjhandle handle=0;
	unsigned char *jpegBuf=NULL;
	int width=0, height=0, jpegSubsamp=-1, jpegColorspace=-1;

	gethandle();

	if((*env)->GetArrayLength(env, src)<jpegSize)
		_throw("Source buffer is not large enough");

	bailif0(jpegBuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));

	if(tjDecompressHeader3(handle, jpegBuf, (unsigned long)jpegSize,
		&width, &height, &jpegSubsamp, &jpegColorspace)==-1)
		_throw(tjGetErrorStr());

	(*env)->ReleasePrimitiveArrayCritical(env, src, jpegBuf, 0);  jpegBuf=NULL;

	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegSubsamp", "I"));
	(*env)->SetIntField(env, obj, _fid, jpegSubsamp);
	if((_fid=(*env)->GetFieldID(env, _cls, "jpegColorspace", "I"))==0)
		(*env)->ExceptionClear(env);
	else
		(*env)->SetIntField(env, obj, _fid, jpegColorspace);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegWidth", "I"));
	(*env)->SetIntField(env, obj, _fid, width);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegHeight", "I"));
	(*env)->SetIntField(env, obj, _fid, height);

	bailout:
	if(jpegBuf) (*env)->ReleasePrimitiveArrayCritical(env, src, jpegBuf, 0);
	return;
}

JNIEXPORT void JNICALL TJDecompressor_decompress
	(JNIEnv *env, jobject obj, jbyteArray src, jint jpegSize, jarray dst,
		jint dstElementSize, jint x, jint y, jint width, jint pitch, jint height,
		jint pf, jint flags)
{
	tjhandle handle=0;
	jsize arraySize=0, actualPitch;
	unsigned char *jpegBuf=NULL, *dstBuf=NULL;

	gethandle();

	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF)
		_throw("Invalid argument in decompress()");
	if(org_libjpegturbo_turbojpeg_TJ_NUMPF!=TJ_NUMPF)
		_throw("Mismatch between Java and C API");

	if((*env)->GetArrayLength(env, src)<jpegSize)
		_throw("Source buffer is not large enough");
	actualPitch=(pitch==0)? width*tjPixelSize[pf]:pitch;
	arraySize=(y+height-1)*actualPitch + (x+width)*tjPixelSize[pf];
	if((*env)->GetArrayLength(env, dst)*dstElementSize<arraySize)
		_throw("Destination buffer is not large enough");

	bailif0(jpegBuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstBuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjDecompress2(handle, jpegBuf, (unsigned long)jpegSize,
		&dstBuf[y*actualPitch + x*tjPixelSize[pf]], width, pitch, height, pf,
		flags)==-1)
		_throw(tjGetErrorStr());

	bailout:
	if(dstBuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstBuf, 0);
	if(jpegBuf) (*env)->ReleasePrimitiveArrayCritical(env, src, jpegBuf, 0);
	return;
}

/* TurboJPEG 1.3.x: TJDecompressor::decompress() byte destination */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompress___3BI_3BIIIIIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint jpegSize, jbyteArray dst,
		jint x, jint y, jint width, jint pitch, jint height, jint pf, jint flags)
{
	TJDecompressor_decompress(env, obj, src, jpegSize, dst, 1, x, y, width,
		pitch, height, pf, flags);
}

/* TurboJPEG 1.2.x: TJDecompressor::decompress() byte destination */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompress___3BI_3BIIIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint jpegSize, jbyteArray dst,
		jint width, jint pitch, jint height, jint pf, jint flags)
{
	TJDecompressor_decompress(env, obj, src, jpegSize, dst, 1, 0, 0, width,
		pitch, height, pf, flags);
}

/* TurboJPEG 1.3.x: TJDecompressor::decompress() int destination */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompress___3BI_3IIIIIIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint jpegSize, jintArray dst,
		jint x, jint y, jint width, jint stride, jint height, jint pf, jint flags)
{
	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF)
		_throw("Invalid argument in decompress()");
	if(tjPixelSize[pf]!=sizeof(jint))
		_throw("Pixel format must be 32-bit when decompressing to an integer buffer.");

	TJDecompressor_decompress(env, obj, src, jpegSize, dst, sizeof(jint), x, y,
		width, stride*sizeof(jint), height, pf, flags);

	bailout:
	return;
}

/* TurboJPEG 1.2.x: TJDecompressor::decompress() int destination */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompress___3BI_3IIIIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint jpegSize, jintArray dst,
		jint width, jint stride, jint height, jint pf, jint flags)
{
	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF)
		_throw("Invalid argument in decompress()");
	if(tjPixelSize[pf]!=sizeof(jint))
		_throw("Pixel format must be 32-bit when decompressing to an integer buffer.");

	TJDecompressor_decompress(env, obj, src, jpegSize, dst, sizeof(jint), 0, 0,
		width, stride*sizeof(jint), height, pf, flags);

	bailout:
	return;

}

/* TurboJPEG 1.4.x: TJDecompressor::decompressToYUV() */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompressToYUV___3BI_3BIIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint jpegSize, jbyteArray dst,
		jint desiredWidth, jint pad, jint desiredHeight, jint flags)
{
	tjhandle handle=0;
	unsigned char *jpegBuf=NULL, *dstBuf=NULL;
	int jpegSubsamp=-1, jpegWidth=0, jpegHeight=0;
	jsize yuvSize;

	gethandle();

	if((*env)->GetArrayLength(env, src)<jpegSize)
		_throw("Source buffer is not large enough");
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegSubsamp", "I"));
	jpegSubsamp=(int)(*env)->GetIntField(env, obj, _fid);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegWidth", "I"));
	jpegWidth=(int)(*env)->GetIntField(env, obj, _fid);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegHeight", "I"));
	jpegHeight=(int)(*env)->GetIntField(env, obj, _fid);

	yuvSize=(jsize)tjBufSizeYUV2(desiredWidth==0? jpegWidth:desiredWidth,
		pad, desiredHeight==0? jpegHeight:desiredHeight, jpegSubsamp);
	if(yuvSize==(unsigned long)-1)
		_throw(tjGetErrorStr());
	if((*env)->GetArrayLength(env, dst)<yuvSize)
		_throw("Destination buffer is not large enough");
	bailif0(jpegBuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstBuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjDecompressToYUV2(handle, jpegBuf, (unsigned long)jpegSize, dstBuf,
		desiredWidth, pad, desiredHeight, flags)==-1)
		_throw(tjGetErrorStr());

	bailout:
	if(dstBuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstBuf, 0);
	if(jpegBuf) (*env)->ReleasePrimitiveArrayCritical(env, src, jpegBuf, 0);
	return;
}

/* TurboJPEG 1.2.x: TJDecompressor::decompressToYUV() */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompressToYUV___3BI_3BI
	(JNIEnv *env, jobject obj, jbyteArray src, jint jpegSize, jbyteArray dst,
		jint flags)
{
	Java_org_libjpegturbo_turbojpeg_TJDecompressor_decompressToYUV___3BI_3BIIII(
		env, obj, src, jpegSize, dst, 0, 4, 0, flags);
}

JNIEXPORT void JNICALL TJDecompressor_decodeYUV
	(JNIEnv *env, jobject obj, jbyteArray src, jint pad, jint subsamp,
		jarray dst, jint dstElementSize, jint x, jint y, jint width, jint pitch,
		jint height, jint pf, jint flags)
{
	tjhandle handle=0;
	jsize arraySize=0, actualPitch;
	unsigned char *srcBuf=NULL, *dstBuf=NULL;

	gethandle();

	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF)
		_throw("Invalid argument in decodeYUV()");
	if(org_libjpegturbo_turbojpeg_TJ_NUMPF!=TJ_NUMPF)
		_throw("Mismatch between Java and C API");

	arraySize=tjBufSizeYUV2(width, pad, height, subsamp);
	if((*env)->GetArrayLength(env, src)<arraySize)
		_throw("Source buffer is not large enough");
	actualPitch=(pitch==0)? width*tjPixelSize[pf]:pitch;
	arraySize=(y+height-1)*actualPitch + (x+width)*tjPixelSize[pf];
	if((*env)->GetArrayLength(env, dst)*dstElementSize<arraySize)
		_throw("Destination buffer is not large enough");

	bailif0(srcBuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstBuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjDecodeYUV(handle, srcBuf, pad, subsamp,
		&dstBuf[y*actualPitch + x*tjPixelSize[pf]], width, pitch, height, pf,
		flags)==-1)
		_throw(tjGetErrorStr());

	bailout:
	if(dstBuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstBuf, 0);
	if(srcBuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcBuf, 0);
	return;
}

/* TurboJPEG 1.4.x: TJDecompressor::decodeYUV() byte destination */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decodeYUV___3BII_3BIIIIIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint pad, jint subsamp,
		jbyteArray dst, jint x, jint y, jint width, jint pitch, jint height,
		jint pf, jint flags)
{
	TJDecompressor_decodeYUV(env, obj, src, pad, subsamp, dst, 1, x, y, width,
		pitch, height, pf, flags);
}

/* TurboJPEG 1.4.x: TJDecompressor::decodeYUV() int destination */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_decodeYUV___3BII_3IIIIIIII
	(JNIEnv *env, jobject obj, jbyteArray src, jint pad, jint subsamp,
		jintArray dst, jint x, jint y, jint width, jint stride, jint height,
		jint pf, jint flags)
{
	if(pf<0 || pf>=org_libjpegturbo_turbojpeg_TJ_NUMPF)
		_throw("Invalid argument in decodeYUV()");
	if(tjPixelSize[pf]!=sizeof(jint))
		_throw("Pixel format must be 32-bit when decoding to an integer buffer.");

	TJDecompressor_decodeYUV(env, obj, src, pad, subsamp, dst, sizeof(jint), x,
		y, width, stride*sizeof(jint), height, pf, flags);

	bailout:
	return;
}

/* TurboJPEG 1.2.x: TJTransformer::init() */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJTransformer_init
	(JNIEnv *env, jobject obj)
{
	jclass cls;
	jfieldID fid;
	tjhandle handle;

	if((handle=tjInitTransform())==NULL) _throw(tjGetErrorStr());

	bailif0(cls=(*env)->GetObjectClass(env, obj));
	bailif0(fid=(*env)->GetFieldID(env, cls, "handle", "J"));
	(*env)->SetLongField(env, obj, fid, (size_t)handle);

	bailout:
	return;
}

typedef struct _JNICustomFilterParams
{
	JNIEnv *env;
	jobject tobj;
	jobject cfobj;
} JNICustomFilterParams;

static int JNICustomFilter(short *coeffs, tjregion arrayRegion,
	tjregion planeRegion, int componentIndex, int transformIndex,
	tjtransform *transform)
{
	JNICustomFilterParams *params=(JNICustomFilterParams *)transform->data;
	JNIEnv *env=params->env;
	jobject tobj=params->tobj, cfobj=params->cfobj;
  jobject arrayRegionObj, planeRegionObj, bufobj, borobj;
	jclass cls;  jmethodID mid;  jfieldID fid;

	bailif0(bufobj=(*env)->NewDirectByteBuffer(env, coeffs,
		sizeof(short)*arrayRegion.w*arrayRegion.h));
	bailif0(cls=(*env)->FindClass(env, "java/nio/ByteOrder"));
  bailif0(mid=(*env)->GetStaticMethodID(env, cls, "nativeOrder",
		"()Ljava/nio/ByteOrder;"));
	bailif0(borobj=(*env)->CallStaticObjectMethod(env, cls, mid));
	bailif0(cls=(*env)->GetObjectClass(env, bufobj));
	bailif0(mid=(*env)->GetMethodID(env, cls, "order",
		"(Ljava/nio/ByteOrder;)Ljava/nio/ByteBuffer;"));
	(*env)->CallObjectMethod(env, bufobj, mid, borobj);
  bailif0(mid=(*env)->GetMethodID(env, cls, "asShortBuffer",
		"()Ljava/nio/ShortBuffer;"));
	bailif0(bufobj=(*env)->CallObjectMethod(env, bufobj, mid));

	bailif0(cls=(*env)->FindClass(env, "java/awt/Rectangle"));
	bailif0(arrayRegionObj=(*env)->AllocObject(env, cls));
	bailif0(fid=(*env)->GetFieldID(env, cls, "x", "I"));
	(*env)->SetIntField(env, arrayRegionObj, fid, arrayRegion.x);
	bailif0(fid=(*env)->GetFieldID(env, cls, "y", "I"));
	(*env)->SetIntField(env, arrayRegionObj, fid, arrayRegion.y);
	bailif0(fid=(*env)->GetFieldID(env, cls, "width", "I"));
	(*env)->SetIntField(env, arrayRegionObj, fid, arrayRegion.w);
	bailif0(fid=(*env)->GetFieldID(env, cls, "height", "I"));
	(*env)->SetIntField(env, arrayRegionObj, fid, arrayRegion.h);

	bailif0(planeRegionObj=(*env)->AllocObject(env, cls));
	bailif0(fid=(*env)->GetFieldID(env, cls, "x", "I"));
	(*env)->SetIntField(env, planeRegionObj, fid, planeRegion.x);
	bailif0(fid=(*env)->GetFieldID(env, cls, "y", "I"));
	(*env)->SetIntField(env, planeRegionObj, fid, planeRegion.y);
	bailif0(fid=(*env)->GetFieldID(env, cls, "width", "I"));
	(*env)->SetIntField(env, planeRegionObj, fid, planeRegion.w);
	bailif0(fid=(*env)->GetFieldID(env, cls, "height", "I"));
	(*env)->SetIntField(env, planeRegionObj, fid, planeRegion.h);

	bailif0(cls=(*env)->GetObjectClass(env, cfobj));
	bailif0(mid=(*env)->GetMethodID(env, cls, "customFilter",
		"(Ljava/nio/ShortBuffer;Ljava/awt/Rectangle;Ljava/awt/Rectangle;IILorg/libjpegturbo/turbojpeg/TJTransform;)V"));
	(*env)->CallVoidMethod(env, cfobj, mid, bufobj, arrayRegionObj,
		planeRegionObj, componentIndex, transformIndex, tobj);

	return 0;

	bailout:
	return -1;
}

/* TurboJPEG 1.2.x: TJTransformer::transform() */
JNIEXPORT jintArray JNICALL Java_org_libjpegturbo_turbojpeg_TJTransformer_transform
	(JNIEnv *env, jobject obj, jbyteArray jsrcBuf, jint jpegSize,
		jobjectArray dstobjs, jobjectArray tobjs, jint flags)
{
	tjhandle handle=0;  int i;
	unsigned char *jpegBuf=NULL, **dstBufs=NULL;  jsize n=0;
	unsigned long *dstSizes=NULL;  tjtransform *t=NULL;
	jbyteArray *jdstBufs=NULL;
	int jpegWidth=0, jpegHeight=0, jpegSubsamp;
	jintArray jdstSizes=0;  jint *dstSizesi=NULL;
	JNICustomFilterParams *params=NULL;

	gethandle();

	if((*env)->GetArrayLength(env, jsrcBuf)<jpegSize)
		_throw("Source buffer is not large enough");
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegWidth", "I"));
	jpegWidth=(int)(*env)->GetIntField(env, obj, _fid);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegHeight", "I"));
	jpegHeight=(int)(*env)->GetIntField(env, obj, _fid);
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "jpegSubsamp", "I"));
	jpegSubsamp=(int)(*env)->GetIntField(env, obj, _fid);

	n=(*env)->GetArrayLength(env, dstobjs);
	if(n!=(*env)->GetArrayLength(env, tobjs))
		_throw("Mismatch between size of transforms array and destination buffers array");

	if((dstBufs=(unsigned char **)malloc(sizeof(unsigned char *)*n))==NULL)
		_throw("Memory allocation failure");
	if((jdstBufs=(jbyteArray *)malloc(sizeof(jbyteArray)*n))==NULL)
		_throw("Memory allocation failure");
	if((dstSizes=(unsigned long *)malloc(sizeof(unsigned long)*n))==NULL)
		_throw("Memory allocation failure");
	if((t=(tjtransform *)malloc(sizeof(tjtransform)*n))==NULL)
		_throw("Memory allocation failure");
	if((params=(JNICustomFilterParams *)malloc(sizeof(JNICustomFilterParams)*n))
		==NULL)
		_throw("Memory allocation failure");
	for(i=0; i<n; i++)
	{
		dstBufs[i]=NULL;  jdstBufs[i]=NULL;  dstSizes[i]=0;
		memset(&t[i], 0, sizeof(tjtransform));
		memset(&params[i], 0, sizeof(JNICustomFilterParams));
	}

	for(i=0; i<n; i++)
	{
		jobject tobj, cfobj;

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

		bailif0(_fid=(*env)->GetFieldID(env, _cls, "cf",
			"Lorg/libjpegturbo/turbojpeg/TJCustomFilter;"));
		cfobj=(*env)->GetObjectField(env, tobj, _fid);
		if(cfobj)
		{
			params[i].env=env;
			params[i].tobj=tobj;
			params[i].cfobj=cfobj;
			t[i].customFilter=JNICustomFilter;
			t[i].data=(void *)&params[i];
		}
	}

	for(i=0; i<n; i++)
	{
		int w=jpegWidth, h=jpegHeight;
		if(t[i].r.w!=0) w=t[i].r.w;
		if(t[i].r.h!=0) h=t[i].r.h;
		bailif0(jdstBufs[i]=(*env)->GetObjectArrayElement(env, dstobjs, i));
		if((unsigned long)(*env)->GetArrayLength(env, jdstBufs[i])
			<tjBufSize(w, h, jpegSubsamp))
			_throw("Destination buffer is not large enough");
	}
	bailif0(jpegBuf=(*env)->GetPrimitiveArrayCritical(env, jsrcBuf, 0));
	for(i=0; i<n; i++)
		bailif0(dstBufs[i]=(*env)->GetPrimitiveArrayCritical(env, jdstBufs[i], 0));

	if(tjTransform(handle, jpegBuf, jpegSize, n, dstBufs, dstSizes, t,
		flags|TJFLAG_NOREALLOC)==-1)
		_throw(tjGetErrorStr());

	for(i=0; i<n; i++)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, jdstBufs[i], dstBufs[i], 0);
		dstBufs[i]=NULL;
	}
	(*env)->ReleasePrimitiveArrayCritical(env, jsrcBuf, jpegBuf, 0);
	jpegBuf=NULL;

	jdstSizes=(*env)->NewIntArray(env, n);
	bailif0(dstSizesi=(*env)->GetIntArrayElements(env, jdstSizes, 0));
	for(i=0; i<n; i++) dstSizesi[i]=(int)dstSizes[i];

	bailout:
	if(dstSizesi) (*env)->ReleaseIntArrayElements(env, jdstSizes, dstSizesi, 0);
	if(dstBufs)
	{
		for(i=0; i<n; i++)
		{
			if(dstBufs[i] && jdstBufs && jdstBufs[i])
				(*env)->ReleasePrimitiveArrayCritical(env, jdstBufs[i], dstBufs[i], 0);
		}
		free(dstBufs);
	}
	if(jpegBuf) (*env)->ReleasePrimitiveArrayCritical(env, jsrcBuf, jpegBuf, 0);
	if(jdstBufs) free(jdstBufs);
	if(dstSizes) free(dstSizes);
	if(t) free(t);
	return jdstSizes;
}

/* TurboJPEG 1.2.x: TJDecompressor::destroy() */
JNIEXPORT void JNICALL Java_org_libjpegturbo_turbojpeg_TJDecompressor_destroy
	(JNIEnv *env, jobject obj)
{
	Java_org_libjpegturbo_turbojpeg_TJCompressor_destroy(env, obj);
}

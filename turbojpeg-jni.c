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

#include "turbojpeg.h"
#include <jni.h>
#include "java/TJCompressor.h"
#include "java/TJDecompressor.h"
#include "java/TJ.h"

#define _throw(msg) {  \
	jclass _exccls=(*env)->FindClass(env, "java/lang/Exception");  \
	if(!_exccls) goto bailout;  \
	(*env)->ThrowNew(env, _exccls, msg);  \
	goto bailout;  \
}

#define bailif0(f) {if(!(f)) goto bailout;}

#define gethandle() {  \
	jclass _cls=(*env)->GetObjectClass(env, obj);  \
	jfieldID _fid;  \
	if(!_cls) goto bailout;  \
	bailif0(_fid=(*env)->GetFieldID(env, _cls, "handle", "J"));  \
	handle=(tjhandle)(long)(*env)->GetLongField(env, obj, _fid);  \
}

JNIEXPORT jlong JNICALL Java_TJ_bufSize
	(JNIEnv *env, jclass cls, jint width, jint height)
{
	return TJBUFSIZE(width, height);
}

JNIEXPORT void JNICALL Java_TJCompressor_init
	(JNIEnv *env, jobject obj)
{
	jclass cls;
	jfieldID fid;
	tjhandle handle;

  if((handle=tjInitCompress())==NULL)
		_throw(tjGetErrorStr());

	bailif0(cls=(*env)->GetObjectClass(env, obj));
	bailif0(fid=(*env)->GetFieldID(env, cls, "handle", "J"));
	(*env)->SetLongField(env, obj, fid, (long)handle);

	bailout:
	return;
}

JNIEXPORT jlong JNICALL Java_TJCompressor_compress
	(JNIEnv *env, jobject obj, jbyteArray src, jint width, jint pitch,
		jint height, jint pixelsize, jbyteArray dst, jint jpegsubsamp,
		jint jpegqual, jint flags)
{
	tjhandle handle=0;
	unsigned long size=0;
	unsigned char *srcbuf=NULL, *dstbuf=NULL;

	gethandle();

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstbuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjCompress(handle, srcbuf, width, pitch, height, pixelsize, dstbuf,
		&size, jpegsubsamp, jpegqual, flags)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		_throw(tjGetErrorStr());
	}

	bailout:
	if(dstbuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
	if(srcbuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
	return size;
}

JNIEXPORT void JNICALL Java_TJCompressor_destroy
	(JNIEnv *env, jobject obj)
{
	tjhandle handle=0;

	gethandle();

	if(tjDestroy(handle)==-1) _throw(tjGetErrorStr());

	bailout:
	return;
}

JNIEXPORT void JNICALL Java_TJDecompressor_init
	(JNIEnv *env, jobject obj)
{
	jclass cls;
	jfieldID fid;
	tjhandle handle;

  if((handle=tjInitDecompress())==NULL) _throw(tjGetErrorStr());

	bailif0(cls=(*env)->GetObjectClass(env, obj));
	bailif0(fid=(*env)->GetFieldID(env, cls, "handle", "J"));
	(*env)->SetLongField(env, obj, fid, (long)handle);

	bailout:
	return;
}

JNIEXPORT jobject JNICALL Java_TJDecompressor_decompressHeader
	(JNIEnv *env, jobject obj, jbyteArray src, jlong size)
{
	jclass jhicls=NULL;
	jfieldID fid;
	tjhandle handle=0;
	unsigned char *srcbuf=NULL;
	int width=0, height=0, jpegsubsamp=-1;
	jobject jhiobj=NULL;

	gethandle();

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));

	if(tjDecompressHeader2(handle, srcbuf, (unsigned long)size, 
		&width, &height, &jpegsubsamp)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		_throw(tjGetErrorStr());
	}
	(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);  srcbuf=NULL;

	bailif0(jhicls=(*env)->FindClass(env, "TJHeaderInfo"));
	bailif0(jhiobj=(*env)->AllocObject(env, jhicls));

	bailif0(fid=(*env)->GetFieldID(env, jhicls, "subsamp", "I"));
	(*env)->SetIntField(env, jhiobj, fid, jpegsubsamp);
	bailif0(fid=(*env)->GetFieldID(env, jhicls, "width", "I"));
	(*env)->SetIntField(env, jhiobj, fid, width);
	bailif0(fid=(*env)->GetFieldID(env, jhicls, "height", "I"));
	(*env)->SetIntField(env, jhiobj, fid, height);

	bailout:
	return jhiobj;
}

JNIEXPORT void JNICALL Java_TJDecompressor_decompress
	(JNIEnv *env, jobject obj, jbyteArray src, jlong size, jbyteArray dst,
		jint width, jint pitch, jint height, jint pixelsize, jint flags)
{
	tjhandle handle=0;
	unsigned char *srcbuf=NULL, *dstbuf=NULL;

	gethandle();

	bailif0(srcbuf=(*env)->GetPrimitiveArrayCritical(env, src, 0));
	bailif0(dstbuf=(*env)->GetPrimitiveArrayCritical(env, dst, 0));

	if(tjDecompress(handle, srcbuf, (unsigned long)size, dstbuf, width, pitch,
		height, pixelsize, flags)==-1)
	{
		(*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
		(*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
		_throw(tjGetErrorStr());
	}

	bailout:
	if(dstbuf) (*env)->ReleasePrimitiveArrayCritical(env, dst, dstbuf, 0);
	if(srcbuf) (*env)->ReleasePrimitiveArrayCritical(env, src, srcbuf, 0);
	return;
}

JNIEXPORT void JNICALL Java_TJDecompressor_destroy
	(JNIEnv *env, jobject obj)
{
	Java_TJCompressor_destroy(env, obj);
}

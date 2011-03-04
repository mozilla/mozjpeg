/* Copyright (C)2004 Landmark Graphics Corporation
 * Copyright (C)2005 Sun Microsystems, Inc.
 * Copyright (C)2010 D. R. Commander
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

#ifndef __RRUTIL_H__
#define __RRUTIL_H__

#ifdef _WIN32
	#include <windows.h>
	#ifndef __MINGW32__
		#define snprintf(str, n, format, ...)  \
			_snprintf_s(str, n, _TRUNCATE, format, __VA_ARGS__)
	#endif
#else
	#include <unistd.h>
	#define stricmp strcasecmp
	#define strnicmp strncasecmp
#endif

#ifndef min
 #define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
 #define max(a,b) ((a)>(b)?(a):(b))
#endif

#define pow2(i) (1<<(i))
#define isPow2(x) (((x)&(x-1))==0)

#ifdef sun
#define __inline inline
#endif

#define byteswap(i) ( \
	(((i) & 0xff000000) >> 24) | \
	(((i) & 0x00ff0000) >>  8) | \
	(((i) & 0x0000ff00) <<  8) | \
	(((i) & 0x000000ff) << 24) )

#define byteswap16(i) ( \
	(((i) & 0xff00) >> 8) | \
	(((i) & 0x00ff) << 8) )

static __inline int littleendian(void)
{
	unsigned int value=1;
	unsigned char *ptr=(unsigned char *)(&value);
	if(ptr[0]==1 && ptr[3]==0) return 1;
	else return 0;
}

#endif

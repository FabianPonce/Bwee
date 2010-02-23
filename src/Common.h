/*
* Bwee IRC Bot
* Copyright (C) 2010 Valroft <http://www.mintwow.com/>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <iostream>
#include <time.h>
#include <map>
#include <vector>
#include <string>
#include <list>
#include <set>
#include <stack>
#include <sstream>

// platform-specific includes
#ifdef WIN32
#include <memory> // tr1
#include <WinSock2.h>
#else
#include <tr1/memory>
#endif

#include <mysql/mysql.h>

using namespace std; // no std:: prefix necessary on string/set/etc

// use secure functions, rename.
#define snprintf _snprintf
#define snscanf _snscanf
#define vsnprintf _vsnprintf

// current platform and compiler
#define PLATFORM_WIN32 0
#define PLATFORM_UNIX  1
#define PLATFORM_APPLE 2

// platform detection
#define UNIX_FLAVOUR_LINUX 1
#define UNIX_FLAVOUR_BSD 2
#define UNIX_FLAVOUR_OTHER 3
#define UNIX_FLAVOUR_OSX 4

#if defined( __WIN32__ ) || defined( WIN32 ) || defined( _WIN32 )
#  define PLATFORM PLATFORM_WIN32
#elif defined( __APPLE_CC__ )
#  define PLATFORM PLATFORM_APPLE
#else
#  define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_UNIX || PLATFORM == PLATFORM_APPLE
#ifdef HAVE_DARWIN
#define PLATFORM_TEXT "MacOSX"
#define UNIX_FLAVOUR UNIX_FLAVOUR_OSX
#else
#ifdef USE_KQUEUE
#define PLATFORM_TEXT "FreeBSD"
#define UNIX_FLAVOUR UNIX_FLAVOUR_BSD
#else
#define PLATFORM_TEXT "Linux"
#define UNIX_FLAVOUR UNIX_FLAVOUR_LINUX
#endif
#endif
#else
#if PLATFORM == PLATFORM_WIN32
#define PLATFORM_TEXT "Win32"
#endif
#endif

// configuration mode. debug/release
#ifdef _DEBUG
#define CONFIG "Debug"
#else
#define CONFIG "Release"
#endif

// inline define, force on windows, strongly suggest on *nix :)
#ifdef WIN32
#define BWEE_INLINE __forceinline
#else
#define BWEE_INLINE inline
#endif

// primitive types
#ifdef WIN32
typedef signed __int64 int64;
typedef signed __int32 int32;
typedef signed __int16 int16;
typedef signed __int8 int8;

typedef unsigned __int64 uint64;
typedef unsigned __int32 uint32;
typedef unsigned __int16 uint16;
typedef unsigned __int8 uint8;

typedef unsigned __int64 uint64_t;
typedef signed __int64 int64_t;
#else

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef uint32_t DWORD;

#endif

// windows needs Sleep(ms) function.
#ifndef WIN32
#define Sleep(ms) usleep(1000*ms)
#endif

// case-sensitive comparisons
#if PLATFORM == PLATFORM_WIN32
#define strcasecmp stricmp
#endif

// newline style
#if PLATFORM == PLATFORM_WIN32
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif

// uint64 printf-format style specifiers
#ifdef WIN32

#define I64FMT "%016I64X"
#define I64FMTD "%I64u"
#define SI64FMTD "%I64d"
#define snprintf _snprintf
#define atoll __atoi64

#else

#define stricmp strcasecmp
#define strnicmp strncasecmp
#define I64FMT "%016llX"
#define I64FMTD "%llu"
#define SI64FMTD "%lld"

#endif

__forceinline uint32 getMSTime()
{
	return (uint32)GetTickCount();
}

#include "Log.h"
extern CLog Log;

#define strlwr _strlwr
#define strnicmp _strnicmp

#endif
// Copyright (C) 2015 SRG Technology, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef ANYRPC_API_H_
#define ANYRPC_API_H_

#include "version.h"

// consolidate the standard library includes - at least for header processing
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <vector>
#include <list>
#include <map>
#include <queue>
#include <cassert>
#include <time.h>
#include <iterator>
#include <cmath>

///////////////////////////////////////////////////////////////////////////////
// Compiler Specific Includes

#if defined(_MSC_VER) || defined(__MINGW32__)
#define NOMINMAX 1
# include <winsock2.h>
# include <windows.h>
# include <ws2tcpip.h>
#endif

#if defined(_MSC_VER)
# include <Mstcpip.h>

#if (_MSC_VER < 1900)
# define snprintf _snprintf
#endif

# define gmtime_r(a,b) gmtime_s(b,a)
# define localtime_r(a,b) localtime_s(b,a)
# define strncasecmp(x,y,z) _strnicmp(x,y,z)
# define strcasecmp(x,y) _stricmp(x,y)
# define setenv(name,value,overwrite) _putenv_s(name,value)

typedef int socklen_t;

#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
// no MSG_NOSIGNAL defined in Windows, but as Windows doesn't support SIGPIPE it isn't needed
# define MSG_NOSIGNAL 0

// no MSG_DONTWAIT defined on Windows
# define MSG_DONTWAIT 0

#endif

#if defined(_MSC_VER)
// Certain version of Visual Studio require special support
# include "internal/stdint.h"
# include "internal/inttypes.h"
#else
// Other compilers should have these
# include <stdint.h>
# include <inttypes.h>
#endif

#if defined(_MSC_VER)

// Disable warning C4390: empty control statement found which can occur when logging is disabled
# pragma warning(disable:4390)

# if (defined(ANYRPC_DLL_BUILD) || defined(ANYRPC_DLL))
// Disable warning C4251: <data member>: <type> needs to have dll-interface to be used by...
// Note: This is also disabled in the log4cplus headers
#  pragma warning(disable:4251)

#  if defined(ANYRPC_DLL_BUILD)
#   define ANYRPC_API __declspec(dllexport)
#  elif defined(ANYRPC_DLL)
#   define ANYRPC_API __declspec(dllimport)
#  endif // if defined(ANYRPC_DLL_BUILD)

# endif // if (defined(ANYRPC_DLL_BUILD) || defined(ANYRPC_DLL))
#endif // if defined(_MSC_VER)

#if !defined(ANYRPC_API)
# define ANYRPC_API
#endif

///////////////////////////////////////////////////////////////////////////////
// ANYRPC_ENDIAN
#define ANYRPC_LITTLEENDIAN  0   //!< Little endian machine
#define ANYRPC_BIGENDIAN     1   //!< Big endian machine

//! Endianness of the machine.
/*!
    \def ANYRPC_ENDIAN
    \ingroup ANYRPC_CONFIG

    GCC 4.6 provided macro for detecting endianness of the target machine. But other
    compilers may not have this. User can define ANYRPC_ENDIAN to either
    \ref ANYRPC_LITTLEENDIAN or \ref ANYRPC_BIGENDIAN.

    Default detection implemented with reference to
    \li https://gcc.gnu.org/onlinedocs/gcc-4.6.0/cpp/Common-Predefined-Macros.html
    \li http://www.boost.org/doc/libs/1_42_0/boost/detail/endian.hpp
*/
#ifndef ANYRPC_ENDIAN
// Detect with GCC 4.6's macro
#  ifdef __BYTE_ORDER__
#    if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#      define ANYRPC_ENDIAN ANYRPC_LITTLEENDIAN
#    elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#      define ANYRPC_ENDIAN ANYRPC_BIGENDIAN
#    else
#      error Unknown machine endianess detected. User needs to define ANYRPC_ENDIAN.
#    endif // __BYTE_ORDER__
// Detect with GLIBC's endian.h
#  elif defined(__GLIBC__)
#    include <endian.h>
#    if (__BYTE_ORDER == __LITTLE_ENDIAN)
#      define ANYRPC_ENDIAN ANYRPC_LITTLEENDIAN
#    elif (__BYTE_ORDER == __BIG_ENDIAN)
#      define ANYRPC_ENDIAN ANYRPC_BIGENDIAN
#    else
#      error Unknown machine endianess detected. User needs to define ANYRPC_ENDIAN.
#   endif // __GLIBC__
// Detect with _LITTLE_ENDIAN and _BIG_ENDIAN macro
#  elif defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
#    define ANYRPC_ENDIAN ANYRPC_LITTLEENDIAN
#  elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
#    define ANYRPC_ENDIAN ANYRPC_BIGENDIAN
// Detect with architecture macros
#  elif defined(__sparc) || defined(__sparc__) || defined(_POWER) || defined(__powerpc__) || defined(__ppc__) || defined(__hpux) || defined(__hppa) || defined(_MIPSEB) || defined(_POWER) || defined(__s390__)
#    define ANYRPC_ENDIAN ANYRPC_BIGENDIAN
#  elif defined(__i386__) || defined(__alpha__) || defined(__ia64) || defined(__ia64__) || defined(_M_IX86) || defined(_M_IA64) || defined(_M_ALPHA) || defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) || defined(__bfin__)
#    define ANYRPC_ENDIAN ANYRPC_LITTLEENDIAN
#  elif defined(ANYRPC_DOXYGEN_RUNNING)
#    define ANYRPC_ENDIAN
#  else
#    error Unknown machine endianess detected. User needs to define ANYRPC_ENDIAN.
#  endif
#endif // ANYRPC_ENDIAN

///////////////////////////////////////////////////////////////////////////////
// ANYRPC_64BIT

//! Whether using 64-bit architecture
#if !defined(ANYRPC_64BIT)
# if defined(__LP64__) || defined(_WIN64)
#  define ANYRPC_64BIT 1
# else
#  define ANYRPC_64BIT 0
# endif
#endif // ANYRPC_64BIT

///////////////////////////////////////////////////////////////////////////////
// ANYRPC_UINT64_C2

//! Construct a 64-bit literal by a pair of 32-bit integer.
/*!
    64-bit literal with or without ULL suffix is prone to compiler warnings.
    UINT64_C() is C macro which cause compilation problems.
    Use this macro to define 64-bit constants by a pair of 32-bit integer.
*/
#if !defined(ANYRPC_UINT64_C2)
# define ANYRPC_UINT64_C2(high32, low32) ((static_cast<uint64_t>(high32) << 32) | static_cast<uint64_t>(low32))
#endif


#define ANYRPC_DATETIME_STRING "AnyRpcDateTime"
#define ANYRPC_BASE64_STRING "AnyRpcBase64"

namespace anyrpc
{
//! Encoding type of data.  This is currently not widely supported in the library since UTF-8 most commonly used
enum EncodingEnum
{
    ASCII,          //!< US-ascii format
    UTF8,           //!< UTF-8 format
    UTF16LE,        //!< UTF-16 little endian format
    UTF16BE,        //!< UTF-16 big endian format
    UTF32LE,        //!< UTF-32 little endian format
    UTF32BE,        //!< UTF-32 big endian format
};
}


#endif // ANYRPC_API_H_

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

#ifndef ANYRPC_TIME_H_
#define ANYRPC_TIME_H_

#ifndef _MSC_VER
#include <sys/time.h>
#endif

namespace anyrpc
{

#ifdef _MSC_VER
int gettimeofday(struct timeval * tp, struct timezone * tzp);
#endif

//! Compute the difference between the two times in milliseconds
ANYRPC_API int MilliTimeDiff(struct timeval &time1, struct timeval &time2);

//! Compute the difference between the two times in microseconds
ANYRPC_API int64_t MicroTimeDiff(struct timeval &time1, struct timeval &time2);

//! Sleep for a time specified in milliseconds
ANYRPC_API void MilliSleep(unsigned ms);

} // namespace anyrpc

#endif // ANYRPC_TIME_H_

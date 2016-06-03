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

#include "anyrpc/api.h"
#include "anyrpc/logger.h"
#include "anyrpc/internal/time.h"

#if BUILD_WITH_LOG4CPLUS
#include <log4cplus/configurator.h>
#include <log4cplus/helpers/stringhelper.h>
#include <iomanip>
#endif

using namespace anyrpc;

#if BUILD_WITH_LOG4CPLUS
TimeLogger::TimeLogger(const log4cplus::Logger& l,
		log4cplus::LogLevel ll,
		const log4cplus::tstring& _msg,
		bool _timing,
		const char* _file,
		int _line,
		char const * _function)
    	: logger(l), loglevel(ll), timing(_timing), msg(_msg), file(_file), function(_function), line(_line), enabled(false)
{
	if (logger.isEnabledFor(loglevel))
    {
        enabled = true;
        if (timing)
        {
        	gettimeofday(&startTime, 0);
        }
    	log4cplus::tostringstream _log4cplus_buf;
    	_log4cplus_buf << LOG4CPLUS_TEXT("ENTER ") << function;
    	if (msg.length() != 0)
    	{
    		_log4cplus_buf << LOG4CPLUS_TEXT(" : ") << msg;
    	}
        logger.forcedLog(loglevel, _log4cplus_buf.str(), file, line, function);
    }
}

TimeLogger::~TimeLogger()
{
    if (enabled)
    {
    	// create the full message to log
    	log4cplus::tostringstream _log4cplus_buf;
    	_log4cplus_buf << LOG4CPLUS_TEXT("EXIT  ") << function;
    	if (msg.length() != 0)
    	{
    		_log4cplus_buf << LOG4CPLUS_TEXT(" : ") << msg;
    	}
    	if (timing)
    	{
        	// determine how much time elapsed
    	    struct timeval endTime;
    	    gettimeofday( &endTime, 0 );
    	    double diffTime = anyrpc::MicroTimeDiff(endTime, startTime) * 1e-6;

			_log4cplus_buf.setf( std::ios::fixed, std:: ios::floatfield );
			_log4cplus_buf.precision(6);
			_log4cplus_buf<< LOG4CPLUS_TEXT(" [") << diffTime << LOG4CPLUS_TEXT(" sec] ");
    	}

		// log the message
        logger.forcedLog(loglevel, _log4cplus_buf.str(), file, line, function);
    }
}
#endif

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

#ifndef ANYRPC_LOGGER_H
#define ANYRPC_LOGGER_H

#if defined(BUILD_WITH_LOG4CPLUS)
# include <log4cplus/logger.h>
# include <log4cplus/loggingmacros.h>
# include <log4cplus/helpers/timehelper.h>
#endif

ANYRPC_API void InitializeLogger();

#if defined(BUILD_WITH_LOG4CPLUS)
# define ANYRPC_FUNCTION_NAME() LOG4CPLUS_MACRO_FUNCTION()
#else
# define ANYRPC_FUNCTION_NAME()
#endif

// Define the logger that will be used in the current namespace or class.
#if defined(BUILD_WITH_LOG4CPLUS)
# define log_define(category) \
   static log4cplus::Logger _getStaticLogger()   \
   {  \
	 static bool _staticLoggerDefined = false; \
     static log4cplus::Logger _staticLogger; \
     if (!_staticLoggerDefined) \
     { \
         _staticLogger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(category)); \
         _staticLoggerDefined = true; \
	 } \
     return _staticLogger; \
   }
#else
# define log_define(category)
#endif

// Log messages using the defined logger
#if defined(BUILD_WITH_LOG4CPLUS)
# define log_fatal(expr) LOG4CPLUS_FATAL(_getStaticLogger(), expr)
# define log_error(expr) LOG4CPLUS_ERROR(_getStaticLogger(), expr)
# define log_warn(expr)  LOG4CPLUS_WARN( _getStaticLogger(), expr)
# define log_info(expr)  LOG4CPLUS_INFO( _getStaticLogger(), expr)
# define log_debug(expr) LOG4CPLUS_DEBUG(_getStaticLogger(), expr)
#else
# define log_fatal(expr)
# define log_error(expr)
# define log_warn(expr)
# define log_info(expr)
# define log_debug(expr)
#endif

// Only log messages if the condition evaluates to true
#if defined(BUILD_WITH_LOG4CPLUS)
# define log_fatal_if(cond, expr) do { if (cond) LOG4CPLUS_FATAL(_getStaticLogger(), expr); } while(false)
# define log_error_if(cond, expr) do { if (cond) LOG4CPLUS_ERROR(_getStaticLogger(), expr); } while(false)
# define log_warn_if(cond, expr)  do { if (cond) LOG4CPLUS_WARN( _getStaticLogger(), expr); } while(false)
# define log_info_if(cond, expr)  do { if (cond) LOG4CPLUS_IFNO( _getStaticLogger(), expr); } while(false)
# define log_debug_if(cond, expr) do { if (cond) LOG4CPLUS_DEBUG(_getStaticLogger(), expr); } while(false)
#else
# define log_fatal_if(cond, expr)
# define log_error_if(cond, expr)
# define log_warn_if(cond, expr)
# define log_info_if(cond, expr)
# define log_debug_if(cond, expr)
#endif

// This case is based on the TraceLogger but forces the function name
// to be output and outputs the time from start to end.
// The message is optional and can be used to differentiate timings
// that are internal to a function.
#if defined(BUILD_WITH_LOG4CPLUS)
class ANYRPC_API TimeLogger
{
public:
    TimeLogger(const log4cplus::Logger& l, log4cplus::LogLevel ll,
    	const log4cplus::tstring& _msg,
   		bool _timing,
        const char* _file = LOG4CPLUS_CALLER_FILE (),
        int _line = LOG4CPLUS_CALLER_LINE (),
        char const * _function = LOG4CPLUS_CALLER_FUNCTION ());

    ~TimeLogger();

private:
    TimeLogger (TimeLogger const &);
    TimeLogger & operator = (TimeLogger const &);

    log4cplus::Logger logger;
    log4cplus::LogLevel loglevel;
    log4cplus::tstring msg;
    bool timing;
    const char* file;
    const char* function;
    int line;
    bool enabled;
    log4cplus::helpers::Time startTime;
};

# define TIME_LEVEL_MACRO(loglevel,msg,timing)                        \
    TimeLogger _time_logger(_getStaticLogger(), loglevel, msg, timing,   \
        __FILE__, __LINE__, LOG4CPLUS_MACRO_FUNCTION ())
#else
# define TIME_LEVEL_MACRO(loglevel,msg,timing)
#endif

#if defined(BUILD_WITH_LOG4CPLUS)
// Log the time that it takes for a function or method.
// Only the MACRO log_time should be used.
// Using log_time() will use the TRACE log level and no additional message.
// Using log_time(level) will use the given log level and no additional message.
// Using log_time(level,msg) will use the given log level and an additional message.
# define log_time_0()              TIME_LEVEL_MACRO(log4cplus::TRACE_LOG_LEVEL, "", true)
# define log_time_1(level)         TIME_LEVEL_MACRO(log4cplus::level ## _LOG_LEVEL, "", true)
# define log_time_2(level,msg)     TIME_LEVEL_MACRO(log4cplus::level ## _LOG_LEVEL, msg, true)

// The following macro expansion is based on the example by jason-deng:
// https://github.com/jason-deng/C99FunctionOverload
# define log_time_FUNC_CHOOSER(_f1, _f2, _f3, ...) _f3
# define log_time_FUNC_RECOMPOSER(argsWithParentheses) log_time_FUNC_CHOOSER argsWithParentheses
# define log_time_CHOOSE_FROM_ARG_COUNT(...) log_time_FUNC_RECOMPOSER((__VA_ARGS__, log_time_2, log_time_1, ))
# define log_time_NO_ARG_EXPANDER() ,,log_time_0
# define log_time_CHOOSER(...) log_time_CHOOSE_FROM_ARG_COUNT(log_time_NO_ARG_EXPANDER __VA_ARGS__ ())

# define log_time(...)	log_time_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
#else
# define log_time(...)
#endif

// This macro is used if an additional message is needed with the default TRACE level.
// Basically the same as log_time(TRACE,msg)
#if defined(BUILD_WITH_LOG4CPLUS)
# define log_time_msg(msg)         TIME_LEVEL_MACRO(log4cplus::TRACE_LOG_LEVEL, msg, true)
#else
# define log_time_msg(msg)
#endif

#if defined(BUILD_WITH_LOG4CPLUS)
// Log function entry/exit without performing timing.
// Only the MACRO log_trace should be used.
// Using log_trace() will use the TRACE log level and no additional message.
// Using log_trace(level) will use the given log level and no additional message.
// Using log_trace(level,msg) will use the given log level and an additional message.
# define log_trace_0()             TIME_LEVEL_MACRO(log4cplus::TRACE_LOG_LEVEL, "", false)
# define log_trace_1(level)        TIME_LEVEL_MACRO(log4cplus::level ## _LOG_LEVEL, "", false)
# define log_trace_2(level,msg)    TIME_LEVEL_MACRO(log4cplus::level ## _LOG_LEVEL, msg, false)

// The following macro expansion is based on the example by jason-deng:
// https://github.com/jason-deng/C99FunctionOverload
# define log_trace_FUNC_CHOOSER(_f1, _f2, _f3, ...) _f3
# define log_trace_FUNC_RECOMPOSER(argsWithParentheses) log_trace_FUNC_CHOOSER argsWithParentheses
# define log_trace_CHOOSE_FROM_ARG_COUNT(...) log_trace_FUNC_RECOMPOSER((__VA_ARGS__, log_trace_2, log_trace_1, ))
# define log_trace_NO_ARG_EXPANDER() ,,log_trace_0
# define log_trace_CHOOSER(...) log_trace_CHOOSE_FROM_ARG_COUNT(log_trace_NO_ARG_EXPANDER __VA_ARGS__ ())

# define log_trace(...)	log_trace_CHOOSER(__VA_ARGS__)(__VA_ARGS__)
#else
# define log_trace(...)
#endif

// Log function entry/exit without performing timing and with an additional message but default TRACE level.
// Basically the same as log_trace(TRACE,msg)
#if defined(BUILD_WITH_LOG4CPLUS)
# define log_trace_msg(msg)        TIME_LEVEL_MACRO(log4cplus::TRACE_LOG_LEVEL, msg, false)
#else
# define log_trace_msg(msg)
#endif

// If the condition given evaluates false, this macro logs it using
// FATAL log level, including the condition's source text.
#if defined(BUILD_WITH_LOG4CPLUS)
# define log_assert(cond,expr)                                                      \
        LOG4CPLUS_SUPPRESS_DOWHILE_WARNING()                                        \
        do {                                                                        \
            if (LOG4CPLUS_UNLIKELY(! (cond)))                                       \
            {                                                                       \
                log_fatal( LOG4CPLUS_CALLER_FILE() <<                               \
                           ":" << LOG4CPLUS_CALLER_LINE() <<                        \
                           ":" << LOG4CPLUS_MACRO_FUNCTION() <<                     \
                           ": Assertion '" << LOG4CPLUS_ASSERT_STRINGIFY (cond) <<  \
                           "' failed - " << expr);                                  \
                assert(cond);                                                       \
            }                                                                       \
        } while (false)                                                             \
        LOG4CPLUS_RESTORE_DOWHILE_WARNING()
#else
# define log_assert(cond,expr)    assert(cond)
#endif

// Insert a log message similar to an assert but without actually performing the assert.
// This is used when you want to throw an exception instead of exiting.
#if defined(BUILD_WITH_LOG4CPLUS)
# define log_assert_message(cond,expr)                                              \
        LOG4CPLUS_SUPPRESS_DOWHILE_WARNING()                                        \
        do {                                                                        \
            log_error( LOG4CPLUS_CALLER_FILE() <<                                   \
                       ":" << LOG4CPLUS_CALLER_LINE() <<                            \
                       ":" << LOG4CPLUS_MACRO_FUNCTION() <<                         \
                       ": Assertion '" << LOG4CPLUS_ASSERT_STRINGIFY (cond) <<      \
                       "' failed - " << expr);                                      \
        } while (false)                                                             \
        LOG4CPLUS_RESTORE_DOWHILE_WARNING()
#else
# define log_assert_message(cond,expr)
#endif

#endif // ANYRPC_LOGGER_H

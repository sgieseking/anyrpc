# AnyRPC Logger

## Overview

AnyRPC uses Log4cplus v2.0 as its basis for logging in the system.
This can be separately enabled with the BUILD_WITH_LOG4CPLUS option.
The Log4cplus system is relatively low overhead so it is suggested
to include it unless you are having problems with building with the
Log4cplus library or need to get the last bit of performance.

## Logging Categories

AnyRPC provides slightly different logging calls from the standard
Log4cplus macros and are adapted from cxxtools.  For each class a
static member can be created to define a new logging category.
This logging category will be carried to derived classes unless a
new category is defined.

For example, the Reader class includes the following definition:

	log_define("AnyRPC.Reader");

The JsonReader class is derived from the Reader class but include
the following definition:

	log_define("AnyRPC.JsonReader");

So the JsonReader would output to a different category than the base
Reader class.  If this new log_define was not used, then it would
output to the same category.

## Logger Configuration File

The Log4cplus libraries requires a configuration file to indicate how
you want the output formatted, where you want it to go, and what
level of information you want for each category of information.
The main directory contains the file log4cplus.properties as a 
starter file that you can copy to your executable directory.

The first line indicates where you want to perform logging and the level.

	log4cplus.rootLogger = TRACE, STDOUT
	
This indicates all logging levels and the STDOUT configuration.  The STDOUT
configuration is defined later in the file to output to the terminal:

	log4cplus.appender.STDOUT = log4cplus::ConsoleAppender
	
This first line can be changed to send the log data to a file:

	log4cplus.rootLogger                   = TRACE, FILE
	log4cplus.appender.FILE                = log4cplus::RollingFileAppender
	log4cplus.appender.FILE.File           = example_output.log
	log4cplus.appender.FILE.MaxFileSize    = 1MB
	log4cplus.appender.FILE.MaxBackupIndex = 9
	
There is nothing special about the names STDOUT as the ConsoleAppender
and FILE for the RollingFileAppender.  They are just name that are
used as part of the later definition (log4cplus.appender.name).

The pattern for the log statement is defined by the pattern lines:

	log4cplus.appender.STDOUT.layout = log4cplus::PatternLayout
	log4cplus.appender.STDOUT.layout.ConversionPattern = %d{%m/%d/%y %H:%M:%S.%Q} [%5.5t] %-5p %-20.20c %4L:%-50.-50M - %m%n
	
The pattern layout can be told how the message should look like. Some of the available variables include:
    
    %n  The platform specific line separator character or characters
    %m  The actual log message
    %p  The log level of that message (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
    %i  The process ID of the process generating the log entry
    %h  The hostname of the system generating the log entry
    %H  The full qualified domain name of the system generating the log entry
    %l  The source code file and line number where this log event was generated
    %t  The name of the thread that generated the logging event
    %c  The category of the logging event
    %L  The line number from where the logging request was issued
    %M  The function name using __FUNCTION__ or similar macro
    %D{%m/%d/%y %H:%M:%S} The time when this log event was generated. %D is local time, %d GMT. 
        The available "time flags" are those from strftime() plus %q and %Q (milliseconds and fractional milliseconds).
        
See the Log4cplus documentation for further information on the pattern specifiers.

## Using the Logger Configuration File

The default configuration file sets all of the categories to the WARN level.
This will disable the majority of the log statements in the program.
If you are having problems with a particular area of the code, you can
comment out that line to give more detailed information.  
The following will enable all of the logging for the AnyRPC.JsonReader category.

	#log4cplus.logger.AnyRPC.JsonReader = WARN
	
## Writing to the Logger

The level for the logging are as follows: trace, debug, info, warn, error, fatal.
Log statements have the following formats:

	log_level( expr );
	log_level_if( cond, expr );
	log_trace();
	log_trace( level );
	log_trace( level, expr );
	log_time();
	log_time( level );
	log_time( level, expr );
	
The following are examples:

	log_warn( "Warning message" );
	log_debug( "Debug, my variable = " << myVar );
	log_trace( INFO, "MyFunction" );
	
The log_trace() calls will give an ENTER log output when the statement is reached and 
an EXIT log output when the object is destroyed.
The log_time() calls are similar to the log_trace() calls but also give the execution
time of the thread between the ENTER and EXIT.  

## Creating a new application

When writing a new application that uses the logging system, it does need to
be initialized.

	#include "anyrpc/anyrpc.h"
	
	log_define("main");

	int main()
	{
		// Start the logger up first so all other code can be logged
    	InitializeLogger();
    	
    	// Your application
    }

## Building with the Logger

AnyRPC is currently being tested with Log4cplus v1.2.x.  If you are building with
BUILD_WITH_THREADING on, then with Linux or mingw you may need to build Log4cplus 
with --std=c++11 included on CMAKE_CXX_FLAGS.

The cmake files will attempt to find the Log4cplus libraries but they may need to 
be manually specified with the build variable: LOG4CPLUS_INCLUDE_DIR, LOG4CPLUS_LIBRARY_DEBUG,
LOG4CPLUS_LIBRARY_RELEASE.
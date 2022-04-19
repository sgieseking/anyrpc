#
# Find the log4cplus includes and library
#
# This module defines
# LOG4CPLUS_INCLUDE_DIRS, where to find headers
# LOG4CPLUS_LIBRARIES, the libraries to link against to use LOG4CPLUS.
# LOG4CPLUS_FOUND, If false, do not try to use LOG4CPLUS.

# also defined, but not for general use are
# LOG4CPLUS_LIBRARY_RELEASE, where to find the LOG4CPLUS library in release mode.
# LOG4CPLUS_LIBRARY_DEBUG, where to find the LOG4CPLUS library in debug mode.


if (WIN32)
	# With a Windows build the debug and release libraries are different
	# The standard paths are also different from a linux system
	
	# Find the header directory
	find_path(LOG4CPLUS_INCLUDE_DIR log4cplus/logger.h
	  	HINTS ${CMAKE_SOURCE_DIR}/../log4cplus/include)
	
	# Find the release library 
  	find_library(LOG4CPLUS_LIBRARY_RELEASE log4cplus
               ${LOG4CPLUS_INCLUDE_DIR}/../build/src/Release
               /usr/local/lib
               /usr/lib)
               
    # Find the debug library
  	find_library(LOG4CPLUS_LIBRARY_DEBUG log4cplusD
               ${LOG4CPLUS_INCLUDE_DIR}/../build/src/Debug
               /usr/local/lib
               /usr/lib)

else ()
	# With a Linux build use PkgConfig to help find the correct parameters
	find_package(PkgConfig)
	pkg_check_modules(PC_LOG4CPLUS QUIET log4cplus)
	set(LOG4CPLUS_DEFINITIONS ${PC_LOG4CPLUS_CFLAGS_OTHER})
	
	# Find the header directory
	find_path(LOG4CPLUS_INCLUDE_DIR logger.h
	  HINTS ${PC_LOG4CPLUS_INCLUDE_DIR} ${PC_LOG4CPLUS_INCLUDE_DIRS}
	  PATH_SUFFIXES log4cplus)
	
	# Get the GCC compiler version
	exec_program(${CMAKE_CXX_COMPILER}
	            ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
	            OUTPUT_VARIABLE _gcc_COMPILER_VERSION
	            OUTPUT_STRIP_TRAILING_WHITESPACE )
	
	# Try to find a library that was compiled with the same compiler version as we currently use.
	find_library(LOG4CPLUS_LIBRARY_RELEASE
		NAMES libjson_linux-gcc-${_gcc_COMPILER_VERSION}_libmt.so liblog4cplus.so
		HINTS ${PC_LOG4CPLUS_LIBDIR} ${PC_LOG4CPLUS_LIBRARY_DIRS}
		PATHS /usr/lib /usr/local/lib)
	
	if (LOG4CPLUS_LIBRARY_RELEASE)
		# create the debug library path variable so that it can be set manually if needed
		set(LOG4CPLUS_LIBRARY_DEBUG ${LOG4CPLUS_LIBRARY_RELEASE} CACHE FILEPATH "")
	endif ()

endif ()

# needed to use find_package_handle_standard_args
include(FindPackageHandleStandardArgs)

if (LOG4CPLUS_INCLUDE_DIR)
	# set the correct variable name for the header directories         
	set(LOG4CPLUS_INCLUDE_DIRS ${LOG4CPLUS_INCLUDE_DIR})
	
    if (LOG4CPLUS_LIBRARY_RELEASE AND LOG4CPLUS_LIBRARY_DEBUG)
    	# set the libaries varible to use the release and debug versions
    	find_package_handle_standard_args(LOG4CPLUS DEFAULT_MSG LOG4CPLUS_INCLUDE_DIR LOG4CPLUS_LIBRARY_RELEASE LOG4CPLUS_LIBRARY_DEBUG)
		set(LOG4CPLUS_LIBRARIES optimized ${LOG4CPLUS_LIBRARY_RELEASE} debug ${LOG4CPLUS_LIBRARY_DEBUG})
		
	elseif (LOG4CPLUS_LIBRARY_RELEASE)
		# only the release library is available
		find_package_handle_standard_args(LOG4CPLUS DEFAULT_MSG LOG4CPLUS_INCLUDE_DIR LOG4CPLUS_LIBRARY_RELEASE)
		set(LOG4CPLUS_LIBRARIES optimized ${LOG4CPLUS_LIBRARY_RELEASE})
		
	elseif (LOG4CPLUS_LIBRARY_DEBUG)
		# only the debug library is available
		find_package_handle_standard_args(LOG4CPLUS DEFAULT_MSG LOG4CPLUS_INCLUDE_DIR LOG4CPLUS_LIBRARY_DEBUG)
		set(LOG4CPLUS_LIBRARIES debug ${LOG4CPLUS_LIBRARY_DEBUG})
		
    else ()
    	# neither library is available - give standard error message
		find_package_handle_standard_args(LOG4CPLUS DEFAULT_MSG LOG4CPLUS_INCLUDE_DIR LOG4CPLUS_LIBRARY_RELEASE LOG4CPLUS_LIBRARY_DEBUG)
		
    endif ()
else ()
	# Didn't find the headers - give a standard error message
	find_package_handle_standard_args(LOG4CPLUS DEFAULT_MSG LOG4CPLUS_INCLUDE_DIR)
endif ()

mark_as_advanced(LOG4CPLUS_INCLUDE_DIR LOG4CPLUS_LIBRARY_RELEASE LOG4CPLUS_LIBRARY_DEBUG)

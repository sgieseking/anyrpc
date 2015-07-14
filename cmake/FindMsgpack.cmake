#
# Find the msgpack-c includes and library
#
# This module defines
# MSGPACK_INCLUDE_DIRS, where to find headers
# MSGPACK_LIBRARIES, the libraries to link against to use MSGPACK.
# MSGPACK_FOUND, If false, do not try to use MSGPACK.

# also defined, but not for general use are
# MSGPACK_LIBRARY_RELEASE, where to find the MSGPACK library in release mode.
# MSGPACK_LIBRARY_DEBUG, where to find the MSGPACK library in debug mode.


if (WIN32)
	# With a Windows build the debug and release libraries are different
	# The standard paths are also different from a linux system
	
	# Find the header directory
	find_path(MSGPACK_INCLUDE_DIR msgpack.h
	  	HINTS ${CMAKE_SOURCE_DIR}/../msgpack-c/include)
	
	# Find the release library 
  	find_library(MSGPACK_LIBRARY_RELEASE msgpack
               ${MSGPACK_INCLUDE_DIR}/../build/Release
               /usr/local/lib
               /usr/lib)
               
    # Find the debug library
  	find_library(MSGPACK_LIBRARY_DEBUG msgpack
               ${MSGPACK_INCLUDE_DIR}/../build/Debug
               /usr/local/lib
               /usr/lib)

else ()
	# With a Linux build use PkgConfig to help find the correct parameters
	find_package(PkgConfig)
	pkg_check_modules(PC_MSGPACK QUIET msgpack)
	set(MSGPACK_DEFINITIONS ${PC_MSGPACK_CFLAGS_OTHER})
	
	# Find the header directory
	find_path(MSGPACK_INCLUDE_DIR msgpack.h
	  HINTS ${PC_MSGPACK_INCLUDE_DIR} ${PC_MSGPACK_INCLUDE_DIRS}
	  PATH_SUFFIXES msgpack)
	
	# Get the GCC compiler version
	exec_program(${CMAKE_CXX_COMPILER}
	            ARGS ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
	            OUTPUT_VARIABLE _gcc_COMPILER_VERSION
	            OUTPUT_STRIP_TRAILING_WHITESPACE )
	
	# Try to find a library that was compiled with the same compiler version as we currently use.
	find_library(MSGPACK_LIBRARY_RELEASE
		NAMES libjson_linux-gcc-${_gcc_COMPILER_VERSION}_libmt.so libmsgpack.so
		HINTS ${PC_MSGPACK_LIBDIR} ${PC_MSGPACK_LIBRARY_DIRS}
		PATHS /usr/lib /usr/local/lib)
	
	if (MSGPACK_LIBRARY_RELEASE)
		# create the debug library path variable so that it can be set manually if needed
		set(MSGPACK_LIBRARY_DEBUG ${MSGPACK_LIBRARY_RELEASE} CACHE FILEPATH "")
	endif ()

endif ()

if (MSGPACK_INCLUDE_DIR)
	# set the correct variable name for the header directories         
	set(MSGPACK_INCLUDE_DIRS ${MSGPACK_INCLUDE_DIR})

	# needed to use find_package_handle_standard_args
	include(FindPackageHandleStandardArgs)
	
    if (MSGPACK_LIBRARY_RELEASE AND MSGPACK_LIBRARY_DEBUG)
    	# set the libaries varible to use the release and debug versions
    	find_package_handle_standard_args(MSGPACK DEFAULT_MSG MSGPACK_INCLUDE_DIR MSGPACK_LIBRARY_RELEASE MSGPACK_LIBRARY_DEBUG)
		set(MSGPACK_LIBRARIES optimized ${MSGPACK_LIBRARY_RELEASE} debug ${MSGPACK_LIBRARY_DEBUG})
		
	elseif (MSGPACK_LIBRARY_RELEASE)
		# only the release library is available
		find_package_handle_standard_args(MSGPACK DEFAULT_MSG MSGPACK_INCLUDE_DIR MSGPACK_LIBRARY_RELEASE)
		set(MSGPACK_LIBRARIES optimized ${MSGPACK_LIBRARY_RELEASE})
		
	elseif (MSGPACK_LIBRARY_DEBUG)
		# only the debug library is available
		find_package_handle_standard_args(MSGPACK DEFAULT_MSG MSGPACK_INCLUDE_DIR MSGPACK_LIBRARY_DEBUG)
		set(MSGPACK_LIBRARIES debug ${MSGPACK_LIBRARY_DEBUG})
		
    else ()
    	# neither library is available - give standard error message
		find_package_handle_standard_args(MSGPACK DEFAULT_MSG MSGPACK_INCLUDE_DIR MSGPACK_LIBRARY_RELEASE MSGPACK_LIBRARY_DEBUG)
		
    endif ()
else ()
	# Didn't find the headers - give a standard error message
	find_package_handle_standard_args(MSGPACK DEFAULT_MSG MSGPACK_INCLUDE_DIR)
endif ()

mark_as_advanced(MSGPACK_INCLUDE_DIR MSGPACK_LIBRARY_RELEASE MSGPACK_LIBRARY_DEBUG)

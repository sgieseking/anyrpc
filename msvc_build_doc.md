# Building AnyRPC with Microsoft Visual C++

## Compatibility

AnyRPC has been tested for compatibility with Microsoft Visual Studio 2015RC, 2013, 2012, and 2010.
With 2010 the thread library is not available so BUILD_WITH_THREADING must be off.
For best compatibility, compile all of the libraries with the same version of
Visual Studio as you are using for AnyRPC.

## Using cmake

Cmake needs to find various support libraries to build AnyRPC.
For MSVC, it will look in the same base directory as you are using for
the AnyRPC project. So if AnyRPC is in directory c:\MyProject\anyrpc,
then it will look for log4cplus in directory c:\MyProject\log4cplus.

If cmake cannot automatically find the other libraries, they can be
manually specified to cmake either as command line options or through
cmake-gui.

The following are the names for the external library references:

	GTEST_INCLUDE_DIRS
	GTEST_LIBRARIES
	
	LOG4CPLUS_INCLUDE_DIR
	LOG4CPLUS_LIBRARY_DEBUG
	LOG4CPLUS_LIBRARY_RELEASE
	
	MSGPACK_INCLUDE_DIR
	MSGPACK_LIBRARY_DEBUG
	MSGPACK_LIBRARY_RELEASE

It is suggested to create a build directory in the anyrpc directory to
use for the cmake files, c:\MyProject\anyrpc\build.  This will keep the
build files separate from the AnyRPC source files.

The cmake files will default to building the AnyRPC library as a shared
library (ANYRPC_LIB_BUILD_SHARED) with threading enabled (BUILD_WITH_THREADING), 
wchar enabled (BUILD_WITH_WCHAR), and logging enabled (BUILD_WITH_LOG4CPLUS).
The examples will be build by default (BUILD_EXAMPLES) but not the unit tests (BUILD_TESTS).

The Visual Studio projects and solution will be placed in the build directory.
Standard cmake generated configurations will be available: Release, Debug, MinSizeRel, and RelWithDebInfo.
The compiled files will be placed in directory build\bin\<BuildType>.
You will need to make the dll files from the other libraries available when
running a file.  It may be easiest to copy these DLLs to the build directory.
Alternately, you can modify the path so that they can be located. 

## Compiling the Unit Tests

The unit test (BUILD_TESTS) require the google test libraries.
It may be necessary to manually change the Runtime library used with
gtest to match that used by AnyRPC.  This can be performed by changing
project properties > C/C++ > Code Generation > Runtime Library.
It may need to be changed to Multi-threaded DLL.

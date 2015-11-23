
# AnyRPC

A multiprotocol remote procedure call system for C++

## Overview
AnyRPC provides a common system to work with a number of different remote procedure call standards. 
This is handled transparently by the server.  

Currently supported RPC standards include:
* JsonRpc
* XmlRpc
* MessagePackRpc

HTTP servers can support multiple protocols based on the content-type field of the header.

TCP servers use netstrings protocol for lower overhead connections but are limited to a single protocol.

Threaded servers are available using optional compilation with c++11 thread support.

Available server types:
* Without threading, call to run for a given amount of time.  Useful for your own threading (i.e. no c++11 thread support).
* Single threaded server.  All message processing is serialized.
* Multi-threaded server.  Separate thread for each client, but higher memory requirements.
* Thread-pool server.  Single thread to wait for messages, but execution given to a set of worker threads.  Higher server overhead for each message, but limited number of threads to service a larger number of connections.

The interface to Values can use wchar_t strings although the internal system uses UTF-8 format.

Logging is optionally provided using Log4cplus.

Value types that are not directly supported by the message format are converted to specially tagged arrays that can be automatically converted to the internal type.
For example Json does not support binary data so this is converted to a two element array, ["AnyRpcBase64", Base64 encoded string].
This conversion can be performed by both the server and the client.

## Compatibility
AnyRPC provides cross-platform support with primary targets for Linux, Visual Studio, and MinGW.  Some platform/compiler combinations which have been tested are shown as follows:

* Ubuntu 14.04/gcc v4.8.4
* Windows 8 with Visual Studio 2015, 2013, 2012, 2010
* Windows 8 with MinGW/gcc v4.8.1

## License
AnyRPC is available under [The MIT license](http://opensource.org/licenses/MIT).  

Copyright (C) 2015 SRG Technology, LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.


## Installation
AnyRPC uses the following software as its dependencies:
* [CMake](http://www.cmake.org) as a general build tool
* (optional) [Log4cplus](https://github.com/log4cplus/log4cplus) for system logging
* (optional) [Doxygen](http://www.doxygen.org) to build documentation
* (optional) [googletest](https://code.google.com/p/googletest/) for unit testing

### Building with CMake
If you are new to using CMake as a build tool, you may want to use cmake-gui instead of the command line version cmake.

### Useful build options

|Parameter | Description |
|----------|-------------|
|BUILD_EXAMPLES |Build the examples from the examples directory.|
|BUILD_TEST |Build the unit tests in the test directory.  This requires [Google Test](https://code.google.com/p/googletest/) to be installed. |
|BUILD_WITH_WCHAR |Build the Value class with the functions for wchar_t/wstring access. |
|BUILD_WITH_LOG4CPLUS |Build with the logging system available.  This requires [Log4cplus](https://github.com/log4cplus/log4cplus) to be installed. |
|BUILD_WITH_THREADING |Build the threaded servers.  This requires a c++11 compiler with thread support.  MinGW thread libraries are provided from project [mingw-std-threads](https://github.com/meganz/mingw-std-threads).  |
|BUILD_WITH_ADDRESS_SANATIZER |Build with address sanatizer enabled.  Only avaiable with gcc builds (Linux, MinGW).  Address sanatizer will detect certain heap access problems but slows the execution of the program. | 

### Building on Linux

	$ git clone https://github.com/sgieseking/anyrpc.git
	$ cd anyrpc
	$ mkdir build
	$ cd build
	$ cmake-gui ..
	$ make
	
	The executables will be in the bin directory.
	To use the AnyRPC library with your application, you will want to install the files to the standard Linux folders.
	
	$ sudo make install
	
### Building on Windows with Visual Studio

	$ git clone https://github.com/sgieseking/anyrpc.git
	$ cd anyrpc
	$ mkdir build
	$ cd build
	$ cmake-gui ..
	
	Open the generated solution in Visual Studio.
	Build the solution.
	The executables will be in the bin/Release or bin/Debug directories.
	To use the AnyRPC library with your application, they can be either included from these directories or copied to a local application directory.
	
### Building on Windows in MinGW with MSys

	$ git clone https://github.com/sgieseking/anyrpc.git
	$ cd anyrpc
	$ mkdir build
	$ cd build
	$ cmake-gui ..
	$ make
	
	The executables will be in the bin directory.
    To use the AnyRPC library with your application, they can be either included from these directories or copied to a local application directory.

## Reasons for Development

### Why development another RPC library?

I was looking for an RPC library that could be used on both Windows and Linux.
Most of the existing libraries are Linux based and have dependencies that looked difficult to port to Windows.
Adding support for multiple RPC standards to an existing library that is not designed for this also looked difficult.

### Why include multiple RPC standards instead of just one?

I was mostly looking for the ability to support both Json and a binary protocol.
A binary protocol should be more efficient for transport and processing for custom clients.
Json is easier for either manual testing or with certain platforms that may not be as open.
Closed platforms may already support an existing protocol such as xmlrpc.
Python has a built-in library for xmlrpc but requires external libraries for JsonRpc.
	
## Inspiration

The following projects influenced the development of AnyRPC.

|Project | Code/Classes Influenced |
|--------|-------------|
|[RapidJson] (https://github.com/miloyip/rapidjson)|The structure of the Value class and the use of Handlers and Streams is modeled on RapidJson.|
|[XmlRpc++] (http://xmlrpcpp.sourceforge.net/)|Some aspects of the Server and Connection classes.|
|[cxxtools] (http://www.tntnet.org/cxxtools.html)|Some aspects of the logger interface built on top of log4cplus. cxxtools supports xmlrpc, jsonrpc, and binary but is only supported on Linux.|

The following projects are also of interest:
* [Xml-Rpc.com] (http://xmlrpc.scripting.com/default.html)
* [Json-Rpc] (http://www.jsonrpc.org/)
* [JsonRpc-Cpp] (http://jsonrpc-cpp.sourceforge.net/)
* [libjson-rpc-cpp] (https://github.com/cinemast/libjson-rpc-cpp)
* [jsonrpc-ns] (https://github.com/flowroute/jsonrpc-ns)
* [MessagePack] (http://msgpack.org/)
* [msgpack-c] (https://github.com/msgpack/msgpack-c)
* [msgpack-rpc] (https://github.com/msgpack-rpc/msgpack-rpc)

## Areas for Development

* Support other message encoding formats: CBOR, BSON, UBJSON, Smile.
* Support JsonRpc v1.0 for duplex connections. 
* User smart pointers (c++11 shared_ptr<>) to share strings and binary data between the application and the Value class.
* Implement extensions from MessagePack as a Value type.


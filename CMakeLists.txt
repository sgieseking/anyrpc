cmake_minimum_required(VERSION 2.8)

Project(AnyRPC CXX)

# Some of the cmake find_package files are part of this distribution
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Read out version from "version" file
file(STRINGS "version" ANYRPC_VERSION_FILE)

#parse the version information into pieces.
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" ANYRPC_VERSION_MAJOR "${ANYRPC_VERSION_FILE}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" ANYRPC_VERSION_MINOR "${ANYRPC_VERSION_FILE}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" ANYRPC_VERSION_PATCH "${ANYRPC_VERSION_FILE}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+ +(.*)" "\\1" ANYRPC_VERSION_QUALIFIER "${ANYRPC_VERSION_FILE}")

set(ANYRPC_VERSION ${ANYRPC_VERSION_MAJOR}.${ANYRPC_VERSION_MINOR}.${ANYRPC_VERSION_PATCH})

message(STATUS "AnyRPC Version: ${ANYRPC_VERSION_MAJOR}.${ANYRPC_VERSION_MINOR}.${ANYRPC_VERSION_PATCH} (${ANYRPC_VERSION_QUALIFIER})")
set(ANYRPC_VERSION_STRING "${ANYRPC_VERSION_MAJOR}.${ANYRPC_VERSION_MINOR}.${ANYRPC_VERSION_PATCH} (${ANYRPC_VERSION_QUALIFIER})" )

# compile in release with debug info mode by default
set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build Type")

# Build all binaries in a separate directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(BUILD_EXAMPLES "Build AnyRPC examples." ON)
option(BUILD_TESTS "Build AnyRPC unit tests." OFF)
option(BUILD_WITH_ADDRESS_SANITIZE "Build address sanitizer." OFF)
option(BUILD_WITH_LOG4CPLUS "Build log4cplus." ON)
option(BUILD_WITH_THREADING "Build with threading." ON)
option(BUILD_WITH_REGEX "Build with regular expression." ON)
option(BUILD_WITH_WCHAR "Build with wide character interface for Value." ON)

option(BUILD_PROTOCOL_JSON "Build with Json protocol included." ON)
option(BUILD_PROTOCOL_XML "Build with Xml procotol included." ON)
option(BUILD_PROTOCOL_MESSAGEPACK "Build with MessgePack protocol included." ON)

set(ANYRPC_ASSERT "throw" CACHE STRING "action to take on failed assertion")
set_property(CACHE ANYRPC_ASSERT PROPERTY STRINGS assert throw no_action)

set(ANYRPC_THROW "throw" CACHE STRING "action to take on throw call")
set_property(CACHE ANYRPC_THROW PROPERTY STRINGS assert throw)

#add extra search paths for libraries and includes
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/include" CACHE PATH "The directory the headers are installed in")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE STRING "Directory where lib will install")

include_directories(${CMAKE_SOURCE_DIR}/include)

set(ANYRPC_INCLUDE_JSON ${BUILD_PROTOCOL_JSON})
set(ANYRPC_INCLUDE_XML ${BUILD_PROTOCOL_XML})
set(ANYRPC_INCLUDE_MESSAGEPACK ${BUILD_PROTOCOL_MESSAGEPACK})

set(ANYRPC_THREADING ${BUILD_WITH_THREADING})
set(ANYRPC_REGEX ${BUILD_WITH_REGEX})
set(ANYRPC_WCHAR ${BUILD_WITH_WCHAR})

if (MSVC)
    add_definitions( -D _CRT_SECURE_NO_WARNINGS )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc" )
elseif (MINGW)
    SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -U__STRICT_ANSI__" )
else (BUILD_WITH_ADDRESS_SANITIZE)
    SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer" )
    SET( ASAN_LIBRARY asan )
endif ()

if (NOT MSVC)
    SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11" )
endif ()

if (ANYRPC_ASSERT STREQUAL "assert")
    set(ANYRPC_ASSERT_LEVEL 2)
elseif (ANYRPC_ASSERT STREQUAL "throw")
    set(ANYRPC_ASSERT_LEVEL 1)
else ()
    set(ANYRPC_ASSERT_LEVEL 0)
endif ()

if (ANYRPC_THROW STREQUAL "assert")
    set(ANYRPC_THROW_LEVEL 2)
else ()
    set(ANYRPC_THROW_LEVEL 1)
endif ()

# File version.h is only regenerated on CMake configure step
CONFIGURE_FILE( "${PROJECT_SOURCE_DIR}/version.h.in"
                "${PROJECT_SOURCE_DIR}/include/anyrpc/version.h" )

if (BUILD_WITH_LOG4CPLUS)
    find_package( Log4cplus )
    if (NOT LOG4CPLUS_FOUND)
        # the find_package call for Log4cplus doesn't generate an error even if marked as required
        message( FATAL_ERROR "LOG4CPLUS library required if BUILD_WITH_LOG4CPLUS on" )
    endif ()
endif ()

add_subdirectory(src)

if (BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

if (BUILD_TESTS)
    find_package( GTest REQUIRED )
    add_subdirectory(test)
endif()

install(DIRECTORY include/anyrpc
    DESTINATION "${INCLUDE_INSTALL_DIR}"
    COMPONENT dev)

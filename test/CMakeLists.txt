#
# Build the unit tests
if (ANYRPC_LIB_BUILD_SHARED)
    # On WIN32, a shared library require proper dllexport/dllimport declarations.
    # With MinGW the default is to export all, but with Visual Studio the default is export none.
    # The header files will do this correctly with the following defines
    add_definitions( -DANYRPC_DLL )
endif ()

include_directories(${GTEST_INCLUDE_DIR})

# The set of source files for the test application, without the .cpp
set(ANYRPC_CPP_TESTS
    testMain.cpp 
    testValue.cpp
    testStream.cpp
    testMethodMap.cpp
    testHttpHeader.cpp
    testServer.cpp
)

if (BUILD_PROTOCOL_JSON)
    set(ANYRPC_CPP_TESTS ${ANYRPC_CPP_TESTS} testJson.cpp)
endif ()

if (BUILD_PROTOCOL_XML)
    set(ANYRPC_CPP_TESTS ${ANYRPC_CPP_TESTS} testXml.cpp)
endif ()

if (BUILD_PROTOCOL_MESSAGEPACK)
    set(ANYRPC_CPP_TESTS ${ANYRPC_CPP_TESTS} testMessagePack.cpp)
endif ()
    
# Add the necessary external library references
if (BUILD_WITH_LOG4CPLUS)
    include_directories(${LOG4CPLUS_INCLUDE_DIRS})
    add_definitions( -DBUILD_WITH_LOG4CPLUS )
else ()
    set( LOG4CPLUS_LIBRARIES "" )
endif ()

# add Visual Studio 2012/2010 definition needed for gtest
if (MSVC)
    add_definitions( -D_VARIADIC_MAX=10 )
endif ()

# Create the executable with the extra files added
add_executable( testAnyRPC ${ANYRPC_CPP_TESTS} )

# Add the necessary external library references
target_link_libraries( testAnyRPC anyrpc ${ASAN_LIBRARY} ${LOG4CPLUS_LIBRARIES} ${GTEST_LIBRARIES} )

# Copy the sample files to the build directory in folder named sample
add_custom_command(TARGET testAnyRPC POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/test/sample $<TARGET_FILE_DIR:testAnyRPC>/sample)

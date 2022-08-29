#
# Build the example applications
#
if (ANYRPC_LIB_BUILD_SHARED)
    # On WIN32, a shared library require proper dllexport/dllimport declarations.
    # With MinGW the default is to export all, but with Visual Studio the default is export none.
    # The header files will do this correctly with the following defines
    add_definitions( -DANYRPC_DLL )
endif ()

# The set of source files for the test application, without the .cpp
set(EXAMPLE_SOURCES
    exampleServer
    exampleClient
)

if (BUILD_PROTOCOL_JSON)
    set(EXAMPLE_SOURCES ${EXAMPLE_SOURCES} exampleJson)
endif ()

if (BUILD_PROTOCOL_XML)
    set(EXAMPLE_SOURCES ${EXAMPLE_SOURCES} exampleXml)
endif ()
	
if (BUILD_PROTOCOL_MESSAGEPACK)
    set(EXAMPLE_SOURCES ${EXAMPLE_SOURCES} exampleMessagePack)
    
    # The message pack examples can be checked using the external library if available
    find_package( Msgpack )
    if (MSGPACK_FOUND)
        include_directories(${MSGPACK_INCLUDE_DIRS})
        add_definitions( -DBUILD_WITH_MSGPACK_LIBRARY )
    else ()
    	message( STATUS "The external Msgpack library provides checking of results but is not required" )
    endif ()
endif ()
    
# Add the necessary external library references
if (BUILD_WITH_LOG4CPLUS)
    include_directories(${LOG4CPLUS_INCLUDE_DIRS})
    add_definitions( -DBUILD_WITH_LOG4CPLUS )
endif ()

# loop through the source files and build each program
foreach( SOURCEFILE ${EXAMPLE_SOURCES} )
    # Create the executable with the extra files added
    add_executable( ${SOURCEFILE} ${SOURCEFILE}.cpp )

    # Add the necessary external library references
    target_link_libraries( ${SOURCEFILE} anyrpc ${ASAN_LIBRARY} ${LOG4CPLUS_LIBRARIES} ${MSGPACK_LIBRARIES})
    
    if (WIN32)
        target_compile_definitions(${SOURCEFILE}
            PRIVATE
                WINVER=0x0A00
                _WIN32_WINNT=0x0A00
        )
    endif ()
endforeach ()


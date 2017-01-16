#
# Build the anyrpc library
#
option(ANYRPC_LIB_BUILD_SHARED "Build anyrpc as a shared library." ON)

# Set whether to build as shared or static library
if (ANYRPC_LIB_BUILD_SHARED)
    set( ANYRPC_LIB_TYPE SHARED )

    # On WIN32, a shared library require proper dllexport/dllimport declarations.
    # With MinGW the default is to export all, but with Visual Studio the default is export none.
    # The header files will do this correctly with the following defines
    add_definitions( -DANYRPC_DLL_BUILD )
else ()
    set( ANYRPC_LIB_TYPE STATIC )
endif ()

# Get all of the header and source files in the appropriate directories
file(GLOB ANYRPC_SOURCES *.cpp)
file(GLOB ANYRPC_INTERNAL_SOURCES internal/*.cpp)
file(GLOB ANYRPC_HEADERS ${CMAKE_SOURCE_DIR}/include/*.h)
file(GLOB ANYRPC_INTERNAL_HEADERS ${CMAKE_SOURCE_DIR}/include/internal/*.h)

if (BUILD_PROTOCOL_JSON)
    file(GLOB ANYRPC_JSON_SOURCES json/*.cpp)
    file(GLOB ANYRPC_JSON_HEADERS ${CMAKE_SOURCE_DIR}/include/json/*.h)
endif ()

if (BUILD_PROTOCOL_XML)
    file(GLOB ANYRPC_XML_SOURCES xml/*.cpp)
    file(GLOB ANYRPC_XML_HEADERS ${CMAKE_SOURCE_DIR}/include/xml/*.h)
endif ()

if (BUILD_PROTOCOL_MESSAGEPACK)
    file(GLOB ANYRPC_MESSAGEPACK_SOURCES messagepack/*.cpp)
    file(GLOB ANYRPC_MESSAGEPACK_HEADERS ${CMAKE_SOURCE_DIR}/include/messagepack/*.h)
endif ()

set(ANYRPC_SOURCES ${ANYRPC_SOURCES} ${ANYRPC_INTERNAL_SOURCES} 
    ${ANYRPC_JSON_SOURCES} ${ANYRPC_XML_SOURCES} ${ANYRPC_MESSAGEPACK_SOURCES})

set(ANYRPC_HEADERS ${ANYRPC_HEADERS} ${ANYRPC_INTERNAL_HEADERS} 
    ${ANYRPC_JSON_HEADERS} ${ANYRPC_XML_HEADERS} ${ANYRPC_MESSAGEPACK_HEADERS})

# Add the necessary external library references
if (BUILD_WITH_LOG4CPLUS)
    include_directories(${LOG4CPLUS_INCLUDE_DIRS})
    add_definitions( -DBUILD_WITH_LOG4CPLUS )
else ()
    set( LOG4CPLUS_LIBRARIES "" )
endif ()

# Create the libraries with these header and source files
add_library( anyrpc ${ANYRPC_LIB_TYPE} ${ANYRPC_SOURCES} ${ANYRPC_HEADERS} )
target_link_libraries( anyrpc ${ASAN_LIBRARY} ${LOG4CPLUS_LIBRARIES})

# Need the winsock library for Windows
if (WIN32)
    target_link_libraries(anyrpc ws2_32)
endif ()

set_target_properties( anyrpc PROPERTIES VERSION ${ANYRPC_VERSION} SOVERSION ${ANYRPC_VERSION_MAJOR} )

# Install instructions for this target
install( TARGETS anyrpc
         RUNTIME DESTINATION bin
         LIBRARY DESTINATION lib
         ARCHIVE DESTINATION lib)

# create MSVC directory structure for all files
set(ANYRPC_FILES ${ANYRPC_HEADERS} ${ANYRPC_CPP_SOURCES})

foreach(FILE ${ANYRPC_FILES}) 
    # get relative path without the file name
    file(RELATIVE_PATH RFILE ${CMAKE_SOURCE_DIR} ${FILE})
    get_filename_component(RFILE ${RFILE} PATH)

    # changes /'s to \\'s
  	string(REPLACE "/" "\\" RFILE "${RFILE}")

    # add to source group under Library Files
    source_group("Library Files\\${RFILE}" FILES "${FILE}")
endforeach()

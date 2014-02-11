SET(OLD_BUILD_TYPE ${CMAKE_BUILD_TYPE})
SET(CMAKE_BUILD_TYPE Release)

#cpp-netlib
add_subdirectory(cpp-netlib)
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/cpp-netlib/http/src/)
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/cpp-netlib/uri/src/)
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/cpp-netlib/message/src/)

SET(CMAKE_BUILD_TYPE ${OLD_BUILD_TYPE})

#asio
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include)

FILE(GLOB_RECURSE asio_headers ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/include/*.hpp)

add_library(asio_headers_lib STATIC ${asio_headers})
set_target_properties(asio_headers_lib PROPERTIES LINKER_LANGUAGE CXX)

#rapidjson
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include)

FILE(GLOB_RECURSE rapidjson_headers ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include/*.h)

add_library(rapidjson_headers_lib STATIC ${rapidjson_headers})
set_target_properties(rapidjson_headers_lib PROPERTIES LINKER_LANGUAGE CXX)

#ejdb
include(ExternalProject)
ExternalProject_Add(ejdb
PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/ejdb
SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ejdb/tcejdb
CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} <SOURCE_DIR>/configure --prefix=${CMAKE_SOURCE_DIR} --libdir=${LIBRARY_OUTPUT_PATH}
BUILD_COMMAND ${MAKE}
BUILD_IN_SOURCE 1
INSTALL_COMMAND ${MAKE_INSTALL}
#TEST_COMMAND make check
)

set(ENV{PKG_CONFIG_PATH} "${LIBRARY_OUTPUT_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")

#ejpp
add_subdirectory(ejpp)
add_dependencies(ejpp ejdb)
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/ejpp/include ${CMAKE_CURRENT_SOURCE_DIR}/ejpp/jbson/include)

set(EXTERN_INCLUDES ${EXTERN_INCLUDES} PARENT_SCOPE)

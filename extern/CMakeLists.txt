SET(CMAKE_BUILD_TYPE Release)
add_subdirectory(cpp-netlib)
add_subdirectory(gmock)

set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include)

FILE(GLOB_RECURSE rapidjson_headers ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include/*.h)

add_library(rapidjson_headers_lib STATIC ${rapidjson_headers})
set_target_properties(rapidjson_headers_lib PROPERTIES LINKER_LANGUAGE CXX)

set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/cpp-netlib/http/src/)
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/cpp-netlib/uri/src/)
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/cpp-netlib/message/src/)

include(ExternalProject)
ExternalProject_Add(ejdb
PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/ejdb
SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ejdb/tcejdb
CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
BUILD_COMMAND ${MAKE}
BUILD_IN_SOURCE 1
)

file(GLOB_RECURSE EJDB_PKG_CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/ejdb/lib*/*tcejdb.pc)
GET_FILENAME_COMPONENT(EJDB_PKG_CONFIG_PATH ${EJDB_PKG_CONFIG_FILE} PATH)

set(ENV{PKG_CONFIG_PATH} "${EJDB_PKG_CONFIG_PATH}:$ENV{PKG_CONFIG_PATH}")

add_subdirectory(ejpp)
add_dependencies(ejpp ejdb)
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/ejpp/include)

set(EXTERN_INCLUDES ${EXTERN_INCLUDES} PARENT_SCOPE)

SET(CMAKE_BUILD_TYPE Release)
add_subdirectory(cpp-netlib)
add_subdirectory(gmock)

set(EXTERN_INCLUDES ${EXTERN_INCLUDES})

set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include)

FILE(GLOB_RECURSE rapidjson_headers ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include/*.h)

add_library(rapidjson_headers_lib STATIC ${rapidjson_headers})
set_target_properties(rapidjson_headers_lib PROPERTIES LINKER_LANGUAGE CXX)

set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/cpp-netlib/http/src/)
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/cpp-netlib/uri/src/)
set(EXTERN_INCLUDES ${EXTERN_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/cpp-netlib/message/src/)

set(EXTERN_INCLUDES ${EXTERN_INCLUDES} PARENT_SCOPE)

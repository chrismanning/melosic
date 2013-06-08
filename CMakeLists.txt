project(melosic CXX C)
cmake_minimum_required(VERSION 2.8)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

add_definitions(-DBOOST_ALL_DYN_LINK)

add_subdirectory(extern)
include_directories(extern/cpp-netlib/http/src/)
include_directories(extern/cpp-netlib/uri/src/)
include_directories(extern/cpp-netlib/message/src/)
include_directories(extern/opqit/include)
include_directories(extern/kcategorizedview)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fPIC -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++1y -pthread")
ADD_DEFINITIONS(-D__GXX_EXPERIMENTAL_CXX0X__=1)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O1 -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -O1 -fsanitize=address")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Os")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Os")

ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

add_subdirectory(src/melosic)

project(melosic CXX D C)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_D_FLAGS "${CMAKE_D_FLAGS} $ENV{DFLAGS} -debug -g -w -property")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++0x")

add_subdirectory(src/melosic)


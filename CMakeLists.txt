project(melosic CXX C)
cmake_minimum_required(VERSION 2.8.11)

option(MELOSIC_SANITIZE_ADDRESS "Use -fsanitize=address where available" ON)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
if(WIN32)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
endif()

add_definitions(-DBOOST_ALL_DYN_LINK)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-warning-option -Wno-unused-private-field -Wno-mismatched-tags")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

add_subdirectory(extern)
include_directories(${EXTERN_INCLUDES})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fno-omit-frame-pointer -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-local-typedefs -Wno-missing-field-initializers -Wno-attributes -Wno-unused-parameter")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
if(NOT WIN32)
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++1y")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O1")

if(MELOSIC_SANITIZE_ADDRESS AND NOT WIN32)
 message(STATUS "MELOSIC_SANITIZE_ADDRESS")
 set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS}")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Os")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Os")

ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(Boost COMPONENTS filesystem iostreams system log log_setup thread locale REQUIRED)

add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED=0)

find_package(Taglib REQUIRED)
SET(TAGLIB_MIN_VERSION "1.8")

include_directories(src ${Boost_INCLUDE_DIRS} ${TAGLIB_INCLUDES})
SET(LINK_LIBS ${LINK_LIBS} ${TAGLIB_LIBRARIES} ${Boost_LIBRARIES})

SET(PN ${PROJECT_NAME})
#test stuff
enable_testing()
function(cxx_test test_name)
  add_executable(${test_name} ${test_name}.cpp)
  target_link_libraries(${test_name} gtest ${PN}_test_main)
  set_target_properties(${test_name} PROPERTIES COMPILE_DEFINITIONS "MELOSIC_DISABLE_LOGGING=1")
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${test_name})
endfunction()

include_directories(extern/gtest/include)
include_directories(extern/gmock/include)

add_subdirectory(src/melosic)

add_subdirectory(test)

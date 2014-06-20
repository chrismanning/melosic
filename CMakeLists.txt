project(melosic CXX C)
cmake_minimum_required(VERSION 2.8.11)

option(MELOSIC_ENABLE_TESTING "Enable testing" ON)
option(MELOSIC_SANITIZE_ADDRESS "Use -fsanitize=address where available" OFF)
option(MELOSIC_DETECT_LEAKS "Use -fsanitize=leak where available" OFF)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
if(WIN32)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
endif()

add_definitions(-DBOOST_ALL_DYN_LINK)
add_definitions(-DBOOST_THREAD_PROVIDES_EXECUTORS -DBOOST_THREAD_VERSION=4 -DBOOST_THREAD_USES_DATETIME)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-warning-option -Wno-unused-private-field -Wno-mismatched-tags")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(Boost COMPONENTS system thread locale filesystem log log_setup chrono date_time REQUIRED)
link_directories(${Boost_LIBRARY_DIR})
include_directories(${Boost_INCLUDE_DIR})

find_package(Asio 1.10.1 REQUIRED)

include_directories(include)
add_subdirectory(extern)
include_directories(${EXTERN_INCLUDES})
link_directories(${CMAKE_BINARY_DIR}/extern/lib)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fno-omit-frame-pointer -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-local-typedefs -Wno-missing-field-initializers")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes -Wno-unused-parameter -Wno-deprecated-register")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-redeclared-class-member -Wno-ignored-qualifiers")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
if(NOT WIN32)
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++1y")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O1")

if(MELOSIC_SANITIZE_ADDRESS AND NOT WIN32)
 message(STATUS "Address sanitizer enabled")
 set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address")
endif()

if(MELOSIC_DETECT_LEAKS AND NOT WIN32)
 message(STATUS "Leak detection enabled")
 set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=leak")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CMAKE_CXX_FLAGS}")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O3")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")

ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED=0)

find_package(Taglib REQUIRED)
SET(TAGLIB_MIN_VERSION "1.8")

include_directories(src ${Boost_INCLUDE_DIRS} ${TAGLIB_INCLUDES})
SET(LINK_LIBS ${LINK_LIBS} ${TAGLIB_LIBRARIES} ${Boost_LIBRARIES} z)
SET(LINK_LIBS ${LINK_LIBS} -ltcejdb -lejpp -lnetwork-uri)

if(UNIX)
 SET(LINK_LIBS ${LINK_LIBS} dl magic)
endif()

#test stuff

#gets all cached vars
get_cmake_property(CACHE_VARS CACHE_VARIABLES)
foreach(CACHE_VAR ${CACHE_VARS})
  get_property(CACHE_VAR_HELPSTRING CACHE ${CACHE_VAR} PROPERTY HELPSTRING)
  if(CACHE_VAR_HELPSTRING STREQUAL "No help, variable specified on the command line.")
    get_property(CACHE_VAR_TYPE CACHE ${CACHE_VAR} PROPERTY TYPE)
    if(CACHE_VAR_TYPE STREQUAL "UNINITIALIZED")
      set(CACHE_VAR_TYPE)
    else()
      set(CACHE_VAR_TYPE :${CACHE_VAR_TYPE})
    endif()
    set(PARENT_CMAKE_ARGS ${PARENT_CMAKE_ARGS} "-D${CACHE_VAR}${CACHE_VAR_TYPE}=${${CACHE_VAR}}")
  endif()
endforeach()

include(ExternalProject)

ExternalProject_Add(
    catch_${PROJECT_NAME}
    GIT_REPOSITORY https://github.com/philsquared/Catch.git
#    GIT_TAG origin/master
    TIMEOUT 10
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
   )
ExternalProject_Get_Property(catch_${PROJECT_NAME} source_dir)
set(CATCH_INCLUDE_DIR ${source_dir}/include CACHE INTERNAL "Path to include folder for Catch")

if(${MELOSIC_ENABLE_TESTING})
enable_testing()
endif()

function(cxx_test test_name)
  add_executable(${test_name} ${test_name}.cpp $<TARGET_OBJECTS:${PROJECT_NAME}_test_main>)
  add_dependencies(${test_name} catch_${PROJECT_NAME} jbson ejpp cpp-netlib)
  target_link_libraries(${test_name} ${LINK_LIBS} ${PROJECT_NAME}lib)
  set_property(TARGET ${test_name} APPEND PROPERTY INCLUDE_DIRECTORIES ${CATCH_INCLUDE_DIR})
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${test_name})
endfunction()

add_subdirectory(src/melosic)

add_subdirectory(test)

add_dependencies(${PROJECT_NAME}_test_main catch_${PROJECT_NAME})

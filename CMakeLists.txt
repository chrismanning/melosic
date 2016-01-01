project(melosic CXX C)
cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")

option(MELOSIC_ENABLE_TESTING "Enable testing" ON)
option(MELOSIC_SANITIZE_THREAD "Use -fsanitize=thread where available" OFF)
option(MELOSIC_SANITIZE_ADDRESS "Use -fsanitize=address where available" OFF)
option(MELOSIC_DETECT_LEAKS "Use -fsanitize=leak where available" OFF)
option(MELOSIC_DETECT_UNDEFINED "Use -fsanitize=undefined where available" OFF)
option(MELOSIC_ASIO_HANDLER_TRACKING "Enable asio handler tracking" OFF)

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
if(WIN32)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_PATH})
endif()

add_definitions(-DBOOST_ALL_DYN_LINK)
add_definitions(-DBOOST_THREAD_PROVIDES_EXECUTORS -DBOOST_THREAD_VERSION=4 -DBOOST_THREAD_USES_DATETIME)
add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED)
add_definitions(-DOPENSSL_NO_SSL2)

if(MELOSIC_ASIO_HANDLER_TRACKING)
add_definitions(-DASIO_ENABLE_HANDLER_TRACKING)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-warning-option -Wno-unused-private-field -Wno-mismatched-tags")
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-c++1z-extensions")
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
 set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flto")
 set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flto")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

find_package(Boost COMPONENTS system thread locale filesystem log log_setup chrono date_time regex random REQUIRED)
link_directories(${Boost_LIBRARY_DIR})
include_directories(SYSTEM ${Boost_INCLUDE_DIR})

find_package(OpenSSL REQUIRED)
find_package(Asio 1.10.1 REQUIRED)

include_directories(include)
add_subdirectory(extern)
include_directories(SYSTEM ${EXTERN_INCLUDES})
link_directories(${CMAKE_BINARY_DIR}/extern/lib)

add_definitions(-DRAPIDJSON_HAS_STDSTRING)

if(NOT WIN32)
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -pthread")

 if(MELOSIC_DETECT_UNDEFINED)
  message(STATUS "Undefined behaviour detection enabled")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
 endif(MELOSIC_DETECT_UNDEFINED)

 if(MELOSIC_SANITIZE_ADDRESS)
  message(STATUS "Address sanitizer enabled")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
 elseif(MELOSIC_SANITIZE_THREAD)
  message(STATUS "Thread sanitizer enabled")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
 elseif(MELOSIC_DETECT_LEAKS)
  message(STATUS "Leak detection enabled")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak")
 endif()

endif(NOT WIN32)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-local-typedefs -Wno-missing-field-initializers")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes -Wno-unused-parameter -Wno-deprecated-register")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-redeclared-class-member -Wno-ignored-qualifiers")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wold-style-cast")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_CXX_FLAGS} -std=c++1y")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

ADD_DEFINITIONS(-DQT_NO_KEYWORDS)

find_package(Taglib REQUIRED)
SET(TAGLIB_MIN_VERSION "1.8")

find_package(CPPREST REQUIRED)

include_directories(src)
include_directories(SYSTEM
    ${TAGLIB_INCLUDES}
    ${CPPREST_INCLUDE_DIR}
)
SET(LINK_LIBS ${LINK_LIBS} ${TAGLIB_LIBRARIES} ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${CPPREST_LIBRARIES} z)
SET(LINK_LIBS ${LINK_LIBS} -ltcejdb -lejpp)

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
set(CATCH_INCLUDE_DIR ${source_dir}/single_include CACHE INTERNAL "Path to include folder for Catch")

if(${MELOSIC_ENABLE_TESTING})
enable_testing()
endif()

function(cxx_test test_name)
  add_executable(${test_name} ${test_name}.cpp $<TARGET_OBJECTS:${PROJECT_NAME}_test_main>)
  add_dependencies(${test_name} catch_${PROJECT_NAME} jbson ejpp)
  target_link_libraries(${test_name} ${PROJECT_NAME}lib ${LINK_LIBS})
  set_property(TARGET ${test_name} APPEND PROPERTY INCLUDE_DIRECTORIES ${CATCH_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/test)
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${test_name})
  set_property(TARGET ${test_name} APPEND PROPERTY COMPILE_DEFINITIONS
    MELOSIC_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/test" MELOSIC_DISABLE_LOGGING)
endfunction()

function(cxx_header_test test_name)
  add_executable(${test_name} ${test_name}.cpp $<TARGET_OBJECTS:${PROJECT_NAME}_test_main_no_link>)
  add_dependencies(${test_name} catch_${PROJECT_NAME} jbson ejpp)
  target_link_libraries(${test_name} ${LINK_LIBS})
  set_property(TARGET ${test_name} APPEND PROPERTY INCLUDE_DIRECTORIES ${CATCH_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/test)
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${test_name})
  set_property(TARGET ${test_name} APPEND PROPERTY COMPILE_DEFINITIONS
    MELOSIC_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/test" MELOSIC_DISABLE_LOGGING)
endfunction()

add_subdirectory(src/melosic)

add_subdirectory(test)

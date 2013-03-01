cmake_minimum_required(VERSION 2.8)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fPIC -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++11 -pthread -stdlib=libc++")
ADD_DEFINITIONS(-D__GXX_EXPERIMENTAL_CXX0X__=1)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O1 -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g -O1 -fsanitize=address")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -Os")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Os")

FIND_PACKAGE(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SRC kcategorizedview.cpp kcategorydrawer.cpp kcategorizedsortfilterproxymodel.cpp
kcategorizedsortfilterproxymodel.h
kcategorizedsortfilterproxymodel_p.h
kcategorizedview.h
kcategorizedview_p.h
kcategorydrawer.h
alphanum.hpp)

add_definitions(-DKDEUI_DEPRECATED=)
add_library(kcategorizedview SHARED ${SRC})
qt5_use_modules(kcategorizedview Widgets)
TARGET_LINK_LIBRARIES(kcategorizedview ${QT5Widgets_LIBRARIES})

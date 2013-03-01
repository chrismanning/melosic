FIND_PACKAGE(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Boost REQUIRED)

add_definitions(-DKDEUI_DEPRECATED=)

SET(qt-gui_SOURCES mainwindow.cpp trackseeker.cpp playlistmodel.cpp configwidget.cpp configurationdialog.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} outputconfwidget.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} playlistcategorymodel.cpp)
SET(qt-gui_HEADERS mainwindow.hpp trackseeker.hpp playlistmodel.hpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ../.. . ${Boost_INCLUDE_DIRS})

ADD_LIBRARY(qt-gui SHARED ${qt-gui_SOURCES} ${qt-gui_HEADERS_MOC})
set_target_properties(qt-gui PROPERTIES PREFIX "")
qt5_use_modules(qt-gui Widgets)
TARGET_LINK_LIBRARIES(qt-gui ${QT5Widgets_LIBRARIES} kcategorizedview melosiccore)

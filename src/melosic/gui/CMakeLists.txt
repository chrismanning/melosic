FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
ADD_DEFINITIONS(-DQT_SHARED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Boost REQUIRED)

SET(qt-gui_SOURCES mainwindow.cpp application.hpp)
SET(qt-gui_HEADERS mainwindow.hpp)
SET(qt-gui_FORMS mainwindow.ui)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ../.. ${Boost_INCLUDE_DIRS})

add_definitions(${Qt5Core_DEFINITIONS})
add_definitions(${Qt5Gui_DEFINITIONS})
add_definitions(${Qt5Widgets_DEFINITIONS})

QT5_WRAP_UI(qt-gui_FORMS_HEADERS ${qt-gui_FORMS})

ADD_LIBRARY(qt-gui SHARED ${qt-gui_SOURCES} ${qt-gui_FORMS_HEADERS})
set_target_properties(qt-gui PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(qt-gui melosiccore)
qt5_use_modules(qt-gui Core Gui Widgets)

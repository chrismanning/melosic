add_definitions(-DMELOSIC_CORE_EXPORTS)

FIND_PACKAGE(Qt5Qml REQUIRED)
FIND_PACKAGE(Qt5Quick REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Test REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Boost REQUIRED)

SET(qt-gui_SOURCES ${qt-gui_SOURCES} mainwindow.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} playlistmodel.cpp)

SET(qt-gui_SOURCES ${qt-gui_SOURCES} playlistmanagermodel.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} playercontrols.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} categoryproxymodel.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} category.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} modeltest.cpp modeltest.h)
SET(qt-gui_HEADERS mainwindow.hpp playlistmodel.hpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} . ${Boost_INCLUDE_DIRS})

QT5_ADD_RESOURCES(RCS_FILES resources.qrc)

ADD_LIBRARY(qt-gui SHARED ${qt-gui_SOURCES} ${qt-gui_HEADERS_MOC} ${RCS_FILES})
set_target_properties(qt-gui PROPERTIES PREFIX "")
qt5_use_modules(qt-gui Widgets Qml Quick Test)
TARGET_LINK_LIBRARIES(qt-gui ${QT5Widgets_LIBRARIES} ${QT5Quick_LIBRARIES} ${QT5Test_LIBRARIES} melosiccore)

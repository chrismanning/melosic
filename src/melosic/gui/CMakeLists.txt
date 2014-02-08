FIND_PACKAGE(Qt5Qml REQUIRED)
FIND_PACKAGE(Qt5Quick REQUIRED COMPONENTS Private)
FIND_PACKAGE(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(qt-gui_SOURCES ${qt-gui_SOURCES} mainwindow.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} playlistmodel.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} playlistmanagermodel.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} playercontrols.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} categoryproxymodel.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} category.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} categorytag.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} quicklogbackend.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} filterpane.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} jsondocmodel.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} librarymanager.cpp)
SET(qt-gui_SOURCES ${qt-gui_SOURCES} selectionmodel.cpp)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} . ${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${Qt5Quick_PRIVATE_INCLUDE_DIRS})

QT5_ADD_RESOURCES(RCS_FILES resources.qrc)

ADD_LIBRARY(qt-gui SHARED ${qt-gui_SOURCES} ${RCS_FILES})
set_target_properties(qt-gui PROPERTIES PREFIX "")
TARGET_LINK_LIBRARIES(qt-gui Qt5::Widgets Qt5::Qml Qt5::Quick melosiclib)

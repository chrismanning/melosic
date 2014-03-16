FIND_PACKAGE(Qt5Qml REQUIRED)
FIND_PACKAGE(Qt5Quick REQUIRED COMPONENTS Private)
FIND_PACKAGE(Qt5Widgets REQUIRED)

find_package(Boost COMPONENTS log log_setup REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(qt_gui_SOURCES ${qt_gui_SOURCES} mainwindow.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} playlistmodel.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} playlistmanagermodel.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} playercontrols.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} categoryproxymodel.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} category.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} categorytag.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} quicklogbackend.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} filterpane.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} jsondocmodel.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} librarymanager.cpp)
SET(qt_gui_SOURCES ${qt_gui_SOURCES} selectionmodel.cpp)

SET(QT_LIBS Qt5::Widgets Qt5::Qml Qt5::Quick)

IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
 SET(qt_gui_SOURCES ${qt_gui_SOURCES} modeltest.cpp)
 SET(qt_gui_SOURCES ${qt_gui_SOURCES} dynamictreemodel.cpp)
 FIND_PACKAGE(Qt5Test REQUIRED)
 SET(QT_LIBS ${QT_LIBS} Qt5::Test)
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Debug")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} . ${Boost_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${Qt5Quick_PRIVATE_INCLUDE_DIRS})

add_subdirectory(test)

QT5_ADD_RESOURCES(RCS_FILES resources.qrc)

ADD_LIBRARY(qt_gui SHARED ${qt_gui_SOURCES} ${RCS_FILES})
set_target_properties(qt_gui PROPERTIES PREFIX "" COMPILE_DEFINITIONS QT_GUI_EXPORTS=qt_gui_EXPORTS)
TARGET_LINK_LIBRARIES(qt_gui ${QT_LIBS} -L${Boost_LIBRARY_DIR} ${Boost_LIBRARIES} ${PN}lib)

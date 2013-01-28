FIND_PACKAGE(Qt5Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Boost REQUIRED)

SET(qt-gui_SOURCES mainwindow.cpp trackseeker.cpp playlistmodel.cpp configwidget.cpp configurationdialog.cpp)
SET(qt-gui_HEADERS mainwindow.hpp trackseeker.hpp playlistmodel.hpp)
SET(qt-gui_FORMS mainwindow.ui)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ../.. . ${Boost_INCLUDE_DIRS})

QT5_WRAP_UI(qt-gui_FORMS_HEADERS ${qt-gui_FORMS})

ADD_LIBRARY(qt-gui SHARED ${qt-gui_SOURCES} ${qt-gui_HEADERS_MOC} ${qt-gui_FORMS_HEADERS})
set_target_properties(qt-gui PROPERTIES PREFIX "")
qt5_use_modules(qt-gui Widgets)
TARGET_LINK_LIBRARIES(qt-gui ${QT5Widgets_LIBRARIES} melosiccore)

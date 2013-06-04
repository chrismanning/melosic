FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Qml REQUIRED)
SET(PN ${PROJECT_NAME})

find_package(Boost COMPONENTS filesystem iostreams system log log_setup thread REQUIRED)

find_package(Taglib REQUIRED)
SET(TAGLIB_MIN_VERSION "1.8")

SET(SRC_LIST main.cpp)

add_subdirectory(core)
add_subdirectory(common)
add_subdirectory(plugins)
add_subdirectory(gui)
add_subdirectory(melin)

add_executable(${PN} ${SRC_LIST})

qt5_use_modules(${PN} Widgets Qml)
SET(LINK_LIBS ${LINK_LIBS} ${Qt5Widgets_LIBRARIES} ${Qt5Qml_LIBRARIES})

include_directories(.. ${Boost_INCLUDE_DIRS} ${Qt5Qml_INCLUDES} ${Qt5Quick_INCLUDES} ${Qt5Widgets_INCLUDES})

IF(${UNIX})
    SET(LINK_LIBS ${LINK_LIBS} /usr/local/lib64/libboost_log.a)
ENDIF(${UNIX})

target_link_libraries(${PN} ${Boost_LIBRARIES} ${TAGLIB_LIBRARIES} melosiccore melin ${LINK_LIBS} qt-gui)

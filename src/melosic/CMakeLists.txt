FIND_PACKAGE(Qt5Widgets REQUIRED)

SET(SRC_LIST main.cpp)

#abbrev
SET(PN ${PROJECT_NAME})

find_package(Boost COMPONENTS filesystem system iostreams REQUIRED)

add_executable(${PN} ${SRC_LIST})

add_subdirectory(core)
add_subdirectory(common)
add_subdirectory(managers)
add_subdirectory(plugins)
add_subdirectory(gui)

include_directories(.. ${Boost_INCLUDE_DIRS})

IF(${WIN32})
    SET(LINK_LIBS ${LINK_LIBS} boost_system)
ELSEIF(${UNIX})
    SET(LINK_LIBS ${LINK_LIBS} dl)
ENDIF(${WIN32})

target_link_libraries(${PN} ${Boost_LIBRARIES} ${LINK_LIBS} melosiccore qt-gui)
qt5_use_modules(${PN} Widgets)


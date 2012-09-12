FIND_PACKAGE(Qt4 REQUIRED)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DQT_SHARED)

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


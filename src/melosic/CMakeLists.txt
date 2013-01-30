FIND_PACKAGE(Qt5Widgets REQUIRED)
ADD_DEFINITIONS(-DQT_SHARED)

#abbrev
SET(PN ${PROJECT_NAME})

find_package(Boost COMPONENTS filesystem iostreams system log log_setup REQUIRED)

find_package(Taglib REQUIRED)
SET(TAGLIB_MIN_VERSION "1.8")

SET(SRC_LIST main.cpp)

add_subdirectory(core)
add_subdirectory(common)
add_subdirectory(managers)
add_subdirectory(plugins)
add_subdirectory(gui)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xlinker ")

add_executable(${PN} ${SRC_LIST})

qt5_use_modules(${PN} Widgets)

include_directories(.. ${Boost_INCLUDE_DIRS} ${Qt5Widgets_INCLUDES})

IF(${UNIX})
    SET(LINK_LIBS ${LINK_LIBS} dl /usr/local/lib64/libboost_log.a cxxrt)
ENDIF(${UNIX})

target_link_libraries(${PN} ${Boost_LIBRARIES} ${TAGLIB_LIBRARIES} melosiccore qt-gui ${LINK_LIBS})



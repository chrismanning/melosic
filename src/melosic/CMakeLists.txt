FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Qml REQUIRED)
SET(PN ${PROJECT_NAME})

find_package(Boost COMPONENTS filesystem iostreams system log log_setup thread REQUIRED)

add_definitions(-DBOOST_FILESYSTEM_NO_DEPRECATED=0)

find_package(Taglib REQUIRED)
SET(TAGLIB_MIN_VERSION "1.8")

include_directories(.. ${Boost_INCLUDE_DIRS})

add_definitions(-DMELOSIC_EXPORTS)

SET(SRC_LIST main.cpp)

set(CORE_SRC_LIST)
add_subdirectory(core)
add_subdirectory(common)
set(MELIN_SRC_LIST)
add_subdirectory(melin)
add_subdirectory(gui)
add_subdirectory(plugins)

add_library(${PN}lib SHARED ${CORE_SRC_LIST} ${MELIN_SRC_LIST})
target_link_libraries(${PN}lib "-Wl,--whole-archive" melin melosiccore "-Wl,--no-whole-archive")
if(UNIX)
 target_link_libraries(${PN}lib dl)
endif()

add_executable(${PN} ${SRC_LIST})

qt5_use_modules(${PN} Widgets Qml)
SET(LINK_LIBS ${LINK_LIBS} ${Qt5Widgets_LIBRARIES} ${Qt5Qml_LIBRARIES})

include_directories(${Qt5Qml_INCLUDES} ${Qt5Quick_INCLUDES} ${Qt5Widgets_INCLUDES})

target_link_libraries(${PN} ${Boost_LIBRARIES} ${TAGLIB_LIBRARIES} qt-gui ${PN}lib ${LINK_LIBS})

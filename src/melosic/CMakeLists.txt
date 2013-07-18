FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Qml REQUIRED)

add_definitions(-DMELOSIC_EXPORTS)

SET(SRC_LIST main.cpp)

set(CORE_SRC_LIST)
add_subdirectory(core)
add_subdirectory(common)
set(MELIN_SRC_LIST)
add_subdirectory(melin)

add_subdirectory(gui)

add_library(${PN}lib SHARED ${CORE_SRC_LIST} ${MELIN_SRC_LIST})
target_link_libraries(${PN}lib)

if(UNIX)
 target_link_libraries(${PN}lib dl)
endif()

include_directories(${Qt5Qml_INCLUDES} ${Qt5Quick_INCLUDES} ${Qt5Widgets_INCLUDES})

#remove_definitions(-DMELOSIC_EXPORTS)
add_subdirectory(plugins)

#create main executable
add_executable(${PN} ${SRC_LIST})
add_library(${PN}_test_main test_main.cpp)

qt5_use_modules(${PN} Widgets Qml)
SET(LINK_LIBS ${Qt5Widgets_LIBRARIES} ${Qt5Qml_LIBRARIES})

target_link_libraries(${PN} ${Boost_LIBRARIES} ${TAGLIB_LIBRARIES} qt-gui ${PN}lib ${LINK_LIBS})
target_link_libraries(${PN}lib ${Boost_LIBRARIES} ${TAGLIB_LIBRARIES} ${LINK_LIBS})

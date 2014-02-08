FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Qml REQUIRED)

add_definitions(-DMELOSIC_EXPORTS)

SET(SRC_LIST main.cpp)

add_subdirectory(asio)
set(CORE_SRC_LIST)
add_subdirectory(core)
add_subdirectory(common)
set(MELIN_SRC_LIST)
add_subdirectory(melin)

add_subdirectory(gui)

add_library(${PN}objects OBJECT ${CORE_SRC_LIST} ${MELIN_SRC_LIST})
add_library(${PN}lib SHARED $<TARGET_OBJECTS:${PN}objects>)

if(UNIX)
 SET(LINK_LIBS ${LINK_LIBS} -ldl)
endif()

include_directories(${Qt5Qml_INCLUDES} ${Qt5Quick_INCLUDES} ${Qt5Widgets_INCLUDES})

add_subdirectory(plugins)

#create main executable
add_executable(${PN} ${SRC_LIST} $<TARGET_OBJECTS:${PN}objects>)
add_library(${PN}_test_main OBJECT test_main.cpp)

add_dependencies(${PN}_test_main gtest_${PROJECT_NAME})

qt5_use_modules(${PN} Widgets Qml)
SET(LINK_LIBS ${LINK_LIBS} ${TAGLIB_LIBRARIES})
SET(LINK_LIBS ${LINK_LIBS} PARENT_SCOPE)

qt5_use_modules(${PN} Widgets Qml)
SET(LINK_LIBS ${LINK_LIBS} ${Qt5Widgets_LIBRARIES} ${Qt5Qml_LIBRARIES})

target_link_libraries(${PN} ${LINK_LIBS} qt-gui ejpp)
target_link_libraries(${PN}lib ${LINK_LIBS} ejpp)

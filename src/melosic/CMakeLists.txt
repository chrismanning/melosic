FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Qml REQUIRED)

add_subdirectory(executors)

add_subdirectory(asio)
set(CORE_SRC_LIST)
add_subdirectory(core)
add_subdirectory(common)
set(MELIN_SRC_LIST)
add_subdirectory(melin)

add_library(${PROJECT_NAME}lib SHARED ${CORE_SRC_LIST} ${MELIN_SRC_LIST})
set_target_properties(${PROJECT_NAME}lib PROPERTIES COMPILE_DEFINITIONS MELOSIC_EXPORTS=${PROJECT_NAME}lib_EXPORTS)

add_dependencies(${PROJECT_NAME}lib jbson ejpp cpp-netlib rapidjson)

add_subdirectory(gui)

include_directories(SYSTEM ${Qt5Qml_INCLUDES} ${Qt5Quick_INCLUDES} ${Qt5Widgets_INCLUDES})

add_subdirectory(plugins)

#create main executable
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} -L${Boost_LIBRARY_DIR} ${Boost_LIBRARIES} ${PROJECT_NAME}lib)

add_library(${PROJECT_NAME}_test_main OBJECT test_main.cpp)
set_property(TARGET ${PROJECT_NAME}_test_main APPEND PROPERTY INCLUDE_DIRECTORIES ${CATCH_INCLUDE_DIR})

add_library(${PROJECT_NAME}_test_main_no_link OBJECT test_main.cpp)
set_property(TARGET ${PROJECT_NAME}_test_main_no_link APPEND PROPERTY INCLUDE_DIRECTORIES ${CATCH_INCLUDE_DIR})
set_property(TARGET ${PROJECT_NAME}_test_main_no_link APPEND PROPERTY COMPILE_DEFINITIONS MELOSIC_TEST_NO_LINK)

add_dependencies(${PROJECT_NAME}_test_main catch_${PROJECT_NAME})

qt5_use_modules(${PROJECT_NAME} Widgets Qml)
SET(LINK_LIBS ${LINK_LIBS} ${TAGLIB_LIBRARIES} executors)
SET(LINK_LIBS ${LINK_LIBS} PARENT_SCOPE)

target_link_libraries(${PROJECT_NAME}lib ${LINK_LIBS})

qt5_use_modules(${PROJECT_NAME} Widgets Qml)
SET(LINK_LIBS ${LINK_LIBS} ${Qt5Widgets_LIBRARIES} ${Qt5Qml_LIBRARIES})

target_link_libraries(${PROJECT_NAME} ${LINK_LIBS} qt_gui)

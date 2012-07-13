SET(SRC_LIST main.d)
SET(SRC_LIST ${SRC_LIST} core/core.d)
SET(SRC_LIST ${SRC_LIST} ../util/util.d)
SET(SRC_LIST ${SRC_LIST} plugins/plugins.d)

add_executable(${PROJECT_NAME} ${SRC_LIST})

add_subdirectory(managers)
add_subdirectory(plugins)

#abbrev
SET(PN ${PROJECT_NAME})

include_directories(..)

target_link_libraries(${PN} dl)


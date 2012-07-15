SET(SRC_LIST main.d)
SET(SRC_LIST ${SRC_LIST} core/core.d)
SET(SRC_LIST ${SRC_LIST} ../util/util.d)
SET(SRC_LIST ${SRC_LIST} plugins/plugins.d)

#abbrev
SET(PN ${PROJECT_NAME})

add_executable(${PN} ${SRC_LIST})

add_subdirectory(managers)
add_subdirectory(plugins)

include_directories(..)
target_link_libraries(${PN} melosicmanagers)


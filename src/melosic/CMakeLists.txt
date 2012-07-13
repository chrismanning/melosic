SET(SRC_LIST main.d)
SET(SRC_LIST ${SRC_LIST} core/core.d)
SET(SRC_LIST ${SRC_LIST} ../util/util.d)
SET(SRC_LIST ${SRC_LIST} managers/config/configmanager.d)
SET(SRC_LIST ${SRC_LIST} managers/input/inputmanager.d)
SET(SRC_LIST ${SRC_LIST} managers/library/librarymanager.d)
SET(SRC_LIST ${SRC_LIST} managers/output/outputmanager.d)
SET(SRC_LIST ${SRC_LIST} managers/ui/uimanager.d)
SET(SRC_LIST ${SRC_LIST} plugins/plugins.d)

add_executable(${PROJECT_NAME} ${SRC_LIST})

#abbrev
SET(PN ${PROJECT_NAME})

include_directories(..)

target_link_libraries(${PN} dl)


SET(SRC_LIST common.d)
SET(SRC_LIST ${SRC_LIST} config/configmanager.d)
SET(SRC_LIST ${SRC_LIST} input/inputmanager.d input/pluginterface.d)
SET(SRC_LIST ${SRC_LIST} library/librarymanager.d)
SET(SRC_LIST ${SRC_LIST} output/outputmanager.d)
SET(SRC_LIST ${SRC_LIST} ui/uimanager.d)

include_directories(../..)

add_library(melosicmanagers ${SRC_LIST})
target_link_libraries(melosicmanagers dl)

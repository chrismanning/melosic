SET(SRC_LIST common.cpp)
SET(SRC_LIST ${SRC_LIST} input/pluginterface.cpp)
SET(SRC_LIST ${SRC_LIST} output/pluginterface.cpp)

include_directories(../..)

add_library(melosiccore SHARED ${SRC_LIST})
target_link_libraries(melosiccore dl)

SET(SRC_LIST common.cpp common.hpp)
SET(SRC_LIST ${SRC_LIST} input/inputmanager.cpp input/inputmanager.hpp input/pluginterface.hpp)
SET(SRC_LIST ${SRC_LIST} output/pluginterface.hpp)

include_directories(../..)

add_library(melosiccore SHARED ${SRC_LIST})
target_link_libraries(melosiccore dl boost_filesystem)

SET(SRC_LIST input/pluginterface.hpp)
SET(SRC_LIST ${SRC_LIST} output/pluginterface.hpp)

include_directories(../..)

add_library(manager_heads STATIC ${SRC_LIST})
set_target_properties(manager_heads PROPERTIES LINKER_LANGUAGE CXX)

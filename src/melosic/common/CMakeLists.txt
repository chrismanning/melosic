SET(SRC_LIST common.hpp exports.hpp ikernel.hpp plugin.hpp file.hpp)
add_library(common_heads STATIC ${SRC_LIST})
set_target_properties(common_heads PROPERTIES LINKER_LANGUAGE CXX)

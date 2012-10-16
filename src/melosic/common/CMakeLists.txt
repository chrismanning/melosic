SET(SRC_LIST common.hpp error.hpp exports.hpp plugin.hpp file.hpp stream.hpp)
SET(SRC_LIST ${SRC_LIST} logging.hpp properties.hpp)
add_library(common_heads STATIC ${SRC_LIST})
set_target_properties(common_heads PROPERTIES LINKER_LANGUAGE CXX)

FILE(GLOB COMMON_SRC_LIST *.hpp)

add_library(common_heads STATIC ${COMMON_SRC_LIST})
set_target_properties(common_heads PROPERTIES LINKER_LANGUAGE CXX)

add_subdirectory(test)

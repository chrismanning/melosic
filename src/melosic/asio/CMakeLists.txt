FILE(GLOB ASIO_SRC_LIST *.hpp)

add_library(asio_heads STATIC ${ASIO_SRC_LIST})
set_target_properties(asio_heads PROPERTIES LINKER_LANGUAGE CXX)

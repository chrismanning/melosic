FILE(GLOB ASIO_SRC_LIST *.hpp)

add_custom_target(asio_heads ALL SOURCES ${ASIO_SRC_LIST})

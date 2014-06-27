FILE(GLOB COMMON_SRC_LIST *.hpp)

add_custom_target(common_heads ALL SOURCES ${COMMON_SRC_LIST})

add_subdirectory(test)

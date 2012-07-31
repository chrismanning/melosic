SET(SRC_LIST main.cpp)
SET(SRC_LIST ${SRC_LIST} core/wav.hpp)

#abbrev
SET(PN ${PROJECT_NAME})

add_executable(${PN} ${SRC_LIST})

add_subdirectory(managers)
add_subdirectory(plugins)
add_subdirectory(gui)

include_directories(..)
target_link_libraries(${PN} melosiccore)


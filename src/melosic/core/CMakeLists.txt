SET(SRC_LIST)
SET(SRC_LIST ${SRC_LIST} kernel.hpp kernel.cpp)
SET(SRC_LIST ${SRC_LIST} wav.hpp)
SET(SRC_LIST ${SRC_LIST} player.hpp player.cpp)
SET(SRC_LIST ${SRC_LIST} playlist.hpp playlist.cpp)
SET(SRC_LIST ${SRC_LIST} track.hpp track.cpp)
SET(SRC_LIST ${SRC_LIST} taglibfile.hpp taglibfile.cpp)

find_package(Boost COMPONENTS filesystem system iostreams thread chrono REQUIRED)

include_directories(../.. ${Boost_INCLUDE_DIRS})

add_library(melosiccore SHARED ${SRC_LIST})

target_link_libraries(melosiccore ${Boost_LIBRARIES})

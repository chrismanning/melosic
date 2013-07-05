add_definitions(-DMELOSIC_CORE_EXPORTS)

SET(SRC_LIST)
SET(SRC_LIST ${SRC_LIST} wav.hpp)
SET(SRC_LIST ${SRC_LIST} player.hpp player.cpp)
SET(SRC_LIST ${SRC_LIST} statemachine.hpp statemachine.cpp)
SET(SRC_LIST ${SRC_LIST} playlist.hpp playlist.cpp)
SET(SRC_LIST ${SRC_LIST} track.hpp track.cpp)
SET(SRC_LIST ${SRC_LIST} taglibfile.hpp taglibfile.cpp)

find_package(Boost COMPONENTS filesystem system iostreams thread chrono log log_setup serialization REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

add_library(melosiccore SHARED ${SRC_LIST})

target_link_libraries(melosiccore melin ${Boost_LIBRARIES})

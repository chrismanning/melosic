SET(CORE_SRC_LIST ${CORE_SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/wav.hpp)
SET(CORE_SRC_LIST ${CORE_SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/player.cpp)
SET(CORE_SRC_LIST ${CORE_SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/statemachine.cpp)
SET(CORE_SRC_LIST ${CORE_SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/playlist.cpp)
SET(CORE_SRC_LIST ${CORE_SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/track.cpp)
SET(CORE_SRC_LIST ${CORE_SRC_LIST} ${CMAKE_CURRENT_SOURCE_DIR}/taglibfile.cpp)
SET(CORE_SRC_LIST ${CORE_SRC_LIST} PARENT_SCOPE)

find_package(Boost COMPONENTS filesystem system iostreams thread chrono log log_setup serialization REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

add_library(melosiccore STATIC ${CORE_SRC_LIST})

target_link_libraries(melosiccore melin ${Boost_LIBRARIES})

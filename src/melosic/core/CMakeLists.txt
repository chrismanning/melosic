SET(SRC_LIST inputmanager.hpp inputmanager.cpp)
SET(SRC_LIST ${SRC_LIST} outputmanager.hpp)# outputmanager.cpp)
SET(SRC_LIST ${SRC_LIST} kernel.hpp kernel.cpp)
SET(SRC_LIST ${SRC_LIST} wav.hpp)

find_package(Boost COMPONENTS filesystem system iostreams REQUIRED)

include_directories(../.. ${Boost_INCLUDE_DIRS})

add_library(melosiccore STATIC ${SRC_LIST})

target_link_libraries(melosiccore ${Boost_LIBRARIES})

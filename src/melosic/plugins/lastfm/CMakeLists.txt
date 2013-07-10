set(LASTFM lastfm)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(SRC lastfm.cpp)
SET(SRC ${SRC} lastfmconfig.cpp)
SET(SRC ${SRC} lastfmconfig.cpp)
SET(SRC ${SRC} scrobbler.cpp)
SET(SRC ${SRC} service.cpp)
SET(SRC ${SRC} tag.cpp)
SET(SRC ${SRC} user.cpp)
SET(SRC ${SRC} artist.cpp)
SET(SRC ${SRC} track.cpp)
SET(SRC ${SRC} biography.cpp)

find_package(Boost COMPONENTS serialization REQUIRED)

add_library(${LASTFM} SHARED ${SRC})

target_link_libraries(${LASTFM} melin ${boost_LIBRARIES} cppnetlib-http-client ssl crypto)
set_target_properties(${LASTFM} PROPERTIES PREFIX "")
set_target_properties(${LASTFM} PROPERTIES SUFFIX ".melin")
include_directories(${Qt5Widgets_INCLUDES} ${Boost_INCLUDE_DIRS})

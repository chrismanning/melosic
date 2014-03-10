set(LASTFM lastfm)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(SRC lastfm.cpp)
SET(SRC ${SRC} scrobbler.cpp)
SET(SRC ${SRC} service.cpp)
SET(SRC ${SRC} tag.cpp)
SET(SRC ${SRC} user.cpp)
SET(SRC ${SRC} artist.cpp)
SET(SRC ${SRC} track.cpp)
SET(SRC ${SRC} biography.cpp)

add_library(${LASTFM} SHARED ${SRC})

if(WIN32)
set(LINK_LIBS network)
else()
set(LINK_LIBS network-http-v2-client)
endif()

target_link_libraries(${LASTFM} ${LINK_LIBS} melosiclib ssl crypto)
set_target_properties(${LASTFM} PROPERTIES PREFIX "")
set_target_properties(${LASTFM} PROPERTIES SUFFIX ".melin")
include_directories(${Qt5Widgets_INCLUDES} ${Boost_INCLUDE_DIRS})

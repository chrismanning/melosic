set(LASTFM lastfm)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(SRC lastfm.cpp)
SET(SRC ${SRC} scrobbler.cpp)
SET(SRC ${SRC} service.cpp)
SET(SRC ${SRC} tag.cpp)
SET(SRC ${SRC} user.cpp)
SET(SRC ${SRC} artist.cpp)
SET(SRC ${SRC} album.cpp)
SET(SRC ${SRC} track.cpp)
SET(SRC ${SRC} error.cpp)
SET(SRC ${SRC} wiki.cpp)
SET(SRC ${SRC} http_client.cpp)

add_library(${LASTFM} SHARED ${SRC})

add_subdirectory(test)

if(WIN32)
set(LINK_LIBS network)
else()
set(LINK_LIBS network-http-v2-client)
endif()

target_link_libraries(${LASTFM} ssl crypto ${LINK_LIBS} melosiclib)
set_target_properties(${LASTFM} PROPERTIES PREFIX "")
set_target_properties(${LASTFM} PROPERTIES SUFFIX ".melin")
set_target_properties(${LASTFM} PROPERTIES COMPILE_DEFINITIONS LASTFM_EXPORTS=${LASTFM}_EXPORTS)
include_directories(SYSTEM ${Qt5Widgets_INCLUDES} ${Boost_INCLUDE_DIR})

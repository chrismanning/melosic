set(LASTFM lastfm)

find_package(Qt5Widgets REQUIRED)

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

qt5_use_modules(${LASTFM} Widgets)

target_link_libraries(${LASTFM} melin ${boost_LIBRARIES})
target_link_libraries(${LASTFM} cppnetlib-uri cppnetlib-http-client cppnetlib-http-client-connections ssl crypto)
set_target_properties(${LASTFM} PROPERTIES PREFIX "")
set_target_properties(${LASTFM} PROPERTIES SUFFIX ".melin")
include_directories(${Qt5Widgets_INCLUDES} ${Boost_INCLUDE_DIRS} ../../..)

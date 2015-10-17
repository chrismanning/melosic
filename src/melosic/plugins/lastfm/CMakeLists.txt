set(LASTFM lastfm)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../)

SET(SRC lastfm.cpp)

add_library(${LASTFM} SHARED ${SRC})

add_dependencies(${LASTFM} lastfmpp_${PROJECT_NAME})

target_link_libraries(${LASTFM} lastfmpp)
set_target_properties(${LASTFM} PROPERTIES PREFIX "")
set_target_properties(${LASTFM} PROPERTIES SUFFIX ".melin")
set_target_properties(${LASTFM} PROPERTIES COMPILE_DEFINITIONS LASTFM_EXPORTS=${LASTFM}_EXPORTS)
include_directories(SYSTEM ${Qt5Widgets_INCLUDES} ${Boost_INCLUDE_DIR})

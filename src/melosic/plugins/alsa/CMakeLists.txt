set(ALSA alsa)
find_package(ALSA REQUIRED)

add_library(${ALSA} MODULE alsa.cpp)

target_link_libraries(${ALSA} ${ALSA_LIBRARIES} melosiclib)
set_target_properties(${ALSA} PROPERTIES PREFIX "")
set_target_properties(${ALSA} PROPERTIES SUFFIX ".melin")
include_directories(../../.. ${ALSA_INCLUDE_DIRS} ${Boost_INCLUDE_DIRS})

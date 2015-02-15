set(FLAC flac)

find_package(FLAC++ REQUIRED)
find_package(FLAC REQUIRED)

add_library(${FLAC} SHARED flac.cpp flacdecoder.cpp exports.hpp)

target_link_libraries(${FLAC} ${FLAC_LIBRARIES} ${FLAC++_LIBRARIES} melosiclib)
set_target_properties(${FLAC} PROPERTIES PREFIX "")
set_target_properties(${FLAC} PROPERTIES SUFFIX ".melin")
include_directories(${FLAC_INCLUDE_DIR} ${FLAC++_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})
set_target_properties(${FLAC} PROPERTIES COMPILE_DEFINITIONS FLAC_MELIN_EXPORTS=${FLAC}_EXPORTS)

add_subdirectory(test)

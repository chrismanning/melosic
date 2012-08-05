set(FLAC flac)

find_package(FLAC++ REQUIRED)
find_package(FLAC REQUIRED)

add_library(${FLAC} SHARED flac.cpp)

target_link_libraries(${FLAC} ${FLAC_LIBRARIES} ${FLAC++_LIBRARIES})
set_target_properties(${FLAC} PROPERTIES PREFIX "")
include_directories(${FLAC_INCLUDE_DIR} ${FLAC++_INCLUDE_DIR} ../../..)

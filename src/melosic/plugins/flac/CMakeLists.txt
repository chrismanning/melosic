set(FLAC flac)
add_library(${FLAC} SHARED flac.cpp)
set_target_properties(${FLAC} PROPERTIES PREFIX "")

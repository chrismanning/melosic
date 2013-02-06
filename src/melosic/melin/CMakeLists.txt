SET(SRC_LIST)
set(SRC_LIST ${SRC_LIST} sigslotmanager.cpp)
set(SRC_LIST ${SRC_LIST} decoder.cpp)
set(SRC_LIST ${SRC_LIST} encoder.cpp)
set(SRC_LIST ${SRC_LIST} input.cpp)
set(SRC_LIST ${SRC_LIST} output.cpp)
set(SRC_LIST ${SRC_LIST} plugin.cpp)
set(SRC_LIST ${SRC_LIST} logging.cpp)
set(SRC_LIST ${SRC_LIST} exports.cpp)
SET(SRC_LIST ${SRC_LIST} config.cpp)

set(HDR_LIST)
set(HDR_LIST ${HDR_LIST} exports.hpp)
set(HDR_LIST ${HDR_LIST} plugin.hpp)

find_package(Boost COMPONENTS filesystem system log log_setup thread serialization REQUIRED)

include_directories(../.. ${Boost_INCLUDE_DIRS})

add_library(melin STATIC ${SRC_LIST} ${HDR_LIST})
target_link_libraries(melin ${Boost_LIBRARIES})

#install(FILES ${HDR_LIST} DESTINATION include/${PROJECT_NAME})
#install(TARGETS melin LIBRARY DESTINATION lib)

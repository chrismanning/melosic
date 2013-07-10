set(MELIN_SRC_LIST ${MELIN_SRC_LIST} decoder.cpp)
set(MELIN_SRC_LIST ${MELIN_SRC_LIST} encoder.cpp)
set(MELIN_SRC_LIST ${MELIN_SRC_LIST} input.cpp)
set(MELIN_SRC_LIST ${MELIN_SRC_LIST} output.cpp)
set(MELIN_SRC_LIST ${MELIN_SRC_LIST} plugin.cpp)
set(MELIN_SRC_LIST ${MELIN_SRC_LIST} logging.cpp)
set(MELIN_SRC_LIST ${MELIN_SRC_LIST} exports.cpp)
SET(MELIN_SRC_LIST ${MELIN_SRC_LIST} config.cpp)
SET(MELIN_SRC_LIST ${MELIN_SRC_LIST} thread.cpp)
SET(MELIN_SRC_LIST ${MELIN_SRC_LIST} playlist.cpp)
SET(MELIN_SRC_LIST ${MELIN_SRC_LIST} kernel.cpp)

set(HDR_LIST)

find_package(Boost COMPONENTS filesystem system log log_setup thread serialization REQUIRED)

find_package(OpenSSL REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

add_library(melin STATIC ${MELIN_SRC_LIST} ${HDR_LIST})
target_link_libraries(melin ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${TAGLIB_LIBRARIES})

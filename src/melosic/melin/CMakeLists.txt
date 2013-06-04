SET(SRC_LIST)
set(SRC_LIST ${SRC_LIST} decoder.cpp)
set(SRC_LIST ${SRC_LIST} encoder.cpp)
set(SRC_LIST ${SRC_LIST} input.cpp)
set(SRC_LIST ${SRC_LIST} output.cpp)
set(SRC_LIST ${SRC_LIST} plugin.cpp)
set(SRC_LIST ${SRC_LIST} logging.cpp)
set(SRC_LIST ${SRC_LIST} exports.cpp)
SET(SRC_LIST ${SRC_LIST} config.cpp)
SET(SRC_LIST ${SRC_LIST} thread.cpp)
SET(SRC_LIST ${SRC_LIST} playlist.cpp)
set(SRC_LIST ${SRC_LIST} sigslots/slots.cpp)
set(SRC_LIST ${SRC_LIST} sigslots/connection.cpp)

set(HDR_LIST)
set(HDR_LIST ${HDR_LIST} sigslots/signals_fwd.hpp sigslots/signals.hpp configvar.hpp)

find_package(Boost COMPONENTS filesystem system log log_setup thread serialization REQUIRED)

find_package(OpenSSL REQUIRED)

include_directories(../.. ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

add_library(melin SHARED ${SRC_LIST} ${HDR_LIST})
target_link_libraries(melin ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})

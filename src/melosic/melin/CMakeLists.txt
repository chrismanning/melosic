FIND_PACKAGE(Qt5Widgets)

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
if(Qt5Widgets_FOUND)
SET(SRC_LIST ${SRC_LIST} gui.cpp)
endif()
set(SRC_LIST ${SRC_LIST} sigslots/slots.cpp)
set(SRC_LIST ${SRC_LIST} sigslots/signals.cpp)
set(SRC_LIST ${SRC_LIST} sigslots/connection.cpp)

set(HDR_LIST)
set(HDR_LIST ${HDR_LIST} sigslots/signals_fwd.hpp configvar.hpp)

find_package(Boost COMPONENTS filesystem system log log_setup thread serialization REQUIRED)

find_package(OpenSSL REQUIRED)

include_directories(../.. ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})

add_library(melin SHARED ${SRC_LIST} ${HDR_LIST})
if(Qt5Widgets_FOUND)
qt5_use_modules(melin Widgets)
endif()
target_link_libraries(melin ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${QT5Widgets_LIBRARIES})

#install(FILES ${HDR_LIST} DESTINATION include/${PROJECT_NAME})
#install(TARGETS melin LIBRARY DESTINATION lib)

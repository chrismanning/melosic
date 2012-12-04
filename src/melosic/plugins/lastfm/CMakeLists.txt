set(LASTFM lastfm)

find_package(LibLastFm REQUIRED)

find_package(Qt4 REQUIRED)
ADD_DEFINITIONS(${QT_DEFINITIONS})
ADD_DEFINITIONS(-DQT_SHARED)
ADD_DEFINITIONS(-Dslots=Q_SLOTS -Dsignals=Q_SIGNALS)
SET(QT_USE_QTXML TRUE)
SET(QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})
QT4_WRAP_CPP(SRC lastfm.hpp)

find_package(Boost REQUIRED)

add_library(${LASTFM} SHARED lastfm.cpp ${SRC})

target_link_libraries(${LASTFM} ${LIBLASTFM_LIBRARIES} melosiccore)
set_target_properties(${LASTFM} PROPERTIES PREFIX "")
set_target_properties(${LASTFM} PROPERTIES SUFFIX ".melin")
include_directories(${LIBLASTFM_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ../../..)

SET(SRC_LIST common.cpp common.hpp)
SET(SRC_LIST ${SRC_LIST} input/inputmanager.cpp input/inputmanager.hpp input/pluginterface.hpp)
SET(SRC_LIST ${SRC_LIST} output/pluginterface.hpp)

include_directories(../..)

add_library(melosiccore SHARED ${SRC_LIST})

SET(LINK_LIBS boost_filesystem)

IF(${WIN32})
    SET(LINK_LIBS ${LINK_LIBS} boost_system)
ELSEIF(${UNIX})
    SET(LINK_LIBS ${LINK_LIBS} dl)
ENDIF(${WIN32})

target_link_libraries(melosiccore ${LINK_LIBS})

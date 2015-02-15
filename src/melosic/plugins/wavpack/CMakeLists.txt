set(WAVPACK wavpack)

FIND_PATH(WAVPACK_INCLUDE_DIR wavpack/wavpack.h)

FIND_LIBRARY(WAVPACK_LIBRARIES NAMES wavpack)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WAVPACK REQUIRED_VARS WAVPACK_LIBRARIES WAVPACK_INCLUDE_DIR)

# show the FLAC_INCLUDE_DIR and FLAC_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(WAVPACK_INCLUDE_DIR WAVPACK_LIBRARIES)

add_library(${WAVPACK} SHARED wavpack.cpp wavpackdecoder.cpp exports.hpp)

target_link_libraries(${WAVPACK} ${WAVPACK_LIBRARIES} melosiclib)
set_target_properties(${WAVPACK} PROPERTIES PREFIX "")
set_target_properties(${WAVPACK} PROPERTIES SUFFIX ".melin")
include_directories(${WAVPACK_INCLUDE_DIR} ${Boost_INCLUDE_DIRS})
set_target_properties(${WAVPACK} PROPERTIES COMPILE_DEFINITIONS WAVPACK_MELIN_EXPORTS=${WAVPACK}_EXPORTS)

add_subdirectory(test)

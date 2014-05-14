# Try to find Asio, http://think-async.com/Asio
# Once done this will define
#
#  ASIO_FOUND - system has Asio
#  ASIO_INCLUDE_DIR - the Asio include directory
#  ASIO_VERSION - Asio version from version.hpp
#  ASIO_MAJOR_VERSION
#  ASIO_MINOR_VERSION
#  ASIO_SUBMINOR_VERSION

FIND_PATH(ASIO_INCLUDE_DIR asio.hpp asio asio/version.hpp)

#INCLUDE(FindPackageHandleStandardArgs)
#FIND_PACKAGE_HANDLE_STANDARD_ARGS(Asio REQUIRED_VARS ASIO_INCLUDE_DIR)

# show the ASIO_INCLUDE_DIR variable only in the advanced view
MARK_AS_ADVANCED(ASIO_INCLUDE_DIR)

if(ASIO_INCLUDE_DIR)
  set(ASIO_VERSION 0)

  file(STRINGS "${ASIO_INCLUDE_DIR}/asio/version.hpp" _asio_VERSION_HPP_CONTENTS REGEX "#define ASIO_VERSION ")
  if("${_asio_VERSION_HPP_CONTENTS}" MATCHES ".*#define ASIO_VERSION ([0-9]+).*")
    set(ASIO_VERSION "${CMAKE_MATCH_1}")
  endif()
  unset(_asio_VERSION_HPP_CONTENTS)

  math(EXPR ASIO_MAJOR_VERSION "${ASIO_VERSION} / 100000")
  math(EXPR ASIO_MINOR_VERSION "${ASIO_VERSION} / 100 % 1000")
  math(EXPR ASIO_SUBMINOR_VERSION "${ASIO_VERSION} % 100")

  if(${Asio_FIND_VERSION})
    # Set ASIO_FOUND based on requested version.
    set(_asio_VERSION "${ASIO_MAJOR_VERSION}.${ASIO_MINOR_VERSION}.${ASIO_SUBMINOR_VERSION}")

    if("${_asio_VERSION}" VERSION_LESS "${Boost_FIND_VERSION}")
      set(ASIO_FOUND 0)
    elseif(Asio_FIND_VERSION_EXACT AND NOT "${_asio_VERSION}" VERSION_EQUAL "${Asio_FIND_VERSION}")
      set(ASIO_FOUND 0)
    else()
      set(ASIO_FOUND 1)
    endif()
  else()
    # Caller will accept any Asio version.
    set(ASIO_FOUND 1)
  endif(${Asio_FIND_VERSION})
else()
  set(ASIO_FOUND 0)
endif(ASIO_INCLUDE_DIR)

if(${ASIO_FOUND})
  message(STATUS "Found Asio version: ${ASIO_MAJOR_VERSION}.${ASIO_MINOR_VERSION}.${ASIO_SUBMINOR_VERSION}")
elseif(${Asio_FIND_REQUIRED})
  message(SEND_ERROR "Unable to find the requested Asio version.")
endif(${ASIO_FOUND})

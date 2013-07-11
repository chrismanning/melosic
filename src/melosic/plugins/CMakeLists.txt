add_subdirectory(flac)
add_subdirectory(lastfm)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_subdirectory(alsa)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

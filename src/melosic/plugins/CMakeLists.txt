add_definitions(-DMELOSIC_PLUGIN_EXPORTS)

add_subdirectory(flac)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_subdirectory(alsa)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

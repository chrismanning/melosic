add_executable(http_client_test http_client_test.cpp $<TARGET_OBJECTS:${PROJECT_NAME}_test_main>)
add_dependencies(http_client_test catch_${PROJECT_NAME} ${LASTFM} cpp-netlib)
target_link_libraries(http_client_test ${LINK_LIBS} ${PROJECT_NAME}lib ssl crypto network-http-v2-client ${LASTFM})
set_property(TARGET http_client_test APPEND PROPERTY INCLUDE_DIRECTORIES ${CATCH_INCLUDE_DIR})
set_property(TARGET http_client_test APPEND PROPERTY COMPILE_DEFINITIONS MELOSIC_DISABLE_LOGGING)

add_executable(service_test service_test.cpp $<TARGET_OBJECTS:${PROJECT_NAME}_test_main>)
add_dependencies(service_test catch_${PROJECT_NAME} ${LASTFM} cpp-netlib)
target_link_libraries(service_test ${LINK_LIBS} ${PROJECT_NAME}lib ssl crypto network-http-v2-client ${LASTFM})
set_property(TARGET service_test APPEND PROPERTY INCLUDE_DIRECTORIES ${CATCH_INCLUDE_DIR})
set_property(TARGET service_test APPEND PROPERTY COMPILE_DEFINITIONS MELOSIC_DISABLE_LOGGING)

cxx_test(lastfm_track_test)
target_link_libraries(lastfm_track_test ${LASTFM})

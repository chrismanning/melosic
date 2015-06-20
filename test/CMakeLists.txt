cxx_test(test1)

FILE(GLOB test_headers_list *.hpp)

add_custom_target(test_headers_target ALL SOURCES ${test_headers_list})

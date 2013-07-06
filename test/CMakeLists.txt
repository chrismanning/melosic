function(cxx_test test_name)
  add_executable(${test_name} ${test_name}.cpp)
  target_link_libraries(${test_name} gtest gtest_main)
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${test_name})
endfunction()

include_directories(../extern/gtest/include)
include_directories(../extern/gmock/include)

cxx_test(test1)

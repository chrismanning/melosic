cxx_test(filter_test)

FIND_PACKAGE(Qt5Test REQUIRED)

function(qt_test test_name)
  add_executable(${test_name} ${test_name}.cpp)
  add_dependencies(${test_name} gtest_${PROJECT_NAME})
  target_link_libraries(${test_name} ${LINK_LIBS} Qt5::Test Qt5::Core Qt5::Widgets Qt5::Qml Qt5::Quick)
  set_target_properties(${test_name} PROPERTIES COMPILE_DEFINITIONS "MELOSIC_DISABLE_LOGGING=1")
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${test_name})
endfunction()

qt_test(categoryproxymodel_test)

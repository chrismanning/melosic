FIND_PACKAGE(Qt5Test REQUIRED)

function(qt_test test_name)
  add_executable(${test_name} ${test_name}.cpp)
  add_dependencies(${test_name} jbson ejpp)
  target_link_libraries(${test_name} qt_gui ${LINK_LIBS})
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${test_name})
endfunction()

qt_test(categoryproxymodel_test)
qt_test(jsondocmodel_test)
qt_test(filter_test)

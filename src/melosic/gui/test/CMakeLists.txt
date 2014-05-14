FIND_PACKAGE(Qt5Test REQUIRED)

function(qt_test test_name)
  add_executable(${test_name} ${test_name}.cpp)
  add_dependencies(${test_name} jbson ejpp cpp-netlib)
  target_link_libraries(${test_name} ${LINK_LIBS} Qt5::Test Qt5::Core Qt5::Widgets Qt5::Qml Qt5::Quick qt_gui)
  set_target_properties(${test_name} PROPERTIES COMPILE_DEFINITIONS "MELOSIC_DISABLE_LOGGING")
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${test_name})
endfunction()

qt_test(categoryproxymodel_test)
qt_test(jsondocmodel_test)
qt_test(filter_test)

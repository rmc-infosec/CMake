enable_testing()

add_executable(test_runner test.c)
add_executable(test_runner2 test.cpp)

add_test(NAME basic_test COMMAND test_runner)
add_test(NAME cxx_test COMMAND test_runner2)
add_test(NAME working_dir_test COMMAND test_runner WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

set_tests_properties(basic_test PROPERTIES
  TIMEOUT 30
  LABELS "unit;fast"
  ENVIRONMENT "FOO=bar;BAZ=1"
  PASS_REGULAR_EXPRESSION "PASS"
  FAIL_REGULAR_EXPRESSION "FAIL"
)

set_tests_properties(cxx_test PROPERTIES
  DISABLED TRUE
  FIXTURES_SETUP "Setup"
  COST 10
)

add_test(NAME fixture_test COMMAND test_runner)
set_tests_properties(fixture_test PROPERTIES
  FIXTURES_REQUIRED "Setup"
  FIXTURES_CLEANUP "Cleanup"
)

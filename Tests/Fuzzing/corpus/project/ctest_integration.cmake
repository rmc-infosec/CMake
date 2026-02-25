enable_testing()
enable_language(C)

add_executable(test_runner test_runner.c)

add_test(NAME basic_test COMMAND test_runner --basic)
add_test(NAME advanced_test COMMAND test_runner --advanced)

set_tests_properties(basic_test PROPERTIES
  TIMEOUT 30
  LABELS "unit;fast"
  PASS_REGULAR_EXPRESSION "PASSED"
  FAIL_REGULAR_EXPRESSION "FAILED"
)

set_tests_properties(advanced_test PROPERTIES
  TIMEOUT 120
  LABELS "integration;slow"
  DEPENDS basic_test
  ENVIRONMENT "TEST_MODE=advanced"
)

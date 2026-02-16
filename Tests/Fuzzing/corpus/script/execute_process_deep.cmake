# Deep execute_process coverage
execute_process(
  COMMAND echo "first"
  COMMAND tr "a-z" "A-Z"
  OUTPUT_VARIABLE piped
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Piped: ${piped}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E echo "cmake echo"
  OUTPUT_VARIABLE cmake_out
  ERROR_VARIABLE cmake_err
  RESULT_VARIABLE cmake_rc
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
  TIMEOUT 10
  WORKING_DIRECTORY "/tmp"
  ENCODING UTF-8
)
message(STATUS "CMake echo: ${cmake_out}, rc=${cmake_rc}")

# Multiple commands in sequence
execute_process(
  COMMAND echo "hello world"
  COMMAND wc -w
  OUTPUT_VARIABLE word_count
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Words: ${word_count}")

# Command error handling
execute_process(
  COMMAND false
  RESULT_VARIABLE fail_rc
  ERROR_QUIET
)
message(STATUS "False returned: ${fail_rc}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E true
  COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
  COMMAND echo "test input"
  OUTPUT_FILE "/tmp/cmake_fuzz_exec_out.txt"
)
file(READ "/tmp/cmake_fuzz_exec_out.txt" exec_content)
message(STATUS "From file: ${exec_content}")
file(REMOVE "/tmp/cmake_fuzz_exec_out.txt")

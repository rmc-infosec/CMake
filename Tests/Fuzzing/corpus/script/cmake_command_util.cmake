# cmake -E utility commands via execute_process
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E make_directory "/tmp/cmake_fuzz_util"
  RESULT_VARIABLE rc
)

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E echo "test output"
  OUTPUT_VARIABLE out
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "Echo: ${out}")

file(WRITE "/tmp/cmake_fuzz_util/test.txt" "content\n")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E md5sum "/tmp/cmake_fuzz_util/test.txt"
  OUTPUT_VARIABLE md5out
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "MD5: ${md5out}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E sha256sum "/tmp/cmake_fuzz_util/test.txt"
  OUTPUT_VARIABLE sha256out
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(STATUS "SHA256: ${sha256out}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E copy "/tmp/cmake_fuzz_util/test.txt" "/tmp/cmake_fuzz_util/test_copy.txt"
)

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E rename "/tmp/cmake_fuzz_util/test_copy.txt" "/tmp/cmake_fuzz_util/test_renamed.txt"
)

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E compare_files "/tmp/cmake_fuzz_util/test.txt" "/tmp/cmake_fuzz_util/test_renamed.txt"
  RESULT_VARIABLE cmp_rc
)
message(STATUS "Compare: ${cmp_rc}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E rm -rf "/tmp/cmake_fuzz_util"
)

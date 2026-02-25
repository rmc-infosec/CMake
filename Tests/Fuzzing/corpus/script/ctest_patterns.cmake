# Patterns used in CTest scripts (subset that works in -P mode)

set(CTEST_PROJECT_NAME "FuzzProject")
set(CTEST_BUILD_NAME "Linux-GCC")
set(CTEST_SITE "fuzzer.test.com")

set(CTEST_SOURCE_DIRECTORY "/tmp/cmake_fuzz_ctest_src")
set(CTEST_BINARY_DIRECTORY "/tmp/cmake_fuzz_ctest_bin")
set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
set(CTEST_BUILD_CONFIGURATION "Release")

message(STATUS "Project: ${CTEST_PROJECT_NAME}")
message(STATUS "Build: ${CTEST_BUILD_NAME}")
message(STATUS "Site: ${CTEST_SITE}")
message(STATUS "Source: ${CTEST_SOURCE_DIRECTORY}")
message(STATUS "Binary: ${CTEST_BINARY_DIRECTORY}")

# Test properties patterns
set(test_list "test1;test2;test3;test4;test5")
foreach(test ${test_list})
  set(${test}_TIMEOUT 300)
  set(${test}_LABELS "unit;fast")
  set(${test}_WILL_FAIL FALSE)
  set(${test}_COST 1.0)
endforeach()

# Filter simulation
set(test_regex "test[1-3]")
foreach(test ${test_list})
  if(test MATCHES "${test_regex}")
    message(STATUS "Matched: ${test}")
  endif()
endforeach()

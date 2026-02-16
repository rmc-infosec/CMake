include_guard(GLOBAL)

set(INCLUDED_VAR "from_include_guard")

# Test include with various options
set(CMAKE_MODULE_PATH "/tmp")
file(WRITE "/tmp/test_module.cmake" "set(MODULE_VAR \"loaded\")\n")
include("/tmp/test_module.cmake" OPTIONAL RESULT_VARIABLE include_result)
message(STATUS "Include result: ${include_result}")
message(STATUS "Module var: ${MODULE_VAR}")

include("/tmp/nonexistent.cmake" OPTIONAL RESULT_VARIABLE missing_result)
message(STATUS "Missing: ${missing_result}")

file(REMOVE "/tmp/test_module.cmake")

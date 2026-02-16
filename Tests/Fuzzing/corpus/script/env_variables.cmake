set(ENV{CMAKE_FUZZ_TEST1} "value1")
set(ENV{CMAKE_FUZZ_TEST2} "value2")
set(ENV{CMAKE_FUZZ_PATH} "/usr/bin:/usr/local/bin")

message(STATUS "PATH=$ENV{PATH}")
message(STATUS "HOME=$ENV{HOME}")
message(STATUS "Test1=$ENV{CMAKE_FUZZ_TEST1}")
message(STATUS "Test2=$ENV{CMAKE_FUZZ_TEST2}")

if(DEFINED ENV{HOME})
  message(STATUS "HOME is defined")
endif()

if(NOT DEFINED ENV{NONEXISTENT_FUZZ_VAR})
  message(STATUS "NONEXISTENT not defined")
endif()

# Use env var in string operations
string(LENGTH "$ENV{CMAKE_FUZZ_PATH}" path_len)
message(STATUS "Path length: ${path_len}")

# Nested variable references
set(var_name "CMAKE_FUZZ_TEST1")
set(indirect_val "$ENV{${var_name}}")
message(STATUS "Indirect: ${indirect_val}")

unset(ENV{CMAKE_FUZZ_TEST1})
unset(ENV{CMAKE_FUZZ_TEST2})
unset(ENV{CMAKE_FUZZ_PATH})

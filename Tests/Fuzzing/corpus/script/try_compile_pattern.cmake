# Patterns from try_compile/try_run (script-mode compatible parts)
# These won't actually compile but exercise the argument parsing

set(CMAKE_C_COMPILER_ID "GNU")
set(CMAKE_CXX_COMPILER_ID "GNU")
set(CMAKE_C_COMPILER_VERSION "13.2.0")

# Simulate version comparison patterns used in try_compile
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "10.0")
    set(HAS_MODERN_GCC TRUE)
  endif()
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(HAS_CLANG TRUE)
elseif(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
  set(HAS_MSVC TRUE)
endif()

# Feature check result caching pattern
set(CHECK_RESULTS "")
foreach(feature HAS_THREADS HAS_ATOMIC HAS_FILESYSTEM HAS_COROUTINES)
  set(${feature} TRUE)
  list(APPEND CHECK_RESULTS "${feature}=${${feature}}")
endforeach()
message(STATUS "Features: ${CHECK_RESULTS}")

# Write source file for try_compile
file(WRITE "/tmp/cmake_fuzz_try.c" [[
#include <stdio.h>
int main(void) {
    printf("Hello\n");
    return 0;
}
]])
file(REMOVE "/tmp/cmake_fuzz_try.c")

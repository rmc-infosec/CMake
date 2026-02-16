set(NAME "TestProject")
set(VERSION_MAJOR 1)
set(VERSION_MINOR 2)
set(VERSION_PATCH 3)

file(CONFIGURE
  OUTPUT "/tmp/cmake_fuzz_configured.txt"
  CONTENT "Project: @NAME@\nVersion: @VERSION_MAJOR@.@VERSION_MINOR@.@VERSION_PATCH@\n"
  @ONLY
)
file(READ "/tmp/cmake_fuzz_configured.txt" configured)
message(STATUS "${configured}")

file(CONFIGURE
  OUTPUT "/tmp/cmake_fuzz_configured2.txt"
  CONTENT "Name=${NAME} Escape=\\@NOT_A_VAR@\n"
  ESCAPE_QUOTES
)

file(REMOVE "/tmp/cmake_fuzz_configured.txt" "/tmp/cmake_fuzz_configured2.txt")

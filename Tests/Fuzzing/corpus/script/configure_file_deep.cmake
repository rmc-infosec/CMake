set(PROJECT_NAME "FuzzProject")
set(PROJECT_VERSION "1.2.3")
set(PROJECT_DESCRIPTION "A test project for fuzzing")
set(ENABLE_FEATURE_X TRUE)
set(MAX_BUFFER_SIZE 4096)
set(CONFIG_ITEMS "item1;item2;item3")

file(WRITE "/tmp/cmake_fuzz_cfgd.h.in" [[
#pragma once

// @PROJECT_NAME@ configuration
#define PROJECT_NAME "@PROJECT_NAME@"
#define PROJECT_VERSION "@PROJECT_VERSION@"
#define PROJECT_DESCRIPTION "@PROJECT_DESCRIPTION@"

#cmakedefine ENABLE_FEATURE_X
#cmakedefine01 ENABLE_FEATURE_Y
#cmakedefine MAX_BUFFER_SIZE @MAX_BUFFER_SIZE@

// Variable substitution
#define CONFIG_ITEMS "${CONFIG_ITEMS}"
]])

configure_file("/tmp/cmake_fuzz_cfgd.h.in" "/tmp/cmake_fuzz_cfgd.h" @ONLY)
file(READ "/tmp/cmake_fuzz_cfgd.h" configured_content)
message(STATUS "Configured:\n${configured_content}")

configure_file("/tmp/cmake_fuzz_cfgd.h.in" "/tmp/cmake_fuzz_cfgd2.h")
configure_file("/tmp/cmake_fuzz_cfgd.h.in" "/tmp/cmake_fuzz_cfgd3.h" ESCAPE_QUOTES @ONLY)
configure_file("/tmp/cmake_fuzz_cfgd.h.in" "/tmp/cmake_fuzz_cfgd4.h" NEWLINE_STYLE UNIX)

file(REMOVE
  "/tmp/cmake_fuzz_cfgd.h.in"
  "/tmp/cmake_fuzz_cfgd.h"
  "/tmp/cmake_fuzz_cfgd2.h"
  "/tmp/cmake_fuzz_cfgd3.h"
  "/tmp/cmake_fuzz_cfgd4.h"
)

# FetchContent-like patterns (script mode compatible parts)
include(FetchContent OPTIONAL)

# Simulate FetchContent patterns with set/if
set(FETCHCONTENT_BASE_DIR "/tmp/cmake_fuzz_fc")
set(FETCHCONTENT_FULLY_DISCONNECTED OFF)
set(FETCHCONTENT_QUIET ON)
set(FETCHCONTENT_UPDATES_DISCONNECTED OFF)

function(declare_dependency name)
  cmake_parse_arguments(ARG "" "GIT_REPOSITORY;GIT_TAG;URL;URL_HASH" "" ${ARGN})
  set(${name}_DECLARED TRUE PARENT_SCOPE)
  if(ARG_GIT_REPOSITORY)
    message(STATUS "Declared ${name}: ${ARG_GIT_REPOSITORY}@${ARG_GIT_TAG}")
  elseif(ARG_URL)
    message(STATUS "Declared ${name}: ${ARG_URL}")
  endif()
endfunction()

declare_dependency(googletest
  GIT_REPOSITORY "https://github.com/google/googletest.git"
  GIT_TAG "release-1.12.1"
)

declare_dependency(json
  URL "https://example.com/json-3.11.2.tar.gz"
  URL_HASH SHA256=d69f9deb6a75e2580465c6c4c5111b89c4dc2fa94e3a85fcd2ffcd9a143d9273
)

message(STATUS "gtest declared: ${googletest_DECLARED}")
message(STATUS "json declared: ${json_DECLARED}")

file(REMOVE_RECURSE "/tmp/cmake_fuzz_fc")

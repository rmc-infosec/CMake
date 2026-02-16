file(MAKE_DIRECTORY "/tmp/cmake_fuzz_glob/src")
file(MAKE_DIRECTORY "/tmp/cmake_fuzz_glob/src/sub")
file(WRITE "/tmp/cmake_fuzz_glob/src/a.cpp" "")
file(WRITE "/tmp/cmake_fuzz_glob/src/b.cpp" "")
file(WRITE "/tmp/cmake_fuzz_glob/src/a.h" "")
file(WRITE "/tmp/cmake_fuzz_glob/src/sub/c.cpp" "")
file(WRITE "/tmp/cmake_fuzz_glob/src/sub/d.h" "")

file(GLOB sources "/tmp/cmake_fuzz_glob/src/*.cpp")
message(STATUS "Sources: ${sources}")

file(GLOB headers RELATIVE "/tmp/cmake_fuzz_glob" "/tmp/cmake_fuzz_glob/src/*.h")
message(STATUS "Headers: ${headers}")

file(GLOB_RECURSE all_cpp "/tmp/cmake_fuzz_glob/src/*.cpp")
message(STATUS "All cpp: ${all_cpp}")

file(GLOB_RECURSE all_files
  RELATIVE "/tmp/cmake_fuzz_glob"
  FOLLOW_SYMLINKS
  "/tmp/cmake_fuzz_glob/src/*"
)
message(STATUS "All files: ${all_files}")

file(GLOB config_files
  LIST_DIRECTORIES false
  CONFIGURE_DEPENDS
  "/tmp/cmake_fuzz_glob/src/*.cpp"
)

file(REMOVE_RECURSE "/tmp/cmake_fuzz_glob")

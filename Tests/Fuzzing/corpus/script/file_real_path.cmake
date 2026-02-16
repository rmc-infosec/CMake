file(MAKE_DIRECTORY "/tmp/cmake_fuzz_rp/a/b/c")
file(WRITE "/tmp/cmake_fuzz_rp/a/b/c/file.txt" "content\n")

file(REAL_PATH "/tmp/cmake_fuzz_rp/a/b/c/../../../a/b/c/file.txt" resolved)
message(STATUS "Resolved: ${resolved}")

file(REAL_PATH "/tmp/cmake_fuzz_rp/a/b/./c/file.txt" resolved2)
message(STATUS "Resolved2: ${resolved2}")

file(REAL_PATH "b/c/file.txt" resolved3 BASE_DIRECTORY "/tmp/cmake_fuzz_rp/a")
message(STATUS "Resolved3: ${resolved3}")

# cmake_path equivalents
cmake_path(SET p "/tmp/cmake_fuzz_rp/a/b/../b/c/file.txt")
cmake_path(NORMAL_PATH p)
message(STATUS "Normalized path: ${p}")

cmake_path(SET rel "c/file.txt")
cmake_path(ABSOLUTE_PATH rel BASE_DIRECTORY "/tmp/cmake_fuzz_rp/a/b")
message(STATUS "Absolute: ${rel}")

file(REMOVE_RECURSE "/tmp/cmake_fuzz_rp")

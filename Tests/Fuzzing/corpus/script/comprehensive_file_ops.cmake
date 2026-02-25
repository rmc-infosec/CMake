# Comprehensive file operation coverage
file(MAKE_DIRECTORY "/tmp/cmake_fuzz_comp/a/b/c")
file(MAKE_DIRECTORY "/tmp/cmake_fuzz_comp/d")

# Write various types
file(WRITE "/tmp/cmake_fuzz_comp/text.txt" "line1\nline2\nline3\n")
file(WRITE "/tmp/cmake_fuzz_comp/empty.txt" "")
file(WRITE "/tmp/cmake_fuzz_comp/a/nested.txt" "nested content\n")

# Append
file(APPEND "/tmp/cmake_fuzz_comp/text.txt" "line4\nline5\n")

# Read with various options
file(READ "/tmp/cmake_fuzz_comp/text.txt" all)
file(READ "/tmp/cmake_fuzz_comp/text.txt" first10 LIMIT 10)
file(READ "/tmp/cmake_fuzz_comp/text.txt" from5 OFFSET 5 LIMIT 10)
file(STRINGS "/tmp/cmake_fuzz_comp/text.txt" lines)
file(STRINGS "/tmp/cmake_fuzz_comp/text.txt" long_lines LENGTH_MINIMUM 5)
file(STRINGS "/tmp/cmake_fuzz_comp/text.txt" short_lines LENGTH_MAXIMUM 3)
file(STRINGS "/tmp/cmake_fuzz_comp/text.txt" max2 LIMIT_COUNT 2)
file(STRINGS "/tmp/cmake_fuzz_comp/text.txt" regex_lines REGEX "line[24]")

# Hashes
file(MD5 "/tmp/cmake_fuzz_comp/text.txt" h1)
file(SHA1 "/tmp/cmake_fuzz_comp/text.txt" h2)
file(SHA224 "/tmp/cmake_fuzz_comp/text.txt" h3)
file(SHA256 "/tmp/cmake_fuzz_comp/text.txt" h4)
file(SHA384 "/tmp/cmake_fuzz_comp/text.txt" h5)
file(SHA512 "/tmp/cmake_fuzz_comp/text.txt" h6)
message(STATUS "Hashes: ${h1} ${h2}")

# Size
file(SIZE "/tmp/cmake_fuzz_comp/text.txt" sz)
message(STATUS "Size: ${sz}")

# Glob operations
file(GLOB txt_files "/tmp/cmake_fuzz_comp/*.txt")
file(GLOB_RECURSE all_txt "/tmp/cmake_fuzz_comp/**/*.txt")
file(GLOB_RECURSE all_files RELATIVE "/tmp/cmake_fuzz_comp" "/tmp/cmake_fuzz_comp/*")

message(STATUS "Glob: ${txt_files}")
message(STATUS "Recurse: ${all_txt}")
message(STATUS "Relative: ${all_files}")

# Cleanup
file(REMOVE_RECURSE "/tmp/cmake_fuzz_comp")

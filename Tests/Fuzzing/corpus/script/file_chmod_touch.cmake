file(WRITE "/tmp/cmake_fuzz_chmod.txt" "test\n")

file(CHMOD "/tmp/cmake_fuzz_chmod.txt"
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
              GROUP_READ GROUP_EXECUTE
              WORLD_READ
)

file(TOUCH "/tmp/cmake_fuzz_touch1.txt" "/tmp/cmake_fuzz_touch2.txt")
file(TOUCH_NOCREATE "/tmp/cmake_fuzz_touch1.txt")

file(MAKE_DIRECTORY "/tmp/cmake_fuzz_link_test")
file(WRITE "/tmp/cmake_fuzz_link_test/original.txt" "original\n")
file(CREATE_LINK "/tmp/cmake_fuzz_link_test/original.txt" "/tmp/cmake_fuzz_link_test/symlink.txt" SYMBOLIC)
file(CREATE_LINK "/tmp/cmake_fuzz_link_test/original.txt" "/tmp/cmake_fuzz_link_test/hardlink.txt" COPY_ON_ERROR)

file(REAL_PATH "/tmp/cmake_fuzz_link_test/symlink.txt" resolved)
message(STATUS "Resolved: ${resolved}")

file(READ_SYMLINK "/tmp/cmake_fuzz_link_test/symlink.txt" link_target)
message(STATUS "Link target: ${link_target}")

file(REMOVE_RECURSE "/tmp/cmake_fuzz_link_test")
file(REMOVE "/tmp/cmake_fuzz_chmod.txt" "/tmp/cmake_fuzz_touch1.txt" "/tmp/cmake_fuzz_touch2.txt")

file(WRITE "/tmp/cmake_fuzz_lock_target.txt" "locked content\n")

file(LOCK "/tmp/cmake_fuzz_lock_target.txt" GUARD FUNCTION TIMEOUT 5)
file(READ "/tmp/cmake_fuzz_lock_target.txt" content)
message(STATUS "Locked content: ${content}")

file(LOCK "/tmp/cmake_fuzz_lock_target.txt" RELEASE)

file(LOCK "/tmp/cmake_fuzz_lock_target.txt" GUARD FILE RESULT_VARIABLE lock_result TIMEOUT 1)
message(STATUS "Lock result: ${lock_result}")
file(LOCK "/tmp/cmake_fuzz_lock_target.txt" RELEASE)

file(LOCK "/tmp/cmake_fuzz_lock_target.txt" DIRECTORY GUARD PROCESS TIMEOUT 2)
file(LOCK "/tmp/cmake_fuzz_lock_target.txt" RELEASE)

file(REMOVE "/tmp/cmake_fuzz_lock_target.txt")

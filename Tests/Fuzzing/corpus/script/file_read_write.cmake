# Deep file READ/WRITE coverage
file(WRITE "/tmp/cmake_fuzz_rw.txt" "Hello World\nSecond Line\nThird Line\n")
file(APPEND "/tmp/cmake_fuzz_rw.txt" "Fourth Line\n")

file(READ "/tmp/cmake_fuzz_rw.txt" content)
message(STATUS "Full: ${content}")

file(READ "/tmp/cmake_fuzz_rw.txt" partial LIMIT 11)
message(STATUS "Partial: ${partial}")

file(READ "/tmp/cmake_fuzz_rw.txt" offset_read OFFSET 6)
message(STATUS "Offset: ${offset_read}")

file(READ "/tmp/cmake_fuzz_rw.txt" hex_content HEX)
message(STATUS "Hex: ${hex_content}")

# WRITE with generator expressions (in script mode, these are literal)
file(WRITE "/tmp/cmake_fuzz_rw2.bin" "binary\x00data\x01\x02\xff")
file(SIZE "/tmp/cmake_fuzz_rw2.bin" binsize)
message(STATUS "Binary size: ${binsize}")

# Temp file
string(RANDOM LENGTH 8 rand_name)
file(WRITE "/tmp/cmake_fuzz_${rand_name}.tmp" "temporary\n")
file(GLOB temp_files "/tmp/cmake_fuzz_${rand_name}*")
message(STATUS "Temp: ${temp_files}")

file(REMOVE "/tmp/cmake_fuzz_rw.txt" "/tmp/cmake_fuzz_rw2.bin")
file(REMOVE ${temp_files})

# file(DOWNLOAD/UPLOAD) patterns - using invalid URLs to test parsing code paths
# The commands will fail but still exercise the parsing and setup code

set(download_url "file:///tmp/cmake_fuzz_dl_source.txt")
file(WRITE "/tmp/cmake_fuzz_dl_source.txt" "download content\n")

file(DOWNLOAD
  "${download_url}"
  "/tmp/cmake_fuzz_dl_dest.txt"
  STATUS dl_status
  TIMEOUT 5
  INACTIVITY_TIMEOUT 3
  SHOW_PROGRESS
)
list(GET dl_status 0 dl_code)
list(GET dl_status 1 dl_msg)
message(STATUS "Download: code=${dl_code} msg=${dl_msg}")

file(DOWNLOAD
  "${download_url}"
  "/tmp/cmake_fuzz_dl_dest2.txt"
  EXPECTED_HASH SHA256=0000000000000000000000000000000000000000000000000000000000000000
  STATUS dl_status2
  TIMEOUT 5
)

file(REMOVE
  "/tmp/cmake_fuzz_dl_source.txt"
  "/tmp/cmake_fuzz_dl_dest.txt"
  "/tmp/cmake_fuzz_dl_dest2.txt"
)

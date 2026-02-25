find_program(PROG_RESULT "cmake" PATHS /usr/bin /usr/local/bin)
find_program(PROG2 NAMES "gcc" "cc" "clang"
  HINTS /usr/bin
  NO_DEFAULT_PATH
  NO_CMAKE_PATH
)
find_program(PROG3 "sh"
  PATHS /bin /usr/bin
  DOC "Shell program"
  NO_CACHE
)
message(STATUS "Programs: ${PROG_RESULT} ${PROG2} ${PROG3}")

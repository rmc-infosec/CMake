find_file(RESULT_VAR "stdio.h" PATHS /usr/include /usr/local/include)
find_file(RESULT2 "stdlib.h"
  PATHS /usr/include
  PATH_SUFFIXES include inc
  NO_DEFAULT_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_CMAKE_PATH
  NO_SYSTEM_ENVIRONMENT_PATH
  NO_CMAKE_SYSTEM_PATH
)
find_file(RESULT3 NAMES "math.h" "cmath" HINTS /usr/include DOC "Math header")
message(STATUS "Found: ${RESULT_VAR} ${RESULT2} ${RESULT3}")

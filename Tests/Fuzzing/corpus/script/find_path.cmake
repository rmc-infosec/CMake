find_path(PATH_RESULT "stdio.h" PATHS /usr/include /usr/local/include)
find_path(PATH2 NAMES "limits.h" "climits"
  HINTS /usr/include
  PATH_SUFFIXES include
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
find_path(PATH3 "unistd.h" DOC "POSIX header path" REQUIRED)
message(STATUS "Paths: ${PATH_RESULT} ${PATH2} ${PATH3}")

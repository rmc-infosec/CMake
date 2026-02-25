find_library(LIB_RESULT "m" PATHS /usr/lib /usr/local/lib)
find_library(LIB2 NAMES "pthread" "pthreads"
  HINTS /usr/lib /lib
  PATH_SUFFIXES lib lib64
  NO_DEFAULT_PATH
)
find_library(LIB3 "c" NAMES_PER_DIR DOC "C library")
message(STATUS "Libs: ${LIB_RESULT} ${LIB2} ${LIB3}")

find_package(Threads)
find_package(PkgConfig QUIET)

if(Threads_FOUND)
  message(STATUS "Threads found: ${CMAKE_THREAD_LIBS_INIT}")
endif()

# find_* commands exercise different search strategies
find_program(PYTHON_EXE NAMES python3 python)
find_library(MATH_LIB m)
find_path(STDIO_INCLUDE NAMES stdio.h)
find_file(NULL_DEV NAMES null PATHS /dev NO_DEFAULT_PATH)

# Exported config-mode package
find_package(NonExistent QUIET CONFIG)
if(NOT NonExistent_FOUND)
  message(STATUS "NonExistent not found (expected)")
endif()

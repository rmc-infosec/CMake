option(MY_OPTION "A test option" ON)
option(ANOTHER_OPT "Another option" OFF)
message(STATUS "MY_OPTION=${MY_OPTION}")
message(STATUS "ANOTHER_OPT=${ANOTHER_OPT}")

set(MY_CACHE_VAR "default" CACHE STRING "A cache variable")
set(MY_BOOL_VAR ON CACHE BOOL "A bool cache var")
set(MY_PATH_VAR "/usr/local" CACHE PATH "A path cache var")
set(MY_FILEPATH_VAR "/usr/bin/cmake" CACHE FILEPATH "A filepath cache var")
set(MY_INTERNAL "hidden" CACHE INTERNAL "Internal var")

mark_as_advanced(MY_CACHE_VAR)
mark_as_advanced(CLEAR MY_CACHE_VAR)
mark_as_advanced(FORCE MY_BOOL_VAR MY_PATH_VAR)

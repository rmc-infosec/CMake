# Cache variable operations
set(CACHE_STR "hello" CACHE STRING "String cache var")
set(CACHE_BOOL ON CACHE BOOL "Bool cache var")
set(CACHE_PATH "/usr/local" CACHE PATH "Path cache var")
set(CACHE_FP "/usr/bin/cmake" CACHE FILEPATH "Filepath cache var")
set(CACHE_INTERNAL "hidden" CACHE INTERNAL "Internal cache var")

# Force overwrite
set(CACHE_STR "forced" CACHE STRING "Forced" FORCE)
message(STATUS "Forced: ${CACHE_STR}")

# Local shadow
set(CACHE_STR "local_shadow")
message(STATUS "Shadowed: ${CACHE_STR}")
unset(CACHE_STR)
message(STATUS "After unset local: ${CACHE_STR}")
unset(CACHE_STR CACHE)

# set_property on CACHE
set(MY_CACHED "value" CACHE STRING "test")
set_property(CACHE MY_CACHED PROPERTY HELPSTRING "Updated help")
set_property(CACHE MY_CACHED PROPERTY TYPE FILEPATH)
set_property(CACHE MY_CACHED PROPERTY STRINGS "opt1;opt2;opt3")
set_property(CACHE MY_CACHED PROPERTY ADVANCED TRUE)

get_property(help CACHE MY_CACHED PROPERTY HELPSTRING)
get_property(type CACHE MY_CACHED PROPERTY TYPE)
message(STATUS "Cache help: ${help}, type: ${type}")
unset(MY_CACHED CACHE)

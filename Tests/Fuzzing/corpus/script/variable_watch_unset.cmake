function(watch_callback var access value current_list_file stack)
  message(STATUS "Watch: ${var} ${access} ${value}")
endfunction()

set(WATCHED_VAR "initial")
variable_watch(WATCHED_VAR watch_callback)
set(WATCHED_VAR "modified")
message(STATUS "Watched: ${WATCHED_VAR}")

# Unset
set(TO_UNSET "exists")
message(STATUS "Before unset: ${TO_UNSET}")
unset(TO_UNSET)
message(STATUS "After unset: ${TO_UNSET}")

set(CACHE_VAR "cached" CACHE STRING "test")
unset(CACHE_VAR CACHE)

set(ENV{MY_FUZZ_VAR} "env_value")
message(STATUS "Env: $ENV{MY_FUZZ_VAR}")
unset(ENV{MY_FUZZ_VAR})
message(STATUS "Env after unset: $ENV{MY_FUZZ_VAR}")

site_name(SITE)
message(STATUS "Site: ${SITE}")

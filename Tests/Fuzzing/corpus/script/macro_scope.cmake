macro(my_macro arg1 arg2)
  message(STATUS "Macro: ${arg1} ${arg2}")
  message(STATUS "ARGC=${ARGC} ARGV=${ARGV} ARGN=${ARGN}")
  set(MACRO_RESULT "${arg1}_${arg2}")
endmacro()

my_macro("hello" "world")
message(STATUS "After macro: ${MACRO_RESULT}")

# Macro vs function scope difference
set(BEFORE "original")
macro(modify_var)
  set(BEFORE "changed_by_macro")
endmacro()
modify_var()
message(STATUS "After macro modify: ${BEFORE}")

function(modify_var_func)
  set(BEFORE "changed_by_func")
endfunction()
modify_var_func()
message(STATUS "After func modify: ${BEFORE}")

# Nested macro/function
macro(outer_macro X)
  function(inner_func Y)
    message(STATUS "Inner: ${Y}, Outer: ${X}")
  endfunction()
  inner_func("${X}_inner")
endmacro()
outer_macro("test")

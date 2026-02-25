# Line continuation patterns
set(LONG_VAR \
  "This is a very long value \
that spans multiple lines \
using line continuations")
message(STATUS "Long: ${LONG_VAR}")

message(\
  STATUS \
  "Continued message command" \
)

if(\
  TRUE \
  AND \
  TRUE \
)
  message(STATUS "Continued if")
endif()

set(multiline_list
  item1
  item2
  item3
  item4
  item5
)
message(STATUS "List: ${multiline_list}")

function(multi_arg_func \
  arg1 \
  arg2 \
  arg3 \
)
  message(STATUS "${arg1} ${arg2} ${arg3}")
endfunction()

multi_arg_func("a" "b" "c")

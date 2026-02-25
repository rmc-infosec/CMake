# More cmake_language patterns
cmake_language(EVAL CODE "
  function(dynamic_func x)
    math(EXPR result \"${x} * 2\")
    message(STATUS \"Dynamic result: \${result}\")
  endfunction()
  dynamic_func(21)
")

# Build command strings dynamically
set(commands
  "set(A 1)"
  "set(B 2)"
  "math(EXPR C \"\${A} + \${B}\")"
  "message(STATUS \"Sum: \${C}\")"
)
foreach(cmd ${commands})
  cmake_language(EVAL CODE "${cmd}")
endforeach()

# Get call IDs for deferred calls
cmake_language(DEFER CALL message STATUS "Deferred 1")
cmake_language(DEFER CALL message STATUS "Deferred 2")
cmake_language(DEFER ID named_call CALL message STATUS "Named deferred")
cmake_language(DEFER GET_CALL_IDS all_ids)
message(STATUS "Deferred IDs: ${all_ids}")
cmake_language(DEFER CANCEL_CALL named_call)
cmake_language(DEFER GET_CALL_IDS remaining_ids)
message(STATUS "Remaining IDs: ${remaining_ids}")

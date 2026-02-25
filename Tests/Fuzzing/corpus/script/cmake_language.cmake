function(my_callback)
  message(STATUS "Callback invoked with: ${ARGN}")
endfunction()

cmake_language(CALL message STATUS "Called via cmake_language")
cmake_language(EVAL CODE "
  set(DYNAMIC_VAR \"dynamic_value\")
  message(STATUS \"Eval: \${DYNAMIC_VAR}\")
")

set(cmd_name "message")
cmake_language(CALL ${cmd_name} STATUS "Dynamic call")

cmake_language(EVAL CODE [[
  foreach(i RANGE 3)
    math(EXPR sq "${i} * ${i}")
    message(STATUS "Square of ${i} = ${sq}")
  endforeach()
]])

cmake_language(DEFER CALL message STATUS "Deferred message")
cmake_language(DEFER ID my_defer CALL message STATUS "Named deferred")
cmake_language(DEFER GET_CALL_IDS ids)
cmake_language(DEFER CANCEL_CALL my_defer)

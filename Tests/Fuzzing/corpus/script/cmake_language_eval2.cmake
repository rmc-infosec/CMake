cmake_language(EVAL CODE "
  set(dynamic_var \"hello from eval\")
  message(STATUS \"eval: \${dynamic_var}\")
")

function(create_function name body)
  cmake_language(EVAL CODE "
    function(${name})
      ${body}
    endfunction()
  ")
endfunction()

create_function(greet "message(STATUS \"Hello!\")")
greet()

set(cmd_name "message")
cmake_language(CALL ${cmd_name} STATUS "called via cmake_language(CALL)")

cmake_language(GET_MESSAGE_LOG_LEVEL log_level)
message(STATUS "log level: ${log_level}")

set(commands string list math)
foreach(cmd IN LISTS commands)
  cmake_language(CALL ${cmd} LENGTH "test" result)
  message(STATUS "${cmd} result: ${result}")
endforeach()

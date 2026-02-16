# Test error handling paths in CMake

# message with different types
message(STATUS "Status message")
message(WARNING "Warning message")
message(AUTHOR_WARNING "Author warning")
message(DEPRECATION "Deprecated feature")
message(NOTICE "Notice message")
message(VERBOSE "Verbose message")
message(DEBUG "Debug message")
message(TRACE "Trace message")
message("Default message")

# Invalid variable references (should not crash)
set(result "${UNDEFINED_VAR}")
message(STATUS "Undefined: '${result}'")

# Nested undefined
set(result2 "${${UNDEFINED_INNER}}")
message(STATUS "Nested undefined: '${result2}'")

# Empty operations
set(empty_list "")
list(LENGTH empty_list elen)
message(STATUS "Empty length: ${elen}")

string(LENGTH "" slen)
message(STATUS "Empty string length: ${slen}")

# Math edge cases
math(EXPR r1 "0 + 0")
math(EXPR r2 "2147483647 + 0")
math(EXPR r3 "-1")
math(EXPR r4 "0xFF" OUTPUT_FORMAT HEXADECIMAL)
message(STATUS "Math: ${r1} ${r2} ${r3} ${r4}")

# Type coercion in if()
set(val "notABool")
if(val)
  message(STATUS "Truthy non-bool")
endif()
set(val2 "0")
if(NOT val2)
  message(STATUS "Falsy zero string")
endif()
set(val3 "OFF")
if(NOT val3)
  message(STATUS "Falsy OFF")
endif()
set(val4 "YES")
if(val4)
  message(STATUS "Truthy YES")
endif()

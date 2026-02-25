# Unquoted argument edge cases
set(A a)
set(B b)

# Unquoted with semicolons creates lists
set(mylist a;b;c;d)
list(LENGTH mylist len)
message(STATUS "Unquoted list: ${len} items")

# Nested variable expansion
set(VAR_NAME "RESULT")
set(RESULT "the_value")
message(STATUS "Indirect: ${${VAR_NAME}}")

# Generator-like patterns in set
set(items)
foreach(i RANGE 5)
  list(APPEND items "item_${i}")
endforeach()
message(STATUS "Generated: ${items}")

# Parentheses in if() with unquoted
set(X "1")
set(Y "2")
if(X AND (Y OR FALSE))
  message(STATUS "Parens in if")
endif()

# Mixed quoted/unquoted
set(path /usr/local/bin)
set(qpath "/usr/local/bin")
if(path STREQUAL qpath)
  message(STATUS "Equal paths")
endif()

# Command with many arguments
message(STATUS
  "arg1"
  "arg2"
  "arg3"
  arg4
  arg5
)

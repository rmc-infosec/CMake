function(early_return val)
  if(NOT val)
    message(STATUS "No value, returning early")
    return()
  endif()
  message(STATUS "Value: ${val}")
endfunction()

early_return("")
early_return("hello")

function(return_propagate)
  set(A "valueA")
  set(B "valueB")
  return(PROPAGATE A B)
endfunction()

return_propagate()
message(STATUS "Propagated A=${A} B=${B}")

macro(macro_return)
  message(STATUS "Before return in macro")
  return()
  message(STATUS "After return - should not see this in calling function")
endmacro()

function(call_macro_return)
  macro_return()
  message(STATUS "After macro_return call - should not see this")
endfunction()

# Don't call call_macro_return from top level as return() would exit script

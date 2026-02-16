# PARENT_SCOPE propagation
function(set_in_parent var val)
  set(${var} "${val}" PARENT_SCOPE)
endfunction()

function(test_parent_scope)
  set_in_parent(RESULT "from_child")
  # Note: RESULT is not set here yet
  message(STATUS "In test: RESULT='${RESULT}'")
endfunction()

test_parent_scope()
message(STATUS "After test: RESULT='${RESULT}'")

# CACHE variable
set(CACHE_VAR "cache_val" CACHE STRING "A cache var")
set(CACHE_VAR "local_val")
message(STATUS "Local shadows cache: ${CACHE_VAR}")
unset(CACHE_VAR)
message(STATUS "After unset local, cache shows: ${CACHE_VAR}")
unset(CACHE_VAR CACHE)

# Scope with foreach
set(outer_var "original")
foreach(i RANGE 2)
  set(outer_var "${outer_var}_${i}")
endforeach()
message(STATUS "After foreach: ${outer_var}")

# Scope with block
block(PROPAGATE propagated_var)
  set(propagated_var "from_block")
  set(not_propagated "from_block")
endblock()
message(STATUS "Propagated: ${propagated_var}")
message(STATUS "Not propagated: ${not_propagated}")

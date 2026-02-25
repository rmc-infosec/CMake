set(OUTER "outer_value")

block()
  set(INNER "inner_value")
  message(STATUS "Inside block: OUTER=${OUTER} INNER=${INNER}")
endblock()

message(STATUS "After block: INNER=${INNER}")

block(SCOPE_FOR VARIABLES)
  set(SCOPED_VAR "scoped")
  set(OUTER "modified_in_block")
endblock()
message(STATUS "Scoped: OUTER=${OUTER}")

block(SCOPE_FOR POLICIES)
  cmake_policy(SET CMP0054 OLD)
endblock()

block(PROPAGATE OUTER)
  set(OUTER "propagated_value")
  set(TEMP "temp_value")
endblock()
message(STATUS "Propagated: OUTER=${OUTER} TEMP=${TEMP}")

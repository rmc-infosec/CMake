message(STATUS [=[This is a bracket argument
  with newlines and "quotes" and ${not_a_variable}
  and @not_a_subst@ and \not\an\escape
  special chars: ()#"\ ${}
]=])

message(STATUS [==[
  Double bracket: ]=] still in argument
  Deeply nested brackets
]==])

set(raw_code [=[
  function(test)
    message("${ARGN}")
  endfunction()
]=])
message(STATUS "Raw: ${raw_code}")

# Bracket comments
#[=[
  This is a bracket comment
  It can span multiple lines
  and contain # characters
  and [=[ nested brackets ]=]
]=]

set(multiline_var [[
line 1
line 2
line 3
]])
message(STATUS "Multiline: ${multiline_var}")

# Empty bracket arg
message(STATUS [[]])
message(STATUS [=[]=])

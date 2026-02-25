string(REPEAT "=" 40 separator)
message(STATUS "${separator}")

string(REPEAT "abc" 0 empty_repeat)
message(STATUS "Empty repeat: '${empty_repeat}'")

string(REPEAT "x" 1 single)
message(STATUS "Single: ${single}")

# Padding simulation with REPEAT and LENGTH
string(REPEAT " " 20 spaces)
set(label "Name")
string(LENGTH "${label}" label_len)
math(EXPR pad_len "20 - ${label_len}")
if(pad_len GREATER 0)
  string(SUBSTRING "${spaces}" 0 ${pad_len} padding)
  message(STATUS "${label}${padding}: value")
endif()

# Large repeat
string(REPEAT "AB" 100 large)
string(LENGTH "${large}" large_len)
message(STATUS "Large length: ${large_len}")

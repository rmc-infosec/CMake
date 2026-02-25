# Variable indirection and dynamic names
set(PREFIX "MY")
set(SUFFIX "VAR")
set(MY_VAR "direct_value")

# Double dereference
set(${PREFIX}_${SUFFIX} "constructed_value")
message(STATUS "Constructed: ${MY_VAR}")

# Dynamic variable names in a loop
foreach(type STRING BOOL PATH FILEPATH)
  set(OPT_${type} "value_${type}")
endforeach()
message(STATUS "STRING=${OPT_STRING} BOOL=${OPT_BOOL}")

# Variable name from another variable
set(which "FIRST")
set(FIRST_VALUE "from_first")
set(SECOND_VALUE "from_second")
set(result "${${which}_VALUE}")
message(STATUS "Indirect: ${result}")

# Nested indirection
set(level1 "level2")
set(level2 "level3")
set(level3 "final_value")
message(STATUS "L1: ${level1}")
message(STATUS "L2: ${${level1}}")
message(STATUS "L3: ${${${level1}}}")

# Generator pattern
function(define_option name type default description)
  set(${name} "${default}" CACHE ${type} "${description}")
  message(STATUS "Option ${name} = ${${name}}")
endfunction()

define_option(FUZZ_ENABLE_X BOOL ON "Enable X")
define_option(FUZZ_PATH_Y PATH "/usr/local" "Path to Y")
define_option(FUZZ_STRING_Z STRING "hello" "String Z")

set(cmd_line "arg1 arg2 arg3 arg4")
separate_arguments(result UNIX_COMMAND "${cmd_line}")
message(STATUS "Unix parsed: ${result}")

set(win_cmd "arg1;arg2;arg3")
separate_arguments(win_result WINDOWS_COMMAND "arg1 \"arg two\" arg3")
message(STATUS "Windows parsed: ${win_result}")

separate_arguments(native_result NATIVE_COMMAND "one two three")
message(STATUS "Native: ${native_result}")

set(complex_cmd "  -DFOO=bar  -DBAZ=\"hello world\"  -DLIST=a\\;b\\;c  ")
separate_arguments(complex_result UNIX_COMMAND "${complex_cmd}")
message(STATUS "Complex: ${complex_result}")

message(STATUS "Status")
message(WARNING "Warn")
message(AUTHOR_WARNING "AuthorWarn")
message(DEPRECATION "Deprecated")
message(NOTICE "Notice")
message(VERBOSE "Verbose")
message(DEBUG "Debug")
message(TRACE "Trace")
message("NoLevel")
message(CHECK_START "Checking something")
message(CHECK_PASS "found")
message(CHECK_START "Checking other")
message(CHECK_FAIL "not found")

# message with multiple arguments
message(STATUS "Part1" " Part2" " Part3")

# Indented messages
list(APPEND CMAKE_MESSAGE_INDENT "  ")
message(STATUS "Indented")
list(APPEND CMAKE_MESSAGE_INDENT "  ")
message(STATUS "Double indented")
list(POP_BACK CMAKE_MESSAGE_INDENT)
message(STATUS "Back to single indent")
list(POP_BACK CMAKE_MESSAGE_INDENT)

# Context
list(APPEND CMAKE_MESSAGE_CONTEXT "fuzzer")
message(VERBOSE "With context")
list(APPEND CMAKE_MESSAGE_CONTEXT "deep")
message(VERBOSE "With deep context")
list(POP_BACK CMAKE_MESSAGE_CONTEXT)
list(POP_BACK CMAKE_MESSAGE_CONTEXT)

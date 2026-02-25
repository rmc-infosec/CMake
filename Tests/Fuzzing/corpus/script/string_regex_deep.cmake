set(text "Hello World 123 foo-bar_baz CamelCase")

string(REGEX MATCH "[0-9]+" num "${text}")
message(STATUS "Number: ${num}")

string(REGEX MATCHALL "[A-Za-z]+" words "${text}")
message(STATUS "Words: ${words}")

string(REGEX REPLACE "([a-z])([A-Z])" "\\1_\\2" snake "${text}")
message(STATUS "Snake: ${snake}")

# Complex regex patterns
set(email "user@example.com, admin@test.org, root@localhost")
string(REGEX MATCHALL "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]+" emails "${email}")
message(STATUS "Emails: ${emails}")

set(version_str "v1.2.3-rc1+build.42")
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" ver_match "${version_str}")
message(STATUS "Version: ${CMAKE_MATCH_0}")
message(STATUS "Major: ${CMAKE_MATCH_1}")
message(STATUS "Minor: ${CMAKE_MATCH_2}")
message(STATUS "Patch: ${CMAKE_MATCH_3}")

# String COMPARE
string(COMPARE LESS "abc" "def" result)
message(STATUS "abc < def: ${result}")
string(COMPARE GREATER "xyz" "abc" result)
message(STATUS "xyz > abc: ${result}")
string(COMPARE EQUAL "same" "same" result)
message(STATUS "same == same: ${result}")
string(COMPARE NOTEQUAL "a" "b" result)
message(STATUS "a != b: ${result}")
string(COMPARE LESS_EQUAL "abc" "abc" result)
message(STATUS "abc <= abc: ${result}")
string(COMPARE GREATER_EQUAL "def" "abc" result)
message(STATUS "def >= abc: ${result}")

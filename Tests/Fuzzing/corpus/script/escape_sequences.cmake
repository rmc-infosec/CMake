# Escape sequences in quoted strings
message(STATUS "Tab:\there")
message(STATUS "Newline:\nhere")
message(STATUS "Backslash: \\")
message(STATUS "Semicolons: a\;b\;c")
message(STATUS "Dollar: \${not_a_var}")
message(STATUS "Quote: \"quoted\"")

# Line continuation
message(STATUS \
  "This is a \
continued line")

set(multiline "line1\
line2\
line3")
message(STATUS "Multi: ${multiline}")

# Special characters in strings
set(special "!@#$%^&*()_+-={}|:<>?/.,';[]")
message(STATUS "Special: ${special}")

set(unicode "Hello \u00e9\u00e8\u00ea")
message(STATUS "Unicode: ${unicode}")

# Empty and whitespace strings
set(empty "")
set(space " ")
set(tabs "\t\t")
set(newlines "\n\n\n")
message(STATUS "Empty='${empty}' Space='${space}'")

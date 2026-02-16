# Torture-test regex patterns for string()
set(input "The quick brown fox jumps over the lazy dog 123-456-7890")

string(REGEX MATCH "[0-9]{3}-[0-9]{3}-[0-9]{4}" phone "${input}")
message(STATUS "Phone: ${phone}")

string(REGEX MATCHALL "[a-z]+" all_lower "${input}")
message(STATUS "Lower words: ${all_lower}")

string(REGEX MATCHALL "[A-Z][a-z]*" cap_words "${input}")
message(STATUS "Cap words: ${cap_words}")

# Greedy vs. patterns
set(html "<div>text1</div><div>text2</div>")
string(REGEX MATCHALL "<div>[^<]*</div>" divs "${html}")
message(STATUS "Divs: ${divs}")

# Alternation
set(mixed "cat 42 dog 99 fish 7")
string(REGEX MATCHALL "([a-z]+|[0-9]+)" tokens "${mixed}")
message(STATUS "Tokens: ${tokens}")

# Character classes
set(test_str "Hello, World! 123 @#$ abc_def")
string(REGEX MATCHALL "[[:alpha:]]+" alpha_words "${test_str}")
message(STATUS "Alpha: ${alpha_words}")
string(REGEX MATCHALL "[[:digit:]]+" digits "${test_str}")
message(STATUS "Digits: ${digits}")
string(REGEX MATCHALL "[[:alnum:]_]+" idents "${test_str}")
message(STATUS "Idents: ${idents}")

# Anchors
string(REGEX MATCH "^The" starts_with "${input}")
message(STATUS "Starts: ${starts_with}")
string(REGEX MATCH "[0-9]+$" ends_with "${input}")
message(STATUS "Ends: ${ends_with}")

# Backreferences in replacement
set(doubled "aabbccdd")
string(REGEX REPLACE "(.)\\1" "\\1" undup "${doubled}")
message(STATUS "Undup: ${undup}")

# Complex replacement
set(camel "thisIsCamelCase")
string(REGEX REPLACE "([a-z])([A-Z])" "\\1_\\2" snake_case "${camel}")
string(TOLOWER "${snake_case}" snake_case)
message(STATUS "Snake: ${snake_case}")

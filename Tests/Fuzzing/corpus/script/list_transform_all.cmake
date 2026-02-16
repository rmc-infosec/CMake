# Comprehensive list TRANSFORM coverage
set(items "  hello  " "  world  " "  cmake  ")

list(TRANSFORM items STRIP OUTPUT_VARIABLE stripped)
message(STATUS "Stripped: ${stripped}")

set(paths "src/main.cpp" "src/util.cpp" "lib/helper.cpp")
list(TRANSFORM paths REPLACE "^src/" "source/" OUTPUT_VARIABLE replaced)
message(STATUS "Replaced: ${replaced}")

set(names "alice" "bob" "charlie")
list(TRANSFORM names TOUPPER OUTPUT_VARIABLE upper)
list(TRANSFORM names TOLOWER OUTPUT_VARIABLE lower)
list(TRANSFORM names APPEND ".txt" OUTPUT_VARIABLE with_ext)
list(TRANSFORM names PREPEND "user_" OUTPUT_VARIABLE with_prefix)
message(STATUS "Upper: ${upper}")
message(STATUS "With ext: ${with_ext}")
message(STATUS "With prefix: ${with_prefix}")

# Selective transforms
set(mixed "a" "b" "c" "d" "e" "f")
list(TRANSFORM mixed TOUPPER AT 0 2 4 OUTPUT_VARIABLE at_even)
message(STATUS "At even: ${at_even}")

list(TRANSFORM mixed TOUPPER FOR 1 3 OUTPUT_VARIABLE for_range)
message(STATUS "For 1-3: ${for_range}")

list(TRANSFORM mixed TOUPPER FOR 0 5 2 OUTPUT_VARIABLE for_step)
message(STATUS "For step 2: ${for_step}")

list(TRANSFORM mixed TOUPPER REGEX "[ace]" OUTPUT_VARIABLE regex_match)
message(STATUS "Regex ace: ${regex_match}")

# GENEX_STRIP on list
set(genex_list "$<1:yes>" "plain" "$<TARGET:foo>")
list(TRANSFORM genex_list GENEX_STRIP OUTPUT_VARIABLE stripped_genex)
message(STATUS "Genex stripped: ${stripped_genex}")

# More list operations for coverage
set(items "apple" "apricot" "banana" "blueberry" "cherry" "cranberry")

list(FILTER items INCLUDE REGEX "^a" OUTPUT_VARIABLE a_items)
message(STATUS "A items: ${a_items}")

list(FILTER items EXCLUDE REGEX "berry$" OUTPUT_VARIABLE no_berry)
message(STATUS "No berry: ${no_berry}")

# SORT with all options
set(nums "10" "2" "1" "20" "3" "11")
list(SORT nums COMPARE NATURAL ORDER ASCENDING)
message(STATUS "Natural asc: ${nums}")

set(nums2 "10" "2" "1" "20" "3" "11")
list(SORT nums2 COMPARE STRING ORDER DESCENDING)
message(STATUS "String desc: ${nums2}")

set(nums3 "10" "2" "1" "20" "3" "11")
list(SORT nums3 COMPARE FILE_BASENAME)
message(STATUS "File basename: ${nums3}")

# Multiple ZIP_LISTS
set(first "A" "B" "C")
set(second "1" "2" "3")
set(third "x" "y" "z")
foreach(a b c IN ZIP_LISTS first second third)
  message(STATUS "Triple: ${a}${b}${c}")
endforeach()

# POP operations with multiple elements
set(stack "1" "2" "3" "4" "5" "6" "7" "8")
list(POP_BACK stack a b c)
message(STATUS "Popped3: ${a} ${b} ${c}, rest: ${stack}")
list(POP_FRONT stack d e)
message(STATUS "PoppedF2: ${d} ${e}, rest: ${stack}")

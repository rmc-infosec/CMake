set(mylist "apple" "banana" "cherry" "date" "elderberry" "fig" "grape")

# TRANSFORM operations
list(TRANSFORM mylist TOUPPER OUTPUT_VARIABLE upper_list)
message(STATUS "Upper: ${upper_list}")

list(TRANSFORM mylist TOLOWER OUTPUT_VARIABLE lower_list)
message(STATUS "Lower: ${lower_list}")

list(TRANSFORM mylist STRIP OUTPUT_VARIABLE stripped)

list(TRANSFORM mylist APPEND "_fruit" OUTPUT_VARIABLE appended)
message(STATUS "Appended: ${appended}")

list(TRANSFORM mylist PREPEND "item_" OUTPUT_VARIABLE prepended)
message(STATUS "Prepended: ${prepended}")

list(TRANSFORM mylist REPLACE "a" "A" OUTPUT_VARIABLE replaced)
message(STATUS "Replaced: ${replaced}")

# TRANSFORM with selectors
list(TRANSFORM mylist TOUPPER AT 0 2 4 OUTPUT_VARIABLE at_selected)
message(STATUS "AT selected: ${at_selected}")

list(TRANSFORM mylist TOUPPER FOR 1 3 OUTPUT_VARIABLE for_selected)
message(STATUS "FOR selected: ${for_selected}")

list(TRANSFORM mylist TOUPPER FOR 0 6 2 OUTPUT_VARIABLE for_step)
message(STATUS "FOR step: ${for_step}")

list(TRANSFORM mylist TOUPPER REGEX "^[a-c]" OUTPUT_VARIABLE regex_sel)
message(STATUS "Regex selected: ${regex_sel}")

# FILTER
list(FILTER mylist INCLUDE REGEX "^[a-d]")
message(STATUS "Filtered include: ${mylist}")

set(mylist2 "apple" "banana" "cherry" "date")
list(FILTER mylist2 EXCLUDE REGEX "^[bc]")
message(STATUS "Filtered exclude: ${mylist2}")

# SORT options
set(sortlist "3" "1" "4" "1" "5" "9" "2" "6")
list(SORT sortlist COMPARE NATURAL ORDER ASCENDING)
message(STATUS "Natural ascending: ${sortlist}")

set(sortlist2 "c" "a" "B" "d" "A")
list(SORT sortlist2 COMPARE STRING CASE INSENSITIVE)
message(STATUS "Case insensitive: ${sortlist2}")

# ZIP_LISTS
set(names "Alice" "Bob" "Charlie")
set(ages "30" "25" "35")
set(cities "NYC" "LA" "CHI")
foreach(n a c IN ZIP_LISTS names ages cities)
  message(STATUS "${n} age ${a} from ${c}")
endforeach()

# POP operations
set(stack "a" "b" "c" "d" "e")
list(POP_BACK stack last)
message(STATUS "Popped back: ${last}, remaining: ${stack}")
list(POP_FRONT stack first)
message(STATUS "Popped front: ${first}, remaining: ${stack}")
list(POP_BACK stack x y)
message(STATUS "Multi pop: ${x} ${y}")

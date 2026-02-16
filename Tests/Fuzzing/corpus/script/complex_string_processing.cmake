# Real-world string processing patterns
set(version_string "project-name-1.2.3-rc1-debug")

# Parse components
string(REGEX MATCH "^([a-z-]+)-([0-9]+\\.[0-9]+\\.[0-9]+)(-.+)?$" _match "${version_string}")
set(name "${CMAKE_MATCH_1}")
set(ver "${CMAKE_MATCH_2}")
set(suffix "${CMAKE_MATCH_3}")
message(STATUS "Name: ${name}, Version: ${ver}, Suffix: ${suffix}")

# Build path from components
set(components "usr" "local" "lib" "cmake" "project")
set(path "")
foreach(comp ${components})
  if(path)
    string(APPEND path "/${comp}")
  else()
    set(path "/${comp}")
  endif()
endforeach()
message(STATUS "Path: ${path}")

# CSV-like parsing
set(csv_line "name,age,city,\"quoted,field\",last")
string(REPLACE "," ";" csv_list "${csv_line}")
message(STATUS "CSV: ${csv_list}")

# Template expansion
set(template "Hello @NAME@, your score is @SCORE@ out of @TOTAL@")
set(NAME "Alice")
set(SCORE "95")
set(TOTAL "100")
string(CONFIGURE "${template}" result @ONLY)
message(STATUS "Template: ${result}")

# Multi-pattern replacement
set(input "foo_bar_baz_qux")
string(REPLACE "_" "-" dashed "${input}")
string(TOUPPER "${dashed}" upper_dashed)
string(REGEX REPLACE "-[A-Z]" "" first_word "${upper_dashed}")
message(STATUS "Processed: ${dashed} -> ${upper_dashed}")

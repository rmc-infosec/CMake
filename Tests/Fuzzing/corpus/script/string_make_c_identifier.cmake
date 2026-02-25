# string(MAKE_C_IDENTIFIER) coverage
string(MAKE_C_IDENTIFIER "my-var.name" id1)
message(STATUS "id1: ${id1}")

string(MAKE_C_IDENTIFIER "123-starts-with-number" id2)
message(STATUS "id2: ${id2}")

string(MAKE_C_IDENTIFIER "spaces in name" id3)
message(STATUS "id3: ${id3}")

string(MAKE_C_IDENTIFIER "ALREADY_VALID" id4)
message(STATUS "id4: ${id4}")

string(MAKE_C_IDENTIFIER "special!@#$%chars" id5)
message(STATUS "id5: ${id5}")

string(MAKE_C_IDENTIFIER "" id6)
message(STATUS "id6: '${id6}'")

string(MAKE_C_IDENTIFIER "a" id7)
message(STATUS "id7: ${id7}")

# Apply to path components
set(path "/usr/local/lib/cmake/my-package/MyPackageConfig.cmake")
cmake_path(GET path STEM LAST_ONLY stem)
string(MAKE_C_IDENTIFIER "${stem}" guard)
string(TOUPPER "${guard}" guard)
message(STATUS "Include guard: ${guard}_H")

string(ASCII 72 101 108 108 111 ascii_str)
message(STATUS "ASCII: ${ascii_str}")

string(HEX "Hello" hex_str)
message(STATUS "Hex: ${hex_str}")

string(CONFIGURE "@CMAKE_VERSION@ and ${CMAKE_COMMAND}" configured @ONLY)
message(STATUS "Configured: ${configured}")

string(REPEAT "ab" 5 repeated)
message(STATUS "Repeat: ${repeated}")

string(LENGTH "test string" len)
message(STATUS "Length: ${len}")

string(SUBSTRING "Hello World" 6 5 sub)
message(STATUS "Substr: ${sub}")

string(FIND "Hello World" "World" pos)
message(STATUS "Find: ${pos}")
string(FIND "Hello World" "xyz" pos2)
message(STATUS "Find missing: ${pos2}")
string(FIND "Hello World Hello" "Hello" rpos REVERSE)
message(STATUS "RFind: ${rpos}")

string(STRIP "  \t  trimmed  \n " stripped)
message(STATUS "Stripped: '${stripped}'")

string(TOLOWER "UPPER" lower)
string(TOUPPER "lower" upper)
message(STATUS "${lower} ${upper}")

# Patterns that exercise target-like property handling
# (adapted for script mode)
set(MY_SOURCES "main.cpp" "util.cpp" "helper.cpp")
set(MY_HEADERS "util.h" "helper.h")
set(MY_INCLUDE_DIRS "/usr/local/include" "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(MY_COMPILE_DEFS "NDEBUG" "PROJECT_VERSION=1")
set(MY_COMPILE_OPTS "-Wall" "-Wextra" "-Werror" "-O2")
set(MY_LINK_LIBS "pthread" "dl" "m")

list(LENGTH MY_SOURCES src_count)
message(STATUS "Sources: ${src_count}")

foreach(src ${MY_SOURCES})
  string(REGEX REPLACE "\\.cpp$" ".o" obj "${src}")
  list(APPEND MY_OBJECTS "${obj}")
endforeach()
message(STATUS "Objects: ${MY_OBJECTS}")

# Simulate target property aggregation
set(ALL_DEFS ${MY_COMPILE_DEFS})
list(APPEND ALL_DEFS "EXTRA_DEF=1")
list(REMOVE_DUPLICATES ALL_DEFS)

string(JOIN " " compile_flags ${MY_COMPILE_OPTS})
message(STATUS "Flags: ${compile_flags}")

# Interface vs private simulation
set(PUBLIC_HEADERS ${MY_HEADERS})
set(PRIVATE_SOURCES ${MY_SOURCES})
set(INTERFACE_DEFS "USE_MY_LIB")
message(STATUS "Public: ${PUBLIC_HEADERS}")
message(STATUS "Private: ${PRIVATE_SOURCES}")
message(STATUS "Interface: ${INTERFACE_DEFS}")

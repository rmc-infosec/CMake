enable_language(C)
add_library(mylib STATIC lib.c)

# file(GENERATE) exercises generator expressions in project context
file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/config_$<CONFIG>.h"
  CONTENT "#define BUILD_CONFIG \"$<CONFIG>\"\n#define TARGET_FILE \"$<TARGET_FILE:mylib>\"\n"
)

file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/target_info.txt"
  CONTENT "Name: $<TARGET_PROPERTY:mylib,NAME>\nType: $<TARGET_PROPERTY:mylib,TYPE>\n"
  CONDITION $<BOOL:TRUE>
)

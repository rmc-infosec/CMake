enable_language(C)

# Check if a simple C program compiles
include(CheckCSourceCompiles)
check_c_source_compiles("
  #include <stdio.h>
  int main() { printf(\"hello\"); return 0; }
" HAVE_PRINTF)

include(CheckCSourceRuns)

include(CheckIncludeFile)
check_include_file("stdint.h" HAVE_STDINT_H)
check_include_file("unistd.h" HAVE_UNISTD_H)

include(CheckTypeSize)
check_type_size("long long" SIZEOF_LONG_LONG)
check_type_size("void*" SIZEOF_VOID_P)

include(CheckSymbolExists)
check_symbol_exists(malloc "stdlib.h" HAVE_MALLOC)

enable_language(C)
add_library(myshared SHARED lib.c)
set_target_properties(myshared PROPERTIES
  VERSION 1.2.3
  SOVERSION 1
  POSITION_INDEPENDENT_CODE ON
)
target_compile_definitions(myshared PUBLIC MY_SHARED_API)

add_library(ext::lib STATIC IMPORTED)
set_target_properties(ext::lib PROPERTIES
  IMPORTED_LOCATION "/usr/lib/libext.a"
  INTERFACE_INCLUDE_DIRECTORIES "/usr/include/ext"
  INTERFACE_COMPILE_DEFINITIONS "USE_EXT=1"
)

enable_language(C)
add_executable(app main.c)
target_link_libraries(app PRIVATE ext::lib)

add_library(mylib STATIC lib.c util.c)

set_target_properties(mylib PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF
  POSITION_INDEPENDENT_CODE ON
  OUTPUT_NAME "mylib_custom"
  PREFIX "lib"
  SUFFIX ".a"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
  COMPILE_DEFINITIONS "FOO=1;BAR=2"
  COMPILE_OPTIONS "-Wall;-Wextra"
  LINK_LIBRARIES ""
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

get_target_property(out_name mylib OUTPUT_NAME)
get_target_property(c_std mylib C_STANDARD)
message(STATUS "Output: ${out_name}, C std: ${c_std}")

set_property(TARGET mylib PROPERTY FOLDER "Libraries")
set_property(TARGET mylib APPEND PROPERTY COMPILE_DEFINITIONS "BAZ=3")

get_property(defs TARGET mylib PROPERTY COMPILE_DEFINITIONS)
message(STATUS "Definitions: ${defs}")

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT mylib)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

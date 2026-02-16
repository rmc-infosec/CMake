add_library(static_lib STATIC lib.c)
add_library(shared_lib SHARED lib.c)
set_target_properties(shared_lib PROPERTIES POSITION_INDEPENDENT_CODE ON)
add_library(module_lib MODULE plugin.cpp)
add_library(object_lib OBJECT util.c helper.c)
add_library(interface_lib INTERFACE)
target_include_directories(interface_lib INTERFACE include)

add_executable(app main.c)
target_link_libraries(app PRIVATE static_lib shared_lib object_lib interface_lib)

add_library(My::static ALIAS static_lib)
add_executable(app2 main.cpp)
target_link_libraries(app2 PRIVATE My::static)

set_target_properties(static_lib PROPERTIES
  OUTPUT_NAME mystaticlib
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
set_target_properties(shared_lib PROPERTIES
  VERSION 1.2.3
  SOVERSION 1
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

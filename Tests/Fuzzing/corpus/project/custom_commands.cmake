enable_language(C)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/generated.h
  COMMAND ${CMAKE_COMMAND} -E echo "#define GENERATED 1" > ${CMAKE_CURRENT_BINARY_DIR}/generated.h
  COMMENT "Generating header"
  VERBATIM
)

add_custom_target(generate_header DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/generated.h)

add_executable(app main.c)
add_dependencies(app generate_header)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

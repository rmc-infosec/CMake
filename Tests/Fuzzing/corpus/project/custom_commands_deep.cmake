add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/generated.h
  COMMAND ${CMAKE_COMMAND} -E echo "#define GEN 1" > ${CMAKE_BINARY_DIR}/generated.h
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib.c
  COMMENT "Generating header"
  VERBATIM
)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/stamp.txt
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/stamp.txt
  BYPRODUCTS ${CMAKE_BINARY_DIR}/side_effect.txt
  COMMENT "Creating stamp"
)

add_custom_target(gen_all
  DEPENDS ${CMAKE_BINARY_DIR}/generated.h ${CMAKE_BINARY_DIR}/stamp.txt
)

add_library(mylib STATIC lib.c)
add_dependencies(mylib gen_all)
target_include_directories(mylib PRIVATE ${CMAKE_BINARY_DIR})

add_executable(app main.c)
target_link_libraries(app PRIVATE mylib)

add_custom_command(TARGET app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Build complete"
  COMMENT "Post-build step"
)

add_custom_command(TARGET app PRE_LINK
  COMMAND ${CMAKE_COMMAND} -E echo "Pre-link step"
)

enable_language(C)
add_library(mylib_real STATIC lib.c)
add_library(MyProject::mylib ALIAS mylib_real)

add_executable(app main.c)
target_link_libraries(app PRIVATE MyProject::mylib)

if(TARGET MyProject::mylib)
  message(STATUS "Alias target exists")
endif()

get_target_property(_type mylib_real TYPE)
message(STATUS "Library type: ${_type}")

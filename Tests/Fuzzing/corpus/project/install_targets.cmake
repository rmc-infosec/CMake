enable_language(C)
add_library(mylib STATIC lib.c)
add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE mylib)

install(TARGETS mylib myapp
  EXPORT MyProjectTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION include
)

install(FILES include/mylib.h DESTINATION include)
install(EXPORT MyProjectTargets
  FILE MyProjectTargets.cmake
  NAMESPACE MyProject::
  DESTINATION lib/cmake/MyProject
)

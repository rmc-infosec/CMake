add_library(mylib SHARED lib.cpp)
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE mylib)
set_target_properties(mylib PROPERTIES
  VERSION 1.2.3
  SOVERSION 1
  PUBLIC_HEADER "include/mylib.h"
)
target_include_directories(mylib PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_compile_definitions(mylib PUBLIC
  "$<$<CONFIG:Debug>:MYLIB_DEBUG>"
)
install(TARGETS mylib myapp
  EXPORT MyLibTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include
)
install(EXPORT MyLibTargets
  FILE MyLibTargets.cmake
  NAMESPACE MyLib::
  DESTINATION lib/cmake/MyLib
)

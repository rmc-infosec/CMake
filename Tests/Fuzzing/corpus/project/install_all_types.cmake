add_library(mylib STATIC lib.c)
add_library(myshared SHARED lib.c)
set_target_properties(myshared PROPERTIES
  VERSION 2.0.0
  SOVERSION 2
)
add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE mylib myshared)

install(TARGETS mylib myshared myapp
  EXPORT MyProjExport
  RUNTIME DESTINATION bin COMPONENT Runtime
  LIBRARY DESTINATION lib COMPONENT Libraries
  ARCHIVE DESTINATION lib COMPONENT Development
  INCLUDES DESTINATION include
)
install(FILES include/mylib.h DESTINATION include COMPONENT Development)
install(DIRECTORY include/ DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
install(EXPORT MyProjExport
  FILE MyProjTargets.cmake
  NAMESPACE MyProj::
  DESTINATION lib/cmake/MyProj
  COMPONENT Development
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_BINARY_DIR}/MyProjConfigVersion.cmake"
  VERSION 2.0.0
  COMPATIBILITY SameMajorVersion
)

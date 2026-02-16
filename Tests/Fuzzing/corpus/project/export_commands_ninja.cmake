add_library(exportlib STATIC lib.c)
add_library(exportlib2 SHARED lib.c)
add_executable(exportapp main.c)
target_link_libraries(exportapp PRIVATE exportlib exportlib2)

set_target_properties(exportlib PROPERTIES
  PUBLIC_HEADER "include/mylib.h;include/config.h"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/include"
)

export(TARGETS exportlib exportlib2 exportapp
  FILE "${CMAKE_BINARY_DIR}/FuzzExport.cmake"
  NAMESPACE Fuzz::
)

export(TARGETS exportlib
  FILE "${CMAKE_BINARY_DIR}/FuzzLibExport.cmake"
  APPEND
)

export(PACKAGE FuzzTest)

install(TARGETS exportlib exportlib2 exportapp
  EXPORT FuzzInstallExport
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include
)

install(EXPORT FuzzInstallExport
  FILE FuzzTargets.cmake
  NAMESPACE Fuzz::
  DESTINATION lib/cmake/Fuzz
)

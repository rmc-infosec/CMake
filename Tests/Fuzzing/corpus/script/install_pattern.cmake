# Install command patterns (script mode compatible setup)
include(GNUInstallDirs OPTIONAL)

set(CMAKE_INSTALL_PREFIX "/usr/local")
if(DEFINED CMAKE_INSTALL_LIBDIR)
  message(STATUS "Libdir: ${CMAKE_INSTALL_LIBDIR}")
endif()
if(DEFINED CMAKE_INSTALL_BINDIR)
  message(STATUS "Bindir: ${CMAKE_INSTALL_BINDIR}")
endif()
if(DEFINED CMAKE_INSTALL_INCLUDEDIR)
  message(STATUS "Includedir: ${CMAKE_INSTALL_INCLUDEDIR}")
endif()

# Simulate install DESTINATION calculation
set(INSTALL_TARGETS
  "mylib:lib"
  "myapp:bin"
  "myheader.h:include"
  "mydata.dat:share/myproject"
)

foreach(entry ${INSTALL_TARGETS})
  string(REPLACE ":" ";" parts "${entry}")
  list(GET parts 0 name)
  list(GET parts 1 dest)
  message(STATUS "Install ${name} -> ${CMAKE_INSTALL_PREFIX}/${dest}")
endforeach()

# RPATH patterns
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
message(STATUS "RPATH: ${CMAKE_INSTALL_RPATH}")

add_library(base STATIC lib.c)
target_link_options(base PUBLIC -Wl,--as-needed)
target_link_directories(base PUBLIC /usr/local/lib)

add_library(shared SHARED lib.c)
target_link_options(shared PRIVATE -Wl,--no-undefined)
set_target_properties(shared PROPERTIES
  LINK_FLAGS "-Wl,-rpath,\\$ORIGIN"
  INSTALL_RPATH "\$ORIGIN/../lib"
  BUILD_RPATH_USE_ORIGIN ON
  BUILD_WITH_INSTALL_RPATH OFF
)

add_executable(app main.c)
target_link_libraries(app PRIVATE base shared)
target_link_options(app PRIVATE -Wl,--gc-sections)
set_target_properties(app PROPERTIES
  ENABLE_EXPORTS ON
  LINK_DEPENDS_NO_SHARED ON
  INSTALL_RPATH_USE_LINK_PATH ON
)

add_link_options(-Wl,--warn-common)

enable_language(C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core library
add_library(core STATIC
  src/core/init.c
  src/core/config.c
)
target_include_directories(core PUBLIC include)
target_compile_definitions(core PUBLIC CORE_VERSION="1.0")

# Utils library depending on core
add_library(utils STATIC
  src/utils/string_util.cpp
  src/utils/file_util.cpp
)
target_link_libraries(utils PUBLIC core)
target_compile_features(utils PUBLIC cxx_std_14)

# Plugin as shared library
add_library(plugin MODULE src/plugin/plugin.cpp)
target_link_libraries(plugin PRIVATE utils)
set_target_properties(plugin PROPERTIES PREFIX "")

# Main executable
add_executable(myapp src/main.cpp)
target_link_libraries(myapp PRIVATE utils)

# Test executable
enable_testing()
add_executable(unit_tests tests/test_main.cpp)
target_link_libraries(unit_tests PRIVATE utils)
add_test(NAME unit COMMAND unit_tests)

# Install rules
install(TARGETS core utils myapp
  EXPORT MyAppTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT MyAppTargets
  FILE MyAppConfig.cmake
  NAMESPACE MyApp::
  DESTINATION lib/cmake/MyApp
)

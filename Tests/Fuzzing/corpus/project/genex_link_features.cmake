add_library(core STATIC lib.cpp)
add_library(utils STATIC util.cpp)
add_library(helper STATIC helper.cpp)
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE
  "$<LINK_ONLY:core>"
  "$<LINK_ONLY:utils>"
  "$<$<CONFIG:Debug>:helper>"
)
set_target_properties(core PROPERTIES
  INTERFACE_LINK_LIBRARIES "$<LINK_ONLY:utils>"
  INTERFACE_COMPILE_DEFINITIONS "$<$<CONFIG:Debug>:CORE_DEBUG>"
  INTERFACE_INCLUDE_DIRECTORIES "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_link_options(myapp PRIVATE
  "$<$<CXX_COMPILER_ID:GNU>:-Wl,--as-needed>"
  "$<$<CONFIG:Release>:-s>"
)
target_link_directories(myapp PRIVATE
  "$<$<CONFIG:Debug>:${CMAKE_CURRENT_BINARY_DIR}/debug>"
  "$<$<CONFIG:Release>:${CMAKE_CURRENT_BINARY_DIR}/release>"
)

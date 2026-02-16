enable_language(CXX)
add_library(mylib STATIC lib.cpp)
target_compile_definitions(mylib PUBLIC
  $<$<CONFIG:Debug>:DEBUG_MODE=1>
  $<$<CONFIG:Release>:NDEBUG>
  $<$<COMPILE_LANGUAGE:CXX>:CXX_ENABLED>
)
target_include_directories(mylib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_options(mylib PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra>
  $<$<CXX_COMPILER_ID:Clang>:-Weverything>
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

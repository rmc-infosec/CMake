enable_language(CXX)
add_library(modern_lib STATIC modern.cpp)
target_compile_features(modern_lib PUBLIC
  cxx_auto_type
  cxx_constexpr
  cxx_decltype
  cxx_lambdas
  cxx_nullptr
  cxx_range_for
  cxx_rvalue_references
  cxx_variadic_templates
)

add_library(cpp17_lib STATIC cpp17.cpp)
target_compile_features(cpp17_lib PUBLIC cxx_std_17)
target_compile_options(cpp17_lib PRIVATE -fno-exceptions)
target_link_options(cpp17_lib PUBLIC -Wl,--as-needed)

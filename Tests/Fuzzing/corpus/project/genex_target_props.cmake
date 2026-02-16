add_library(mylib STATIC lib.cpp)
add_library(mylib2 STATIC util.cpp)
add_executable(myapp main.cpp)
set_target_properties(mylib PROPERTIES
  OUTPUT_NAME "foobar"
  PREFIX "lib"
  SUFFIX ".a"
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  POSITION_INDEPENDENT_CODE ON
  INTERPROCEDURAL_OPTIMIZATION ON
  AUTOMOC OFF
  AUTOUIC OFF
  AUTORCC OFF
)
target_compile_features(mylib PUBLIC cxx_std_17)
target_compile_features(myapp PRIVATE cxx_std_17)
target_sources(mylib PRIVATE
  "$<$<COMPILE_LANGUAGE:CXX>:plugin.cpp>"
)
target_compile_options(mylib PRIVATE
  "$<$<CXX_COMPILER_ID:GNU>:-Wall;-Wextra;-Wpedantic>"
  "$<$<CXX_COMPILER_ID:Clang>:-Weverything>"
  "$<$<CONFIG:Debug>:-O0;-g>"
  "$<$<CONFIG:Release>:-O2>"
)
target_link_libraries(myapp PRIVATE mylib mylib2)

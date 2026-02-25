enable_language(C)
enable_language(CXX)

add_library(c_core STATIC core.c)
target_include_directories(c_core PUBLIC include)

add_library(cxx_wrapper STATIC wrapper.cpp)
target_link_libraries(cxx_wrapper PUBLIC c_core)
target_compile_features(cxx_wrapper PUBLIC cxx_std_11)

add_executable(hybrid_app main.cpp c_helper.c)
target_link_libraries(hybrid_app PRIVATE cxx_wrapper)
set_source_files_properties(c_helper.c PROPERTIES LANGUAGE C)

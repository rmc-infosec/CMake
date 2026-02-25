enable_language(C)
add_library(mylib STATIC lib.c)
target_include_directories(mylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(mylib PRIVATE MY_LIB_BUILDING=1)

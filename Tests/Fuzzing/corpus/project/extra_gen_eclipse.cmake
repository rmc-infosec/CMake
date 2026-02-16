add_library(mylib STATIC lib.cpp)
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE mylib)
install(TARGETS mylib myapp DESTINATION lib)

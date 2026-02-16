add_library(mylib STATIC lib.c util.c)
target_include_directories(mylib PUBLIC include)
target_compile_definitions(mylib PRIVATE MYLIB_BUILDING=1)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE mylib)

add_library(cxxlib STATIC lib.cpp util.cpp)
target_compile_features(cxxlib PUBLIC cxx_std_14)

add_executable(cxxapp main.cpp)
target_link_libraries(cxxapp PRIVATE cxxlib mylib)

install(TARGETS mylib cxxlib myapp
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

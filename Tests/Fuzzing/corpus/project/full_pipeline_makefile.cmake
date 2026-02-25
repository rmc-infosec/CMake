add_library(mylib STATIC lib.c util.c)
target_include_directories(mylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(mylib PRIVATE MYLIB_BUILDING=1)
target_compile_definitions(mylib PUBLIC MYLIB_API=1)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE mylib)
target_compile_options(myapp PRIVATE -Wall)

add_library(cxxlib STATIC lib.cpp util.cpp)
target_compile_features(cxxlib PUBLIC cxx_std_11)

add_executable(cxxapp main.cpp)
target_link_libraries(cxxapp PRIVATE cxxlib mylib)

install(TARGETS mylib cxxlib myapp
  EXPORT MyTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
install(EXPORT MyTargets
  FILE MyTargets.cmake
  NAMESPACE My::
  DESTINATION lib/cmake/My
)

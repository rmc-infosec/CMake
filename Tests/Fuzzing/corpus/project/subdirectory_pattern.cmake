# Simulate subdirectory by creating a subdirectory CMakeLists.txt
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/sub/CMakeLists.txt"
  "add_library(sublib STATIC ../lib.c)\n"
  "target_include_directories(sublib PUBLIC ../include)\n"
)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sub")
add_subdirectory(sub)

add_executable(app main.c)
target_link_libraries(app PRIVATE sublib)

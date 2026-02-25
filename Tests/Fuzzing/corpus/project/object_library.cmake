enable_language(CXX)
add_library(common_objs OBJECT
  src/util.cpp
  src/helper.cpp
)
target_include_directories(common_objs PUBLIC src)

add_executable(app1 app1.cpp)
target_link_libraries(app1 PRIVATE common_objs)

add_executable(app2 app2.cpp)
target_link_libraries(app2 PRIVATE common_objs)

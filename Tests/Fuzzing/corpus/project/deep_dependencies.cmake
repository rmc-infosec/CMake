add_library(base STATIC core.c)
target_include_directories(base PUBLIC include)
target_compile_definitions(base PUBLIC BASE_VERSION=1)

add_library(mid1 STATIC util.c)
target_link_libraries(mid1 PUBLIC base)

add_library(mid2 STATIC helper.c)
target_link_libraries(mid2 PUBLIC base)
target_link_libraries(mid2 PRIVATE mid1)

add_library(top STATIC lib.c)
target_link_libraries(top PUBLIC mid1 mid2)
target_compile_features(top PUBLIC cxx_std_11)

add_executable(app1 main.c)
target_link_libraries(app1 PRIVATE top)

add_executable(app2 app.c)
target_link_libraries(app2 PRIVATE mid1 mid2 base)

add_executable(test_app test.c)
target_link_libraries(test_app PRIVATE top)
enable_testing()
add_test(NAME basic COMMAND test_app)

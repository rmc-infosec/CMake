add_library(mylib STATIC lib.cpp)
target_compile_definitions(mylib PUBLIC
  $<$<CONFIG:Debug>:DEBUG_MODE=1>
  $<$<CONFIG:Release>:NDEBUG>
  $<$<COMPILE_LANGUAGE:CXX>:CXX_CODE>
  $<$<COMPILE_LANGUAGE:C>:C_CODE>
)
target_include_directories(mylib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_options(mylib PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-Wall>
  $<$<CXX_COMPILER_ID:Clang>:-Wall>
)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE mylib)
target_compile_definitions(app PRIVATE
  "APP_VERSION=\"$<TARGET_PROPERTY:app,OUTPUT_NAME>\""
)

file(GENERATE
  OUTPUT "${CMAKE_BINARY_DIR}/info_$<CONFIG>.txt"
  CONTENT "target=$<TARGET_FILE:app>\nlib=$<TARGET_FILE:mylib>\n"
)

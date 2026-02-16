add_library(mylib STATIC lib.cpp)
add_executable(myapp main.cpp)
target_compile_definitions(mylib PRIVATE
  "$<$<CONFIG:Debug>:DEBUG_MODE>"
  "$<$<CONFIG:Release>:NDEBUG>"
  "$<$<CONFIG:RelWithDebInfo>:NDEBUG>"
  "$<$<COMPILE_LANGUAGE:CXX>:LANG_CXX>"
  "$<$<COMPILE_LANGUAGE:C>:LANG_C>"
  "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:GNU_CXX>"
  "$<$<COMPILE_LANG_AND_ID:CXX,Clang>:CLANG_CXX>"
  "VERSION=$<TARGET_PROPERTY:mylib,VERSION>"
)
target_compile_definitions(mylib PUBLIC
  "$<$<BOOL:${ENABLE_FEATURE}>:FEATURE_ENABLED>"
  "$<IF:$<CONFIG:Debug>,DEBUG_BUILD,RELEASE_BUILD>"
)
target_compile_definitions(myapp PRIVATE
  "$<TARGET_PROPERTY:mylib,INTERFACE_COMPILE_DEFINITIONS>"
)

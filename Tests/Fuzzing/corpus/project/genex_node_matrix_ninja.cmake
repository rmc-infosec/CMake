add_library(genex_core STATIC core.c)
add_library(genex_iface INTERFACE)
add_library(genex_shared SHARED lib.c)
add_executable(genex_app main.c)

target_link_libraries(genex_app PRIVATE
  genex_core
  genex_shared
  "$<IF:$<BOOL:1>,genex_iface,genex_core>"
)

target_compile_definitions(genex_core PUBLIC
  "$<$<AND:$<BOOL:1>,$<NOT:$<BOOL:0>>>:GENEX_BOOL_CHAIN>"
  "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:GENEX_DBG_LIKE>"
  "GENEX_VERSION=$<VERSION_LESS:1.2.3,9.0.0>"
)

target_compile_options(genex_core PRIVATE
  "$<$<COMPILE_LANGUAGE:C>:-Wall>"
  "$<$<C_COMPILER_ID:GNU,Clang>:-Wextra>"
)

target_include_directories(genex_core PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)

set_target_properties(genex_core PROPERTIES
  OUTPUT_NAME "core_$<LOWER_CASE:$<CONFIG>>"
)

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/genex_matrix_$<CONFIG>.txt"
  CONTENT
    "cfg=$<CONFIG>\n"
    "platform=$<PLATFORM_ID>\n"
    "core_type=$<TARGET_PROPERTY:genex_core,TYPE>\n"
    "exists=$<BOOL:$<TARGET_EXISTS:genex_core>>\n"
    "joined=$<JOIN:$<TARGET_PROPERTY:genex_core,INCLUDE_DIRECTORIES>,:>\n"
    "name_if=$<TARGET_NAME_IF_EXISTS:genex_shared>\n"
)

install(TARGETS genex_core genex_shared genex_app
  EXPORT GenexExport
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(EXPORT GenexExport FILE GenexTargets.cmake NAMESPACE Genex:: DESTINATION lib/cmake/Genex)

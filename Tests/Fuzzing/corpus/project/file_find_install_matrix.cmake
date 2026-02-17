set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/seed_prefix")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/seed_pkg/FuzzSeed")
file(WRITE "${CMAKE_BINARY_DIR}/seed_pkg/FuzzSeed/FuzzSeedConfig.cmake"
  "add_library(FuzzSeed::Lib INTERFACE IMPORTED)\n"
  "set_target_properties(FuzzSeed::Lib PROPERTIES INTERFACE_COMPILE_DEFINITIONS \"FUZZ_SEED=1\")\n"
  "set(FuzzSeed_FOUND TRUE)\n"
)
file(WRITE "${CMAKE_BINARY_DIR}/seed_pkg/FuzzSeed/FuzzSeedConfigVersion.cmake"
  "set(PACKAGE_VERSION \"3.4.5\")\n"
  "set(PACKAGE_VERSION_COMPATIBLE TRUE)\n"
)
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/seed_pkg")
find_package(FuzzSeed 3.4 QUIET CONFIG)
find_package(Threads QUIET)
find_package(PkgConfig QUIET)

add_library(seedlib STATIC lib.c util.c)
add_library(seedobj OBJECT helper.c)
add_library(seedshared SHARED plugin.c)
add_executable(seedapp main.c app.c)
target_sources(seedapp PRIVATE "$<TARGET_OBJECTS:seedobj>")

target_link_libraries(seedapp PRIVATE
  seedlib
  seedshared
  "$<$<TARGET_EXISTS:FuzzSeed::Lib>:FuzzSeed::Lib>"
  "$<$<TARGET_EXISTS:Threads::Threads>:Threads::Threads>"
)

target_compile_definitions(seedlib PUBLIC
  "$<$<CONFIG:Debug>:SEED_DEBUG>"
  "SEED_IF=$<IF:$<BOOL:1>,ON,OFF>"
  "SEED_EQ=$<STREQUAL:$<TARGET_PROPERTY:seedlib,TYPE>,STATIC_LIBRARY>"
)

target_include_directories(seedlib PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)

file(WRITE "${CMAKE_BINARY_DIR}/seed_input.txt" "one\ntwo\nthree\n")
file(READ "${CMAKE_BINARY_DIR}/seed_input.txt" _seed_read)
file(STRINGS "${CMAKE_BINARY_DIR}/seed_input.txt" _seed_lines)
file(SHA1 "${CMAKE_BINARY_DIR}/seed_input.txt" _seed_sha1)
file(TIMESTAMP "${CMAKE_BINARY_DIR}/seed_input.txt" _seed_ts UTC)

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/seed_gen_$<CONFIG>.txt"
  CONTENT "cfg=$<CONFIG>\nfile=$<TARGET_FILE:seedapp>\ninc=$<JOIN:$<TARGET_PROPERTY:seedlib,INCLUDE_DIRECTORIES>,:>\n"
)

install(TARGETS seedlib seedshared seedapp
  EXPORT SeedExport
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/mylib.h" DESTINATION include)
install(EXPORT SeedExport FILE SeedTargets.cmake NAMESPACE Seed:: DESTINATION lib/cmake/Seed)

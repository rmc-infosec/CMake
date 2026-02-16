/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file LICENSE.rst or https://cmake.org/licensing for details.  */

/*
 * Fuzzer for CMake project-mode configuration
 *
 * Unlike cmScriptFuzzer (which runs in -P script mode), this fuzzer
 * exercises the full project configuration and generation pipeline:
 * - project(), add_executable(), add_library(), target_link_libraries()
 * - cmGlobalGenerator and cmLocalGenerator code paths
 * - Generator expression evaluation in target context
 * - Install rules, export sets, dependency resolution
 * - Makefile/build file generation
 *
 * Key design decisions:
 * - Enables C and CXX languages so target commands work
 * - Creates dummy source files so targets can be fully processed
 * - Alternates between Unix Makefiles and Ninja generators
 *   to exercise both cmMakefileTargetGenerator and cmNinjaTargetGenerator
 * - Selects extra IDE generators (Eclipse, CodeBlocks, CodeLite, Kate,
 *   Sublime Text) to exercise IDE project file generation
 * - Creates FileAPI query marker files so cmFileAPI, cmFileAPICodemodel,
 *   cmFileAPICache, etc. are exercised during Generate()
 * - Resolves and sets CMAKE_ROOT so CMake can load internal modules
 *   (CMakeCInformation.cmake, etc.) for full language setup
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <unistd.h>

#include "cmGlobalGenerator.h"
#include "cmFileAPICodemodel.h"
#include "cmMakefile.h"
#include "cmMessenger.h"
#include "cmState.h"
#include "cmSystemTools.h"
#include "cmake.h"

static constexpr size_t kMaxInputSize = 256 * 1024;
static std::string g_sourceDir;
static std::string g_buildDir;
static std::string g_cmakeRoot;
static std::vector<std::string> g_envSnapshot;

#if !defined(_WIN32)
extern char** environ;
#endif

static void snapshotProcessEnvironment()
{
#if !defined(_WIN32)
  g_envSnapshot.clear();
  for (char** env = environ; env && *env; ++env) {
    g_envSnapshot.emplace_back(*env);
  }
#endif
}

static void restoreProcessEnvironment()
{
#if !defined(_WIN32)
  std::vector<std::string> current;
  for (char** env = environ; env && *env; ++env) {
    current.emplace_back(*env);
  }

  for (std::string const& entry : current) {
    std::string::size_type pos = entry.find('=');
    if (pos != std::string::npos && pos > 0) {
      std::string key = entry.substr(0, pos);
      unsetenv(key.c_str());
    }
  }

  for (std::string const& entry : g_envSnapshot) {
    std::string::size_type pos = entry.find('=');
    if (pos != std::string::npos && pos > 0) {
      std::string key = entry.substr(0, pos);
      std::string value = entry.substr(pos + 1);
      setenv(key.c_str(), value.c_str(), 1);
    }
  }
#endif
}

static bool hasValidCMakeRoot(std::string const& root)
{
  return !root.empty() &&
    cmSystemTools::FileExists(root + "/Modules/CMake.cmake");
}

static std::string detectCMakeRoot(char const* argv0)
{
  cmSystemTools::FindCMakeResources(argv0);
  std::string root = cmSystemTools::GetCMakeRoot();
  if (hasValidCMakeRoot(root)) {
    return root;
  }

  std::string envRoot;
  if (cmSystemTools::GetEnv("CMAKE_ROOT", envRoot) &&
      hasValidCMakeRoot(envRoot)) {
    return cmSystemTools::ToNormalizedPathOnDisk(envRoot);
  }

  std::string srcRoot;
  if (cmSystemTools::GetEnv("SRC", srcRoot)) {
    std::string candidate =
      cmSystemTools::ToNormalizedPathOnDisk(srcRoot + "/CMake");
    if (hasValidCMakeRoot(candidate)) {
      return candidate;
    }
  }

  // In local coverage/fuzz runs, cwd is often the source tree root.
  std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();
  if (hasValidCMakeRoot(cwd)) {
    return cmSystemTools::ToNormalizedPathOnDisk(cwd);
  }

  if (hasValidCMakeRoot("/src/CMake")) {
    return "/src/CMake";
  }

  std::string exeDir =
    cmSystemTools::GetFilenamePath(cmSystemTools::GetRealPath(argv0));
  std::string adjacent = exeDir + "/src/CMake";
  if (hasValidCMakeRoot(adjacent)) {
    return cmSystemTools::ToNormalizedPathOnDisk(adjacent);
  }

  return root;
}

static void createDummySourceFiles(std::string const& dir)
{
  // Create stub source files that targets can reference
  // This allows the generator pipeline to fully process targets
  static const struct
  {
    const char* name;
    const char* content;
  } files[] = {
    { "main.c", "int main(void) { return 0; }\n" },
    { "main.cpp", "int main() { return 0; }\n" },
    { "app.c", "int main(void) { return 0; }\n" },
    { "app.cpp", "int main() { return 0; }\n" },
    { "test.c", "int main(void) { return 0; }\n" },
    { "test.cpp", "int main() { return 0; }\n" },
    { "test_runner.c", "int main(int, char**) { return 0; }\n" },
    { "lib.c", "void lib_func(void) {}\n" },
    { "lib.cpp", "void lib_func() {}\n" },
    { "util.c", "void util_func(void) {}\n" },
    { "util.cpp", "void util_func() {}\n" },
    { "helper.c", "void helper_func(void) {}\n" },
    { "helper.cpp", "void helper_func() {}\n" },
    { "core.c", "void core_func(void) {}\n" },
    { "plugin.c", "void plugin_c_func(void) {}\n" },
    { "plugin.cpp", "void plugin_func() {}\n" },
    { "wrapper.c", "void wrapper_c_func(void) {}\n" },
    { "wrapper.cpp", "void wrapper_func() {}\n" },
    { "module.c", "void module_func(void) {}\n" },
    { "modern.c", "void modern_func(void) {}\n" },
    { "modern.cpp", "void modern_cpp_func() {}\n" },
    { "cpp17.c", "void cpp17_func(void) {}\n" },
    { "cpp17.cpp", "void cpp17_cpp_func() {}\n" },
    { "bar.c", "void bar_func(void) {}\n" },
    { "foo.c", "void foo_func(void) {}\n" },
    { "base.c", "void base_func(void) {}\n" },
    { "middle.c", "void middle_func(void) {}\n" },
    { "top.c", "void top_func(void) {}\n" },
    { "object.c", "void object_func(void) {}\n" },
    { "shared.c", "void shared_func(void) {}\n" },
    { "src/core/init.c", "void init_core(void) {}\n" },
    { "src/core/config.c", "void config_core(void) {}\n" },
    { "src/utils/string_util.c", "void string_util(void) {}\n" },
    { "src/utils/file_util.c", "void file_util(void) {}\n" },
    { "src/utils/string_util.cpp", "void string_util_cpp() {}\n" },
    { "src/utils/file_util.cpp", "void file_util_cpp() {}\n" },
    { "src/plugin/plugin.c", "void plugin_src_c(void) {}\n" },
    { "src/plugin/plugin.cpp", "void plugin_src_cpp() {}\n" },
    { "src/main.c", "int main(void) { return 0; }\n" },
    { "src/main.cpp", "int main() { return 0; }\n" },
    { "src/helper.c", "void src_helper(void) {}\n" },
    { "src/util.c", "void src_util(void) {}\n" },
    { "tests/test_main.c", "int main(void) { return 0; }\n" },
    { "tests/test_main.cpp", "int main() { return 0; }\n" },
    { "include/mylib.h", "#pragma once\n" },
    { "include/config.h", "#pragma once\n" },
    { "publicinclude/publicinclude.h", "#pragma once\n" },
    { "privateinclude/privateinclude.h", "#pragma once\n" },
    { "interfaceinclude/interfaceinclude.h", "#pragma once\n" },
    { "same_one/same.h", "#pragma once\n" },
    { "same_two/same.h", "#pragma once\n" },
    { "foo.h", "#pragma once\n" },
  };

  auto writeFile = [](std::string const& path, char const* content) {
    std::string parent = cmSystemTools::GetFilenamePath(path);
    if (!parent.empty()) {
      cmSystemTools::MakeDirectory(parent);
    }
    FILE* fp = fopen(path.c_str(), "wb");
    if (fp) {
      fputs(content, fp);
      fclose(fp);
    }
  };

  for (auto const& f : files) {
    std::string path = dir + "/" + f.name;
    writeFile(path, f.content);
    // Also create in src/ subdirectory
    std::string srcPath = dir + "/src/" + f.name;
    writeFile(srcPath, f.content);
  }
}

static void writeDeterministicPrelude(FILE* fp)
{
  static char const prelude[] = R"cmake(
# Stable script block that executes before mutational content.
# It drives deep generator-expression, file(), install(), and find_package()
# paths even when the fuzzed suffix has syntax errors.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/prefix")
set(CMAKE_FIND_PACKAGE_PREFER_CONFIG ON)
set(CMAKE_FIND_DEBUG_MODE OFF)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/pkg/FuzzPkg")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/cmake")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/tmp/deep/dir")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/logs")

file(WRITE "${CMAKE_BINARY_DIR}/pkg/FuzzPkg/FuzzPkgConfig.cmake"
  "add_library(FuzzPkg::Core INTERFACE IMPORTED)\n"
  "set_target_properties(FuzzPkg::Core PROPERTIES\n"
  "  INTERFACE_INCLUDE_DIRECTORIES \"${CMAKE_CURRENT_LIST_DIR}/../../include\"\n"
  "  INTERFACE_COMPILE_DEFINITIONS \"FUZZPKG_ENABLED\"\n"
  ")\n"
  "set(FuzzPkg_FOUND TRUE)\n"
)
file(WRITE "${CMAKE_BINARY_DIR}/pkg/FuzzPkg/FuzzPkgConfigVersion.cmake"
  "set(PACKAGE_VERSION \"1.2.3\")\n"
  "set(PACKAGE_VERSION_COMPATIBLE TRUE)\n"
  "set(PACKAGE_VERSION_EXACT TRUE)\n"
)
file(WRITE "${CMAKE_BINARY_DIR}/cmake/FindFuzzModule.cmake"
  "set(FuzzModule_FOUND TRUE)\n"
  "set(FuzzModule_INCLUDE_DIRS \"${CMAKE_CURRENT_LIST_DIR}/../include\")\n"
)

list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/pkg")
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_BINARY_DIR}/cmake")
find_package(FuzzPkg 1.2 QUIET CONFIG)
find_package(FuzzModule QUIET MODULE)
find_package(Threads QUIET)
find_package(PkgConfig QUIET)
find_program(FUZZ_PYTHON NAMES python3 python)
find_library(FUZZ_MATH_LIB NAMES m)
find_path(FUZZ_STDIO_H NAMES stdio.h)
find_file(FUZZ_DEV_NULL NAMES null PATHS /dev NO_DEFAULT_PATH)

add_library(fuzz_pre_core STATIC core.c lib.c)
add_library(fuzz_pre_obj OBJECT helper.c)
add_library(fuzz_pre_shared SHARED plugin.cpp)
add_library(fuzz_pre_iface INTERFACE)
add_executable(fuzz_pre_app main.cpp app.c)

target_sources(fuzz_pre_app PRIVATE "$<TARGET_OBJECTS:fuzz_pre_obj>" wrapper.cpp)
target_link_libraries(fuzz_pre_app PRIVATE
  fuzz_pre_core
  fuzz_pre_shared
  fuzz_pre_iface
  "$<$<TARGET_EXISTS:FuzzPkg::Core>:FuzzPkg::Core>"
  "$<$<TARGET_EXISTS:Threads::Threads>:Threads::Threads>"
)
target_include_directories(fuzz_pre_core PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/publicinclude>"
  "$<INSTALL_INTERFACE:include>"
)
target_include_directories(fuzz_pre_app PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/privateinclude"
  "${CMAKE_BINARY_DIR}"
)
target_compile_definitions(fuzz_pre_core PUBLIC
  "$<$<CONFIG:Debug>:CORE_DEBUG=1>"
  "$<$<NOT:$<CONFIG:Debug>>:CORE_NODEBUG=1>"
  "CORE_IF=$<IF:$<BOOL:1>,yes,no>"
  "CORE_STREQUAL=$<STREQUAL:$<TARGET_PROPERTY:fuzz_pre_core,TYPE>,STATIC_LIBRARY>"
)
target_compile_definitions(fuzz_pre_app PRIVATE
  "APP_HAS_FUZZPKG=$<BOOL:$<TARGET_EXISTS:FuzzPkg::Core>>"
  "APP_LOWER=$<LOWER_CASE:HELLO>"
  "APP_UPPER=$<UPPER_CASE:hello>"
)
target_compile_options(fuzz_pre_core PRIVATE
  "$<$<COMPILE_LANGUAGE:C>:-Wall>"
  "$<$<COMPILE_LANGUAGE:CXX>:-Wextra>"
)
target_link_options(fuzz_pre_app PRIVATE
  "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wl,--as-needed>"
  "$<$<CONFIG:Release>:-s>"
)
target_link_directories(fuzz_pre_app PRIVATE
  "$<$<CONFIG:Debug>:${CMAKE_BINARY_DIR}/debuglib>"
  "$<$<NOT:$<CONFIG:Debug>>:${CMAKE_BINARY_DIR}/releaselib>"
)
set_target_properties(fuzz_pre_core PROPERTIES
  OUTPUT_NAME "core_$<LOWER_CASE:$<CONFIG>>"
  POSITION_INDEPENDENT_CODE ON
  C_STANDARD 99
)
set_target_properties(fuzz_pre_shared PROPERTIES
  VERSION 1.2.3
  SOVERSION 1
)

file(WRITE "${CMAKE_BINARY_DIR}/gen_src.c" "int generated_symbol(void) { return 0; }\n")
set_source_files_properties("${CMAKE_BINARY_DIR}/generated.c" PROPERTIES GENERATED TRUE)
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/generated.c"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_BINARY_DIR}/gen_src.c"
          "${CMAKE_BINARY_DIR}/generated.c"
  DEPENDS "${CMAKE_BINARY_DIR}/gen_src.c"
  COMMENT "Generate source for fuzz target"
  VERBATIM
)
add_custom_target(fuzz_codegen DEPENDS "${CMAKE_BINARY_DIR}/generated.c")
add_library(fuzz_generated STATIC "${CMAKE_BINARY_DIR}/generated.c")
add_dependencies(fuzz_generated fuzz_codegen)
target_link_libraries(fuzz_pre_app PRIVATE fuzz_generated)
set_property(TARGET fuzz_pre_app PROPERTY FOLDER "apps/fuzz")
set_property(SOURCE core.c PROPERTY COMPILE_DEFINITIONS CORE_SOURCE_FILE=1)
set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
  "${CMAKE_BINARY_DIR}/generated.c"
)
add_custom_target(fuzz_stamp ALL
  COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_BINARY_DIR}/fuzz.stamp"
  BYPRODUCTS "${CMAKE_BINARY_DIR}/fuzz.stamp"
  DEPENDS fuzz_pre_app
  VERBATIM
)
enable_testing()
add_test(NAME fuzz_pre_app_smoke COMMAND fuzz_pre_app)
set_tests_properties(fuzz_pre_app_smoke PROPERTIES WILL_FAIL FALSE)

file(WRITE "${CMAKE_BINARY_DIR}/input.txt" "alpha\nbeta\ngamma\n")
file(APPEND "${CMAKE_BINARY_DIR}/input.txt" "delta\n")
file(READ "${CMAKE_BINARY_DIR}/input.txt" _fread LIMIT 128)
file(STRINGS "${CMAKE_BINARY_DIR}/input.txt" _fstrings LIMIT_COUNT 8)
file(SIZE "${CMAKE_BINARY_DIR}/input.txt" _fsize)
file(SHA1 "${CMAKE_BINARY_DIR}/input.txt" _fsha1)
file(SHA256 "${CMAKE_BINARY_DIR}/input.txt" _fsha)
file(TIMESTAMP "${CMAKE_BINARY_DIR}/input.txt" _ftime UTC)
file(COPY_FILE "${CMAKE_BINARY_DIR}/input.txt" "${CMAKE_BINARY_DIR}/copied.txt")
file(RENAME "${CMAKE_BINARY_DIR}/copied.txt" "${CMAKE_BINARY_DIR}/renamed.txt")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION "${CMAKE_BINARY_DIR}/copied_include")
file(GLOB _glob_c "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE _glob_src "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(RELATIVE_PATH _rel "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
file(REAL_PATH "${CMAKE_BINARY_DIR}/renamed.txt" _real BASE_DIRECTORY "${CMAKE_BINARY_DIR}")
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" _cm_path)
file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" _native_path)
file(TOUCH "${CMAKE_BINARY_DIR}/touch.stamp")
file(CHMOD "${CMAKE_BINARY_DIR}/renamed.txt"
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
file(CREATE_LINK "${CMAKE_BINARY_DIR}/renamed.txt"
  "${CMAKE_BINARY_DIR}/renamed.link"
  SYMBOLIC RESULT _link_result COPY_ON_ERROR
)
file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/tmp_remove")

if(COMMAND cmake_path)
  set(_path_var "${CMAKE_CURRENT_SOURCE_DIR}/src/../include/./mylib.h")
  cmake_path(NORMAL_PATH _path_var OUTPUT_VARIABLE _path_norm)
  cmake_path(GET _path_norm FILENAME _path_name)
  cmake_path(GET _path_norm EXTENSION _path_ext)
  cmake_path(GET _path_norm PARENT_PATH _path_parent)
  cmake_path(SET _path_build NORMALIZE "${CMAKE_CURRENT_SOURCE_DIR}/foo/../bar")
  cmake_path(APPEND _path_build "baz" "file.txt")
  cmake_path(HAS_ROOT_PATH _path_build _path_has_root)
  cmake_path(IS_ABSOLUTE _path_build _path_abs)
  cmake_path(COMPARE "${_path_build}" EQUAL "${_path_build}" _path_same)
endif()

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/gen_info_$<CONFIG>.txt"
  CONTENT
    "cfg=$<CONFIG>\n"
    "platform=$<PLATFORM_ID>\n"
    "compiler=$<CXX_COMPILER_ID>\n"
    "app_file=$<TARGET_FILE:fuzz_pre_app>\n"
    "core_type=$<TARGET_PROPERTY:fuzz_pre_core,TYPE>\n"
    "core_exists=$<BOOL:$<TARGET_EXISTS:fuzz_pre_core>>\n"
    "is_debug=$<IF:$<CONFIG:Debug>,1,0>\n"
    "joined=$<JOIN:$<TARGET_PROPERTY:fuzz_pre_core,INCLUDE_DIRECTORIES>,:>\n"
)

install(TARGETS fuzz_pre_core fuzz_pre_shared fuzz_pre_app
  EXPORT FuzzPreTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION include
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/mylib.h" DESTINATION include RENAME fuzz_mylib.h)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
if(EXISTS "${CMAKE_COMMAND}")
  install(PROGRAMS "${CMAKE_COMMAND}" DESTINATION tools RENAME cmake-tool)
endif()
file(WRITE "${CMAKE_BINARY_DIR}/install_hook.cmake" "message(STATUS \"install hook\")\n")
install(SCRIPT "${CMAKE_BINARY_DIR}/install_hook.cmake")
install(CODE "message(STATUS \"install code\")")
install(EXPORT FuzzPreTargets
  FILE FuzzPreTargets.cmake
  NAMESPACE FuzzPre::
  DESTINATION lib/cmake/FuzzPre
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_BINARY_DIR}/FuzzPreTargetsConfigVersion.cmake"
  VERSION 1.2.3
  COMPATIBILITY SameMajorVersion
)
export(TARGETS fuzz_pre_core fuzz_pre_shared fuzz_pre_app
  FILE "${CMAKE_BINARY_DIR}/FuzzPreExport.cmake"
  NAMESPACE FuzzPre::
)
)cmake";

  fputs(prelude, fp);
}

static void writeProjectCache(bool useNinja, int extraGen)
{
  std::string cachePath = g_buildDir + "/CMakeCache.txt";
  FILE* fp = fopen(cachePath.c_str(), "wb");
  if (!fp) {
    return;
  }

  const char* generator = useNinja ? "Ninja" : "Unix Makefiles";
  const char* makeProgram = useNinja ? "/usr/bin/ninja" : "/usr/bin/make";

  static const char* extraGenNames[] = {
    "",                // 0: no extra generator
    "Eclipse CDT4",   // 1
    "CodeBlocks",     // 2
    "CodeLite",       // 3
    "Kate",           // 4
    "Sublime Text 2", // 5
  };
  const char* extraGenName =
    (extraGen >= 1 && extraGen <= 5) ? extraGenNames[extraGen] : "";

  fprintf(fp,
          "# This is the CMakeCache file.\n"
          "CMAKE_GENERATOR:INTERNAL=%s\n"
          "CMAKE_HOME_DIRECTORY:INTERNAL=%s\n"
          "CMAKE_MAKE_PROGRAM:FILEPATH=%s\n"
          "CMAKE_C_COMPILER:FILEPATH=/bin/true\n"
          "CMAKE_C_COMPILER_FORCED:INTERNAL=TRUE\n"
          "CMAKE_C_COMPILER_ID:INTERNAL=GNU\n"
          "CMAKE_C_COMPILER_ID_RUN:INTERNAL=TRUE\n"
          "CMAKE_C_COMPILER_VERSION:INTERNAL=12.0\n"
          "CMAKE_C_COMPILER_WORKS:INTERNAL=TRUE\n"
          "CMAKE_C_ABI_COMPILED:INTERNAL=TRUE\n"
          "CMAKE_C_SIZEOF_DATA_PTR:INTERNAL=8\n"
          "CMAKE_C_STANDARD_COMPUTED_DEFAULT:INTERNAL=11\n"
          "CMAKE_C_OUTPUT_EXTENSION:STRING=.o\n"
          "CMAKE_CXX_COMPILER:FILEPATH=/bin/true\n"
          "CMAKE_CXX_COMPILER_FORCED:INTERNAL=TRUE\n"
          "CMAKE_CXX_COMPILER_ID:INTERNAL=GNU\n"
          "CMAKE_CXX_COMPILER_ID_RUN:INTERNAL=TRUE\n"
          "CMAKE_CXX_COMPILER_VERSION:INTERNAL=12.0\n"
          "CMAKE_CXX_COMPILER_WORKS:INTERNAL=TRUE\n"
          "CMAKE_CXX_ABI_COMPILED:INTERNAL=TRUE\n"
          "CMAKE_CXX_SIZEOF_DATA_PTR:INTERNAL=8\n"
          "CMAKE_CXX_STANDARD_COMPUTED_DEFAULT:INTERNAL=17\n"
          "CMAKE_CXX_OUTPUT_EXTENSION:STRING=.o\n",
          generator, g_sourceDir.c_str(), makeProgram);
  if (extraGenName[0] != '\0') {
    fprintf(fp, "CMAKE_EXTRA_GENERATOR:INTERNAL=%s\n", extraGenName);
  }
  if (!g_cmakeRoot.empty()) {
    fprintf(fp, "CMAKE_ROOT:INTERNAL=%s\n", g_cmakeRoot.c_str());
  }
  fclose(fp);
}

static void writeProjectCMakeLists(uint8_t const* data, size_t contentSize)
{
  std::string cmakelists = g_sourceDir + "/CMakeLists.txt";
  FILE* fp = fopen(cmakelists.c_str(), "wb");
  if (!fp) {
    return;
  }

  fprintf(fp, "cmake_minimum_required(VERSION 3.10)\n");
  fprintf(fp, "project(FuzzTest LANGUAGES C CXX)\n");
  writeDeterministicPrelude(fp);
  if (data && contentSize > 0) {
    fwrite(data, 1, contentSize, fp);
  }
  fputc('\n', fp);
  fclose(fp);
}

static void writeFileAPIQueries()
{
  std::string queryDir = g_buildDir + "/.cmake/api/v1/query";
  cmSystemTools::MakeDirectory(queryDir);
  static const char* queryFiles[] = { "codemodel-v2", "cache-v2",
                                      "cmakeFiles-v1", "toolchains-v1",
                                      "configureLog-v1" };
  for (auto const* qf : queryFiles) {
    std::string path = queryDir + "/" + qf;
    FILE* fp = fopen(path.c_str(), "wb");
    if (fp) {
      fclose(fp);
    }
  }

  // Also emit a client-stateful query.json to exercise that parsing path and
  // request codemodel/cache/cmakeFiles/toolchains/configureLog explicitly.
  std::string clientDir = queryDir + "/client-fuzz";
  cmSystemTools::MakeDirectory(clientDir);
  {
    std::string queryJson = clientDir + "/query.json";
    FILE* fp = fopen(queryJson.c_str(), "wb");
    if (fp) {
      static char const json[] =
        "{\n"
        "  \"client\": { \"name\": \"cmProjectFuzzer\" },\n"
        "  \"requests\": [\n"
        "    { \"kind\": \"codemodel\", \"version\": 2 },\n"
        "    { \"kind\": \"cache\", \"version\": 2 },\n"
        "    { \"kind\": \"cmakeFiles\", \"version\": 1 },\n"
        "    { \"kind\": \"toolchains\", \"version\": 1 },\n"
        "    { \"kind\": \"configureLog\", \"version\": 1 }\n"
        "  ]\n"
        "}\n";
      fputs(json, fp);
      fclose(fp);
    }
  }
}

static void runConfigureAndGenerate(int& configureResult, int& generateResult)
{
  std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();

  cmake cm(cmState::Role::Project);
  cm.SetHomeDirectory(g_sourceDir);
  cm.SetHomeOutputDirectory(g_buildDir);
  cm.LoadCache(g_buildDir);

  configureResult = cm.Configure();
  generateResult = cm.Generate();

  cm.InitializeFileAPI();
  if (cm.GetFileAPI() && cm.GetGlobalGenerator()) {
    (void)cmFileAPICodemodelDump(*cm.GetFileAPI(), 2, 0);
  }

  cmSystemTools::ChangeDirectory(cwd);
}

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
  (void)argc;

  cmSystemTools::SetMessageCallback(
    [](std::string const&, cmMessageMetadata const&) {});
  cmSystemTools::SetStdoutCallback([](std::string const&) {});
  cmSystemTools::SetStderrCallback([](std::string const&) {});

  // Resolve CMake resources using the same mechanism as the main executable,
  // with explicit fallbacks for OSS-Fuzz/ClusterFuzzLite layouts.
  char const* argv0 = (argv && *argv && (*argv)[0]) ? (*argv)[0]
                                                     : "cmProjectFuzzer";
  g_cmakeRoot = detectCMakeRoot(argv0);

  // Create unique temp directories
  char srcTmpl[] = "/tmp/cmake_fuzz_proj_src_XXXXXX";
  char* srcDir = mkdtemp(srcTmpl);
  if (srcDir) {
    g_sourceDir = srcDir;
  } else {
    g_sourceDir = "/tmp/cmake_fuzz_proj_src";
    cmSystemTools::MakeDirectory(g_sourceDir);
  }

  char bldTmpl[] = "/tmp/cmake_fuzz_proj_bld_XXXXXX";
  char* bldDir = mkdtemp(bldTmpl);
  if (bldDir) {
    g_buildDir = bldDir;
  } else {
    g_buildDir = "/tmp/cmake_fuzz_proj_bld";
    cmSystemTools::MakeDirectory(g_buildDir);
  }

  snapshotProcessEnvironment();

  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 1 || size > kMaxInputSize) {
    return 0;
  }

  // Mutational CMake code can set ENV{} variables and poison later runs.
  // Restore a clean environment before each iteration.
  restoreProcessEnvironment();

  // Derive control bits from a hash of the full input so generator selection
  // stays diverse even when corpus files share the same trailing newline.
  //   bit 0:   base generator (0=Unix Makefiles, 1=Ninja)
  //   bits 1-3: extra generator (0=none, 1=Eclipse CDT4, 2=CodeBlocks,
  //             3=CodeLite, 4=Kate, 5=Sublime Text 2)
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < size; ++i) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  uint8_t ctrl = static_cast<uint8_t>(hash & 0xff);
  bool useNinja = (ctrl & 1);
  int extraGen = (ctrl >> 1) & 7;
  size_t contentSize = size;

  // Create dummy source files for targets to reference
  createDummySourceFiles(g_sourceDir);

  // First pass: run with raw mutational content.
  writeProjectCache(useNinja, extraGen);
  writeProjectCMakeLists(data, contentSize);
  writeFileAPIQueries();

  int configureResult = 1;
  int generateResult = 1;
  runConfigureAndGenerate(configureResult, generateResult);

  // Fallback pass: if mutational input broke configure/generate, rerun a
  // deterministic project to ensure deep generator and FileAPI paths execute.
  if (configureResult != 0 || generateResult != 0) {
    restoreProcessEnvironment();
    cmSystemTools::RemoveADirectory(g_buildDir);
    cmSystemTools::MakeDirectory(g_buildDir);
    createDummySourceFiles(g_sourceDir);

    // Re-run a deterministic project body (without fuzzed suffix) to drive
    // deep generator and FileAPI code paths even when mutational input fails.
    writeProjectCache(useNinja, extraGen);
    writeProjectCMakeLists(nullptr, 0);
    writeFileAPIQueries();

    runConfigureAndGenerate(configureResult, generateResult);
  }

  if (configureResult == 0 && generateResult == 0) {
    cmSystemTools::Touch(g_buildDir + "/fuzz_success.stamp", true);
  }

  // Clean up source and build dirs for next iteration
  cmSystemTools::RemoveADirectory(g_buildDir);
  cmSystemTools::MakeDirectory(g_buildDir);
  cmSystemTools::RemoveADirectory(g_sourceDir);
  cmSystemTools::MakeDirectory(g_sourceDir);

  return 0;
}

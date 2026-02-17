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
#include "cmGeneratorExpression.h"
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
  "if(NOT TARGET FuzzPkg::Core)\n"
  "  add_library(FuzzPkg::Core INTERFACE IMPORTED)\n"
  "  set_target_properties(FuzzPkg::Core PROPERTIES\n"
  "    INTERFACE_INCLUDE_DIRECTORIES \"${CMAKE_CURRENT_LIST_DIR}/../../include\"\n"
  "    INTERFACE_COMPILE_DEFINITIONS \"FUZZPKG_ENABLED\"\n"
  "  )\n"
  "endif()\n"
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
set(FuzzPkg_ROOT "${CMAKE_BINARY_DIR}/pkg")
find_package(FuzzPkg QUIET CONFIG GLOBAL)
find_package(NotHere QUIET CONFIGS NotHereConfig.cmake)
find_package(NotHere QUIET CONFIG NAMES NotHereA NotHereB
  PATHS "${CMAKE_BINARY_DIR}/pkg" NO_DEFAULT_PATH
)
find_package(Python QUIET COMPONENTS Interpreter)
find_package(ZLIB QUIET)
find_package(OpenSSL QUIET)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/pkgconfig")
file(WRITE "${CMAKE_BINARY_DIR}/pkgconfig/fuzzlocal.pc"
  "Name: fuzzlocal\n"
  "Description: local fuzz package\n"
  "Version: 1.0.0\n"
  "Cflags: -I${CMAKE_CURRENT_SOURCE_DIR}/include\n"
)
if(COMMAND cmake_pkg_config)
  set(CMAKE_PKG_CONFIG_PC_PATH "${CMAKE_BINARY_DIR}/pkgconfig")
  cmake_pkg_config(EXTRACT fuzzlocal QUIET)
  cmake_pkg_config(POPULATE fuzzlocal QUIET)
  cmake_pkg_config(IMPORT fuzzlocal QUIET)
endif()
if(POLICY CMP0153)
  cmake_policy(SET CMP0153 OLD)
endif()
if(POLICY CMP0054)
  cmake_policy(PUSH)
  cmake_policy(SET CMP0054 NEW)
  cmake_policy(GET CMP0054 _cmp0054_state)
  cmake_policy(POP)
endif()
enable_language(C)
enable_language(CXX)

add_library(fuzz_pre_core STATIC core.c lib.c)
add_library(fuzz_pre_obj OBJECT helper.c)
add_library(fuzz_pre_shared SHARED plugin.cpp)
add_library(fuzz_pre_iface INTERFACE)
add_executable(fuzz_pre_app main.cpp app.c)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

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
  "APP_CONFIG=$<CONFIG>"
  "APP_PLATFORM=$<PLATFORM_ID>"
  "APP_TARGET_EXISTS=$<TARGET_EXISTS:fuzz_pre_core>"
  "APP_TARGET_NAME=$<TARGET_NAME_IF_EXISTS:fuzz_pre_core>"
  "APP_TARGET_TYPE=$<TARGET_PROPERTY:fuzz_pre_core,TYPE>"
  "APP_TARGET_FILE=$<TARGET_FILE:fuzz_pre_app>"
  "APP_TARGET_FILE_NAME=$<TARGET_FILE_NAME:fuzz_pre_app>"
  "APP_TARGET_FILE_BASE=$<TARGET_FILE_BASE_NAME:fuzz_pre_app>"
  "APP_TARGET_FILE_DIR=$<TARGET_FILE_DIR:fuzz_pre_app>"
  "APP_LINKER_FILE=$<TARGET_LINKER_FILE:fuzz_pre_shared>"
  "APP_LINKER_FILE_NAME=$<TARGET_LINKER_FILE_NAME:fuzz_pre_shared>"
  "APP_LINKER_FILE_BASE=$<TARGET_LINKER_FILE_BASE_NAME:fuzz_pre_shared>"
  "APP_SONAME_FILE=$<TARGET_SONAME_FILE:fuzz_pre_shared>"
  "APP_SONAME_FILE_NAME=$<TARGET_SONAME_FILE_NAME:fuzz_pre_shared>"
  "APP_GENEX_EVAL=$<GENEX_EVAL:$<TARGET_PROPERTY:fuzz_pre_core,TYPE>>"
  "APP_TARGET_GENEX_EVAL=$<TARGET_GENEX_EVAL:fuzz_pre_core,$<TARGET_PROPERTY:fuzz_pre_core,TYPE>>"
  "APP_IF_LIST=$<IF:$<IN_LIST:Debug,Debug;Release>,yes,no>"
  "APP_BOOL_CHAIN=$<AND:$<BOOL:1>,$<NOT:$<BOOL:0>>>"
  "APP_JOIN=$<JOIN:aa;bb;cc,:>"
  "APP_EQUAL=$<EQUAL:7,7>"
  "APP_STRLESS=$<STRLESS:aa,bb>"
  "APP_STRGE=$<STRGREATER_EQUAL:bb,aa>"
  "APP_VER_GT=$<VERSION_GREATER:2.1,2.0>"
  "APP_VER_EQ=$<VERSION_EQUAL:1.2.3,1.2.3>"
  "APP_FILTER=$<FILTER:a;b;c;d,EXCLUDE,^[cd]>"
  "APP_RMDUP=$<REMOVE_DUPLICATES:x;x;y;z;z>"
  "APP_PATH_BASE=$<PATH:GET_BASENAME,/tmp/a/b/c.txt>"
  "APP_PATH_EXT=$<PATH:GET_EXTENSION,/tmp/a/b/c.txt>"
  "APP_PATH_FILENAME=$<PATH:GET_FILENAME,/tmp/a/b/c.txt>"
  "APP_PATH_ROOT=$<PATH:HAS_ROOT_PATH,/tmp/a/b/c.txt>"
  "APP_SHELL_PATH=$<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}>"
  "APP_COMMA=$<COMMA>"
  "APP_SEMI=$<SEMICOLON>"
  "APP_QUOTE=$<QUOTE>"
)
target_compile_options(fuzz_pre_core PRIVATE
  "$<$<COMPILE_LANGUAGE:C>:-Wall>"
  "$<$<COMPILE_LANGUAGE:CXX>:-Wextra>"
)
target_compile_options(fuzz_pre_app PRIVATE
  "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:-DAPP_COMPILE_C=1>"
  "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:-DAPP_COMPILE_CXX=1>"
)
target_link_options(fuzz_pre_app PRIVATE
  "$<$<CXX_COMPILER_ID:GNU,Clang>:-Wl,--as-needed>"
  "$<$<CONFIG:Release>:-s>"
  "$<$<LINK_LANG_AND_ID:CXX,GNU,Clang>:-Wl,--no-undefined>"
)
target_link_directories(fuzz_pre_app PRIVATE
  "$<$<CONFIG:Debug>:${CMAKE_BINARY_DIR}/debuglib>"
  "$<$<NOT:$<CONFIG:Debug>>:${CMAKE_BINARY_DIR}/releaselib>"
)
target_link_libraries(fuzz_pre_app PRIVATE
  "$<LINK_ONLY:fuzz_pre_core>"
  "$<LINK_ONLY:fuzz_pre_shared>"
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
target_compile_features(fuzz_pre_app PRIVATE cxx_std_11)
if(COMMAND target_precompile_headers)
  target_precompile_headers(fuzz_pre_app PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mylib.h"
  )
endif()
get_target_property(_fuzz_pre_core_type fuzz_pre_core TYPE)

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
set_directory_properties(PROPERTIES
  ADDITIONAL_MAKE_CLEAN_FILES "${CMAKE_BINARY_DIR}/legacy_write.txt"
)
get_directory_property(_dir_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
get_directory_property(_dir_project_name DEFINITION PROJECT_NAME)
add_custom_target(fuzz_stamp ALL
  COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_BINARY_DIR}/fuzz.stamp"
  BYPRODUCTS "${CMAKE_BINARY_DIR}/fuzz.stamp"
  DEPENDS fuzz_pre_app
  VERBATIM
)
enable_testing()
add_test(NAME fuzz_pre_app_smoke COMMAND fuzz_pre_app)
set_tests_properties(fuzz_pre_app_smoke PROPERTIES WILL_FAIL FALSE)
get_test_property(fuzz_pre_app_smoke WILL_FAIL _test_will_fail)
get_cmake_property(_all_vars VARIABLES)
site_name(_site_name_value)
set(_try_compile_result 0)
set(_try_run_result 0)
set(_remove_list alpha beta gamma delta)
remove(_remove_list beta delta)
write_file("${CMAKE_BINARY_DIR}/legacy_write.txt" "legacy_write_file\n")
write_file("${CMAKE_BINARY_DIR}/legacy_write.txt" "legacy_append\n" APPEND)
if(COMMAND cmake_file_api)
  cmake_file_api(
    QUERY
    API_VERSION 1
    CODEMODEL 2
    CACHE 2
    CMAKEFILES 1
    TOOLCHAINS 1
  )
endif()
if(COMMAND cmake_instrumentation)
  cmake_instrumentation(
    API_VERSION 1
    DATA_VERSION 1
    HOOKS postGenerate
    OPTIONS trace
    CALLBACK ${CMAKE_COMMAND} -E echo instrumentation_callback
  )
endif()
cmake_language(EVAL CODE "set(_lang_eval_value 1)")
cmake_language(CALL message STATUS "cmake_language_call")
cmake_language(DEFER CALL message STATUS "cmake_language_defer")
cmake_language(DEFER GET_CALL_IDS _lang_defer_ids)
set(_sep_args "arg1 arg2 \"arg 3\"")
separate_arguments(_sep_args UNIX_COMMAND "${_sep_args}")
get_filename_component(_cfg_name "${CMAKE_BINARY_DIR}/cfg.out" NAME)
function(fuzz_helper value)
  set(_fuzz_helper_value "${value}" PARENT_SCOPE)
endfunction()
macro(fuzz_assign name value)
  set(${name} "${value}")
endmacro()
fuzz_helper("helper_value")
fuzz_assign(_macro_value "macro_set")
cmake_parse_arguments(FZ "OPTIONAL" "ONE" "MULTI" OPTIONAL ONE one MULTI a b c)
block(SCOPE_FOR VARIABLES)
  set(_block_count 0)
  foreach(_it IN ITEMS a b c d)
    if(_it STREQUAL "b")
      continue()
    endif()
    math(EXPR _block_count "${_block_count} + 1")
  endforeach()
endblock()
set(_sample_list one two three)
list(APPEND _sample_list four)
list(PREPEND _sample_list zero)
list(INSERT _sample_list 2 inserted)
list(POP_BACK _sample_list _sample_last)
list(POP_FRONT _sample_list _sample_first)
list(REMOVE_AT _sample_list 0)
list(REMOVE_ITEM _sample_list two)
list(JOIN _sample_list ":" _sample_joined)
list(REMOVE_DUPLICATES _sample_list)
list(REVERSE _sample_list)
list(SORT _sample_list)
list(SUBLIST _sample_list 0 2 _sample_sublist)
list(FILTER _sample_list INCLUDE REGEX "^[A-Za-z]")
list(TRANSFORM _sample_list APPEND "_x")
list(TRANSFORM _sample_list TOUPPER)
list(LENGTH _sample_list _sample_len)
list(GET _sample_list 0 _sample_first_item)
list(FIND _sample_list "THREE_X" _sample_find_idx)
list(TRANSFORM _sample_list PREPEND "P_")
list(TRANSFORM _sample_list REPLACE "^P_" "")
list(TRANSFORM _sample_list STRIP)
string(LENGTH "abcdef" _strlen)
string(CONCAT _concat_value A B C)
string(SUBSTRING "abcdef" 1 3 _substr)
string(REPLACE ":" ";" _replace_out "${_sample_joined}")
string(FIND "abcdef" "cd" _find_index)
string(FIND "abcabc" "bc" _find_reverse REVERSE)
string(REPEAT "x" 3 _repeat_value)
string(REGEX REPLACE "a" "A" _regex_value "banana")
string(TOUPPER "mixed" _upper_value)
string(TOLOWER "MIXED" _lower_value)
string(JSON _json_value GET "{\"obj\":{\"k\":\"v\"}}" obj k)
string(MAKE_C_IDENTIFIER "fuzz-value-1" _c_ident)
string(GENEX_STRIP "$<CONFIG>" _genex_stripped)
string(TIMESTAMP _timestamp_value "%Y-%m-%dT%H:%M:%S" UTC)
string(UUID _uuid_value
  NAMESPACE 6ba7b810-9dad-11d1-80b4-00c04fd430c8
  NAME fuzz
  TYPE SHA1
)
set(_string_value "alpha")
string(APPEND _string_value "_beta")
string(PREPEND _string_value "pre_")
string(JOIN "|" _string_joined one two three)
string(HEX "fuzz" _hex_value)
string(COMPARE LESS "abc" "bcd" _cmp_less)
string(COMPARE EQUAL "${_string_value}" "${_string_value}" _cmp_equal)
string(REGEX MATCHALL "[a-z]+" _regex_matches "A1b2c3")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E echo execute_process_path
  OUTPUT_VARIABLE _execute_process_out
)
exec_program("${CMAKE_COMMAND}"
  ARGS "-E echo exec_program_path"
  OUTPUT_VARIABLE _exec_program_out
  RETURN_VALUE _exec_program_ret
)
file(WRITE "${CMAKE_BINARY_DIR}/cfg.in" "project=@PROJECT_NAME@\n")
configure_file("${CMAKE_BINARY_DIR}/cfg.in" "${CMAKE_BINARY_DIR}/cfg.out" @ONLY)

file(WRITE "${CMAKE_BINARY_DIR}/input.txt" "alpha\nbeta\ngamma\n")
file(APPEND "${CMAKE_BINARY_DIR}/input.txt" "delta\n")
file(READ "${CMAKE_BINARY_DIR}/input.txt" _fread LIMIT 128)
file(READ "${CMAKE_BINARY_DIR}/input.txt" _fread_hex HEX LIMIT 32 OFFSET 1)
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
file(GLOB _glob_rel LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE _glob_rel_src LIST_DIRECTORIES false RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(RELATIVE_PATH _rel "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_BINARY_DIR}")
file(REAL_PATH "${CMAKE_BINARY_DIR}/renamed.txt" _real BASE_DIRECTORY "${CMAKE_BINARY_DIR}")
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" _cm_path)
file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}" _native_path)
file(TOUCH "${CMAKE_BINARY_DIR}/touch.stamp")
file(TOUCH_NOCREATE "${CMAKE_BINARY_DIR}/touch.stamp" "${CMAKE_BINARY_DIR}/not_created.stamp")
file(CONFIGURE
  OUTPUT "${CMAKE_BINARY_DIR}/file_configured.txt"
  CONTENT "name=@PROJECT_NAME@\n"
  @ONLY
)
file(CHMOD "${CMAKE_BINARY_DIR}/renamed.txt"
  PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
)
file(CREATE_LINK "${CMAKE_BINARY_DIR}/renamed.txt"
  "${CMAKE_BINARY_DIR}/renamed.link"
  SYMBOLIC RESULT _link_result COPY_ON_ERROR
)
file(CREATE_LINK "${CMAKE_BINARY_DIR}/renamed.txt"
  "${CMAKE_BINARY_DIR}/renamed_symlink"
  SYMBOLIC RESULT _symlink_result
)
file(COPY_FILE "${CMAKE_BINARY_DIR}/input.txt"
  "${CMAKE_BINARY_DIR}/copied_if_different.txt"
  ONLY_IF_DIFFERENT
  INPUT_MAY_BE_RECENT
  RESULT _copy_if_diff_result
)
file(RENAME "${CMAKE_BINARY_DIR}/copied_if_different.txt"
  "${CMAKE_BINARY_DIR}/renamed_no_replace.txt"
  RESULT _rename_result
  NO_REPLACE
)
if(_symlink_result STREQUAL "0")
  file(READ_SYMLINK "${CMAKE_BINARY_DIR}/renamed_symlink" _read_symlink_value)
endif()
file(CHMOD_RECURSE "${CMAKE_BINARY_DIR}/copied_include"
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
  DIRECTORY_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
file(COPY "${CMAKE_BINARY_DIR}/renamed_symlink"
  DESTINATION "${CMAKE_BINARY_DIR}/copied_symlink_chain"
  FOLLOW_SYMLINK_CHAIN
)
file(LOCK "${CMAKE_BINARY_DIR}/lock-area"
  DIRECTORY
  GUARD FILE
  RESULT_VARIABLE _lock_result
  TIMEOUT 0
)
file(LOCK "${CMAKE_BINARY_DIR}/lock-area" DIRECTORY RELEASE)
file(DOWNLOAD "file://${CMAKE_BINARY_DIR}/input.txt" "${CMAKE_BINARY_DIR}/downloaded.txt"
  STATUS _download_status
  LOG _download_log
)
if(EXISTS "${CMAKE_COMMAND}")
  file(GET_RUNTIME_DEPENDENCIES
    RESOLVED_DEPENDENCIES_VAR _runtime_deps
    UNRESOLVED_DEPENDENCIES_VAR _runtime_unresolved
    EXECUTABLES "${CMAKE_COMMAND}"
    POST_EXCLUDE_REGEXES "^$"
  )
endif()
file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/tmp_remove")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/mk/a" "${CMAKE_BINARY_DIR}/mk/b" RESULT _mk_result)
file(REMOVE "${CMAKE_BINARY_DIR}/mk/b/does_not_exist.txt")

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

source_group("fuzz/legacy" FILES main.c app.c)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "tree" FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/mylib.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
)
define_property(SOURCE PROPERTY FUZZ_SOURCE_TAG
  BRIEF_DOCS "fuzz source tag"
  FULL_DOCS "fuzz source tag"
)
set_property(SOURCE main.c PROPERTY FUZZ_SOURCE_TAG "seeded")
get_source_file_property(_main_language main.c LANGUAGE)
build_command(_fuzz_build_cmd
  CONFIGURATION Debug
  PARALLEL_LEVEL 2
  TARGET fuzz_pre_app
)
cmake_host_system_information(RESULT _host_info
  QUERY NUMBER_OF_LOGICAL_CORES HOSTNAME OS_NAME
)
set(_loop_count 0)
while(_loop_count LESS 3)
  math(EXPR _loop_count "${_loop_count} + 1")
endwhile()

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/gen_info_$<CONFIG>.txt"
  CONTENT
    "cfg=$<CONFIG>\n"
    "platform=$<PLATFORM_ID>\n"
    "compiler=$<CXX_COMPILER_ID>\n"
    "app_file=$<TARGET_FILE:fuzz_pre_app>\n"
    "core_type=$<TARGET_PROPERTY:fuzz_pre_core,TYPE>\n"
    "core_exists=$<BOOL:$<TARGET_EXISTS:fuzz_pre_core>>\n"
    "core_target_name=$<TARGET_NAME_IF_EXISTS:fuzz_pre_core>\n"
    "is_debug=$<IF:$<CONFIG:Debug>,1,0>\n"
    "joined=$<JOIN:$<TARGET_PROPERTY:fuzz_pre_core,INCLUDE_DIRECTORIES>,:>\n"
    "target_file=$<TARGET_FILE:fuzz_pre_app>\n"
    "target_file_name=$<TARGET_FILE_NAME:fuzz_pre_app>\n"
    "target_file_dir=$<TARGET_FILE_DIR:fuzz_pre_app>\n"
    "target_file_base=$<TARGET_FILE_BASE_NAME:fuzz_pre_app>\n"
    "linker_file=$<TARGET_LINKER_FILE:fuzz_pre_shared>\n"
    "linker_file_name=$<TARGET_LINKER_FILE_NAME:fuzz_pre_shared>\n"
    "soname_file=$<TARGET_SONAME_FILE:fuzz_pre_shared>\n"
    "soname_file_name=$<TARGET_SONAME_FILE_NAME:fuzz_pre_shared>\n"
    "type_eval=$<GENEX_EVAL:$<TARGET_PROPERTY:fuzz_pre_core,TYPE>>\n"
    "target_eval=$<TARGET_GENEX_EVAL:fuzz_pre_core,$<TARGET_PROPERTY:fuzz_pre_core,TYPE>>\n"
)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/gen_info2_$<CONFIG>.txt"
  CONTENT
    "equal=$<EQUAL:42,42>\n"
    "str_eq=$<STREQUAL:abc,abc>\n"
    "str_less=$<STRLESS:abc,bcd>\n"
    "str_greater=$<STRGREATER:bcd,abc>\n"
    "ver_le=$<VERSION_LESS_EQUAL:3.0,3.1>\n"
    "ver_ge=$<VERSION_GREATER_EQUAL:3.1,3.0>\n"
    "in_list=$<IN_LIST:beta,alpha;beta;gamma>\n"
    "filter=$<FILTER:one;two;three;four,INCLUDE,o>\n"
    "rmdup=$<REMOVE_DUPLICATES:a;a;b;c;c>\n"
    "path_name=$<PATH:GET_FILENAME,/tmp/fuzz/a.txt>\n"
    "path_stem=$<PATH:GET_STEM,/tmp/fuzz/a.txt>\n"
    "path_ext=$<PATH:GET_EXTENSION,/tmp/fuzz/a.txt>\n"
    "path_root=$<PATH:HAS_ROOT_PATH,/tmp/fuzz/a.txt>\n"
    "target_policy=$<TARGET_POLICY:CMP0054>\n"
    "compile_features=$<COMPILE_FEATURES:cxx_std_11>\n"
    "compile_lang_id_c=$<COMPILE_LANG_AND_ID:C,GNU,Clang>\n"
    "compile_lang_id_cxx=$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>\n"
    "link_lang_id=$<LINK_LANG_AND_ID:CXX,GNU,Clang>\n"
    "c_linker_id=$<C_COMPILER_LINKER_ID>\n"
    "cxx_linker_id=$<CXX_COMPILER_LINKER_ID>\n"
    "c_linker_frontend=$<C_COMPILER_LINKER_FRONTEND_VARIANT>\n"
    "cxx_linker_frontend=$<CXX_COMPILER_LINKER_FRONTEND_VARIANT>\n"
    "target_prefix=$<TARGET_FILE_PREFIX:fuzz_pre_app>\n"
    "target_suffix=$<TARGET_FILE_SUFFIX:fuzz_pre_app>\n"
    "linker_prefix=$<TARGET_LINKER_FILE_PREFIX:fuzz_pre_shared>\n"
    "linker_suffix=$<TARGET_LINKER_FILE_SUFFIX:fuzz_pre_shared>\n"
    "linker_lib_file=$<TARGET_LINKER_LIBRARY_FILE:fuzz_pre_shared>\n"
    "linker_lib_name=$<TARGET_LINKER_LIBRARY_FILE_NAME:fuzz_pre_shared>\n"
    "linker_lib_dir=$<TARGET_LINKER_LIBRARY_FILE_DIR:fuzz_pre_shared>\n"
    "linker_lib_base=$<TARGET_LINKER_LIBRARY_FILE_BASE_NAME:fuzz_pre_shared>\n"
    "target_intermediate=$<TARGET_INTERMEDIATE_DIR:fuzz_pre_app>\n"
    "source_exists=$<SOURCE_EXISTS:${CMAKE_CURRENT_SOURCE_DIR}/main.c>\n"
    "source_prop=$<SOURCE_PROPERTY:${CMAKE_CURRENT_SOURCE_DIR}/main.c,LANGUAGE>\n"
    "fileset_exists=$<FILE_SET_EXISTS:fuzz_pre_app,headers>\n"
    "fileset_type=$<FILE_SET_PROPERTY:fuzz_pre_app,headers,TYPE>\n"
)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/genex_matrix_$<CONFIG>.txt"
  CONTENT
    "strless_equal=$<STRLESS_EQUAL:abc,abc>\n"
    "strgreater=$<STRGREATER:bcd,abc>\n"
    "target_name=$<TARGET_NAME:fuzz_pre_app>\n"
    "path_equal=$<PATH_EQUAL:/tmp/a/../a,/tmp/a>\n"
    "path_root_name=$<PATH:GET_ROOT_NAME,/tmp/fuzz/a.txt>\n"
    "path_root_dir=$<PATH:GET_ROOT_DIRECTORY,/tmp/fuzz/a.txt>\n"
    "path_root_path=$<PATH:GET_ROOT_PATH,/tmp/fuzz/a.txt>\n"
    "path_filename=$<PATH:GET_FILENAME,/tmp/fuzz/a.txt>\n"
    "path_ext=$<PATH:GET_EXTENSION,/tmp/fuzz/archive.tar.gz>\n"
    "path_ext_last=$<PATH:GET_EXTENSION,LAST_ONLY,/tmp/fuzz/archive.tar.gz>\n"
    "path_stem=$<PATH:GET_STEM,/tmp/fuzz/archive.tar.gz>\n"
    "path_stem_last=$<PATH:GET_STEM,LAST_ONLY,/tmp/fuzz/archive.tar.gz>\n"
    "path_rel_part=$<PATH:GET_RELATIVE_PART,/tmp/fuzz/a.txt>\n"
    "path_parent=$<PATH:GET_PARENT_PATH,/tmp/fuzz/a.txt>\n"
    "path_has_root_name=$<PATH:HAS_ROOT_NAME,/tmp/fuzz/a.txt>\n"
    "path_has_root_dir=$<PATH:HAS_ROOT_DIRECTORY,/tmp/fuzz/a.txt>\n"
    "path_has_root_path=$<PATH:HAS_ROOT_PATH,/tmp/fuzz/a.txt>\n"
    "path_has_filename=$<PATH:HAS_FILENAME,/tmp/fuzz/a.txt>\n"
    "path_has_ext=$<PATH:HAS_EXTENSION,/tmp/fuzz/a.txt>\n"
    "path_has_stem=$<PATH:HAS_STEM,/tmp/fuzz/a.txt>\n"
    "path_has_rel=$<PATH:HAS_RELATIVE_PART,/tmp/fuzz/a.txt>\n"
    "path_has_parent=$<PATH:HAS_PARENT_PATH,/tmp/fuzz/a.txt>\n"
    "path_is_abs=$<PATH:IS_ABSOLUTE,/tmp/fuzz/a.txt>\n"
    "path_is_rel=$<PATH:IS_RELATIVE,foo/bar>\n"
    "path_is_prefix=$<PATH:IS_PREFIX,/tmp/fuzz,/tmp/fuzz/a/b>\n"
    "path_is_prefix_norm=$<PATH:IS_PREFIX,NORMALIZE,/tmp/fuzz/./a,/tmp/fuzz/a/b>\n"
    "path_cmake=$<PATH:CMAKE_PATH,/tmp/fuzz/a/b>\n"
    "path_cmake_norm=$<PATH:CMAKE_PATH,NORMALIZE,/tmp/fuzz/a/../b>\n"
    "path_native=$<PATH:NATIVE_PATH,/tmp/fuzz/a/b>\n"
    "path_native_norm=$<PATH:NATIVE_PATH,NORMALIZE,/tmp/fuzz/a/../b>\n"
    "path_append=$<PATH:APPEND,/tmp,fuzz,a,b.txt>\n"
    "path_rm_filename=$<PATH:REMOVE_FILENAME,/tmp/fuzz/a.txt>\n"
    "path_replace_filename=$<PATH:REPLACE_FILENAME,/tmp/fuzz/a.txt,b.txt>\n"
    "path_rm_ext=$<PATH:REMOVE_EXTENSION,/tmp/fuzz/archive.tar.gz>\n"
    "path_rm_ext_last=$<PATH:REMOVE_EXTENSION,LAST_ONLY,/tmp/fuzz/archive.tar.gz>\n"
    "path_replace_ext=$<PATH:REPLACE_EXTENSION,/tmp/fuzz/a.txt,.cfg>\n"
    "path_replace_ext_last=$<PATH:REPLACE_EXTENSION,LAST_ONLY,/tmp/fuzz/archive.tar.gz,.xz>\n"
    "path_normal=$<PATH:NORMAL_PATH,/tmp/fuzz/a/../b//c.txt>\n"
    "path_relative=$<PATH:RELATIVE_PATH,/tmp/fuzz/a/b,/tmp/fuzz>\n"
    "path_absolute=$<PATH:ABSOLUTE_PATH,rel/path,/tmp/fuzz>\n"
    "path_absolute_norm=$<PATH:ABSOLUTE_PATH,NORMALIZE,../x,/tmp/fuzz/a>\n"
    "string_len=$<STRING:LENGTH,abcdef>\n"
    "string_sub=$<STRING:SUBSTRING,abcdef,1,3>\n"
    "string_find=$<STRING:FIND,abcbc,bc>\n"
    "string_find_from_end=$<STRING:FIND,abcbc,bc,FROM:END>\n"
    "string_match_once=$<STRING:MATCH,abc123,^[a-z]+,SEEK:ONCE>\n"
    "string_match_all=$<STRING:MATCH,a1b2c3,[a-z],SEEK:ALL>\n"
    "string_join=$<STRING:JOIN,|,aa,bb,cc>\n"
    "string_ascii=$<STRING:ASCII,65,66,67>\n"
    "string_timestamp=$<STRING:TIMESTAMP,%Y-%m-%d,UTC>\n"
    "string_random=$<STRING:RANDOM,ALPHABET:abc123,LENGTH:8,RANDOM_SEED:7>\n"
    "string_uuid=$<STRING:UUID,NAMESPACE:6ba7b810-9dad-11d1-80b4-00c04fd430c8,NAME:fuzz,TYPE:SHA1,CASE:UPPER>\n"
    "string_replace=$<STRING:REPLACE,bananas,na,XX>\n"
    "string_replace_regex=$<STRING:REPLACE,REGEX,a1b2c3,[0-9],_>\n"
    "string_append=$<STRING:APPEND,alpha,_beta,_gamma>\n"
    "string_prepend=$<STRING:PREPEND,tail,head_>\n"
    "string_tolower=$<STRING:TOLOWER,HeLLo>\n"
    "string_toupper=$<STRING:TOUPPER,HeLLo>\n"
    "string_strip=$<STRING:STRIP,SPACES,  trim me  >\n"
    "string_quote=$<STRING:QUOTE,REGEX,a+b?.*>\n"
    "string_hex=$<STRING:HEX,fuzz>\n"
    "string_hash=$<STRING:HASH,fuzz-data,ALGORITHM:SHA256>\n"
    "string_cid=$<STRING:MAKE_C_IDENTIFIER,fuzz-value-2>\n"
    "list_len=$<LIST:LENGTH,a$<SEMICOLON>b$<SEMICOLON>c>\n"
    "list_get=$<LIST:GET,a$<SEMICOLON>b$<SEMICOLON>c,0,2>\n"
    "list_join=$<LIST:JOIN,a$<SEMICOLON>b$<SEMICOLON>c,:>\n"
    "list_sublist=$<LIST:SUBLIST,a$<SEMICOLON>b$<SEMICOLON>c$<SEMICOLON>d,1,2>\n"
    "list_find=$<LIST:FIND,a$<SEMICOLON>b$<SEMICOLON>c,b>\n"
    "list_append=$<LIST:APPEND,a$<SEMICOLON>b,c,d>\n"
    "list_prepend=$<LIST:PREPEND,a$<SEMICOLON>b,z,y>\n"
    "list_insert=$<LIST:INSERT,a$<SEMICOLON>b$<SEMICOLON>c,1,x,y>\n"
    "list_pop_back=$<LIST:POP_BACK,a$<SEMICOLON>b$<SEMICOLON>c>\n"
    "list_pop_front=$<LIST:POP_FRONT,a$<SEMICOLON>b$<SEMICOLON>c>\n"
    "list_rmdup=$<LIST:REMOVE_DUPLICATES,a$<SEMICOLON>a$<SEMICOLON>b>\n"
    "list_rm_item=$<LIST:REMOVE_ITEM,a$<SEMICOLON>b$<SEMICOLON>c,b>\n"
    "list_rm_at=$<LIST:REMOVE_AT,a$<SEMICOLON>b$<SEMICOLON>c,1>\n"
    "list_filter_inc=$<LIST:FILTER,a1$<SEMICOLON>b2$<SEMICOLON>c3,INCLUDE,^[ab]>\n"
    "list_filter_exc=$<LIST:FILTER,a1$<SEMICOLON>b2$<SEMICOLON>c3,EXCLUDE,[0-9]>\n"
    "list_transform_append=$<LIST:TRANSFORM,a$<SEMICOLON>b,APPEND,_x>\n"
    "list_transform_prepend=$<LIST:TRANSFORM,a$<SEMICOLON>b,PREPEND,p_>\n"
    "list_transform_upper=$<LIST:TRANSFORM,a$<SEMICOLON>b,TOUPPER>\n"
    "list_transform_lower=$<LIST:TRANSFORM,A$<SEMICOLON>B,TOLOWER>\n"
    "list_transform_strip=$<LIST:TRANSFORM, a $<SEMICOLON> b ,STRIP>\n"
    "list_transform_replace=$<LIST:TRANSFORM,a1$<SEMICOLON>b2,REPLACE,[0-9],_>\n"
    "list_transform_regex=$<LIST:TRANSFORM,a1$<SEMICOLON>b2$<SEMICOLON>c3,TOUPPER,REGEX,^[ab]>\n"
    "list_transform_at=$<LIST:TRANSFORM,a$<SEMICOLON>b$<SEMICOLON>c,PREPEND,p_,AT,0$<SEMICOLON>2>\n"
    "list_transform_for=$<LIST:TRANSFORM,a$<SEMICOLON>b$<SEMICOLON>c$<SEMICOLON>d,APPEND,_z,FOR,1,3,1>\n"
    "list_reverse=$<LIST:REVERSE,a$<SEMICOLON>b$<SEMICOLON>c>\n"
    "list_sort=$<LIST:SORT,b$<SEMICOLON>a$<SEMICOLON>c,COMPARE:STRING,CASE:INSENSITIVE,ORDER:DESCENDING>\n"
)

install(TARGETS fuzz_pre_core fuzz_pre_shared fuzz_pre_app
  EXPORT FuzzPreTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES DESTINATION include
)
install(TARGETS fuzz_pre_shared
  LIBRARY DESTINATION lib
  COMPONENT Runtime
  NAMELINK_COMPONENT Development
)
install(TARGETS fuzz_pre_app
  RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES "^libc\\."
    POST_EXCLUDE_REGEXES "^$"
  RUNTIME DESTINATION bin/runtime_dep
)
add_executable(fuzz_imported_tool IMPORTED GLOBAL)
set_target_properties(fuzz_imported_tool PROPERTIES
  IMPORTED_LOCATION "${CMAKE_COMMAND}"
)
install(IMPORTED_RUNTIME_ARTIFACTS fuzz_imported_tool
  DESTINATION tools/imported
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/include/mylib.h" DESTINATION include RENAME fuzz_mylib.h)
install(FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/mylib.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h"
  TYPE INCLUDE
)
install(PROGRAMS "${CMAKE_CURRENT_SOURCE_DIR}/app.c" TYPE BIN)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/" DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir/CMakeLists.txt" "
add_library(legacy_targets STATIC legacy_obj.c)
add_executable(legacy_app legacy_app.c)
target_include_directories(legacy_targets PRIVATE \"${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir/include\")
")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir/include")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir/legacy_obj.c" "int legacy_obj(void) { return 0; }\n")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir/legacy_app.c" "int main(void) { return 0; }\n")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir/include/legacy_headers.h" "#pragma once\n")
file(INSTALL
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir/include"
  DESTINATION "${CMAKE_BINARY_DIR}/legacy-subdir-copy"
  FILES_MATCHING PATTERN "*.h"
  FILE_PERMISSIONS OWNER_READ OWNER_WRITE
  USE_SOURCE_PERMISSIONS
)
subdirs(legacy_subdir)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/modern_subdir")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/modern_subdir/CMakeLists.txt" "
add_library(modern_targets STATIC modern_obj.c)
")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/modern_subdir/modern_obj.c" "int modern_obj(void) { return 0; }\n")
link_libraries(fuzz_pre_core)
add_subdirectory(modern_subdir)
get_directory_property(_modern_project_name
  DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/modern_subdir"
  DEFINITION PROJECT_NAME
)
install_targets(/legacy legacy_targets legacy_app)
install_files(/legacy .h mylib)
install_files(/legacy "^.*\\.h$")
install_files(/legacy FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/include/mylib.h"
)
install_programs(/legacy-programs app.c)
install_programs(/legacy-programs FILES
  "${CMAKE_CURRENT_SOURCE_DIR}/app.c"
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
export(EXPORT FuzzPreTargets
  FILE "${CMAKE_BINARY_DIR}/FuzzPreExportSet.cmake"
  NAMESPACE FuzzPreSet::
)
export(PACKAGE FuzzPre)
target_sources(fuzz_pre_app PRIVATE
  FILE_SET headers TYPE HEADERS
  BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
  FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mylib.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/publicinclude.h"
)
install(TARGETS fuzz_pre_app
  FILE_SET headers
  DESTINATION include/fuzz_pre_headers
)
target_sources(legacy_targets PRIVATE
  FILE_SET legacy_headers TYPE HEADERS
  BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir/include"
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir/include/legacy_headers.h"
)
aux_source_directory("${CMAKE_CURRENT_SOURCE_DIR}/legacy_subdir" LEGACY_SUBDIR_SRCS)
add_library(legacy_aux STATIC ${LEGACY_SUBDIR_SRCS})
install(TARGETS legacy_aux
  RUNTIME_DEPENDENCY_SET legacy_dep_set
)
install(RUNTIME_DEPENDENCY_SET legacy_dep_set
  DESTINATION "legacy/dep"
  PRE_EXCLUDE_REGEXES "^libc\\."
  POST_EXCLUDE_REGEXES ""
)
install(TARGETS legacy_targets EXPORT LegacyExport)
install(EXPORT LegacyExport DESTINATION legacy/export FILE FuzzLegacyExport.cmake)
if(EXISTS "${CMAKE_BINARY_DIR}/CMakeCache.txt")
  load_cache("${CMAKE_BINARY_DIR}" READ_WITH_PREFIX _cached_
    CMAKE_GENERATOR CMAKE_COMMAND
  )
endif()

file(ARCHIVE_CREATE
  OUTPUT "${CMAKE_BINARY_DIR}/sample_archive.tar"
  PATHS "${CMAKE_CURRENT_SOURCE_DIR}/include/mylib.h"
)
file(ARCHIVE_EXTRACT
  INPUT "${CMAKE_BINARY_DIR}/sample_archive.tar"
  DESTINATION "${CMAKE_BINARY_DIR}/sample_archive_extract"
)
)cmake";

  fputs(prelude, fp);
}

static void writeReliableFallbackBody(FILE* fp)
{
  static char const body[] = R"cmake(
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/prefix")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

file(WRITE "${CMAKE_BINARY_DIR}/gen.c" "int generated(void){return 0;}\n")
add_custom_command(
  OUTPUT "${CMAKE_BINARY_DIR}/generated.c"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_BINARY_DIR}/gen.c"
          "${CMAKE_BINARY_DIR}/generated.c"
  DEPENDS "${CMAKE_BINARY_DIR}/gen.c"
  VERBATIM
)
add_custom_target(fuzz_codegen DEPENDS "${CMAKE_BINARY_DIR}/generated.c")

add_library(fuzz_core STATIC core.c lib.c "${CMAKE_BINARY_DIR}/generated.c")
add_dependencies(fuzz_core fuzz_codegen)
add_library(fuzz_shared SHARED plugin.cpp)
add_executable(fuzz_app main.cpp app.c wrapper.cpp)
target_link_libraries(fuzz_app PRIVATE fuzz_core fuzz_shared)
target_include_directories(fuzz_core PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_compile_definitions(fuzz_app PRIVATE
  "APP_CONFIG=$<CONFIG>"
  "APP_TARGET=$<TARGET_FILE_NAME:fuzz_app>"
  "APP_PLATFORM=$<PLATFORM_ID>"
)

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/gen_info_$<CONFIG>.txt"
  CONTENT "cfg=$<CONFIG>\napp=$<TARGET_FILE_NAME:fuzz_app>\n")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/genex_matrix_fb_$<CONFIG>.txt"
  CONTENT
    "strless_equal=$<STRLESS_EQUAL:abc,abc>\n"
    "strgreater=$<STRGREATER:bcd,abc>\n"
    "target_name=$<TARGET_NAME:fuzz_app>\n"
    "path_equal=$<PATH_EQUAL:/tmp/a/../a,/tmp/a>\n"
    "path_root_name=$<PATH:GET_ROOT_NAME,/tmp/fuzz/a.txt>\n"
    "path_root_dir=$<PATH:GET_ROOT_DIRECTORY,/tmp/fuzz/a.txt>\n"
    "path_root_path=$<PATH:GET_ROOT_PATH,/tmp/fuzz/a.txt>\n"
    "path_filename=$<PATH:GET_FILENAME,/tmp/fuzz/a.txt>\n"
    "path_ext=$<PATH:GET_EXTENSION,/tmp/fuzz/archive.tar.gz>\n"
    "path_ext_last=$<PATH:GET_EXTENSION,LAST_ONLY,/tmp/fuzz/archive.tar.gz>\n"
    "path_stem=$<PATH:GET_STEM,/tmp/fuzz/archive.tar.gz>\n"
    "path_stem_last=$<PATH:GET_STEM,LAST_ONLY,/tmp/fuzz/archive.tar.gz>\n"
    "path_rel_part=$<PATH:GET_RELATIVE_PART,/tmp/fuzz/a.txt>\n"
    "path_parent=$<PATH:GET_PARENT_PATH,/tmp/fuzz/a.txt>\n"
    "path_has_root_name=$<PATH:HAS_ROOT_NAME,/tmp/fuzz/a.txt>\n"
    "path_has_root_dir=$<PATH:HAS_ROOT_DIRECTORY,/tmp/fuzz/a.txt>\n"
    "path_has_root_path=$<PATH:HAS_ROOT_PATH,/tmp/fuzz/a.txt>\n"
    "path_has_filename=$<PATH:HAS_FILENAME,/tmp/fuzz/a.txt>\n"
    "path_has_ext=$<PATH:HAS_EXTENSION,/tmp/fuzz/a.txt>\n"
    "path_has_stem=$<PATH:HAS_STEM,/tmp/fuzz/a.txt>\n"
    "path_has_rel=$<PATH:HAS_RELATIVE_PART,/tmp/fuzz/a.txt>\n"
    "path_has_parent=$<PATH:HAS_PARENT_PATH,/tmp/fuzz/a.txt>\n"
    "path_is_abs=$<PATH:IS_ABSOLUTE,/tmp/fuzz/a.txt>\n"
    "path_is_rel=$<PATH:IS_RELATIVE,foo/bar>\n"
    "path_is_prefix=$<PATH:IS_PREFIX,/tmp/fuzz,/tmp/fuzz/a/b>\n"
    "path_is_prefix_norm=$<PATH:IS_PREFIX,NORMALIZE,/tmp/fuzz/./a,/tmp/fuzz/a/b>\n"
    "path_cmake=$<PATH:CMAKE_PATH,/tmp/fuzz/a/b>\n"
    "path_cmake_norm=$<PATH:CMAKE_PATH,NORMALIZE,/tmp/fuzz/a/../b>\n"
    "path_native=$<PATH:NATIVE_PATH,/tmp/fuzz/a/b>\n"
    "path_native_norm=$<PATH:NATIVE_PATH,NORMALIZE,/tmp/fuzz/a/../b>\n"
    "path_append=$<PATH:APPEND,/tmp,fuzz,a,b.txt>\n"
    "path_rm_filename=$<PATH:REMOVE_FILENAME,/tmp/fuzz/a.txt>\n"
    "path_replace_filename=$<PATH:REPLACE_FILENAME,/tmp/fuzz/a.txt,b.txt>\n"
    "path_rm_ext=$<PATH:REMOVE_EXTENSION,/tmp/fuzz/archive.tar.gz>\n"
    "path_rm_ext_last=$<PATH:REMOVE_EXTENSION,LAST_ONLY,/tmp/fuzz/archive.tar.gz>\n"
    "path_replace_ext=$<PATH:REPLACE_EXTENSION,/tmp/fuzz/a.txt,.cfg>\n"
    "path_replace_ext_last=$<PATH:REPLACE_EXTENSION,LAST_ONLY,/tmp/fuzz/archive.tar.gz,.xz>\n"
    "path_normal=$<PATH:NORMAL_PATH,/tmp/fuzz/a/../b//c.txt>\n"
    "path_relative=$<PATH:RELATIVE_PATH,/tmp/fuzz/a/b,/tmp/fuzz>\n"
    "path_absolute=$<PATH:ABSOLUTE_PATH,rel/path,/tmp/fuzz>\n"
    "path_absolute_norm=$<PATH:ABSOLUTE_PATH,NORMALIZE,../x,/tmp/fuzz/a>\n"
    "string_len=$<STRING:LENGTH,abcdef>\n"
    "string_sub=$<STRING:SUBSTRING,abcdef,1,3>\n"
    "string_find=$<STRING:FIND,abcbc,bc>\n"
    "string_find_from_end=$<STRING:FIND,abcbc,bc,FROM:END>\n"
    "string_match_once=$<STRING:MATCH,abc123,^[a-z]+,SEEK:ONCE>\n"
    "string_match_all=$<STRING:MATCH,a1b2c3,[a-z],SEEK:ALL>\n"
    "string_join=$<STRING:JOIN,|,aa,bb,cc>\n"
    "string_ascii=$<STRING:ASCII,65,66,67>\n"
    "string_timestamp=$<STRING:TIMESTAMP,%Y-%m-%d,UTC>\n"
    "string_random=$<STRING:RANDOM,ALPHABET:abc123,LENGTH:8,RANDOM_SEED:7>\n"
    "string_uuid=$<STRING:UUID,NAMESPACE:6ba7b810-9dad-11d1-80b4-00c04fd430c8,NAME:fuzz,TYPE:SHA1,CASE:UPPER>\n"
    "string_replace=$<STRING:REPLACE,bananas,na,XX>\n"
    "string_replace_regex=$<STRING:REPLACE,REGEX,a1b2c3,[0-9],_>\n"
    "string_append=$<STRING:APPEND,alpha,_beta,_gamma>\n"
    "string_prepend=$<STRING:PREPEND,tail,head_>\n"
    "string_tolower=$<STRING:TOLOWER,HeLLo>\n"
    "string_toupper=$<STRING:TOUPPER,HeLLo>\n"
    "string_strip=$<STRING:STRIP,SPACES,  trim me  >\n"
    "string_quote=$<STRING:QUOTE,REGEX,a+b?.*>\n"
    "string_hex=$<STRING:HEX,fuzz>\n"
    "string_hash=$<STRING:HASH,fuzz-data,ALGORITHM:SHA256>\n"
    "string_cid=$<STRING:MAKE_C_IDENTIFIER,fuzz-value-2>\n"
    "list_len=$<LIST:LENGTH,a$<SEMICOLON>b$<SEMICOLON>c>\n"
    "list_get=$<LIST:GET,a$<SEMICOLON>b$<SEMICOLON>c,0,2>\n"
    "list_join=$<LIST:JOIN,a$<SEMICOLON>b$<SEMICOLON>c,:>\n"
    "list_sublist=$<LIST:SUBLIST,a$<SEMICOLON>b$<SEMICOLON>c$<SEMICOLON>d,1,2>\n"
    "list_find=$<LIST:FIND,a$<SEMICOLON>b$<SEMICOLON>c,b>\n"
    "list_append=$<LIST:APPEND,a$<SEMICOLON>b,c,d>\n"
    "list_prepend=$<LIST:PREPEND,a$<SEMICOLON>b,z,y>\n"
    "list_insert=$<LIST:INSERT,a$<SEMICOLON>b$<SEMICOLON>c,1,x,y>\n"
    "list_pop_back=$<LIST:POP_BACK,a$<SEMICOLON>b$<SEMICOLON>c>\n"
    "list_pop_front=$<LIST:POP_FRONT,a$<SEMICOLON>b$<SEMICOLON>c>\n"
    "list_rmdup=$<LIST:REMOVE_DUPLICATES,a$<SEMICOLON>a$<SEMICOLON>b>\n"
    "list_rm_item=$<LIST:REMOVE_ITEM,a$<SEMICOLON>b$<SEMICOLON>c,b>\n"
    "list_rm_at=$<LIST:REMOVE_AT,a$<SEMICOLON>b$<SEMICOLON>c,1>\n"
    "list_filter_inc=$<LIST:FILTER,a1$<SEMICOLON>b2$<SEMICOLON>c3,INCLUDE,^[ab]>\n"
    "list_filter_exc=$<LIST:FILTER,a1$<SEMICOLON>b2$<SEMICOLON>c3,EXCLUDE,[0-9]>\n"
    "list_transform_append=$<LIST:TRANSFORM,a$<SEMICOLON>b,APPEND,_x>\n"
    "list_transform_prepend=$<LIST:TRANSFORM,a$<SEMICOLON>b,PREPEND,p_>\n"
    "list_transform_upper=$<LIST:TRANSFORM,a$<SEMICOLON>b,TOUPPER>\n"
    "list_transform_lower=$<LIST:TRANSFORM,A$<SEMICOLON>B,TOLOWER>\n"
    "list_transform_strip=$<LIST:TRANSFORM, a $<SEMICOLON> b ,STRIP>\n"
    "list_transform_replace=$<LIST:TRANSFORM,a1$<SEMICOLON>b2,REPLACE,[0-9],_>\n"
    "list_transform_regex=$<LIST:TRANSFORM,a1$<SEMICOLON>b2$<SEMICOLON>c3,TOUPPER,REGEX,^[ab]>\n"
    "list_transform_at=$<LIST:TRANSFORM,a$<SEMICOLON>b$<SEMICOLON>c,PREPEND,p_,AT,0$<SEMICOLON>2>\n"
    "list_transform_for=$<LIST:TRANSFORM,a$<SEMICOLON>b$<SEMICOLON>c$<SEMICOLON>d,APPEND,_z,FOR,1,3,1>\n"
    "list_reverse=$<LIST:REVERSE,a$<SEMICOLON>b$<SEMICOLON>c>\n"
    "list_sort=$<LIST:SORT,b$<SEMICOLON>a$<SEMICOLON>c,COMPARE:STRING,CASE:INSENSITIVE,ORDER:DESCENDING>\n"
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/subdir")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/subdir/CMakeLists.txt"
  "add_library(sub_lib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/sub_lib.c)\n")
file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/sub_lib.c" "int sub_lib(void){return 0;}\n")
add_subdirectory(subdir)
target_link_libraries(fuzz_app PRIVATE sub_lib)

enable_testing()
add_test(NAME fuzz_app_smoke COMMAND fuzz_app)

install(TARGETS fuzz_core fuzz_shared fuzz_app
  EXPORT FuzzTargets
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(EXPORT FuzzTargets
  FILE FuzzTargets.cmake
  NAMESPACE Fuzz::
  DESTINATION lib/cmake/Fuzz
)
)cmake";

  fputs(body, fp);
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
  if (data && contentSize > 0) {
    writeDeterministicPrelude(fp);
    fwrite(data, 1, contentSize, fp);
  } else {
    writeReliableFallbackBody(fp);
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

  if (cm.GetGlobalGenerator()) {
    auto const& lgs = cm.GetGlobalGenerator()->GetLocalGenerators();
    if (!lgs.empty()) {
      cmLocalGenerator* lg = lgs.front().get();
      cmGeneratorTarget const* headTarget =
        cm.GetGlobalGenerator()->FindGeneratorTarget("fuzz_pre_app");
      if (!headTarget) {
        headTarget = cm.GetGlobalGenerator()->FindGeneratorTarget("fuzz_app");
      }

      static char const* const genexCases[] = {
        "$<STRLESS_EQUAL:abc,abc>",
        "$<STRGREATER:bcd,abc>",
        "$<TARGET_EXISTS:fuzz_pre_core>",
        "$<TARGET_EXISTS:fuzz_core>",
        "$<TARGET_NAME_IF_EXISTS:fuzz_pre_core>",
        "$<TARGET_NAME_IF_EXISTS:fuzz_core>",
        "$<TARGET_NAME_IF_EXISTS:fuzz_pre_app>",
        "$<TARGET_NAME_IF_EXISTS:fuzz_app>",
        "$<TARGET_PROPERTY:fuzz_pre_core,TYPE>",
        "$<TARGET_PROPERTY:fuzz_core,TYPE>",
        "$<TARGET_PROPERTY:fuzz_pre_core,INCLUDE_DIRECTORIES>",
        "$<TARGET_PROPERTY:fuzz_core,INCLUDE_DIRECTORIES>",
        "$<TARGET_PROPERTY:fuzz_pre_app,LINK_LIBRARIES>",
        "$<TARGET_PROPERTY:fuzz_app,LINK_LIBRARIES>",
        "$<TARGET_FILE:fuzz_pre_app>",
        "$<TARGET_FILE:fuzz_app>",
        "$<TARGET_FILE_NAME:fuzz_pre_app>",
        "$<TARGET_FILE_NAME:fuzz_app>",
        "$<TARGET_FILE_DIR:fuzz_pre_app>",
        "$<TARGET_FILE_DIR:fuzz_app>",
        "$<TARGET_FILE_BASE_NAME:fuzz_pre_app>",
        "$<TARGET_FILE_BASE_NAME:fuzz_app>",
        "$<TARGET_FILE_PREFIX:fuzz_pre_app>",
        "$<TARGET_FILE_PREFIX:fuzz_app>",
        "$<TARGET_FILE_SUFFIX:fuzz_pre_app>",
        "$<TARGET_FILE_SUFFIX:fuzz_app>",
        "$<TARGET_LINKER_FILE:fuzz_pre_shared>",
        "$<TARGET_LINKER_FILE:fuzz_shared>",
        "$<TARGET_LINKER_FILE_NAME:fuzz_pre_shared>",
        "$<TARGET_LINKER_FILE_NAME:fuzz_shared>",
        "$<TARGET_LINKER_FILE_BASE_NAME:fuzz_pre_shared>",
        "$<TARGET_LINKER_FILE_BASE_NAME:fuzz_shared>",
        "$<TARGET_LINKER_LIBRARY_FILE:fuzz_pre_shared>",
        "$<TARGET_LINKER_LIBRARY_FILE:fuzz_shared>",
        "$<TARGET_LINKER_LIBRARY_FILE_NAME:fuzz_pre_shared>",
        "$<TARGET_LINKER_LIBRARY_FILE_NAME:fuzz_shared>",
        "$<TARGET_INTERMEDIATE_DIR:fuzz_pre_app>",
        "$<TARGET_INTERMEDIATE_DIR:fuzz_app>",
        "$<TARGET_POLICY:CMP0054>",
        "$<TARGET_POLICY:CMP0077>",
        "$<TARGET_OBJECTS:fuzz_pre_obj>",
        "$<GENEX_EVAL:$<TARGET_PROPERTY:fuzz_pre_core,TYPE>>",
        "$<GENEX_EVAL:$<TARGET_PROPERTY:fuzz_core,TYPE>>",
        "$<TARGET_GENEX_EVAL:fuzz_pre_core,$<TARGET_PROPERTY:fuzz_pre_core,TYPE>>",
        "$<TARGET_GENEX_EVAL:fuzz_core,$<TARGET_PROPERTY:fuzz_core,TYPE>>",
        "$<SOURCE_EXISTS:main.c>",
        "$<SOURCE_PROPERTY:main.c,LANGUAGE>",
        "$<FILE_SET_EXISTS:fuzz_pre_app,headers>",
        "$<FILE_SET_PROPERTY:fuzz_pre_app,headers,TYPE>",
        "$<COMPILE_FEATURES:cxx_std_11>",
        "$<COMPILE_LANGUAGE:C>",
        "$<COMPILE_LANGUAGE:CXX>",
        "$<COMPILE_LANG_AND_ID:C,GNU,Clang>",
        "$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>",
        "$<LINK_LANG_AND_ID:CXX,GNU,Clang>",
        "$<C_COMPILER_LINKER_ID>",
        "$<CXX_COMPILER_LINKER_ID>",
        "$<C_COMPILER_LINKER_FRONTEND_VARIANT>",
        "$<CXX_COMPILER_LINKER_FRONTEND_VARIANT>",
        "$<PATH_EQUAL:/tmp/a/../a,/tmp/a>",
        "$<PATH:GET_ROOT_NAME,/tmp/fuzz/a.txt>",
        "$<PATH:GET_ROOT_DIRECTORY,/tmp/fuzz/a.txt>",
        "$<PATH:GET_ROOT_PATH,/tmp/fuzz/a.txt>",
        "$<PATH:GET_FILENAME,/tmp/fuzz/a.txt>",
        "$<PATH:GET_EXTENSION,/tmp/fuzz/archive.tar.gz>",
        "$<PATH:GET_EXTENSION,LAST_ONLY,/tmp/fuzz/archive.tar.gz>",
        "$<PATH:GET_STEM,/tmp/fuzz/archive.tar.gz>",
        "$<PATH:GET_STEM,LAST_ONLY,/tmp/fuzz/archive.tar.gz>",
        "$<PATH:GET_RELATIVE_PART,/tmp/fuzz/a.txt>",
        "$<PATH:GET_PARENT_PATH,/tmp/fuzz/a.txt>",
        "$<PATH:HAS_ROOT_NAME,/tmp/fuzz/a.txt>",
        "$<PATH:HAS_ROOT_DIRECTORY,/tmp/fuzz/a.txt>",
        "$<PATH:HAS_ROOT_PATH,/tmp/fuzz/a.txt>",
        "$<PATH:HAS_FILENAME,/tmp/fuzz/a.txt>",
        "$<PATH:HAS_EXTENSION,/tmp/fuzz/a.txt>",
        "$<PATH:HAS_STEM,/tmp/fuzz/a.txt>",
        "$<PATH:HAS_RELATIVE_PART,/tmp/fuzz/a.txt>",
        "$<PATH:HAS_PARENT_PATH,/tmp/fuzz/a.txt>",
        "$<PATH:IS_ABSOLUTE,/tmp/fuzz/a.txt>",
        "$<PATH:IS_RELATIVE,foo/bar>",
        "$<PATH:IS_PREFIX,/tmp/fuzz,/tmp/fuzz/a/b>",
        "$<PATH:IS_PREFIX,NORMALIZE,/tmp/fuzz/./a,/tmp/fuzz/a/b>",
        "$<PATH:CMAKE_PATH,/tmp/fuzz/a/b>",
        "$<PATH:CMAKE_PATH,NORMALIZE,/tmp/fuzz/a/../b>",
        "$<PATH:NATIVE_PATH,/tmp/fuzz/a/b>",
        "$<PATH:NATIVE_PATH,NORMALIZE,/tmp/fuzz/a/../b>",
        "$<PATH:APPEND,/tmp,fuzz,a,b.txt>",
        "$<PATH:REMOVE_FILENAME,/tmp/fuzz/a.txt>",
        "$<PATH:REPLACE_FILENAME,/tmp/fuzz/a.txt,b.txt>",
        "$<PATH:REMOVE_EXTENSION,/tmp/fuzz/archive.tar.gz>",
        "$<PATH:REMOVE_EXTENSION,LAST_ONLY,/tmp/fuzz/archive.tar.gz>",
        "$<PATH:REPLACE_EXTENSION,/tmp/fuzz/a.txt,.cfg>",
        "$<PATH:REPLACE_EXTENSION,LAST_ONLY,/tmp/fuzz/archive.tar.gz,.xz>",
        "$<PATH:NORMAL_PATH,/tmp/fuzz/a/../b//c.txt>",
        "$<PATH:RELATIVE_PATH,/tmp/fuzz/a/b,/tmp/fuzz>",
        "$<PATH:ABSOLUTE_PATH,rel/path,/tmp/fuzz>",
        "$<PATH:ABSOLUTE_PATH,NORMALIZE,../x,/tmp/fuzz/a>",
        "$<STRING:LENGTH,abcdef>",
        "$<STRING:SUBSTRING,abcdef,1,3>",
        "$<STRING:FIND,abcbc,bc>",
        "$<STRING:FIND,abcbc,bc,FROM:END>",
        "$<STRING:MATCH,abc123,^[a-z]+,SEEK:ONCE>",
        "$<STRING:MATCH,a1b2c3,[a-z],SEEK:ALL>",
        "$<STRING:JOIN,|,aa,bb,cc>",
        "$<STRING:ASCII,65,66,67>",
        "$<STRING:TIMESTAMP,%Y-%m-%d,UTC>",
        "$<STRING:RANDOM,ALPHABET:abc123,LENGTH:8,RANDOM_SEED:7>",
        "$<STRING:UUID,NAMESPACE:6ba7b810-9dad-11d1-80b4-00c04fd430c8,NAME:fuzz,TYPE:SHA1,CASE:UPPER>",
        "$<STRING:REPLACE,bananas,na,XX>",
        "$<STRING:REPLACE,REGEX,a1b2c3,[0-9],_>",
        "$<STRING:APPEND,alpha,_beta,_gamma>",
        "$<STRING:PREPEND,tail,head_>",
        "$<STRING:TOLOWER,HeLLo>",
        "$<STRING:TOUPPER,HeLLo>",
        "$<STRING:STRIP,SPACES,  trim me  >",
        "$<STRING:QUOTE,REGEX,a+b?.*>",
        "$<STRING:HEX,fuzz>",
        "$<STRING:HASH,fuzz-data,ALGORITHM:SHA256>",
        "$<STRING:MAKE_C_IDENTIFIER,fuzz-value-2>",
        "$<LIST:LENGTH,a;b;c>",
        "$<LIST:GET,a;b;c,0,2>",
        "$<LIST:JOIN,a;b;c,:>",
        "$<LIST:SUBLIST,a;b;c;d,1,2>",
        "$<LIST:FIND,a;b;c,b>",
        "$<LIST:APPEND,a;b,c,d>",
        "$<LIST:PREPEND,a;b,z,y>",
        "$<LIST:INSERT,a;b;c,1,x,y>",
        "$<LIST:POP_BACK,a;b;c>",
        "$<LIST:POP_FRONT,a;b;c>",
        "$<LIST:REMOVE_DUPLICATES,a;a;b>",
        "$<LIST:REMOVE_ITEM,a;b;c,b>",
        "$<LIST:REMOVE_AT,a;b;c,1>",
        "$<LIST:FILTER,a1;b2;c3,INCLUDE,^[ab]>",
        "$<LIST:FILTER,a1;b2;c3,EXCLUDE,[0-9]>",
        "$<LIST:TRANSFORM,a;b,APPEND,_x>",
        "$<LIST:TRANSFORM,a;b,PREPEND,p_>",
        "$<LIST:TRANSFORM,a;b,TOUPPER>",
        "$<LIST:TRANSFORM,A;B,TOLOWER>",
        "$<LIST:TRANSFORM, a ; b ,STRIP>",
        "$<LIST:TRANSFORM,a1;b2,REPLACE,[0-9],_>",
        "$<LIST:TRANSFORM,a1;b2;c3,TOUPPER,REGEX,^[ab]>",
        "$<LIST:TRANSFORM,a;b;c,PREPEND,p_,AT,0;2>",
        "$<LIST:TRANSFORM,a;b;c;d,APPEND,_z,FOR,1,3,1>",
        "$<LIST:REVERSE,a;b;c>",
        "$<LIST:SORT,b;a;c,COMPARE:STRING,CASE:INSENSITIVE,ORDER:DESCENDING>",
      };

      for (char const* expr : genexCases) {
        static char const* const configs[] = { "", "Debug", "Release" };
        for (char const* cfg : configs) {
          (void)cmGeneratorExpression::Evaluate(expr, lg, cfg, headTarget);
          (void)cmGeneratorExpression::Evaluate(expr, lg, cfg, headTarget,
                                                nullptr, nullptr, "C");
          (void)cmGeneratorExpression::Evaluate(expr, lg, cfg, headTarget,
                                                nullptr, nullptr, "CXX");
        }
      }
    }
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
  cmSystemTools::ResetErrorOccurredFlag();

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
    cmSystemTools::ResetErrorOccurredFlag();
    cmSystemTools::RemoveADirectory(g_buildDir);
    cmSystemTools::MakeDirectory(g_buildDir);
    createDummySourceFiles(g_sourceDir);

    // Re-run a deterministic project body (without fuzzed suffix) to drive
    // deep generator and FileAPI code paths even when mutational input fails.
    writeProjectCache(false, 0);
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

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
 * - Sets CMAKE_MODULE_PATH so CMake can find its internal modules
 *   (CMakeCInformation.cmake, etc.) for full language setup
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include <unistd.h>

#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmMessenger.h"
#include "cmState.h"
#include "cmSystemTools.h"
#include "cmake.h"

static constexpr size_t kMaxInputSize = 256 * 1024;
static std::string g_sourceDir;
static std::string g_buildDir;
static std::string g_modulesPath;
static std::string g_cmakeRoot;

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
    { "lib.c", "void lib_func(void) {}\n" },
    { "lib.cpp", "void lib_func() {}\n" },
    { "app.c", "int main(void) { return 0; }\n" },
    { "app.cpp", "int main() { return 0; }\n" },
    { "test.c", "int main(void) { return 0; }\n" },
    { "test.cpp", "int main() { return 0; }\n" },
    { "util.c", "void util_func(void) {}\n" },
    { "util.cpp", "void util_func() {}\n" },
    { "helper.c", "void helper_func(void) {}\n" },
    { "helper.cpp", "void helper_func() {}\n" },
    { "core.c", "void core_func(void) {}\n" },
    { "plugin.cpp", "void plugin_func() {}\n" },
    { "wrapper.cpp", "void wrapper_func() {}\n" },
    { "module.c", "void module_func(void) {}\n" },
    { "include/mylib.h", "#pragma once\n" },
    { "include/config.h", "#pragma once\n" },
  };

  cmSystemTools::MakeDirectory(dir + "/include");
  cmSystemTools::MakeDirectory(dir + "/src");

  for (auto const& f : files) {
    std::string path = dir + "/" + f.name;
    FILE* fp = fopen(path.c_str(), "wb");
    if (fp) {
      fputs(f.content, fp);
      fclose(fp);
    }
    // Also create in src/ subdirectory
    std::string srcPath = dir + "/src/" + f.name;
    fp = fopen(srcPath.c_str(), "wb");
    if (fp) {
      fputs(f.content, fp);
      fclose(fp);
    }
  }
}

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
  (void)argc;

  cmSystemTools::SetMessageCallback(
    [](std::string const&, cmMessageMetadata const&) {});
  cmSystemTools::SetStdoutCallback([](std::string const&) {});
  cmSystemTools::SetStderrCallback([](std::string const&) {});

  // Find the CMake Modules directory. In OSS-Fuzz, the CMake source tree
  // is copied to <exe_dir>/src/CMake/ during the build. We need the
  // Modules/ path so enable_language() can load CMakeCInformation.cmake
  // and other internal modules needed for the generate pipeline.
  {
    std::string exeDir =
      cmSystemTools::GetFilenamePath(cmSystemTools::GetRealPath((*argv)[0]));
    std::string candidate = exeDir + "/src/CMake/Modules";
    if (cmSystemTools::FileIsDirectory(candidate)) {
      g_modulesPath = candidate;
      g_cmakeRoot = exeDir + "/src/CMake";
    }
  }

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

  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 2 || size > kMaxInputSize) {
    return 0;
  }

  // Use the last byte as a control byte for generator/feature selection.
  // This byte is NOT written to the CMakeLists.txt file — only the
  // preceding bytes are used as CMake script content.
  //   bit 0:   base generator (0=Unix Makefiles, 1=Ninja)
  //   bits 1-3: extra generator (0=none, 1=Eclipse CDT4, 2=CodeBlocks,
  //             3=CodeLite, 4=Kate, 5=Sublime Text 2)
  uint8_t ctrl = data[size - 1];
  bool useNinja = (ctrl & 1);
  int extraGen = (ctrl >> 1) & 7;
  size_t contentSize = size - 1;

  // Create dummy source files for targets to reference
  createDummySourceFiles(g_sourceDir);

  // Write a pre-populated CMakeCache.txt in the build directory.
  // This is equivalent to having already run cmake once — it tells CMake
  // the compiler is /bin/true (GNU 12.0) and skips all compiler detection.
  {
    std::string cachePath = g_buildDir + "/CMakeCache.txt";
    FILE* fp = fopen(cachePath.c_str(), "wb");
    if (fp) {
      // Alternate between generators based on control byte.
      // Ninja exercises cmNinjaTargetGenerator; Makefiles exercises
      // cmMakefileTargetGenerator — different code paths.
      const char* generator = useNinja ? "Ninja" : "Unix Makefiles";
      const char* makeProgram = useNinja ? "/usr/bin/ninja" : "/usr/bin/make";

      // Extra generator name table (0 = none)
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
  }

  // Write fuzz input as CMakeLists.txt with required preamble
  {
    std::string cmakelists = g_sourceDir + "/CMakeLists.txt";
    FILE* fp = fopen(cmakelists.c_str(), "wb");
    if (!fp)
      return 0;

    // Preamble: set CMAKE_MODULE_PATH so enable_language() can find
    // internal CMake modules, then declare the project with C and CXX.
    // Compiler detection is skipped because the CMakeCache.txt already
    // has all compiler info (COMPILER_FORCED=TRUE).
    fprintf(fp, "cmake_minimum_required(VERSION 3.10)\n");
    if (!g_modulesPath.empty()) {
      fprintf(fp, "set(CMAKE_MODULE_PATH \"%s\")\n", g_modulesPath.c_str());
    }
    fprintf(fp, "project(FuzzTest LANGUAGES C CXX)\n");
    fwrite(data, 1, contentSize, fp);
    fclose(fp);
  }

  // Create FileAPI query marker files so that cmake's ActualConfigure()
  // and Generate() exercise the FileAPI reply-writing code paths
  // (cmFileAPI, cmFileAPICodemodel, cmFileAPICache, etc.).
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
  }

  // Save CWD
  {
    std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();

    // Create cmake and run full pipeline
    cmake cm(cmState::Role::Project);
    cm.SetHomeDirectory(g_sourceDir);
    cm.SetHomeOutputDirectory(g_buildDir);

    // Load the CMakeCache.txt we wrote. This is critical — without this,
    // Configure() never sees our cache entries (compiler bypass, generator).
    cm.LoadCache(g_buildDir);
    cm.Configure();
    cm.Generate();

    cmSystemTools::ChangeDirectory(cwd);
  }

  // Clean up source and build dirs for next iteration
  cmSystemTools::RemoveADirectory(g_buildDir);
  cmSystemTools::MakeDirectory(g_buildDir);
  cmSystemTools::RemoveADirectory(g_sourceDir);
  cmSystemTools::MakeDirectory(g_sourceDir);

  return 0;
}

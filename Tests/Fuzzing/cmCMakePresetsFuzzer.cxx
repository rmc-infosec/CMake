/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file LICENSE.rst or https://cmake.org/licensing for details.  */

/*
 * Fuzzer for CMakePresets.json parsing
 *
 * Exercises the CMake presets subsystem:
 * - cmCMakePresetsGraph JSON parsing and schema validation
 * - Preset inheritance and macro expansion
 * - Condition evaluation
 * - All preset types: configure, build, test, package, workflow
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

#include <unistd.h>

#include "cmCMakePresetsGraph.h"
#include "cmSystemTools.h"

static constexpr size_t kMaxInputSize = 256 * 1024;
static std::string g_tempDir;

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
  (void)argc;
  (void)argv;

  cmSystemTools::SetMessageCallback(
    [](std::string const&, cmMessageMetadata const&) {});
  cmSystemTools::SetStdoutCallback([](std::string const&) {});
  cmSystemTools::SetStderrCallback([](std::string const&) {});

  char tmpl[] = "/tmp/cmake_fuzz_presets_XXXXXX";
  char* dir = mkdtemp(tmpl);
  if (dir) {
    g_tempDir = dir;
  } else {
    g_tempDir = "/tmp/cmake_fuzz_presets";
    cmSystemTools::MakeDirectory(g_tempDir);
  }

  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size < 2 || size > kMaxInputSize) {
    return 0;
  }

  // Write fuzz input as CMakePresets.json
  {
    std::string presetsPath = g_tempDir + "/CMakePresets.json";
    FILE* fp = fopen(presetsPath.c_str(), "wb");
    if (!fp) {
      return 0;
    }
    fwrite(data, 1, size, fp);
    fclose(fp);
  }

  // Parse it through the presets graph
  cmCMakePresetsGraph graph;
  graph.ReadProjectPresets(g_tempDir, false);

  // Clean up the file for next iteration
  cmSystemTools::RemoveFile(g_tempDir + "/CMakePresets.json");
  cmSystemTools::RemoveFile(g_tempDir + "/CMakeUserPresets.json");

  return 0;
}

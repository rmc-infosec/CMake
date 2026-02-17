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
#include <algorithm>
#include <string>

#include <unistd.h>

#include "cmCMakePresetsGraph.h"
#include "cmSystemTools.h"

static constexpr size_t kMaxInputSize = 256 * 1024;
static std::string g_tempDir;

static std::string jsonEscapePayload(uint8_t const* data, size_t size)
{
  size_t const n = std::min<size_t>(size, 4096);
  std::string out;
  out.reserve(n * 2);

  for (size_t i = 0; i < n; ++i) {
    unsigned char c = data[i];
    switch (c) {
      case '\"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (c >= 0x20 && c <= 0x7e) {
          out.push_back(static_cast<char>(c));
        } else {
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned int>(c));
          out += buf;
        }
        break;
    }
  }

  return out;
}

static std::string buildStructuredPresetsJSON(std::string const& fuzzPayload)
{
  std::string json;
  json.reserve(4096 + fuzzPayload.size());
  json += "{\n";
  json += "  \"version\": 4,\n";
  json += "  \"cmakeMinimumRequired\": { \"major\": 3, \"minor\": 20, \"patch\": 0 },\n";
  json += "  \"vendor\": { \"fuzz\": { \"blob\": \"";
  json += fuzzPayload;
  json += "\" } },\n";
  json += "  \"configurePresets\": [\n";
  json += "    {\n";
  json += "      \"name\": \"base\",\n";
  json += "      \"displayName\": \"base-";
  json += fuzzPayload;
  json += "\",\n";
  json += "      \"generator\": \"Ninja\",\n";
  json += "      \"binaryDir\": \"${sourceDir}/build/${presetName}\",\n";
  json += "      \"cacheVariables\": {\n";
  json += "        \"CMAKE_BUILD_TYPE\": \"Debug\",\n";
  json += "        \"FUZZ_PAYLOAD\": \"";
  json += fuzzPayload;
  json += "\"\n";
  json += "      },\n";
  json += "      \"environment\": {\n";
  json += "        \"FUZZ_ENV\": \"";
  json += fuzzPayload;
  json += "\"\n";
  json += "      },\n";
  json += "      \"warnings\": { \"dev\": true },\n";
  json += "      \"errors\": { \"dev\": false }\n";
  json += "    },\n";
  json += "    {\n";
  json += "      \"name\": \"release\",\n";
  json += "      \"inherits\": \"base\",\n";
  json += "      \"cacheVariables\": { \"CMAKE_BUILD_TYPE\": \"Release\" }\n";
  json += "    },\n";
  json += "    {\n";
  json += "      \"name\": \"makefiles\",\n";
  json += "      \"inherits\": [\"base\"],\n";
  json += "      \"generator\": \"Unix Makefiles\",\n";
  json += "      \"binaryDir\": \"${sourceDir}/build/${presetName}\"\n";
  json += "    }\n";
  json += "  ],\n";
  json += "  \"buildPresets\": [\n";
  json += "    { \"name\": \"build-base\", \"configurePreset\": \"base\", \"jobs\": 2 },\n";
  json += "    { \"name\": \"build-release\", \"configurePreset\": \"release\", \"targets\": [\"all\"] }\n";
  json += "  ],\n";
  json += "  \"testPresets\": [\n";
  json += "    { \"name\": \"test-base\", \"configurePreset\": \"base\", \"output\": { \"outputOnFailure\": true } }\n";
  json += "  ]\n";
  json += "}\n";
  return json;
}

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

  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < size; ++i) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  bool structuredMode = ((hash & 1u) == 0u);

  // Write fuzz input as CMakePresets.json
  {
    std::string presetsPath = g_tempDir + "/CMakePresets.json";
    FILE* fp = fopen(presetsPath.c_str(), "wb");
    if (!fp) {
      return 0;
    }
    if (structuredMode) {
      std::string fuzzPayload = jsonEscapePayload(data, size);
      std::string json = buildStructuredPresetsJSON(fuzzPayload);
      fwrite(json.data(), 1, json.size(), fp);
    } else {
      fwrite(data, 1, size, fp);
    }
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

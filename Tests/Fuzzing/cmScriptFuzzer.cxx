/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file LICENSE.rst or https://cmake.org/licensing for details.  */

/*
 * Fuzzer for CMake script execution
 *
 * This fuzzer executes CMake scripts in script mode (-P).
 * This exercises the majority of CMake's codebase including:
 * - All built-in commands
 * - Variable expansion
 * - Control flow (if, foreach, while, function, macro)
 * - String/list/file operations
 * - Generator expressions
 *
 * This is the highest-impact fuzzer for coverage.
 *
 * Performance notes:
 * - Uses memfd_create on Linux for memory-backed file I/O
 * - Falls back to temp files on other platforms
 */

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <unistd.h>

#include "cmCMakePolicyCommand.h"
#include "cmExecutionStatus.h"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmMessenger.h"
#include "cmState.h"
#include "cmStateSnapshot.h"
#include "cmSystemTools.h"
#include "cmake.h"

#ifdef __linux__
#  include <sys/mman.h>
#  ifndef MFD_CLOEXEC
#    define MFD_CLOEXEC 0x0001U
#  endif
#endif

static constexpr size_t kMaxInputSize = 256 * 1024;
static constexpr char kDeterministicPrelude[] = R"cmake(
set(CMAKE_FIND_DEBUG_MODE OFF)
file(MAKE_DIRECTORY "${_fuzz_root}/tmp/sub")
file(WRITE "${_fuzz_root}/tmp/in.txt" "alpha\nbeta\ngamma\n")
file(APPEND "${_fuzz_root}/tmp/in.txt" "delta\n")
file(READ "${_fuzz_root}/tmp/in.txt" _fuzz_read)
file(STRINGS "${_fuzz_root}/tmp/in.txt" _fuzz_lines LIMIT_COUNT 16)
file(SIZE "${_fuzz_root}/tmp/in.txt" _fuzz_size)
file(SHA1 "${_fuzz_root}/tmp/in.txt" _fuzz_sha1)
file(TIMESTAMP "${_fuzz_root}/tmp/in.txt" _fuzz_ts UTC)
file(COPY_FILE "${_fuzz_root}/tmp/in.txt" "${_fuzz_root}/tmp/copy.txt" RESULT _copy_res)
file(RENAME "${_fuzz_root}/tmp/copy.txt" "${_fuzz_root}/tmp/renamed.txt" RESULT _rename_res)
file(GLOB _fuzz_glob "${_fuzz_root}/tmp/*.txt")
file(REAL_PATH "${_fuzz_root}/tmp/renamed.txt" _fuzz_real BASE_DIRECTORY "${_fuzz_root}")
file(TO_CMAKE_PATH "${_fuzz_root}" _fuzz_cmake_path)
file(TO_NATIVE_PATH "${_fuzz_root}" _fuzz_native_path)
file(TOUCH "${_fuzz_root}/tmp/touch.stamp")
file(REMOVE "${_fuzz_root}/tmp/missing.txt")

set(_fuzz_list one two three)
list(APPEND _fuzz_list four)
list(PREPEND _fuzz_list zero)
list(TRANSFORM _fuzz_list TOUPPER)
list(FILTER _fuzz_list INCLUDE REGEX "ONE|TWO|THREE")
list(JOIN _fuzz_list ":" _fuzz_joined)
list(REVERSE _fuzz_list)
list(SORT _fuzz_list)
list(LENGTH _fuzz_list _fuzz_len)

string(REPLACE ":" ";" _fuzz_repl "${_fuzz_joined}")
string(SUBSTRING "${_fuzz_repl}" 0 8 _fuzz_sub)
string(FIND "${_fuzz_repl}" "TWO" _fuzz_idx)
string(TOLOWER "${_fuzz_repl}" _fuzz_lower)
string(TOUPPER "${_fuzz_repl}" _fuzz_upper)
string(JSON _fuzz_json GET "{\"k\":{\"v\":1}}" k v)
string(MAKE_C_IDENTIFIER "fuzz-value-1" _fuzz_ident)
string(TIMESTAMP _fuzz_now "%Y-%m-%dT%H:%M:%S" UTC)
string(GENEX_STRIP "$<CONFIG>" _fuzz_genex)
math(EXPR _fuzz_math "41 + 1")

find_program(_fuzz_sh NAMES sh bash)
find_file(_fuzz_devnull NAMES null PATHS /dev NO_DEFAULT_PATH)
find_path(_fuzz_stdio NAMES stdio.h)
find_library(_fuzz_libm NAMES m)
execute_process(COMMAND "${CMAKE_COMMAND}" -E echo script_prelude OUTPUT_VARIABLE _fuzz_echo)

if(COMMAND cmake_path)
  set(_fuzz_path "${_fuzz_root}/tmp/../tmp/in.txt")
  cmake_path(NORMAL_PATH _fuzz_path OUTPUT_VARIABLE _fuzz_norm_path)
  cmake_path(GET _fuzz_norm_path FILENAME _fuzz_filename)
endif()

if(COMMAND cmake_language)
  cmake_language(EVAL CODE "set(_fuzz_eval_ok 1)")
endif()
)cmake";
static std::string g_testDir;
static std::string g_scriptFile;
static bool g_useMemfd = false;

static bool writeScriptContent(std::string const& content)
{
#ifdef __linux__
  if (g_useMemfd) {
    // Extract fd from path and write directly
    int fd = std::atoi(g_scriptFile.c_str() + 14); // "/proc/self/fd/"
    ftruncate(fd, 0);
    lseek(fd, 0, SEEK_SET);
    return write(fd, content.data(), content.size()) ==
      static_cast<ssize_t>(content.size());
  }
#endif

  FILE* fp = fopen(g_scriptFile.c_str(), "wb");
  if (!fp) {
    return false;
  }
  bool ok = fwrite(content.data(), 1, content.size(), fp) == content.size();
  fclose(fp);
  return ok;
}

extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv)
{
  (void)argc;
  (void)argv;

  // Suppress output during fuzzing (set once at init)
  cmSystemTools::SetMessageCallback(
    [](std::string const&, cmMessageMetadata const&) {});
  cmSystemTools::SetStdoutCallback([](std::string const&) {});
  cmSystemTools::SetStderrCallback([](std::string const&) {});

  // Create unique test directory (even with memfd, scripts can create files)
  char tmpl[] = "/tmp/cmake_fuzz_script_XXXXXX";
  char* dir = mkdtemp(tmpl);
  if (dir) {
    g_testDir = dir;
  } else {
    g_testDir = "/tmp/cmake_fuzz_script";
    cmSystemTools::MakeDirectory(g_testDir);
  }

#ifdef __linux__
  // Try to use memfd for better performance
  int fd = memfd_create("cmake_fuzz", MFD_CLOEXEC);
  if (fd >= 0) {
    g_useMemfd = true;
    // Create path via /proc/self/fd
    g_scriptFile = "/proc/self/fd/" + std::to_string(fd);
    // Keep fd open - will be reused
  } else
#endif
  {
    g_scriptFile = g_testDir + "/fuzz_script.cmake";
  }

  return 0;
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const* data, size_t size)
{
  if (size == 0 || size > kMaxInputSize) {
    return 0;
  }

  std::string scriptContent;
  scriptContent.reserve(size + sizeof(kDeterministicPrelude) + 64);
  scriptContent += "set(_fuzz_root \"";
  scriptContent += g_testDir;
  scriptContent += "\")\n";
  scriptContent += kDeterministicPrelude;
  scriptContent.append(reinterpret_cast<char const*>(data), size);
  scriptContent.push_back('\n');

  if (!writeScriptContent(scriptContent)) {
    return 0;
  }

  // Save CWD in case script uses file(CHDIR)
  std::string cwd = cmSystemTools::GetCurrentWorkingDirectory();

  // Create cmake instance for script mode
  cmake cm(cmState::Role::Script);
  cm.SetHomeDirectory(g_testDir);
  cm.SetHomeOutputDirectory(g_testDir);

  // Run the script
  std::vector<std::string> args;
  args.push_back("cmake");
  args.push_back("-P");
  args.push_back(g_scriptFile);

  (void)cm.Run(args, false);

  // Restore CWD before cleanup (script may have changed it via file(CHDIR))
  cmSystemTools::ChangeDirectory(cwd);

  // Cleanup temp file (memfd doesn't need cleanup)
  if (!g_useMemfd) {
    unlink(g_scriptFile.c_str());
  }

  // Clean up any files the script may have created in g_testDir
  // This prevents disk growth and non-determinism from previous iterations
  cmSystemTools::RemoveADirectory(g_testDir);
  cmSystemTools::MakeDirectory(g_testDir);

  return 0;
}

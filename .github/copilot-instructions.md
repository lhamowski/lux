# Lux C++ Library - Copilot Coding Agent Instructions

## Project Overview

**lux** is a modern C++23 utility library providing:
- **lux core**: Logging (spdlog-based), support utilities (result types, strong typedefs, path handling, error handling), buffer I/O, memory arenas, and timing utilities
- **lux-io** (optional): Async networking (TCP/UDP sockets, SSL/TLS via BoringSSL), timers, retry policies, and coroutine integration using Boost.Asio

**Language**: C++23 (strictly enforced)  
**Build System**: CMake 3.30+, Ninja generator  
**Testing**: Catch2 v3.8.0 with `catch_discover_tests`  
**Dependencies**: Managed via CPM.cmake (FetchContent wrapper); Boost 1.87.0 required for lux-io  
**Codegen**: CFlex.cmake—a custom CMake module providing `cflex_add_library`/`cflex_add_executable` wrappers that apply compiler/linker flags per-target and per-build-type

---

## Build System Details

### CMake Presets (CMakePresets.json)

All builds **must** use presets. The repository defines:
- **Platforms**: `windows`, `linux` (includes Darwin)
- **Toolchains**: `msvc`, `gcc`, `clang`, `gcc-arm64-cross`
- **Build Types**: `debug`, `release`, `coverage`, `asan`, `tsan`, `ubsan`, `msan`

**Common presets** (use these for local development and CI reproduction):
- Linux GCC: `linux-gcc-debug`, `linux-gcc-release`, `linux-gcc-coverage`, `linux-gcc-asan`, `linux-gcc-tsan`, `linux-gcc-ubsan`
- Linux Clang: `linux-clang-debug`, `linux-clang-release`, `linux-clang-coverage`, `linux-clang-asan`, `linux-clang-tsan`, `linux-clang-ubsan`, `linux-clang-msan`
- Windows MSVC: `windows-msvc-debug`, `windows-msvc-release`, `windows-msvc-coverage`, `windows-msvc-asan`
- Visual Studio IDE: `vs2022` (uses VS generator instead of Ninja)

**Build directories**: `build/<preset-name>` (e.g., `build/linux-gcc-release`)  
**Install prefix**: `install/<preset-name>`

### CFlex.cmake

Located in `cmake/CFlex.cmake`, CFlex provides:
- `cflex_add_library(<target> [STATIC|SHARED|MODULE] SOURCES ...)`: Creates library and applies build options
- `cflex_add_executable(<target> SOURCES ...)`: Creates executable and applies build options
- `cflex_set_cxx_flags`, `cflex_set_linker_flags`: Configure compiler/linker flags per-scope (PUBLIC/PRIVATE/INTERFACE) and per-build-type

**Key points**:
- Targets defined with `cflex_add_library`/`cflex_add_executable` automatically get compiler/linker flags from `cmake/build_options/<toolchain>.cmake`
- Toolchain files: `cmake/toolchain/{gcc,clang,msvc,gcc-arm64-cross}.cmake` set compilers
- Build option files: `cmake/build_options/{gcc,clang,msvc}.cmake` configure warnings, sanitizers, optimization levels via CFlex macros
- CFlex logs applied options if `CFLEX_LOG_BUILD_OPTIONS=ON` (default)

### Dependency Management (CPM.cmake)

`external/CMakeLists.txt` fetches:
- **fmt** 11.2.0 (formatting library)
- **spdlog** 1.15.3 (logging, uses external fmt)
- **magic_enum** v0.9.7 (enum reflection)
- **BoringSSL** 0.20251124.0 (if `LUX_ENABLE_IO=ON` and `LUX_FETCH_BORINGSSL=ON`; otherwise expects system OpenSSL)
- **Catch2** v3.8.0 (if `LUX_TEST=ON`)

**Boost 1.87.0** is **required** for lux-io but **not** fetched by CPM. On Linux CI it is preinstalled in Docker images; on Windows CI it is installed via GitHub Actions. For local builds, install Boost manually and pass `-DCMAKE_PREFIX_PATH=<boost-root>` to CMake.

**CMake Options**:
- `LUX_TEST` (default ON): Build tests
- `LUX_ENABLE_IO` (default ON): Build lux-io module (requires Boost + OpenSSL/BoringSSL)
- `LUX_FETCH_DEPS` (default ON): Fetch dependencies via CPM (turn OFF to use system packages)
- `LUX_FETCH_FMT`, `LUX_FETCH_SPDLOG`, `LUX_FETCH_BORINGSSL` (default ON): Control individual dependency fetching

**CPM caching**: Set `CPM_SOURCE_CACHE` environment variable (e.g., `~/cpm-cache`) to avoid re-downloading dependencies across builds.

---

## Build Instructions

### Prerequisites

**All platforms**:
- CMake 3.30+ (repo requires this minimum)
- Ninja 1.10+
- Boost 1.87.0 (headers + compiled libraries if building lux-io)

**Linux**:
- GCC 14+ or Clang 19+ (C++23 support required)
- OpenSSL development libraries (if using system OpenSSL instead of BoringSSL)

**Windows**:
- Visual Studio 2022 (v143 toolset) with C++ workload
- MSVC Developer Command Prompt (for `cl.exe` availability)
- Ninja (install via `choco install ninja` or download from GitHub)

**Optional**:
- ccache (for faster rebuilds; CI uses ccache via `-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache`)
- clang-format 18+ (for code formatting)

### Linux Build (GCC)

```bash
# 1. Bootstrap dependencies (automatic via CPM on first configure)
export CPM_SOURCE_CACHE=~/cpm-cache  # optional, speeds up repeated configures

# 2. Configure
cmake --preset=linux-gcc-release \
      -DCMAKE_PREFIX_PATH=/path/to/boost_1.87.0

# 3. Build
cmake --build build/linux-gcc-release -j

# 4. Test
cd build/linux-gcc-release && ctest --output-on-failure
```

**Debug build**: Use `linux-gcc-debug` preset  
**Sanitizer builds**: Use `linux-gcc-asan`, `linux-gcc-tsan`, or `linux-gcc-ubsan` presets (run tests under sanitizer with `ctest -VV --output-on-failure`)

### Linux Build (Clang)

```bash
export CPM_SOURCE_CACHE=~/cpm-cache

cmake --preset=linux-clang-release \
      -DCMAKE_PREFIX_PATH=/path/to/boost_1.87.0

cmake --build build/linux-clang-release -j

cd build/linux-clang-release && ctest --output-on-failure
```

**Coverage build**: Use `linux-clang-coverage` preset. After running tests, generate coverage report:
```bash
cd build/linux-clang-coverage
LLVM_PROFILE_FILE=coverage-%p.profraw ctest --output-on-failure
llvm-profdata merge --sparse test/*.profraw -o coverage.profdata
find ./src -name '*.o' | xargs -I{} echo "-object={}" > object_list.txt
llvm-cov export ./test/lux-test \
    -instr-profile=coverage.profdata \
    -format=lcov \
    -ignore-filename-regex=".*test/.*" \
    $(cat object_list.txt) \
    > coverage.info
```

### Windows Build (MSVC + Ninja)

```powershell
# 1. Open "x64 Native Tools Command Prompt for VS 2022"
# 2. Ensure Ninja is in PATH (or use full path)

# 3. Install Boost (if not already present):
#    - Download prebuilt Boost 1.87.0 for MSVC 2022 from boost.org
#    - OR use vcpkg: vcpkg install boost:x64-windows
#    - OR build from source

# 4. Configure
cmake --preset=windows-msvc-release -DCMAKE_PREFIX_PATH=C:\path\to\boost_1_87_0

# 5. Build
cmake --build build\windows-msvc-release

# 6. Test
cd build\windows-msvc-release
ctest --output-on-failure
```

**Visual Studio IDE**: Use `vs2022` preset to generate a `.sln` file (skips Ninja, uses VS generator):
```powershell
cmake --preset=vs2022 -DCMAKE_PREFIX_PATH=C:\path\to\boost_1_87_0
cmake --open build\vs2022
```

### Common Build Errors and Mitigations

1. **"Boost not found"**: Ensure `-DCMAKE_PREFIX_PATH` points to Boost root (containing `lib/cmake/Boost-*/` or `include/boost/`)
2. **"Could not find OpenSSL"**: If `LUX_FETCH_BORINGSSL=OFF`, install OpenSSL dev packages (`libssl-dev` on Ubuntu, or pass `-DOPENSSL_ROOT_DIR=...`)
3. **"C++23 feature not supported"**: Update compiler (GCC 14+, Clang 19+, MSVC 2022)
4. **Preset not found on Windows**: Ensure `hostSystemName` matches (presets use conditional logic; Windows presets require running from Windows)
5. **Ninja not found**: Install Ninja and ensure it's in PATH, or specify `-DCMAKE_MAKE_PROGRAM=/path/to/ninja`
6. **CMake cache issues**: Delete `build/<preset>` directory and reconfigure from scratch
7. **CPM re-downloads on every configure**: Set `CPM_SOURCE_CACHE` environment variable to persistent directory

### Clean Build

To force a clean build:
```bash
rm -rf build/<preset-name>
cmake --preset=<preset-name> ...
```

Do **not** run `cmake --build build/<preset> --target clean` as this does not regenerate CFlex configurations.

---

## Project Layout

```
lux/
├── CMakeLists.txt               # Top-level: options (LUX_TEST, LUX_ENABLE_IO), find Boost, add subdirs
├── CMakePresets.json            # Preset definitions (linux-gcc-*, windows-msvc-*, etc.)
├── .github/
│   └── workflows/ci.yml         # CI: builds/tests for linux-gcc-release, sanitizers, windows-msvc-release, linux-clang-coverage
├── cmake/
│   ├── CFlex.cmake              # Custom target/flag management (cflex_add_library, cflex_add_executable)
│   ├── LuxSourceGroup.cmake     # IDE folder organization (lux_source_group)
│   ├── CPM.cmake                # CPM.cmake (dependency manager)
│   ├── toolchain/               # Compiler selection (gcc.cmake, clang.cmake, msvc.cmake, gcc-arm64-cross.cmake)
│   └── build_options/           # Per-toolchain flags (gcc.cmake, clang.cmake, msvc.cmake) - applied via CFlex
├── external/
│   └── CMakeLists.txt           # CPM fetches: fmt, spdlog, magic_enum, BoringSSL, Catch2
├── include/lux/                 # Public headers (logger, support, utils, io)
├── src/
│   ├── CMakeLists.txt           # Defines lux (core) and lux-io libraries via cflex_add_library
│   └── lux/                     # Implementation (.cpp files)
└── test/
    ├── CMakeLists.txt           # Defines lux-test via cflex_add_executable, uses catch_discover_tests
    ├── logger/, support/, utils/, io/  # Test files (one per module)
    └── <module>_test.cpp        # Catch2 tests

Key targets:
- lux::lux        (static lib, always built)
- lux::io         (static lib, built if LUX_ENABLE_IO=ON)
- lux-test        (executable, built if LUX_TEST=ON)
```

### CI Workflow (.github/workflows/ci.yml)

**Jobs**:
1. **build-and-test-linux-gcc-release**: Ubuntu 24.04, GCC 14, Boost 1.87.0 (Docker: `lhamowski/devops:ubuntu-24.04-cpp-gcc-14-boost-1.87-latest`)
2. **build-and-test-linux-gcc-sanitizers**: Matrix of `asan`, `tsan`, `ubsan` presets (GCC 14, runs only on `master`)
3. **build-and-test-windows-msvc-release**: Windows Server 2022, MSVC 2022, Boost 1.87.0 (installed via GHA action)
4. **build-and-test-linux-clang-coverage**: Ubuntu 24.04, Clang 19, generates coverage report, uploads to Codecov

**Reproduce CI locally**:
- Use same preset names (e.g., `linux-gcc-release`, `windows-msvc-release`)
- Pass `-DCMAKE_PREFIX_PATH=/path/to/boost_1.87.0` or set Boost in environment
- For sanitizers: `cmake --preset=linux-gcc-asan && cmake --build build/linux-gcc-asan && cd build/linux-gcc-asan && ctest -VV --output-on-failure`

**CI uses**:
- ccache (set via `-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache`)
- CPM caching (GitHub Actions cache with key `cache-cmake-dependency-sources`)
- Tests run with `ctest -VV --output-on-failure` (verbose on failure)

---

## Code Style and Formatting

**Follow `.clang-format`** (included in repo root):
- **Indentation**: 4 spaces, no tabs
- **Brace style**: Allman (opening brace on new line) for classes, functions, namespaces, control structures
- **Line length**: 120 characters
- **Pointer alignment**: Left (`int* ptr`)
- **Include order**: Do **not** sort includes (SortIncludes: false in config)

**Naming conventions** (inferred from codebase):
- **Private members**: Trailing underscore (`value_`, `logger_`)
- **Functions/methods**: snake_case (`get_value`, `set_value`)
- **Types**: PascalCase or snake_case (check surrounding code)

**Formatting command**:
```bash
clang-format -i <file>
```

**Before committing**: Format all modified `.cpp` and `.hpp` files. CI does **not** enforce formatting, but consistency is expected.

**File endings**: All files must end with a newline; no trailing whitespace.

---

## Testing

**Framework**: Catch2 v3.8.0  
**Test discovery**: Automatic via `catch_discover_tests(lux-test)` in `test/CMakeLists.txt`  
**Run all tests**: `cd build/<preset> && ctest --output-on-failure`  
**Run specific test**: `cd build/<preset> && ./test/lux-test "[tag]"` (e.g., `"[logger]"`, `"[support]"`)  
**Verbose output**: `ctest -VV --output-on-failure` (shows test stdout/stderr on failure)

**Test structure**: One test file per module (e.g., `logger_test.cpp`, `result_test.cpp`). Tests use Catch2 `TEST_CASE` and `SECTION` macros.

**Test dependencies**: lux-test links `Catch2::Catch2WithMain`, `lux::lux`, and `lux::io` (if `LUX_ENABLE_IO=ON`).

**Known test requirements**:
- io/net tests may bind to localhost ports (ensure no port conflicts)
- sanitizer tests (asan/tsan/ubsan) may take longer; use `ctest -VV` to monitor progress

---

## Linting and Static Analysis

**clang-format**: See "Code Style and Formatting" section above.  
**clang-tidy**: Not configured in repo (no `.clang-tidy` file).  
**cmake-format**: Not used (no `.cmake-format` or `cmakelint` config).

If adding static analysis, integrate via `CMAKE_CXX_CLANG_TIDY` or `CMAKE_CXX_CPPCHECK` variables in presets.

---

## CFlex-Specific Workflow

**What is CFlex?** A custom CMake module (`cmake/CFlex.cmake`) providing target creation functions with automatic flag application.

**Generated files**: None. CFlex is purely CMake-time configuration; it does not generate code. Contrast with Flex/Bison (lexer/parser generators), which this project does **not** use despite the name similarity.

**Workflow**:
1. Toolchain files (`cmake/toolchain/*.cmake`) set compilers.
2. Build option files (`cmake/build_options/*.cmake`) call `cflex_set_cxx_flags`, `cflex_set_linker_flags` to configure flags.
3. `src/CMakeLists.txt` and `test/CMakeLists.txt` use `cflex_add_library`/`cflex_add_executable` instead of raw `add_library`/`add_executable`.
4. CFlex applies flags from step 2 to targets created in step 3 via `cflex_configure_target`.

**Regenerate CMake configuration**: Run `cmake --preset=<name>` again (no special "regenerate" step needed).

**Clean regeneration**: Delete `build/<preset>` and reconfigure.

**CMake cache**: Presets store cache in `build/<preset>/CMakeCache.txt`. To change options, either edit and reconfigure, or delete and reconfigure.

---

## Important Notes

1. **Always use presets**: Do not run `cmake -B build -G Ninja` directly; use `cmake --preset=<name>`.
2. **Boost is mandatory for lux-io**: If `LUX_ENABLE_IO=ON` (default), you **must** provide Boost 1.87.0 via `-DCMAKE_PREFIX_PATH`.
3. **C++23 is non-negotiable**: The codebase uses C++23 features (e.g., `std::format`, concepts). Older compilers will fail.
4. **CFlex logs are informational**: `CFlex: <message>` output during configure is expected; it shows target configuration.
5. **Catch2 discovery happens at configure time**: Adding new tests requires reconfiguring CMake (or use `PRE_TEST` discovery mode on cross-compile).
6. **Sanitizer builds may report false positives**: Especially tsan/msan. Review CI logs for known issues.
7. **BoringSSL vs OpenSSL**: Default is BoringSSL (fetched). To use system OpenSSL, set `-DLUX_FETCH_BORINGSSL=OFF` and ensure OpenSSL is findable.

---

## Common Pitfalls

- **Forgetting `-DCMAKE_PREFIX_PATH` for Boost**: Results in "Boost 1.87.0 not found" error during configure.
- **Using wrong preset for platform**: Windows presets fail on Linux and vice versa (conditional logic in CMakePresets.json).
- **Mixing build types in same build dir**: Do not run `linux-gcc-debug` then `linux-gcc-release` in same `build/` folder; each preset gets its own `build/<preset>`.
- **Not cleaning after changing toolchain files**: If modifying `cmake/toolchain/*.cmake` or `cmake/build_options/*.cmake`, delete `build/<preset>` and reconfigure.
- **Expecting Flex/Bison**: Despite "CFlex" name, this is **not** related to Flex lexer generator. It's a custom CMake flag management system.
- **Running tests from wrong directory**: `ctest` must be run from `build/<preset>` (or use `--test-dir build/<preset>`).

---

## Source of Truth Hierarchy

1. **This file** (`.github/copilot-instructions.md`) for build/workflow/conventions
2. **CMakePresets.json** for preset names and options
3. **CMakeLists.txt** (root, src, test) for targets and dependencies
4. **cmake/CFlex.cmake** for flag application logic
5. **.github/workflows/ci.yml** for CI matrix and exact commands
6. **.clang-format** for code style

When instructions are unclear or outdated, inspect these files in order.

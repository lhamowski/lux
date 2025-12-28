# Lux C++ Library - Copilot Coding Agent Instructions

## Project Overview

**lux** is a C++23 utility library with:
- **lux core**: Logging (spdlog), support utilities (result types, strong typedefs), buffer I/O, memory arenas, timing
- **lux-io** (optional): Async networking (TCP/UDP/SSL via BoringSSL), timers, coroutines (Boost.Asio)

**Tech Stack**: C++23 | CMake 3.30+ | Ninja | Catch2 v3.8.0 | CPM.cmake | Boost 1.87.0 (for lux-io)  
**CFlex.cmake**: Custom module providing `cflex_add_library`/`cflex_add_executable` with automatic compiler/linker flag application

## Build System

### CMake Presets (ALWAYS use presets, never plain `cmake -B build`)

**Common presets**: `linux-gcc-{debug,release,coverage,asan,tsan,ubsan}`, `linux-clang-{debug,release,coverage,asan,tsan,ubsan,msan}`, `windows-msvc-{debug,release,asan}`, `vs2022` (VS IDE)  
**Build dirs**: `build/<preset>` | **Install**: `install/<preset>`

### CFlex (`cmake/CFlex.cmake`)

Wraps `add_library`/`add_executable` to apply flags from `cmake/build_options/{gcc,clang,msvc}.cmake`. Toolchains in `cmake/toolchain/`. No code generation—purely build config.

### Dependencies (CPM)

`external/CMakeLists.txt` fetches: fmt 11.2.0, spdlog 1.15.3, magic_enum 0.9.7, BoringSSL 0.20251124.0 (or system OpenSSL), Catch2 v3.8.0.  
**Boost 1.87.0 required** for lux-io (install manually, pass `-DCMAKE_PREFIX_PATH=<boost-root>`).  
**Options**: `LUX_TEST` (ON), `LUX_ENABLE_IO` (ON), `LUX_FETCH_DEPS` (ON), `LUX_FETCH_BORINGSSL` (ON).  
**Tip**: `export CPM_SOURCE_CACHE=~/cpm-cache` to avoid re-downloads.

## Build Instructions

**Prerequisites**: CMake 3.30+, Ninja, Boost 1.87.0 | **Linux**: GCC 14+/Clang 19+, OpenSSL dev libs | **Windows**: VS 2022, MSVC dev prompt

### Linux (GCC/Clang)

```bash
export CPM_SOURCE_CACHE=~/cpm-cache  # optional
cmake --preset=linux-gcc-release -DCMAKE_PREFIX_PATH=/path/to/boost_1.87.0
cmake --build build/linux-gcc-release -j
cd build/linux-gcc-release && ctest --output-on-failure
```
**Sanitizers**: Use `linux-gcc-{asan,tsan,ubsan}` presets. **Coverage** (Clang):
```bash
cmake --preset=linux-clang-coverage -DCMAKE_PREFIX_PATH=...
cmake --build build/linux-clang-coverage -j
cd build/linux-clang-coverage
LLVM_PROFILE_FILE=coverage-%p.profraw ctest --output-on-failure
llvm-profdata merge --sparse test/*.profraw -o coverage.profdata
find ./src -name '*.o' | xargs -I{} echo "-object={}" > object_list.txt
llvm-cov export ./test/lux-test -instr-profile=coverage.profdata -format=lcov \
  -ignore-filename-regex=".*test/.*" $(cat object_list.txt) > coverage.info
```

### Windows (MSVC)

```powershell
# In "x64 Native Tools Command Prompt for VS 2022"
cmake --preset=windows-msvc-release -DCMAKE_PREFIX_PATH=C:\path\to\boost_1_87_0
cmake --build build\windows-msvc-release
cd build\windows-msvc-release && ctest --output-on-failure
```
**VS IDE**: `cmake --preset=vs2022 -DCMAKE_PREFIX_PATH=... && cmake --open build\vs2022`

### Common Errors

1. **Boost not found**: Missing `-DCMAKE_PREFIX_PATH` with Boost root
2. **C++23 not supported**: Update compiler (GCC 14+, Clang 19+, MSVC 2022)
3. **Ninja not found**: Install and add to PATH
4. **Cache issues**: Delete `build/<preset>` and reconfigure
5. **CPM re-downloads**: Set `CPM_SOURCE_CACHE` env var

## Project Layout

```
lux/
├── CMakeLists.txt           # Top-level: options (LUX_TEST, LUX_ENABLE_IO), find Boost, add subdirs
├── CMakePresets.json        # Preset definitions (40+ presets for all toolchain/build-type combos)
├── .clang-format            # Code formatting rules (4-space, Allman braces, 120-char lines)
├── .github/
│   └── workflows/ci.yml     # CI: 4 jobs (gcc-release, gcc-sanitizers, msvc-release, clang-coverage)
├── cmake/                   # Build system configuration
│   ├── CFlex.cmake          # Custom target wrappers (cflex_add_library/executable) with flag application
│   ├── LuxSourceGroup.cmake # IDE folder organization helper (lux_source_group)
│   ├── CPM.cmake            # Dependency manager (fetches fmt, spdlog, magic_enum, BoringSSL, Catch2)
│   ├── toolchain/           # Compiler selection per-toolchain
│   │   ├── gcc.cmake        # Sets g++/gcc
│   │   ├── clang.cmake      # Sets clang++/clang
│   │   ├── msvc.cmake       # Sets cl.exe
│   │   └── gcc-arm64-cross.cmake  # ARM64 cross-compilation
│   └── build_options/       # Compiler/linker flags per-toolchain (applied via CFlex)
│       ├── gcc.cmake        # GCC warnings, sanitizers, optimization levels
│       ├── clang.cmake      # Clang warnings, sanitizers, optimization levels
│       └── msvc.cmake       # MSVC warnings (/W4, /WX), optimization levels
├── external/CMakeLists.txt  # CPM dependency fetching (fmt 11.2.0, spdlog 1.15.3, etc.)
├── include/lux/             # Public API headers (install target)
│   ├── fwd.hpp              # Forward declarations
│   ├── logger/              # Logging subsystem (logger, logger_manager, log_level)
│   ├── support/             # Core utilities (result, strong_typedef, assert, exception, path, hash)
│   ├── utils/               # Buffer I/O (buffer_reader/writer), memory_arena, stopwatch, lifetime_guard
│   └── io/                  # Async I/O module (requires Boost, LUX_ENABLE_IO=ON)
│       ├── promise.hpp      # Promise/future utilities
│       ├── coro/            # Coroutine helpers (awaitable_event, algorithms, common)
│       ├── net/             # Networking (tcp_socket, udp_socket, tcp_acceptor, socket_factory, SSL types)
│       └── time/            # Timers and retry (interval_timer, retry_executor, timer_factory)
├── src/                     # Implementation files
│   ├── CMakeLists.txt       # Defines lux::lux and lux::io targets via cflex_add_library
│   └── lux/
│       ├── logger/          # logger_manager.cpp, log_level.cpp
│       ├── support/         # path.cpp
│       └── io/
│           ├── net/         # TCP/UDP/SSL implementation, asio_impl.cpp (Boost.Asio compilation unit)
│           └── time/        # retry_executor.cpp
└── test/                    # Catch2 v3 tests (one test file per module)
    ├── CMakeLists.txt       # Defines lux-test via cflex_add_executable, uses catch_discover_tests
    ├── logger/              # logger_test.cpp
    ├── support/             # result_test.cpp, strong_typedef_test.cpp, exception_test.cpp, etc.
    ├── utils/               # buffer_reader_test.cpp, buffer_writer_test.cpp, memory_arena_test.cpp, etc.
    └── io/                  # I/O module tests (requires LUX_ENABLE_IO=ON)
        ├── promise_test.cpp
        ├── coro/            # algorithms_test.cpp, awaitable_event_test.cpp
        ├── net/             # tcp_socket_test.cpp, udp_socket_test.cpp, tcp_acceptor_test.cpp, test_utils.hpp
        └── time/            # interval_timer_test.cpp, retry_executor_test.cpp, timer_factory_test.cpp, mocks/
```
**Key Targets**: `lux::lux` (core static lib, always built), `lux::io` (I/O static lib, if `LUX_ENABLE_IO=ON`), `lux-test` (test executable, if `LUX_TEST=ON`)  
**Find headers in**: `include/lux/<module>/` | **Find implementation in**: `src/lux/<module>/` | **Find tests in**: `test/<module>/`

### CI (.github/workflows/ci.yml)

**Jobs**:
1. `build-and-test-linux-gcc-release`: Ubuntu 24.04, GCC 14, Boost 1.87.0 (Docker: `lhamowski/devops:ubuntu-24.04-cpp-gcc-14-boost-1.87-latest`)
2. `build-and-test-linux-gcc-sanitizers`: Matrix of asan/tsan/ubsan (GCC 14, master only)
3. `build-and-test-windows-msvc-release`: Windows Server 2022, MSVC 2022, Boost 1.87.0 (via GHA)
4. `build-and-test-linux-clang-coverage`: Ubuntu 24.04, Clang 19, uploads to Codecov

**Reproduce CI**: Use same preset (e.g., `linux-gcc-release`), pass `-DCMAKE_PREFIX_PATH=<boost>`, run `ctest -VV --output-on-failure`.

## Code Style & Testing

**Formatting** (`.clang-format`): 4-space indent, Allman braces, 120-char lines, left pointer alignment. Run: `clang-format -i <file>`  
**Naming**: Private members end with `_` (e.g., `value_`), functions are `snake_case`, no include sorting.  
**Testing**: Catch2 v3. Run all: `cd build/<preset> && ctest --output-on-failure`. Run specific: `./test/lux-test "[logger]"`. Tests link `Catch2::Catch2WithMain`, `lux::lux`, `lux::io`.

## CFlex Workflow

CFlex is **not** Flex/Bison—it's CMake-time flag management. No generated files.  
**Workflow**: Toolchains set compilers → build_options set flags via CFlex macros → src/test use `cflex_add_*` → CFlex applies flags.  
**Regenerate**: `cmake --preset=<name>` (or delete `build/<preset>` for clean).

## Critical Notes

1. **Always use presets** (never `cmake -B build`)
2. **Boost 1.87.0 mandatory** for lux-io (pass `-DCMAKE_PREFIX_PATH`)
3. **C++23 required** (GCC 14+, Clang 19+, MSVC 2022)
4. **Each preset → separate build dir** (don't mix debug/release in same dir)
5. **CFlex logs are normal** (shows target config during configure)
6. **Clean after toolchain changes**: Delete `build/<preset>`
7. **BoringSSL default** (use `-DLUX_FETCH_BORINGSSL=OFF` for system OpenSSL)

## Common Pitfalls

- Missing `-DCMAKE_PREFIX_PATH` for Boost → configure fails
- Wrong preset for platform (Windows presets fail on Linux)
- Running `ctest` from wrong dir → use `cd build/<preset>` or `--test-dir`
- Not cleaning after modifying `cmake/{toolchain,build_options}/*.cmake`
- Expecting Flex lexer generator (CFlex is unrelated)

## Source of Truth

1. This file for build/workflow/conventions
2. `CMakePresets.json` for preset names/options
3. `CMakeLists.txt` (root/src/test) for targets/deps
4. `cmake/CFlex.cmake` for flag logic
5. `.github/workflows/ci.yml` for CI commands
6. `.clang-format` for code style

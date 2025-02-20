include_guard(GLOBAL)

set(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++" CACHE STRING "CXX Compiler")
set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc" CACHE STRING "C Compiler")
set(CMAKE_CROSSCOMPILING_EMULATOR "qemu-aarch64" CACHE STRING "Crosscompiling Emulator")
set(CMAKE_SYSTEM_PROCESSOR "aarch64" CACHE STRING "System Processor")
mark_as_advanced(CMAKE_SYSTEM_PROCESSOR)
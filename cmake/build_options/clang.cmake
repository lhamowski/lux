include_guard(GLOBAL)

include(${PROJECT_SOURCE_DIR}/cmake/CFlex.cmake)

cflex_set_cxx_flags(
    PRIVATE 
        -Wall # Enable all warnings
        -Wextra # Enable extra warnings
        -Wpedantic # Enable pedantic warnings
        -Werror # Treat warnings as errors
)

cflex_set_cxx_flags(
    BUILD_TYPES Debug
    GLOBAL 
        -O0 # Disable optimizations
        -g # Generate debug information
)

cflex_set_cxx_flags(
    BUILD_TYPES Release
    GLOBAL 
        -O3 # Enable optimizations
        -DNDEBUG # Disable assertions
)

cflex_set_cxx_flags(
    BUILD_TYPES Coverage
    GLOBAL
        -O0 # Disable optimizations
        -g # Generate debug information
    PRIVATE 
        -fprofile-instr-generate # Enable profiling instrumentation
        -fcoverage-mapping # Enable coverage mapping
)

cflex_set_cxx_flags(
    BUILD_TYPES TSan
    GLOBAL
        -O1 # Enable optimizations level 1
        -g # Generate debug information
        -DNDEBUG # Disable assertions
        -fsanitize=thread # Enable thread sanitizer
)

cflex_set_cxx_flags(
    BUILD_TYPES ASan
    GLOBAL
        -O1 # Enable optimizations level 1
        -g # Generate debug information
        -DNDEBUG # Disable assertions
        -fsanitize=address # Enable address sanitizer
        -fno-omit-frame-pointer # Keep frame pointers
)

cflex_set_cxx_flags(
    BUILD_TYPES UBSan
    GLOBAL
        -O1 # Enable optimizations level 1
        -g # Generate debug information
        -DNDEBUG # Disable assertions
        -fsanitize=address # Enable address sanitizer
        -fno-omit-frame-pointer # Keep frame pointers
)

cflex_set_cxx_flags(
    BUILD_TYPES MSan
    GLOBAL
        -O1 # Enable optimizations level 1
        -g # Generate debug information
        -DNDEBUG # Disable assertions
        -fsanitize=memory # Enable memory sanitizer
        -fno-omit-frame-pointer # Keep frame pointers
)

cflex_set_linker_flags(
    GLOBAL
        -Wl,--exclude-libs=ALL # It specifies a list of archive libraries from which symbols should not be automatically exported
        -Wl,--as-needed # Only link libraries that are needed
        -Wl,-z,defs # Ensure that all symbols are defined
)

cflex_set_linker_flags(
    BUILD_TYPES Coverage
    PRIVATE
        -fprofile-instr-generate # Enable profiling instrumentation
        -fcoverage-mapping # Enable coverage mapping
)
include_guard(GLOBAL)

include(${PROJECT_SOURCE_DIR}/cmake/CFlex.cmake)

cflex_set_cxx_flags(
    GLOBAL
        /EHsc # Enable unwind semantics
    PRIVATE
        /W4 # Enable all warnings
        /WX # Treat warnings as errors
        /permissive- # Enable conformance mode
        /volatile:iso # Enable ISO volatile semantics
)

cflex_set_cxx_flags(
    BUILD_TYPES Debug
    GLOBAL
        /Od # Disable optimizations
        /Zi # Generate debug information in PDB format
        /JMC # Enable just my code
)

cflex_set_cxx_flags(
    BUILD_TYPES Release
    GLOBAL
        /O2 # Enable optimizations
        /Zi # Generate debug information in PDB format
        /DNDEBUG # Disable assertions
)

cflex_set_cxx_flags(
    BUILD_TYPES ASan
    GLOBAL
        /fsanitize=address # Enable address sanitizer
        /Od # Disable optimizations
        /Zi # Generate debug information in PDB format
)

cflex_set_linker_flags(
    BUILD_TYPES Debug
    GLOBAL
        /DEBUG # Generate debug information in PDB format
)

cflex_set_linker_flags(
    BUILD_TYPES Release
    GLOBAL
        /DEBUG # Generate debug information in PDB format
        /INCREMENTAL:NO # Disable incremental linking 
)
include(../cmake/CPM.cmake)

option(LUX_FETCH_FMT "Fetch fmt library" ON)
option(LUX_FETCH_SPDLOG "Fetch spdlog library" ON)
option(LUX_FETCH_BORINGSSL "Fetch BoringSSL library (only if LUX_ENABLE_IO is ON)" ON)

if (LUX_FETCH_FMT)
    CPMAddPackage(
        NAME fmt
        URL https://github.com/fmtlib/fmt/archive/11.2.0.zip
        URL_HASH SHA256=ebb31f63e14048ffba9200bbe27b339ec50226cbe8e96ca8b62ee16a0a4fb1af
    )
else()
    message(STATUS "Using external fmt library")
endif()

if (LUX_FETCH_SPDLOG)
    CPMAddPackage(
        NAME spdlog
        URL https://github.com/gabime/spdlog/archive/v1.15.3.zip
        URL_HASH SHA256=b74274c32c8be5dba70b7006c1d41b7d3e5ff0dff8390c8b6390c1189424e094
        OPTIONS "SPDLOG_FMT_EXTERNAL ON"
    )
else()
    message(STATUS "Using external spdlog library")
endif()

if ((LUX_ENABLE_IO OR LUX_ENABLE_CRYPTO) AND LUX_FETCH_BORINGSSL)
    CPMAddPackage(
        NAME boringssl
        GITHUB_REPOSITORY google/boringssl
        GIT_TAG 0.20251124.0
        OPTIONS
            "OPENSSL_NO_ASM ON" # TODO: Download NASM and enable assembly optimizations
            "BUILD_TESTING OFF"
    )
else()
    message(STATUS "Using external OpenSSL library")
endif()

CPMAddPackage(
    NAME magic_enum
    GITHUB_REPOSITORY Neargye/magic_enum
    GIT_TAG v0.9.7
)

if(LUX_TEST)
    CPMAddPackage(
        NAME Catch2
        URL https://github.com/catchorg/Catch2/archive/v3.8.0.zip
        URL_HASH SHA256=bffd2c45a84e5a4b0c17e695798e8d2f65931cbaf5c7556d40388d1d8d04eb83
    )

    set_target_properties(Catch2 Catch2WithMain PROPERTIES FOLDER external)
    if(COMMAND target_precompile_headers)
        target_precompile_headers(Catch2 PRIVATE <catch2/catch_all.hpp>)
    endif()
endif()


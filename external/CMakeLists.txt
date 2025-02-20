include(../cmake/CPM.cmake)

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


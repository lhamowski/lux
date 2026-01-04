#include "test_case.hpp"

#include <lux/utils/random_bytes.hpp>

#include <catch2/catch_all.hpp>

LUX_TEST_CASE("random_bytes", "generates random bytes", "[utils][random_bytes]")
{
    SECTION("Generates correct number of bytes")
    {
        const std::size_t count = 16;
        auto bytes = lux::random_bytes(count);
        REQUIRE(bytes.size() == count);
    }

    SECTION("Generates different bytes on subsequent calls")
    {
        const std::size_t count = 16;
        auto bytes1 = lux::random_bytes(count);
        auto bytes2 = lux::random_bytes(count);
        REQUIRE(bytes1 != bytes2);
    }

    SECTION("Generates zero bytes when count is zero")
    {
        const std::size_t count = 0;
        auto bytes = lux::random_bytes(count);
        REQUIRE(bytes.empty());
    }
}
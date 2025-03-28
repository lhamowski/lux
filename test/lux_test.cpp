#include <lux/lux.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("Lux test", "[lux]")
{
    CHECK(lux::foo() == 1);
    CHECK(lux::bar() == 42);
}
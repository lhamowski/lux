#include "test_case.hpp"

#include <lux/support/scoped_value.hpp>

#include <catch2/catch_all.hpp>

LUX_TEST_CASE("scoped_value", "temporarily changes variable and restores on scope exit", "[scoped_value][support]")
{
    SECTION("Scoped value changes variable within scope and restores it after")
    {
        int value = 10;
        {
            LUX_SCOPED_VALUE(value, 20);
            CHECK(value == 20);
        }
        CHECK(value == 10);
    }

    SECTION("Multiple scoped values nest correctly")
    {
        int value = 5;
        {
            LUX_SCOPED_VALUE(value, 15);
            CHECK(value == 15);

            {
                LUX_SCOPED_VALUE(value, 25);
                CHECK(value == 25);
            }

            CHECK(value == 15);
        }

        CHECK(value == 5);
    }
}

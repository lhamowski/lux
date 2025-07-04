#include <lux/support/finally.hpp>

#include <catch2/catch_all.hpp>

#include <filesystem>
#include <fstream>

TEST_CASE("Finally macro basic functionality", "[finally]")
{
    SECTION("Finally executes code at scope exit")
    {
        bool executed = false;

        {
            LUX_FINALLY(executed = true);
            CHECK_FALSE(executed); // Should not be executed yet
        }

        CHECK(executed); // Should be executed after scope exit
    }

    SECTION("Finally executes even when exception is thrown")
    {
        bool cleanup_executed = false;

        try
        {
            LUX_FINALLY({cleanup_executed = true;});
            throw std::runtime_error("test exception");
        }
        catch (const std::exception&)
        {
            // Exception caught
        }

        CHECK(cleanup_executed);
    }
}

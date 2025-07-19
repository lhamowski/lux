#include <lux/support/exception.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("Formatted exception", "[exception]")
{
    SECTION("Formatted exception with arguments")
    {
        try
        {
            throw lux::formatted_exception("Error code: {}, message: {}", 404, "Not Found");
        }
        catch (const lux::formatted_exception& e)
        {
            CHECK(std::string(e.what()) == "Error code: 404, message: Not Found");
        }
    }

    SECTION("Formatted exception with single argument")
    {
        try
        {
            throw lux::formatted_exception("Single error message");
        }
        catch (const lux::formatted_exception& e)
        {
            CHECK(std::string(e.what()) == "Single error message");
        }
    }

    SECTION("Formatted exception with no arguments")
    {
        try
        {
            throw lux::formatted_exception("No arguments");
        }
        catch (const lux::formatted_exception& e)
        {
            CHECK(std::string(e.what()) == "No arguments");
        }
    }
}
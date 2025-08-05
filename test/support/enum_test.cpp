#include <lux/support/enum.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("Enum to_underlying conversion", "[enum][support]")
{
    enum class color : int
    {
        red = 1,
        green = 2,
        blue = 3
    };

    SECTION("Convert enum to underlying type")
    {
        CHECK(lux::to_underlying(color::red) == 1);
        CHECK(lux::to_underlying(color::green) == 2);
        CHECK(lux::to_underlying(color::blue) == 3);
    }

    SECTION("Underlying type is correct")
    {
        static_assert(std::is_same_v<decltype(lux::to_underlying(color::red)), int>);
    }
}

TEST_CASE("Enum to_string_view conversion", "[enum][support]")
{
    enum class status
    {
        ok = 0,
        error = 1,
        pending = 2
    };

    SECTION("Convert enum to string_view")
    {
        CHECK(lux::to_string_view(status::ok) == "ok");
        CHECK(lux::to_string_view(status::error) == "error");
        CHECK(lux::to_string_view(status::pending) == "pending");
        CHECK(lux::to_string_view(status(-1)) == "");
    }

    SECTION("String representation is correct")
    {
        static_assert(std::is_same_v<decltype(lux::to_string_view(status::ok)), std::string_view>);
    }
}
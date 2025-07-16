#include <lux/support/enum.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("Enum to_underlying conversion", "[enum]")
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
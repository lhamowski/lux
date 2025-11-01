#include <lux/support/result.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("Result basic usage", "[result][support]")
{
	SECTION("Successful result with value")
	{
		lux::result<int> res = 42;
		REQUIRE(res.has_value());
		REQUIRE(res.value() == 42);
	}

	SECTION("Successful result with void type")
	{
		lux::result<> res = {};
		REQUIRE(res.has_value());
	}

	SECTION("Error result with message")
	{
		lux::result<int> res = std::unexpected<std::string>("An error occurred");
		REQUIRE(!res.has_value());
		REQUIRE(res.error() == "An error occurred");
	}

	SECTION("Error result with void type")
	{
		lux::result<> res = std::unexpected<std::string>("Void error");
		REQUIRE(!res.has_value());
		REQUIRE(res.error() == "Void error");
	}
}
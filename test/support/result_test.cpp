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
		lux::status res = {};
		REQUIRE(res.has_value());
	}

	SECTION("Error result with message")
	{
		lux::result<int> res = std::unexpected<std::string>("An error occurred");
		REQUIRE(!res.has_value());
		REQUIRE(res.error().join() == "An error occurred");
	}

	SECTION("Error result with void type")
	{
		lux::result<> res = std::unexpected<std::string>("Void error");
		REQUIRE(!res.has_value());
		REQUIRE(res.error().join() == "Void error");
	}

	SECTION("Error with formatted message")
	{
		int error_code = 404;
		std::string resource = "config.json";
		lux::result<int> res = lux::err("Failed to load '{}' with error code {}", resource, error_code);
		REQUIRE(!res.has_value());
		REQUIRE(res.error().join() == "Failed to load 'config.json' with error code 404");
	}

	SECTION("Error with simple string")
	{
		lux::result<int> res = lux::err("Simple error message");
		REQUIRE(!res.has_value());
		REQUIRE(res.error().join() == "Simple error message");
	}
}
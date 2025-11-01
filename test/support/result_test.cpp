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

TEST_CASE("Result ok() helper", "[result][support]")
{
	SECTION("ok() with value creates successful result")
	{
		auto res = lux::ok(42);
		REQUIRE(res.has_value());
		REQUIRE(res.value() == 42);
	}

	SECTION("ok() with string creates successful result")
	{
		auto res = lux::ok(std::string("success"));
		REQUIRE(res.has_value());
		REQUIRE(res.value() == "success");
	}

	SECTION("ok() with movable type")
	{
		auto res = lux::ok(std::make_unique<int>(100));
		REQUIRE(res.has_value());
		REQUIRE(*res.value() == 100);
	}

	SECTION("ok() without argument creates void result")
	{
		auto res = lux::ok();
		REQUIRE(res.has_value());
		static_assert(std::is_same_v<decltype(res), lux::status>);
	}
}

TEST_CASE("Result err() helper", "[result][support]")
{
	SECTION("err() creates error for result<int>")
	{
		lux::result<int> res = lux::err("Something went wrong");
		REQUIRE(!res.has_value());
		REQUIRE(res.error() == "Something went wrong");
	}

	SECTION("err() creates error for result<std::string>")
	{
		lux::result<std::string> res = lux::err("Failed to process");
		REQUIRE(!res.has_value());
		REQUIRE(res.error() == "Failed to process");
	}

	SECTION("err() with string move")
	{
		std::string msg = "Error message";
		lux::result<int> res = lux::err(std::move(msg));
		REQUIRE(!res.has_value());
		REQUIRE(res.error() == "Error message");
	}

	SECTION("err() creates error for void result")
	{
		lux::status res = lux::err("Operation failed");
		REQUIRE(!res.has_value());
		REQUIRE(res.error() == "Operation failed");
	}
}

TEST_CASE("Result status alias", "[result][support]")
{
	SECTION("status is same as result<void>")
	{
		static_assert(std::is_same_v<lux::status, lux::result<void>>);
		static_assert(std::is_same_v<lux::status, lux::result<>>);
	}

	SECTION("status with success")
	{
		lux::status res = lux::ok();
		REQUIRE(res.has_value());
	}

	SECTION("status with error")
	{
		lux::status res = lux::err("Failed");
		REQUIRE(!res.has_value());
		REQUIRE(res.error() == "Failed");
	}

	SECTION("status as return type")
	{
		auto func = []() -> lux::status {
			return lux::ok();
		};

		auto result = func();
		REQUIRE(result.has_value());
	}
}
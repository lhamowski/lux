#include <lux/utils/string_chain.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("string_chain functionality", "[string_chain][utils]")
{
	SECTION("Append and join strings")
	{
		lux::string_chain chain;
		chain.append("Hello");
		chain.append("World");
		std::string result = chain.join(", ");
		REQUIRE(result == "Hello, World");
	}

	SECTION("Prepend and join strings")
	{
		lux::string_chain chain;
		chain.append("World");
		chain.prepend("Hello");
		std::string result = chain.join(" ");
		REQUIRE(result == "Hello World");
	}

	SECTION("Join with default delimiter")
	{
        lux::string_chain chain{"Line 1"};
		chain.append("Line 2");
		chain.append("Line 3");
		std::string result = chain.join();
		REQUIRE(result == "Line 1\nLine 2\nLine 3");
	}

	SECTION("Chaining append and prepend")
	{
		lux::string_chain chain;
		chain.append("Middle").prepend("Start").append("End");
		std::string result = chain.join(" - ");
		REQUIRE(result == "Start - Middle - End");
	}
}

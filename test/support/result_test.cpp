#include <lux/support/result.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("result: represents success or error with optional value", "[result][support]")
{
    SECTION("Successful result with value")
    {
        lux::result<int> res = 42;
        REQUIRE(res.has_value());
        CHECK(res.value() == 42);
    }

    SECTION("Successful result with void type")
    {
        lux::status res = {};
        CHECK(res.has_value());
    }

    SECTION("Status with error message")
    {
        lux::status res = lux::err("Operation failed");
        REQUIRE_FALSE(res.has_value());
        CHECK(res.error().str() == "Operation failed\n");
    }

    SECTION("Status with formatted error message")
    {
        lux::status res = lux::err("Failed with code: {}", 404);
        REQUIRE_FALSE(res.has_value());
        CHECK(res.error().str() == "Failed with code: 404\n");
    }

    SECTION("Result with ok() helper")
    {
        lux::status res = lux::ok();
        REQUIRE(res.has_value());
    }

    SECTION("Result with ok(value) helper")
    {
        auto res = lux::ok(123);
        REQUIRE(res.has_value());
        CHECK(res.value() == 123);
    }
}

TEST_CASE("result: error message construction", "[result][support]")
{
    SECTION("Error message from string")
    {
        lux::error_message msg{"First error"};
        CHECK(msg.str() == "First error\n");
    }

    SECTION("Error message with append")
    {
        lux::error_message msg{"First error"};
        msg.append("Second error");
        CHECK(msg.str() == "First error\nSecond error\n");
    }

    SECTION("Error message with prepend")
    {
        lux::error_message msg{"Original error"};
        msg.prepend("Context");
        CHECK(msg.str() == "Context\nOriginal error\n");
    }

    SECTION("Error message chaining")
    {
        lux::error_message msg;
        msg.append("Error 1").append("Error 2").append("Error 3");
        CHECK(msg.str() == "Error 1\nError 2\nError 3\n");

        lux::error_message msg2{msg};
        msg2.append(lux::move(msg));
        CHECK(msg2.str() == "Error 1\nError 2\nError 3\nError 1\nError 2\nError 3\n\n");
    }

    SECTION("Error message string conversion")
    {
        lux::error_message msg{"Test error"};
        std::string str = msg;
        CHECK(str == "Test error\n");
    }

    SECTION("Empty error message")
    {
        lux::error_message msg;
        CHECK(msg.empty());

        msg.append("Not empty anymore");
        CHECK_FALSE(msg.empty());
    }
}
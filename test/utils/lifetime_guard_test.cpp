#include <lux/utils/lifetime_guard.hpp>

#include <catch2/catch_all.hpp>

#include <memory>
#include <utility>
#include <type_traits>

TEST_CASE("Lifetime guard basic functionality", "[lux::lifetime_guard][utils]")
{
    SECTION("Token is valid when guard exists")
    {
        lux::lifetime_guard guard;
        auto token = guard.create_token();

        CHECK(token.is_valid());
    }

    SECTION("Token becomes invalid when guard is destroyed")
    {
        auto token = [] {
            lux::lifetime_guard guard;
            return guard.create_token();
        }();

        CHECK_FALSE(token.is_valid());
    }

    SECTION("Token can be copied")
    {
        lux::lifetime_guard guard;
        auto token1 = guard.create_token();
        auto token2 = token1;

        CHECK(token1.is_valid());
        CHECK(token2.is_valid());
    }

    SECTION("Token can be copy assigned")
    {
        lux::lifetime_guard guard;
        auto token1 = guard.create_token();
        auto token2 = guard.create_token();

        token2 = token1;

        CHECK(token1.is_valid());
        CHECK(token2.is_valid());
    }

    SECTION("Token can be moved")
    {
        lux::lifetime_guard guard;
        auto token1 = guard.create_token();
        auto token2 = std::move(token1);

        CHECK(token2.is_valid());
    }

    SECTION("Token can be move assigned")
    {
        lux::lifetime_guard guard;
        auto token1 = guard.create_token();
        auto token2 = guard.create_token();

        token2 = std::move(token1);

        CHECK(token2.is_valid());
    }

    SECTION("Copied tokens share lifetime tracking")
    {
        auto tokens = []() {
            lux::lifetime_guard guard;
            auto token1 = guard.create_token();
            auto token2 = token1;
            return std::make_pair(token1, token2);
        }();

        CHECK_FALSE(tokens.first.is_valid());
        CHECK_FALSE(tokens.second.is_valid());
    }

    SECTION("Guard is not copy constructible")
    {
        static_assert(!std::is_copy_constructible_v<lux::lifetime_guard>);
    }

    SECTION("Guard is not copy assignable")
    {
        static_assert(!std::is_copy_assignable_v<lux::lifetime_guard>);
    }

    SECTION("Guard is not move constructible")
    {
        static_assert(!std::is_move_constructible_v<lux::lifetime_guard>);
    }

    SECTION("Guard is not move assignable")
    {
        static_assert(!std::is_move_assignable_v<lux::lifetime_guard>);
    }
}
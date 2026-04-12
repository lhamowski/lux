#include "test_case.hpp"

#include <lux/support/strong_typedef.hpp>

#include <catch2/catch_all.hpp>

#include <numeric>
#include <vector>

LUX_TEST_CASE("strong_typedef", "wraps underlying type with strong typing", "[strong_typedef][support]")
{
    LUX_STRONG_TYPEDEF(my_int, int);

    SECTION("Construct and access value")
    {
        const my_int my_value{42};
        CHECK(my_value.get() == 42);
        CHECK(static_cast<int>(my_value) == 42);
    }

    SECTION("Comparison operators")
    {
        const my_int a{10};
        const my_int b{20};
        CHECK(a < b);
        CHECK(b > a);
        CHECK(a <= a);
        CHECK(b >= b);
        CHECK(a != b);
        CHECK(a == a);
    }

    SECTION("Implicit conversion to underlying type")
    {
        const my_int my_value{100};
        const int value = my_value;
        CHECK(value == 100);

        const my_int const_value{200};
        const int const_value_int = const_value;
        CHECK(const_value_int == 200);
    }

    SECTION("Arithmetic operators - addition")
    {
        const my_int a{10};
        const my_int b{20};
        const auto result = a + b;
        CHECK(result.get() == 30);
        CHECK(static_cast<int>(result) == 30);
    }

    SECTION("Arithmetic operators - subtraction")
    {
        const my_int a{30};
        const my_int b{12};
        const auto result = a - b;
        CHECK(result.get() == 18);
    }

    SECTION("Arithmetic operators - multiplication")
    {
        const my_int a{5};
        const my_int b{7};
        const auto result = a * b;
        CHECK(result.get() == 35);
    }

    SECTION("Arithmetic operators - division")
    {
        const my_int a{42};
        const my_int b{6};
        const auto result = a / b;
        CHECK(result.get() == 7);
    }

    SECTION("Arithmetic operators - modulo")
    {
        const my_int a{17};
        const my_int b{5};
        const auto result = a % b;
        CHECK(result.get() == 2);
    }

    SECTION("Compound assignment operators - addition")
    {
        my_int a{10};
        const my_int b{20};
        a += b;
        CHECK(a.get() == 30);
    }

    SECTION("Compound assignment operators - subtraction")
    {
        my_int a{30};
        const my_int b{12};
        a -= b;
        CHECK(a.get() == 18);
    }

    SECTION("Compound assignment operators - multiplication")
    {
        my_int a{5};
        const my_int b{7};
        a *= b;
        CHECK(a.get() == 35);
    }

    SECTION("Compound assignment operators - division")
    {
        my_int a{42};
        const my_int b{6};
        a /= b;
        CHECK(a.get() == 7);
    }

    SECTION("Compound assignment operators - modulo")
    {
        my_int a{17};
        const my_int b{5};
        a %= b;
        CHECK(a.get() == 2);
    }

    SECTION("std::accumulate with strong_typedef")
    {
        const std::vector<my_int> numbers{my_int{1}, my_int{2}, my_int{3}, my_int{4}, my_int{5}};
        const auto sum = std::accumulate(numbers.begin(), numbers.end(), my_int{0});
        CHECK(sum.get() == 15);
    }

    SECTION("Floating point arithmetic")
    {
        LUX_STRONG_TYPEDEF(my_double, double);

        const my_double a{3.5};
        const my_double b{2.0};
        const auto result = a + b;
        CHECK(result.get() == 5.5);

        const auto result2 = a * b;
        CHECK(result2.get() == 7.0);
    }
}

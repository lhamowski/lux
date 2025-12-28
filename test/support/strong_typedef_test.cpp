#include <lux/support/strong_typedef.hpp>

#include <catch2/catch_all.hpp>

TEST_CASE("strong_typedef: wraps underlying type with strong typing", "[strong_typedef][support]")
{
    LUX_STRONG_TYPEDEF(my_int, int);

    SECTION("Construct and access value")
    {
        my_int my_value{42};
        CHECK(my_value.get() == 42);
        CHECK(static_cast<int>(my_value) == 42);
    }

    SECTION("Comparison operators")
    {
        my_int a{10};
        my_int b{20};
        CHECK(a < b);
        CHECK(b > a);
        CHECK(a <= a);
        CHECK(b >= b);
        CHECK(a != b);
        CHECK(a == a);
    }

    SECTION("Implicit conversion to underlying type")
    {
        my_int my_value{100};
        int value = my_value;
        CHECK(value == 100);

        const my_int const_value{200};
        const int const_value_int = const_value;
        CHECK(const_value_int == 200);
    }
}

#include <lux/support/container.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <string_view>

TEST_CASE("string_unordered_map: basic functionality", "[container][support]")
{
    lux::string_unordered_map<int> my_map;

    SECTION("Insert and retrieve elements")
    {
        my_map["one"] = 1;
        my_map["two"] = 2;
        my_map["three"] = 3;
        CHECK(my_map["one"] == 1);
        CHECK(my_map["two"] == 2);
        CHECK(my_map["three"] == 3);
    }

    SECTION("Heterogeneous lookup support")
    {
        my_map["alpha"] = 10;
        my_map["beta"] = 20;
        CHECK(my_map.find("alpha") != my_map.end());
        CHECK(my_map.find(std::string_view{"beta"}) != my_map.end());
        CHECK(my_map.find("gamma") == my_map.end());
    }

    SECTION("Erase elements")
    {
        my_map["key"] = 42;
        CHECK(my_map.find("key") != my_map.end());
        my_map.erase("key");
        CHECK(my_map.find("key") == my_map.end());
    }
}
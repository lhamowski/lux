#include <lux/support/hash.hpp>

#include <catch2/catch_all.hpp>

#include <functional>
#include <unordered_map>

TEST_CASE("String hash function", "[hash][support]")
{
    lux::string_hash hasher;

    SECTION("Hashing std::string")
    {
        std::string str = "hello";
        size_t hash1 = hasher(str);
        size_t hash2 = std::hash<std::string>{}(str);
        CHECK(hash1 == hash2);
    }

    SECTION("Hashing std::string_view")
    {
        std::string_view str_view = "world";
        size_t hash1 = hasher(str_view);
        size_t hash2 = std::hash<std::string_view>{}(str_view);
        CHECK(hash1 == hash2);
    }

    SECTION("Hashing const char*")
    {
        const char* cstr = "test";
        size_t hash1 = hasher(cstr);
        size_t hash2 = std::hash<std::string_view>{}(cstr);
        CHECK(hash1 == hash2);
    }

    SECTION("Heterogeneous lookup support")
    {
        std::unordered_map<std::string, int, lux::string_hash, std::equal_to<>> my_map;
        my_map["key1"] = 1;
        my_map["key2"] = 2;
        CHECK(my_map.find("key1") != my_map.end());
        CHECK(my_map.find(std::string_view{"key2"}) != my_map.end());
        CHECK(my_map.find("key3") == my_map.end());
    }
}
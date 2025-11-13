#include <lux/io/net/base/address_v4.hpp>

#include <catch2/catch_all.hpp>

using namespace lux::net::base;

TEST_CASE("address_v4 basic functionality", "[io][net]")
{
    SECTION("Construct from bytes")
    {
        const address_v4::bytes_type bytes = {std::byte{192}, std::byte{168}, std::byte{1}, std::byte{1}};
        const address_v4 addr{bytes};
        CHECK(addr.to_bytes() == bytes);
        CHECK(addr.to_uint() == 0xC0A80101);
        CHECK(addr.to_string() == "192.168.1.1");
    }

    SECTION("Construct from uint")
    {
        const address_v4 addr{0xC0A80101};
        CHECK(addr.to_bytes() == address_v4::bytes_type{std::byte{192}, std::byte{168}, std::byte{1}, std::byte{1}});
        CHECK(addr.to_uint() == 0xC0A80101);
        CHECK(addr.to_string() == "192.168.1.1");
    }

    SECTION("Construct from string")
    {
        const auto addr = lux::net::base::make_address_v4("192.168.1.1");
        REQUIRE(addr.has_value());
        CHECK(addr->to_bytes() == address_v4::bytes_type{std::byte{192}, std::byte{168}, std::byte{1}, std::byte{1}});
        CHECK(addr->to_uint() == 0xC0A80101);
        CHECK(addr->to_string() == "192.168.1.1");

        const auto invalid_addr = lux::net::base::make_address_v4("999.999.999.999");
        CHECK(!invalid_addr.has_value());
    }

    SECTION("Comparison operators")
    {
        const address_v4 addr1{*lux::net::base::make_address_v4("192.168.1.1")};
        const address_v4 addr2{*lux::net::base::make_address_v4("192.168.1.2")};
        const address_v4 addr3{*lux::net::base::make_address_v4("192.168.1.2")};

        CHECK(addr1 < addr2);
        CHECK(addr2 > addr1);
        CHECK(addr2 == addr3);
        CHECK(addr1 != addr2);
        CHECK(addr1 <= addr2);
        CHECK(addr2 >= addr1);
        CHECK(addr2 >= addr3);
        CHECK(addr1 <= addr1);
        CHECK(addr2 >= addr2);
    }

    SECTION("Predefined addresses")
    {
        CHECK(lux::net::base::localhost == *lux::net::base::make_address_v4("127.0.0.1"));
        CHECK(lux::net::base::any_address == address_v4{0x00000000});
        CHECK(lux::net::base::broadcast_address == address_v4{0xFFFFFFFF});
        CHECK(lux::net::base::localhost.to_string() == "127.0.0.1");
    }
}
#include "test_case.hpp"

#include <lux/io/net/base/endpoint.hpp>

#include <catch2/catch_all.hpp>

using namespace lux::net::base;

LUX_TEST_CASE("endpoint", "constructs from address and port", "[io][net]")
{
    SECTION("Default constructor")
    {
        const lux::net::base::endpoint ep;
        CHECK(ep.address().to_uint() == 0);
        CHECK(ep.port() == 0);
    }

    SECTION("Construct from address and port")
    {
        const lux::net::base::address_v4 addr{0xC0A80101}; //
        const lux::net::base::endpoint ep{addr, 8080};
        CHECK(ep.address().to_uint() == 0xC0A80101);
        CHECK(ep.port() == 8080);
    }

    SECTION("Equality and inequality operators")
    {
        const lux::net::base::address_v4 addr1{0xC0A80101};
        const lux::net::base::address_v4 addr2{0xC0A80102};
        const lux::net::base::endpoint ep1{addr1, 8080};
        const lux::net::base::endpoint ep2{addr1, 8080};
        const lux::net::base::endpoint ep3{addr2, 8080};
        CHECK(ep1 == ep2);
        CHECK(ep1 != ep3);
        CHECK(ep2 != ep3);
    }
}

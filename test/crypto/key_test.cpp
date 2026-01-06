#include <lux/crypto/key.hpp>

#include <catch2/catch_all.hpp>

#include "test_case.hpp"

LUX_TEST_CASE("key", "generates ed25519 keypair successfully", "[key][crypto]")
{
    auto result = lux::crypto::generate_ed25519_keypair();
    REQUIRE(result.has_value());

    const auto& keypair = result.value();
    CHECK(keypair.private_key.size() == 32);
    CHECK(keypair.public_key.size() == 32);
}
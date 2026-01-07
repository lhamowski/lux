#include <lux/crypto/key.hpp>

#include <catch2/catch_all.hpp>

#include "test_case.hpp"

LUX_TEST_CASE("ed25519_private_key", "generates successfully with correct size", "[crypto][key][ed25519]")
{
    const auto result = lux::crypto::generate_ed25519_private_key();

    REQUIRE(result.has_value());
    CHECK(result->data.size() == lux::crypto::ed25519_private_key::size);
}

LUX_TEST_CASE("ed25519_private_key", "generates unique keys on each call", "[crypto][key][ed25519]")
{
    const auto result1 = lux::crypto::generate_ed25519_private_key();
    const auto result2 = lux::crypto::generate_ed25519_private_key();

    REQUIRE(result1.has_value());
    REQUIRE(result2.has_value());
    CHECK(result1->data != result2->data);
}

LUX_TEST_CASE("ed25519_public_key", "derives successfully from private key", "[crypto][key][ed25519]")
{
    const auto private_key_result = lux::crypto::generate_ed25519_private_key();
    REQUIRE(private_key_result.has_value());

    const auto public_key_result = lux::crypto::derive_public_key(*private_key_result);

    REQUIRE(public_key_result.has_value());
    CHECK(public_key_result->data.size() == lux::crypto::ed25519_public_key::size);
}

LUX_TEST_CASE("ed25519_public_key", "derives consistent key from same private key", "[crypto][key][ed25519]")
{
    const auto private_key_result = lux::crypto::generate_ed25519_private_key();
    REQUIRE(private_key_result.has_value());

    const auto public_key1 = lux::crypto::derive_public_key(*private_key_result);
    const auto public_key2 = lux::crypto::derive_public_key(*private_key_result);

    REQUIRE(public_key1.has_value());
    REQUIRE(public_key2.has_value());
    CHECK(public_key1->data == public_key2->data);
}

LUX_TEST_CASE("ed25519_public_key", "derives different keys from different private keys", "[crypto][key][ed25519]")
{
    const auto private_key1 = lux::crypto::generate_ed25519_private_key();
    const auto private_key2 = lux::crypto::generate_ed25519_private_key();

    REQUIRE(private_key1.has_value());
    REQUIRE(private_key2.has_value());

    const auto public_key1 = lux::crypto::derive_public_key(*private_key1);
    const auto public_key2 = lux::crypto::derive_public_key(*private_key2);

    REQUIRE(public_key1.has_value());
    REQUIRE(public_key2.has_value());
    CHECK(public_key1->data != public_key2->data);
}

LUX_TEST_CASE("ed25519_private_key", "converts to PEM format successfully", "[crypto][key][ed25519][pem]")
{
    const auto private_key_result = lux::crypto::generate_ed25519_private_key();
    REQUIRE(private_key_result.has_value());

    const auto pem_result = lux::crypto::to_pem(*private_key_result);

    REQUIRE(pem_result.has_value());
    CHECK_FALSE(pem_result->empty());
    CHECK(pem_result->find("-----BEGIN PRIVATE KEY-----") != std::string::npos);
    CHECK(pem_result->find("-----END PRIVATE KEY-----") != std::string::npos);
}

LUX_TEST_CASE("ed25519_public_key", "converts to PEM format successfully", "[crypto][key][ed25519][pem]")
{
    const auto private_key_result = lux::crypto::generate_ed25519_private_key();
    REQUIRE(private_key_result.has_value());

    const auto public_key_result = lux::crypto::derive_public_key(*private_key_result);
    REQUIRE(public_key_result.has_value());

    const auto pem_result = lux::crypto::to_pem(*public_key_result);

    REQUIRE(pem_result.has_value());
    CHECK_FALSE(pem_result->empty());
    CHECK(pem_result->find("-----BEGIN PUBLIC KEY-----") != std::string::npos);
    CHECK(pem_result->find("-----END PUBLIC KEY-----") != std::string::npos);
}

LUX_TEST_CASE("ed25519_private_key", "produces consistent PEM for same key", "[crypto][key][ed25519][pem]")
{
    const auto private_key_result = lux::crypto::generate_ed25519_private_key();
    REQUIRE(private_key_result.has_value());

    const auto pem1 = lux::crypto::to_pem(*private_key_result);
    const auto pem2 = lux::crypto::to_pem(*private_key_result);

    REQUIRE(pem1.has_value());
    REQUIRE(pem2.has_value());
    CHECK(*pem1 == *pem2);
}

LUX_TEST_CASE("ed25519_public_key", "produces consistent PEM for same key", "[crypto][key][ed25519][pem]")
{
    const auto private_key_result = lux::crypto::generate_ed25519_private_key();
    REQUIRE(private_key_result.has_value());

    const auto public_key_result = lux::crypto::derive_public_key(*private_key_result);
    REQUIRE(public_key_result.has_value());

    const auto pem1 = lux::crypto::to_pem(*public_key_result);
    const auto pem2 = lux::crypto::to_pem(*public_key_result);

    REQUIRE(pem1.has_value());
    REQUIRE(pem2.has_value());
    CHECK(*pem1 == *pem2);
}
#include <lux/crypto/cert.hpp>
#include <lux/crypto/key.hpp>

#include <catch2/catch_all.hpp>

#include "test_case.hpp"

#include <string>

LUX_TEST_CASE("csr", "generates CSR successfully with minimal subject info", "[csr][crypto]")
{
    const auto keypair_result = lux::crypto::generate_ed25519_keypair();
    REQUIRE(keypair_result.has_value());

    const lux::crypto::subject_info subject{.common_name = "example.com"};

    const auto csr_result = lux::crypto::generate_csr(keypair_result.value(), subject, lux::crypto::csr_format::pem);

    REQUIRE(csr_result.has_value());
    CHECK(csr_result->format == lux::crypto::csr_format::pem);
    CHECK_FALSE(csr_result->data.empty());

    const auto& data = csr_result->data;
    const std::string pem_str(reinterpret_cast<const char*>(data.data()), data.size());
    CHECK(pem_str.find("-----BEGIN CERTIFICATE REQUEST-----") != std::string::npos);
    CHECK(pem_str.find("-----END CERTIFICATE REQUEST-----") != std::string::npos);
}

LUX_TEST_CASE("csr", "generates CSR successfully with complete subject info", "[csr][crypto]")
{
    const auto keypair_result = lux::crypto::generate_ed25519_keypair();
    REQUIRE(keypair_result.has_value());

    const lux::crypto::subject_info subject{.common_name = "example.com",
                                            .country = "US",
                                            .state = "California",
                                            .locality = "San Francisco",
                                            .organization = "Example Inc",
                                            .organizational_unit = "IT Department",
                                            .email = "admin@example.com"};

    const auto csr_result = lux::crypto::generate_csr(keypair_result.value(), subject, lux::crypto::csr_format::pem);

    REQUIRE(csr_result.has_value());
    CHECK(csr_result->format == lux::crypto::csr_format::pem);
    CHECK_FALSE(csr_result->data.empty());
}

LUX_TEST_CASE("csr", "generates CSR in DER format successfully", "[csr][crypto]")
{
    const auto keypair_result = lux::crypto::generate_ed25519_keypair();
    REQUIRE(keypair_result.has_value());

    const lux::crypto::subject_info subject{.common_name = "example.com",
                                            .country = "US",
                                            .organization = "Example Inc"};

    const auto csr_result = lux::crypto::generate_csr(keypair_result.value(), subject, lux::crypto::csr_format::der);

    REQUIRE(csr_result.has_value());
    CHECK(csr_result->format == lux::crypto::csr_format::der);
    CHECK_FALSE(csr_result->data.empty());

    const auto& data = csr_result->data;
    const std::string der_str(reinterpret_cast<const char*>(data.data()), data.size());
    CHECK(der_str.find("-----BEGIN") == std::string::npos);
}

LUX_TEST_CASE("csr", "returns error when common name is empty", "[csr][crypto]")
{
    const auto keypair_result = lux::crypto::generate_ed25519_keypair();
    REQUIRE(keypair_result.has_value());

    const lux::crypto::subject_info subject{.common_name = "", .country = "US"};

    const auto csr_result = lux::crypto::generate_csr(keypair_result.value(), subject, lux::crypto::csr_format::pem);

    REQUIRE_FALSE(csr_result.has_value());
    CHECK(csr_result.error().str() == "Common name (CN) is required for CSR generation\n");
}

LUX_TEST_CASE("csr", "generates different CSRs for different keypairs", "[csr][crypto]")
{
    const auto keypair1_result = lux::crypto::generate_ed25519_keypair();
    const auto keypair2_result = lux::crypto::generate_ed25519_keypair();
    REQUIRE(keypair1_result.has_value());
    REQUIRE(keypair2_result.has_value());

    const lux::crypto::subject_info subject{.common_name = "example.com"};

    const auto csr1_result = lux::crypto::generate_csr(keypair1_result.value(), subject, lux::crypto::csr_format::pem);
    const auto csr2_result = lux::crypto::generate_csr(keypair2_result.value(), subject, lux::crypto::csr_format::pem);

    REQUIRE(csr1_result.has_value());
    REQUIRE(csr2_result.has_value());
    CHECK(csr1_result->data != csr2_result->data);
}

LUX_TEST_CASE("csr", "generates different CSRs for different subjects", "[csr][crypto]")
{
    const auto keypair_result = lux::crypto::generate_ed25519_keypair();
    REQUIRE(keypair_result.has_value());

    const lux::crypto::subject_info subject1{.common_name = "example1.com"};
    const lux::crypto::subject_info subject2{.common_name = "example2.com"};

    const auto csr1_result = lux::crypto::generate_csr(keypair_result.value(), subject1, lux::crypto::csr_format::pem);
    const auto csr2_result = lux::crypto::generate_csr(keypair_result.value(), subject2, lux::crypto::csr_format::pem);

    REQUIRE(csr1_result.has_value());
    REQUIRE(csr2_result.has_value());
    CHECK(csr1_result->data != csr2_result->data);
}

LUX_TEST_CASE("csr", "handles optional subject fields correctly", "[csr][crypto]")
{
    const auto keypair_result = lux::crypto::generate_ed25519_keypair();
    REQUIRE(keypair_result.has_value());

    SECTION("With only common name")
    {
        const lux::crypto::subject_info subject{.common_name = "test.com"};

        const auto csr_result = lux::crypto::generate_csr(keypair_result.value(),
                                                          subject,
                                                          lux::crypto::csr_format::pem);

        REQUIRE(csr_result.has_value());
        CHECK_FALSE(csr_result->data.empty());
    }

    SECTION("With country only")
    {
        const lux::crypto::subject_info subject{.common_name = "test.com", .country = "PL"};

        const auto csr_result = lux::crypto::generate_csr(keypair_result.value(),
                                                          subject,
                                                          lux::crypto::csr_format::pem);

        REQUIRE(csr_result.has_value());
        CHECK_FALSE(csr_result->data.empty());
    }

    SECTION("With organization only")
    {
        const lux::crypto::subject_info subject{.common_name = "test.com", .organization = "Test Org"};

        const auto csr_result = lux::crypto::generate_csr(keypair_result.value(),
                                                          subject,
                                                          lux::crypto::csr_format::pem);

        REQUIRE(csr_result.has_value());
        CHECK_FALSE(csr_result->data.empty());
    }

    SECTION("With email only")
    {
        const lux::crypto::subject_info subject{.common_name = "test.com", .email = "test@test.com"};

        const auto csr_result = lux::crypto::generate_csr(keypair_result.value(),
                                                          subject,
                                                          lux::crypto::csr_format::pem);

        REQUIRE(csr_result.has_value());
        CHECK_FALSE(csr_result->data.empty());
    }
}

LUX_TEST_CASE("csr", "generates consistent output format for same inputs", "[csr][crypto]")
{
    const auto keypair_result = lux::crypto::generate_ed25519_keypair();
    REQUIRE(keypair_result.has_value());

    const lux::crypto::subject_info subject{.common_name = "example.com", .country = "US"};

    const auto csr1_result = lux::crypto::generate_csr(keypair_result.value(), subject, lux::crypto::csr_format::pem);
    const auto csr2_result = lux::crypto::generate_csr(keypair_result.value(), subject, lux::crypto::csr_format::pem);

    REQUIRE(csr1_result.has_value());
    REQUIRE(csr2_result.has_value());
    CHECK(csr1_result->data == csr2_result->data);
}

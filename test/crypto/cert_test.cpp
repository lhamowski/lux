#include <lux/crypto/cert.hpp>
#include <lux/crypto/key.hpp>

#include <catch2/catch_all.hpp>

#include "test_case.hpp"

LUX_TEST_CASE("generate_csr", "generates CSR successfully with basic subject info", "[crypto][cert][csr]")
{
    const auto private_key_result{lux::crypto::generate_ed25519_private_key()};
    REQUIRE(private_key_result.has_value());

    lux::crypto::subject_info subject{};
    subject.common_name = "test.example.com";

    const auto csr_result{lux::crypto::generate_csr(*private_key_result, subject)};

    REQUIRE(csr_result.has_value());
    CHECK_FALSE(csr_result->get().empty());
}

LUX_TEST_CASE("generate_csr", "generates CSR successfully with full subject info", "[crypto][cert][csr]")
{
    const auto private_key_result{lux::crypto::generate_ed25519_private_key()};
    REQUIRE(private_key_result.has_value());

    lux::crypto::subject_info subject{};
    subject.common_name = "test.example.com";
    subject.country = "US";
    subject.state = "California";
    subject.locality = "San Francisco";
    subject.organization = "Test Organization";
    subject.organizational_unit = "IT Department";
    subject.email = "admin@example.com";

    const auto csr_result{lux::crypto::generate_csr(*private_key_result, subject)};

    REQUIRE(csr_result.has_value());
    CHECK_FALSE(csr_result->get().empty());
}

LUX_TEST_CASE("generate_csr", "generates CSR successfully with subject alternative names", "[crypto][cert][csr]")
{
    const auto private_key_result{lux::crypto::generate_ed25519_private_key()};
    REQUIRE(private_key_result.has_value());

    lux::crypto::subject_info subject{};
    subject.common_name = "test.example.com";
    subject.subject_alt_names = {"www.example.com", "api.example.com", "example.com"};

    const auto csr_result{lux::crypto::generate_csr(*private_key_result, subject)};

    REQUIRE(csr_result.has_value());
    CHECK_FALSE(csr_result->get().empty());
}

LUX_TEST_CASE("generate_csr", "generates CSR successfully with all fields", "[crypto][cert][csr]")
{
    const auto private_key_result{lux::crypto::generate_ed25519_private_key()};
    REQUIRE(private_key_result.has_value());

    lux::crypto::subject_info subject{};
    subject.common_name = "test.example.com";
    subject.country = "US";
    subject.state = "California";
    subject.locality = "San Francisco";
    subject.organization = "Test Organization";
    subject.organizational_unit = "IT Department";
    subject.email = "admin@example.com";
    subject.subject_alt_names = {"www.example.com", "api.example.com"};

    const auto csr_result{lux::crypto::generate_csr(*private_key_result, subject)};

    REQUIRE(csr_result.has_value());
    CHECK_FALSE(csr_result->get().empty());
}

LUX_TEST_CASE("generate_csr", "generates different CSRs with different keys", "[crypto][cert][csr]")
{
    const auto private_key1{lux::crypto::generate_ed25519_private_key()};
    const auto private_key2{lux::crypto::generate_ed25519_private_key()};
    REQUIRE(private_key1.has_value());
    REQUIRE(private_key2.has_value());

    lux::crypto::subject_info subject{};
    subject.common_name = "test.example.com";

    const auto csr1{lux::crypto::generate_csr(*private_key1, subject)};
    const auto csr2{lux::crypto::generate_csr(*private_key2, subject)};

    REQUIRE(csr1.has_value());
    REQUIRE(csr2.has_value());
    CHECK(csr1->get() != csr2->get());
}

LUX_TEST_CASE("csr_pem", "converts DER CSR to PEM format successfully", "[crypto][cert][csr][pem]")
{
    const auto private_key_result{lux::crypto::generate_ed25519_private_key()};
    REQUIRE(private_key_result.has_value());

    lux::crypto::subject_info subject{};
    subject.common_name = "test.example.com";

    const auto csr_der_result{lux::crypto::generate_csr(*private_key_result, subject)};
    REQUIRE(csr_der_result.has_value());

    const auto csr_pem_result{lux::crypto::to_pem(*csr_der_result)};

    REQUIRE(csr_pem_result.has_value());
    CHECK_FALSE(csr_pem_result->get().empty());
    CHECK(csr_pem_result->get().find("-----BEGIN CERTIFICATE REQUEST-----") != std::string::npos);
    CHECK(csr_pem_result->get().find("-----END CERTIFICATE REQUEST-----") != std::string::npos);
}

LUX_TEST_CASE("csr_pem", "produces consistent PEM for same DER CSR", "[crypto][cert][csr][pem]")
{
    const auto private_key_result{lux::crypto::generate_ed25519_private_key()};
    REQUIRE(private_key_result.has_value());

    lux::crypto::subject_info subject{};
    subject.common_name = "test.example.com";

    const auto csr_der_result{lux::crypto::generate_csr(*private_key_result, subject)};
    REQUIRE(csr_der_result.has_value());

    const auto pem1{lux::crypto::to_pem(*csr_der_result)};
    const auto pem2{lux::crypto::to_pem(*csr_der_result)};

    REQUIRE(pem1.has_value());
    REQUIRE(pem2.has_value());
    CHECK(pem1->get() == pem2->get());
}

LUX_TEST_CASE("csr_pem", "converts CSR with SANs to PEM successfully", "[crypto][cert][csr][pem]")
{
    const auto private_key_result{lux::crypto::generate_ed25519_private_key()};
    REQUIRE(private_key_result.has_value());

    lux::crypto::subject_info subject{};
    subject.common_name = "test.example.com";
    subject.subject_alt_names = {"www.example.com", "api.example.com"};

    const auto csr_der_result{lux::crypto::generate_csr(*private_key_result, subject)};
    REQUIRE(csr_der_result.has_value());

    const auto csr_pem_result{lux::crypto::to_pem(*csr_der_result)};

    REQUIRE(csr_pem_result.has_value());
    CHECK_FALSE(csr_pem_result->get().empty());
    CHECK(csr_pem_result->get().find("-----BEGIN CERTIFICATE REQUEST-----") != std::string::npos);
    CHECK(csr_pem_result->get().find("-----END CERTIFICATE REQUEST-----") != std::string::npos);
}

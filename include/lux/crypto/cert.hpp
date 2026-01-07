#pragma once

#include <lux/crypto/key.hpp>

#include <lux/support/result.hpp>
#include <lux/support/strong_typedef.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lux::crypto {

/**
 * Information about the subject for a CSR.
 */
struct subject_info
{
    std::string common_name;
    std::optional<std::string> country = std::nullopt;
    std::optional<std::string> state = std::nullopt;
    std::optional<std::string> locality = std::nullopt;
    std::optional<std::string> organization = std::nullopt;
    std::optional<std::string> organizational_unit = std::nullopt;
    std::optional<std::string> email = std::nullopt;
    std::vector<std::string> subject_alt_names;
};

/**
 * DER-encoded CSR.
 */
using csr_der = lux::strong_typedef<std::vector<std::byte>, struct csr_der_tag>;

/**
 * PEM-encoded CSR.
 */
using csr_pem = lux::strong_typedef<std::string, struct csr_pem_tag>;

/**
 * Generates a CSR using the provided ED25519 private key and subject information.
 *
 * @param private_key The ED25519 private key to sign the CSR.
 * @param subject The subject information to include in the CSR.
 * @return A result containing the DER-encoded CSR on success, or an error message on failure.
 */
lux::result<csr_der> generate_csr(const lux::crypto::ed25519_private_key& private_key, const subject_info& subject);

/**
 * Converts a DER-encoded CSR to PEM format.
 *
 * @param der_csr The DER-encoded CSR to convert.
 * @return A result containing the PEM-encoded CSR on success, or an error message on failure.
 */
lux::result<csr_pem> to_pem(const csr_der& der_csr);

} // namespace lux::crypto
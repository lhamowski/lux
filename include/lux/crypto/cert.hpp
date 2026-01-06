#pragma once

#include <lux/support/result.hpp>
#include <lux/crypto/key.hpp>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lux::crypto {

struct subject_info
{
    std::string common_name;
    std::optional<std::string> country = std::nullopt;
    std::optional<std::string> state = std::nullopt;
    std::optional<std::string> locality = std::nullopt;
    std::optional<std::string> organization = std::nullopt;
    std::optional<std::string> organizational_unit = std::nullopt;
    std::optional<std::string> email = std::nullopt;
};

enum class csr_format
{
    pem,
    der
};

struct csr_result
{
    std::vector<std::byte> data;
    csr_format format;
};

lux::result<csr_result> generate_csr(const ed25519_keypair& keys, const subject_info& sub, csr_format fmt);

} // namespace lux::crypto
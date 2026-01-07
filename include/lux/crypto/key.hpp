#pragma once

#include <lux/crypto/container.hpp>

#include <lux/support/result.hpp>

#include <array>
#include <cstddef>

namespace lux::crypto {

struct ed25519_private_key
{
    static constexpr std::size_t size = 32;

    ed25519_private_key() : data{size, std::byte{0}}
    {
    }

    lux::crypto::secure_vector<std::byte> data;
};

struct ed25519_public_key
{
    static constexpr std::size_t size = 32;
    std::array<std::byte, size> data;
};

lux::result<ed25519_private_key> generate_ed25519_private_key();
lux::result<ed25519_public_key> derive_public_key(const ed25519_private_key& private_key);

lux::result<lux::crypto::secure_string> to_pem(const ed25519_private_key& private_key);
lux::result<std::string> to_pem(const ed25519_public_key& public_key);

} // namespace lux::crypto
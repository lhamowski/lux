#pragma once

#include <lux/support/result.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace lux::crypto {

struct ed25519_keypair
{
    std::vector<std::byte> private_key;
    std::vector<std::byte> public_key;
};

lux::result<ed25519_keypair> generate_ed25519_keypair();

} // namespace lux::crypto
#pragma once

#include <lux/net/base/address_v4.hpp>

#include <cstdint>
namespace lux::net::base {

class endpoint
{
public:
    endpoint() : address_{}, port_{0}
    {
    }

    explicit endpoint(const address_v4& addr, std::uint16_t port) : address_{addr}, port_{port}
    {
    }

    lux::net::base::address_v4 address() const noexcept
    {
        return address_;
    }

    std::uint16_t port() const noexcept
    {
        return port_;
    }

    bool operator==(const endpoint& other) const noexcept
    {
        return address_ == other.address_ && port_ == other.port_;
    }

    bool operator!=(const endpoint& other) const noexcept
    {
        return !(*this == other);
    }

private:
    lux::net::base::address_v4 address_;
    std::uint16_t port_;
};

} // namespace lux::net::base
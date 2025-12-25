#pragma once

#include <lux/io/net/base/address_v4.hpp>

#include <cstdint>
#include <string>
#include <string_view>

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

class hostname_endpoint
{
public:
    hostname_endpoint() : host_{}, port_{0}
    {
    }

    hostname_endpoint(std::string_view host, std::uint16_t port) : host_{host}, port_{port}
    {
    }

    std::string_view host() const noexcept
    {
        return host_;
    }

    std::uint16_t port() const noexcept
    {
        return port_;
    }

    bool operator==(const hostname_endpoint& other) const noexcept
    {
        return host_ == other.host_ && port_ == other.port_;
    }

    bool operator!=(const hostname_endpoint& other) const noexcept
    {
        return !(*this == other);
    }

private:
    std::string host_;
    std::uint16_t port_;
};

} // namespace lux::net::base
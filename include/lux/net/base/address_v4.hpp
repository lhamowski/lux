#pragma once

#include <boost/asio/ip/address_v4.hpp>

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace lux::net::base {

class address_v4
{
public:
    using bytes_type = std::array<std::byte, 4>;
    using uint_type = std::uint32_t;

public:
    explicit address_v4(const bytes_type& bytes) : addr_{bytes}
    {
    }

    explicit address_v4(uint_type addr)
    {
        addr_[0] = static_cast<std::byte>((addr >> 24) & 0xFF);
        addr_[1] = static_cast<std::byte>((addr >> 16) & 0xFF);
        addr_[2] = static_cast<std::byte>((addr >> 8) & 0xFF);
        addr_[3] = static_cast<std::byte>(addr & 0xFF);
    }

    explicit address_v4(std::string_view addr) : address_v4{boost::asio::ip::make_address_v4(addr).to_uint()}
    {
    }

public:
    auto operator<=>(const address_v4& other) const = default;

public:
    bytes_type to_bytes() const noexcept
    {
        return addr_;
    }

    uint_type to_uint() const noexcept
    {
        return (static_cast<uint_type>(addr_[0]) << 24) | (static_cast<uint_type>(addr_[1]) << 16) |
               (static_cast<uint_type>(addr_[2]) << 8) | static_cast<uint_type>(addr_[3]);
    }

    std::string to_string() const
    {
        return std::to_string(static_cast<uint_type>(addr_[0])) + "." +
               std::to_string(static_cast<uint_type>(addr_[1])) + "." +
               std::to_string(static_cast<uint_type>(addr_[2])) + "." +
               std::to_string(static_cast<uint_type>(addr_[3]));
    }

private:
    bytes_type addr_{};
};

inline address_v4 localhost = address_v4{0x7F000001};
inline address_v4 any_address = address_v4{0x00000000};
inline address_v4 broadcast_address = address_v4{0xFFFFFFFF};

} // namespace lux::net::base
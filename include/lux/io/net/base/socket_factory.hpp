#pragma once

#include <lux/io/net/base/udp_socket.hpp>

#include <memory>

namespace lux::net::base {

class socket_factory
{
public:
    virtual ~socket_factory() = default;

public:
    /**
     * Creates a UDP socket with the specified handler.
     * @param handler The handler that will manage the UDP socket events.
     * @return A unique pointer to the created UDP socket.
     */
    virtual std::unique_ptr<lux::net::base::udp_socket>
        create_udp_socket(const lux::net::base::udp_socket_config& config,
                          lux::net::base::udp_socket_handler& handler) = 0;
};

} // namespace lux::net::base
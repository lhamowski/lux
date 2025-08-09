#pragma once

#include <lux/io/net/base/tcp_socket.hpp>
#include <lux/io/net/base/udp_socket.hpp>

#include <memory>

namespace lux::net::base {

class socket_factory
{
public:
    virtual ~socket_factory() = default;

public:
    /**
     * Creates a UDP socket with the specified configuration and handler.
     * @param config The configuration for the UDP socket.
     * @param handler The handler that will manage the UDP socket events.
     * @return A unique pointer to the created UDP socket.
     */
    virtual lux::net::base::udp_socket_ptr create_udp_socket(const lux::net::base::udp_socket_config& config,
                                                             lux::net::base::udp_socket_handler& handler) = 0;

    /**
     * Creates a TCP socket with the specified configuration and handler.
     * @param config The configuration for the TCP socket.
     * @param handler The handler that will manage the TCP socket events.
     * @return A unique pointer to the created TCP socket.
     */
    virtual lux::net::base::tcp_socket_ptr create_tcp_socket(const lux::net::base::tcp_socket_config& config,
                                                             lux::net::base::tcp_socket_handler& handler) = 0;
};

} // namespace lux::net::base
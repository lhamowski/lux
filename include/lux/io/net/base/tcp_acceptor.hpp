#pragma once

#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/tcp_socket.hpp>

namespace lux::net::base {

class tcp_acceptor_handler
{
public:
    /**
     * Called when a new connection is accepted.
     * @param socket The TCP socket representing the accepted connection.
     * @param local_endpoint The local endpoint of the acceptor that accepted the connection.
     */
    virtual void on_accepted(lux::net::base::tcp_socket& socket, const lux::net::base::endpoint& local_endpoint) = 0;

protected:
    virtual ~tcp_acceptor_handler() = default;
};

class tcp_acceptor
{
public:
    /**
     * Starts listening for incoming connections on the specified endpoint.
     * @param endpoint The endpoint to listen on.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code listen(const lux::net::base::endpoint& endpoint) = 0;

    /**
     * Starts listening for incoming connections on the specified host and port.
     * Uses resolved host and port to create an endpoint.
     * @param host The host to listen on.
     * @param port The port to listen on.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code listen(std::string_view host, std::uint16_t port) = 0;

    /**
     * Closes the acceptor.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code close() = 0;

    /**
     * Checks if the acceptor is currently listening for incoming connections.
     * @return True if the acceptor is listening, false otherwise.
     */
    virtual bool is_listening() const = 0;

protected:
    virtual ~tcp_acceptor() = default;
};

} // namespace lux::net::base
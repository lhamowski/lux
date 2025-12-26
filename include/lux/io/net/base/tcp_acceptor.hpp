#pragma once

#include <lux/fwd.hpp>
#include <lux/io/net/base/socket_config.hpp>
#include <lux/io/net/base/tcp_socket.hpp>

#include <memory>
#include <system_error>

namespace lux::net::base {

struct tcp_acceptor_config
{
    /**
     * Keep-alive settings
     * If true, enables TCP keep-alive for accepted connections.
     */
    bool keep_alive{false};

    /**
     * If true, allows the socket to reuse the already bound address.
     */
    bool reuse_address{true};

    /**
     * Buffer configuration for the TCP socket.
     * This structure holds various buffer-related settings for the TCP socket.
     */
    lux::net::base::socket_buffer_config socket_buffer{};
};

/**
 * Abstract base class for TCP acceptors.
 * Provides an interface for accepting incoming TCP connections.
 */
class tcp_acceptor
{
public:
    virtual ~tcp_acceptor() = default;

public:
    /**
     * Listens for incoming TCP connections. If a connection is accepted, the handler's on_accepted method is called.
     * @param endpoint The endpoint to listen on.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code listen(const lux::net::base::endpoint& endpoint) = 0;

    /**
     * Closes the acceptor, cancelling any pending operations and stopping acceptance of new connections.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code close() = 0;

    /**
    * Gets the local endpoint of the acceptor.
    * @return The local endpoint if started, otherwise std::nullopt.
    */
    virtual std::optional<lux::net::base::endpoint> local_endpoint() const = 0;
};

class tcp_acceptor_handler
{
public:
    /**
     * Called when a new TCP connection is accepted.
     * @param socket_ptr A unique pointer to the accepted TCP inbound socket.
     */
    virtual void on_accepted(lux::net::base::tcp_inbound_socket_ptr&& socket_ptr) = 0;

    /**
     * Called when an error occurs during the accept operation.
     * @param ec The error code indicating the reason for the error.
     */
    virtual void on_accept_error(const std::error_code& ec) = 0;

protected:
    virtual ~tcp_acceptor_handler() = default;
};

using tcp_acceptor_ptr = std::unique_ptr<tcp_acceptor>;

} // namespace lux::net::base
#pragma once

#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/http_response.hpp>
#include <lux/io/net/base/http_request.hpp>
#include <lux/io/net/base/tcp_acceptor.hpp>

#include <memory>
#include <optional>
#include <system_error>

namespace lux::net::base {

struct http_server_config
{
    /**
     * TCP acceptor configuration for incoming connections.
     */
    lux::net::base::tcp_acceptor_config acceptor_config{};
};

class http_server
{
public:
    virtual ~http_server() = default;

public:
    /**
     * Starts the HTTP server on the specified endpoint.
     * @param ep The endpoint to bind the server to.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code serve(const lux::net::base::endpoint& ep) = 0;

    /**
     * Stops the HTTP server.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code stop() = 0;

    /**
     * Retrieves the local endpoint the server is bound to.
     * @return An optional endpoint representing the local endpoint, or std::nullopt if not bound.
     */
    virtual std::optional<lux::net::base::endpoint> local_endpoint() const = 0;
};

using http_server_ptr = std::unique_ptr<http_server>;

class http_server_handler
{
public:
    virtual ~http_server_handler() = default;

public:
    /**
     * Called when the server has successfully started and is ready to accept connections.
     */
    virtual void on_server_started() = 0;

    /**
     * Called when the server has stopped.
     */
    virtual void on_server_stopped() = 0;

    /**
     * Called when there is an error in the server.
     * @param ec The error code representing the error.
     */
    virtual void on_server_error(const std::error_code& ec) = 0;

    /**
     * Handles an incoming HTTP request and generates an appropriate response.
     * @param request The incoming HTTP request.
     * @return The generated HTTP response.
     */
    virtual lux::net::base::http_response handle_request(const lux::net::base::http_request& request) = 0;
};

} // namespace lux::net::base
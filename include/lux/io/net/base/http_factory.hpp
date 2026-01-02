#pragma once

#include <lux/io/net/base/http_client.hpp>
#include <lux/io/net/base/http_server.hpp>
#include <lux/io/net/base/ssl.hpp>

namespace lux::net::base {

class http_factory
{
public:
    /**
     * Creates an HTTP server with the specified configuration and handler.
     * @param config The configuration for the HTTP server.
     * @param handler The handler that will manage the HTTP server events.
     * @return A unique pointer to the created HTTP server.
     */
    virtual lux::net::base::http_server_ptr create_http_server(const lux::net::base::http_server_config& config,
                                                               lux::net::base::http_server_handler& handler) = 0;

    /**
     * Creates an HTTPS server with the specified configuration, handler, and SSL context.
     * @param config The configuration for the HTTPS server.
     * @param handler The handler that will manage the HTTPS server events.
     * @param ssl_context The SSL context to be used by the HTTPS server.
     * @return A unique pointer to the created HTTPS server.
     */
    virtual lux::net::base::http_server_ptr create_https_server(const lux::net::base::http_server_config& config,
                                                                lux::net::base::http_server_handler& handler,
                                                                lux::net::base::ssl_context& ssl_context) = 0;

    /**
     * Creates an HTTP client with the specified destination and configuration.
     * @param destination The destination endpoint for the HTTP client.
     * @param config The configuration for the HTTP client.
     * @return A unique pointer to the created HTTP client.
     */
    virtual lux::net::base::http_client_ptr create_http_client(const lux::net::base::hostname_endpoint& destination,
                                                               const lux::net::base::http_client_config& config) = 0;

    /**
     * Creates an HTTPS client with the specified destination, configuration, and SSL context.
     * @param destination The destination endpoint for the HTTPS client.
     * @param config The configuration for the HTTPS client.
     * @param ssl_context The SSL context to be used by the HTTPS client.
     * @return A unique pointer to the created HTTPS client.
     */
    virtual lux::net::base::http_client_ptr create_https_client(const lux::net::base::hostname_endpoint& destination,
                                                                const lux::net::base::http_client_config& config,
                                                                lux::net::base::ssl_context& ssl_context) = 0;

protected:
    virtual ~http_factory() = default;
};

} // namespace lux::net::base
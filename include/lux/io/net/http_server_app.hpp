#pragma once

#include <lux/fwd.hpp>

#include <lux/io/net/base/http_response.hpp>
#include <lux/io/net/base/http_request.hpp>
#include <lux/io/net/base/http_server.hpp>
#include <lux/io/net/base/ssl.hpp>

#include <lux/io/net/http_router.hpp>

#include <functional>
#include <string_view>
#include <system_error>

namespace lux::net {

struct http_server_app_config
{
    /**
     * Configuration for the underlying HTTP server.
     */
    lux::net::base::http_server_config server_config{};

    /**
     * Name of the server to be used in the Server HTTP header.
     */
    std::string server_name{"LuxHTTPServer"};
};

/**
 * Represents a facade for an HTTP application, providing methods to register request handlers and manage the server
 * lifecycle. It supports both HTTP and HTTPS protocols.
 */
class http_server_app : public lux::net::base::http_server_handler
{
public:
    using handler_type = lux::net::http_router::handler_type;
    using error_handler_type = std::function<void(const std::error_code&)>;

public:
    http_server_app(const lux::net::http_server_app_config& config, lux::net::base::http_factory& factory);
    http_server_app(const lux::net::http_server_app_config& config,
                    lux::net::base::http_factory& factory,
                    lux::net::base::ssl_context& ssl_context);

    ~http_server_app();

    http_server_app(const http_server_app&) = delete;
    http_server_app& operator=(const http_server_app&) = delete;
    http_server_app(http_server_app&&) = default;
    http_server_app& operator=(http_server_app&&) = delete;

public:
    /**
     * Starts the HTTP server on the specified endpoint.
     * @param ep The endpoint to bind the server to.
     * @return An error code indicating success or failure.
     */
    std::error_code serve(const lux::net::base::endpoint& ep);

    /**
     * Stops the HTTP server.
     * @return An error code indicating success or failure.
     */
    std::error_code stop();

    /**
     * Registers a handler for HTTP GET requests to a specific target.
     * @param target The request target (path) to handle.
     * @param handler The function to handle the request and generate a response.
     */
    void get(std::string_view target, handler_type handler);

    /**
     * Registers a handler for HTTP POST requests to a specific target.
     * @param target The request target (path) to handle.
     * @param handler The function to handle the request and generate a response.
     */
    void post(std::string_view target, handler_type handler);

    /**
     * Registers a handler for HTTP PUT requests to a specific target.
     * @param target The request target (path) to handle.
     * @param handler The function to handle the request and generate a response.
     */
    void put(std::string_view target, handler_type handler);

    /**
     * Registers a handler for HTTP DELETE requests to a specific target.
     * @param target The request target (path) to handle.
     * @param handler The function to handle the request and generate a response.
     */
    void del(std::string_view target, handler_type handler);

    /**
     * Sets a custom error handler to be invoked on server errors.
     * @param handler The function to handle server errors.
     */
    void set_on_error_handler(error_handler_type handler);

    /**
     * Retrieves the local endpoint the server is bound to.
     * @return An optional endpoint representing the local endpoint, or std::nullopt if not bound.
     */
    std::optional<lux::net::base::endpoint> local_endpoint() const;

private:
    // lux::net::base::http_server_handler implementation declarations
    void on_server_started() override;
    void on_server_stopped() override;
    void on_server_error(const std::error_code& ec) override;
    lux::net::base::http_response handle_request(const lux::net::base::http_request& request) override;

private:
    const lux::net::http_server_app_config config_;
    lux::net::base::http_server_ptr server_ptr_{nullptr};
    lux::net::http_router router_;

private:
    error_handler_type on_error_handler_;
};

} // namespace lux::net
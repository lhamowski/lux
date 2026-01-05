#pragma once

#include <lux/fwd.hpp>
#include <lux/io/net/base/http_client.hpp>
#include <lux/io/net/base/http_request.hpp>
#include <lux/io/net/base/ssl.hpp>

#include <string>
#include <string_view>

namespace lux::net {

struct http_client_app_config
{
    /**
     * Configuration for the underlying HTTP client.
     */
    lux::net::base::http_client_config client_config{};
};

class http_client_app
{
public:
    http_client_app(const lux::net::base::hostname_endpoint& destination,
                    lux::net::base::http_factory& factory,
                    const lux::net::http_client_app_config& config = {});
    http_client_app(const lux::net::base::hostname_endpoint& destination,
                    lux::net::base::http_factory& factory,
                    lux::net::base::ssl_context& ssl_context,
                    const lux::net::http_client_app_config& config = {});

    ~http_client_app() = default;

    http_client_app(const http_client_app&) = delete;
    http_client_app& operator=(const http_client_app&) = delete;
    http_client_app(http_client_app&&) = default;
    http_client_app& operator=(http_client_app&&) = default;

public:
    using headers_type = lux::net::base::http_request::headers_type;
    using handler_type = lux::net::base::http_client_handler_type;

public:
    /**
     * Send a GET request.
     * @param target The target path for the GET request.
     * @param handler The callback to invoke when the request completes.
     * @param headers The headers to include in the request.
     */
    void get(std::string_view target, handler_type handler, headers_type headers = {});

    /**
     * Send a POST request.
     * @param target The target path for the POST request.
     * @param handler The callback to invoke when the request completes.
     * @param headers The headers to include in the request.
     * @param body The body of the request.
     */
    void post(std::string_view target, handler_type handler, headers_type headers = {}, const std::string& body = {});

    /**
     * Send a PUT request.
     * @param target The target path for the PUT request.
     * @param handler The callback to invoke when the request completes.
     * @param headers The headers to include in the request.
     * @param body The body of the request.
     */
    void put(std::string_view target, handler_type handler, headers_type headers = {}, const std::string& body = {});

    /**
     * Send a DELETE request.
     * @param target The target path for the DELETE request.
     * @param handler The callback to invoke when the request completes.
     * @param headers The headers to include in the request.
     * @param body The body of the request.
     */
    void del(std::string_view target, handler_type handler, headers_type headers = {}, const std::string& body = {});

private:
    lux::net::base::http_client_ptr client_ptr_{nullptr};
};

} // namespace lux::net
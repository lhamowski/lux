#pragma once

#include <lux/fwd.hpp>
#include <lux/io/net/base/tcp_socket.hpp>

#include <expected>
#include <functional>
#include <memory>
#include <system_error>

namespace lux::net::base {

struct http_client_config
{
    /**
     * Keep-alive settings
     * If true, enables TCP keep-alive to maintain the connection.
     */
    bool keep_alive{false};

    /**
     * Buffer configuration for the TCP socket.
     * This structure holds various buffer-related settings for the TCP socket.
     */
    lux::net::base::socket_buffer_config buffer{};
};

using http_request_result = std::expected<lux::net::base::http_response, std::error_code>;
using http_client_handler_type = std::function<void(const http_request_result&)>;

class http_client
{
public:
    virtual ~http_client() = default;

public:
    /**
     * Sends an HTTP request.
     * @param request The HTTP request to send.
     * @param handler The callback to invoke when the request completes.
     * @note This function is asynchronous and may not complete immediately.
     */
    virtual void request(const lux::net::base::http_request& request, http_client_handler_type handler) = 0;
};

using http_client_ptr = std::unique_ptr<lux::net::base::http_client>;

} // namespace lux::net::base
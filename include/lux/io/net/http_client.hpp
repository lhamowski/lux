#pragma once

#include <lux/fwd.hpp>

#include <lux/io/net/base/http_client.hpp>
#include <lux/io/net/base/ssl.hpp>

#include <memory>
#include <system_error>

namespace lux::net {

class http_client : public lux::net::base::http_client
{
public:
    http_client(const lux::net::base::hostname_endpoint& destination,
                const lux::net::base::http_client_config& config,
                lux::net::base::socket_factory& socket_factory);

    http_client(const lux::net::base::hostname_endpoint& destination,
                const lux::net::base::http_client_config& config,
                lux::net::base::socket_factory& socket_factory,
                lux::net::base::ssl_context& ssl_context);

    ~http_client();

    http_client(const http_client&) = delete;
    http_client& operator=(const http_client&) = delete;
    http_client(http_client&&) = default;
    http_client& operator=(http_client&&) = default;

public:
    // lux::net::base::http_client implementation
    void request(const lux::net::base::http_request& request,
                 lux::net::base::http_client_handler_type handler) override;

private:
    class impl;
    std::unique_ptr<impl> impl_;
};

} // namespace lux::net
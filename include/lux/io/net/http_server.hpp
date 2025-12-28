#pragma once

#include <lux/fwd.hpp>

#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/http_server.hpp>
#include <lux/io/net/base/ssl.hpp>

#include <memory>
#include <system_error>

namespace lux::net {

class http_server : public lux::net::base::http_server
{
public:
    // ctor for non-SSL server
    http_server(const lux::net::base::http_server_config& config,
                lux::net::base::http_server_handler& handler,
                lux::net::base::socket_factory& socket_factory);

    // ctor for SSL server
    http_server(const lux::net::base::http_server_config& config,
                lux::net::base::http_server_handler& handler,
                lux::net::base::socket_factory& socket_factory,
                lux::net::base::ssl_context& ssl_context);

    ~http_server();

    http_server(const http_server&) = delete;
    http_server& operator=(const http_server&) = delete;
    http_server(http_server&&) = default;
    http_server& operator=(http_server&&) = default;

public:
    // lux::net::base::http_server implementation declarations
    std::error_code serve(const lux::net::base::endpoint& ep) override;
    std::error_code stop() override;
    std::optional<lux::net::base::endpoint> local_endpoint() const override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

} // namespace lux::net
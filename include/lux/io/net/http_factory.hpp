#pragma once

#include <lux/fwd.hpp>
#include <lux/io/net/base/http_factory.hpp>

namespace lux::net {

class http_factory : public lux::net::base::http_factory
{
public:
    http_factory(lux::net::base::socket_factory& socket_factory);

public:
    // lux::net::base::http_factory implementation
    lux::net::base::http_server_ptr create_http_server(const lux::net::base::http_server_config& config,
                                                       lux::net::base::http_server_handler& handler) override;

    lux::net::base::http_server_ptr create_https_server(const lux::net::base::http_server_config& config,
                                                        lux::net::base::http_server_handler& handler,
                                                        lux::net::base::ssl_context& ssl_context) override;

private:
    lux::net::base::socket_factory& socket_factory_;
};

} // namespace lux::net
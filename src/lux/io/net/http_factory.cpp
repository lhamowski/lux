#include <lux/io/net/http_factory.hpp>

#include <lux/io/net/base/ssl.hpp>
#include <lux/io/net/base/socket_factory.hpp>

#include <lux/io/net/http_server.hpp>
#include <lux/io/net/http_client.hpp>

#include <memory>

namespace lux::net {

http_factory::http_factory(lux::net::base::socket_factory& socket_factory) : socket_factory_(socket_factory)
{
}

lux::net::base::http_server_ptr http_factory::create_http_server(const lux::net::base::http_server_config& config,
                                                                 lux::net::base::http_server_handler& handler)
{
    return std::make_unique<lux::net::http_server>(config, handler, socket_factory_);
}

lux::net::base::http_server_ptr http_factory::create_https_server(const lux::net::base::http_server_config& config,
                                                                  lux::net::base::http_server_handler& handler,
                                                                  lux::net::base::ssl_context& ssl_context)
{
    return std::make_unique<lux::net::http_server>(config, handler, socket_factory_, ssl_context);
}

lux::net::base::http_client_ptr http_factory::create_http_client(const lux::net::base::hostname_endpoint& destination,
                                                                 const lux::net::base::http_client_config& config)
{
    return std::make_unique<lux::net::http_client>(destination, config, socket_factory_);
}

lux::net::base::http_client_ptr http_factory::create_https_client(const lux::net::base::hostname_endpoint& destination,
                                                                  const lux::net::base::http_client_config& config,
                                                                  lux::net::base::ssl_context& ssl_context)
{
    return std::make_unique<lux::net::http_client>(destination, config, socket_factory_, ssl_context);
}

} // namespace lux::net

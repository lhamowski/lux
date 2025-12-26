#include <lux/io/net/socket_factory.hpp>

#include <lux/io/net/tcp_acceptor.hpp>
#include <lux/io/net/tcp_socket.hpp>
#include <lux/io/net/udp_socket.hpp>

namespace lux::net {

net::socket_factory::socket_factory(boost::asio::any_io_executor exe) : executor_{exe}, timer_factory_{exe}
{
}

lux::net::base::udp_socket_ptr socket_factory::create_udp_socket(const lux::net::base::udp_socket_config& config,
                                                                 lux::net::base::udp_socket_handler& handler)
{
    return std::make_unique<lux::net::udp_socket>(executor_, handler, config);
}

lux::net::base::tcp_socket_ptr socket_factory::create_tcp_socket(const lux::net::base::tcp_socket_config& config,
                                                                 lux::net::base::tcp_socket_handler& handler)
{
    return std::make_unique<lux::net::tcp_socket>(executor_, handler, config, timer_factory_);
}

lux::net::base::tcp_socket_ptr socket_factory::create_ssl_tcp_socket(const lux::net::base::tcp_socket_config& config,
                                                                     lux::net::base::ssl_context& ssl_context,
                                                                     lux::net::base::ssl_mode ssl_mode,
                                                                     lux::net::base::tcp_socket_handler& handler)
{
    return std::make_unique<lux::net::ssl_tcp_socket>(executor_,
                                                      handler,
                                                      config,
                                                      timer_factory_,
                                                      ssl_context,
                                                      ssl_mode);
}

lux::net::base::tcp_acceptor_ptr socket_factory::create_tcp_acceptor(const lux::net::base::tcp_acceptor_config& config,
                                                                     lux::net::base::tcp_acceptor_handler& handler)
{
    return std::make_unique<lux::net::tcp_acceptor>(executor_, handler, config);
}

lux::net::base::tcp_acceptor_ptr
    socket_factory::create_ssl_tcp_acceptor(const lux::net::base::tcp_acceptor_config& config,
                                            lux::net::base::ssl_context& ssl_context,
                                            lux::net::base::tcp_acceptor_handler& handler)
{
    return std::make_unique<lux::net::ssl_tcp_acceptor>(executor_, handler, config, ssl_context);
}

} // namespace lux::net
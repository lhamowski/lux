#include <lux/io/net/socket_factory.hpp>

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

} // namespace lux::net
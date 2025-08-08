#include <lux/io/net/socket_factory.hpp>

#include <lux/io/net/udp_socket.hpp>

namespace lux::net {

net::socket_factory::socket_factory(boost::asio::any_io_executor exe) : executor_(exe)
{
}

std::unique_ptr<lux::net::base::udp_socket>
    net::socket_factory::create_udp_socket(const lux::net::base::udp_socket_config& config,
                                           lux::net::base::udp_socket_handler& handler)
{
    return std::make_unique<lux::net::udp_socket>(executor_, handler, config);
}

} // namespace lux::net
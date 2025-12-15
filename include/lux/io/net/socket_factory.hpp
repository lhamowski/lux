#pragma once

#include <lux/io/net/base/socket_factory.hpp>
#include <lux/io/net/base/tcp_socket.hpp>
#include <lux/io/net/base/udp_socket.hpp>

#include <lux/io/time/timer_factory.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace lux::net {

class socket_factory : public lux::net::base::socket_factory
{
public:
    socket_factory(boost::asio::any_io_executor exe);

public:
    // lux::net::base::socket_factory implementation
    lux::net::base::udp_socket_ptr create_udp_socket(const lux::net::base::udp_socket_config& config,
                                                     lux::net::base::udp_socket_handler& handler) override;

    lux::net::base::tcp_socket_ptr create_tcp_socket(const lux::net::base::tcp_socket_config& config,
                                                     lux::net::base::tcp_socket_handler& handler) override;

    lux::net::base::tcp_socket_ptr create_ssl_tcp_socket(const lux::net::base::tcp_socket_config& config,
                                                         lux::net::base::ssl_context& ssl_context,
                                                         lux::net::base::ssl_mode ssl_mode,
                                                         lux::net::base::tcp_socket_handler& handler) override;

private:
    boost::asio::any_io_executor executor_;
    lux::time::timer_factory timer_factory_;
};

} // namespace lux::net
#pragma once

#include <lux/io/net/base/udp_socket.hpp>
#include <lux/io/net/base/socket_factory.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace lux::net {

class socket_factory : public lux::net::base::socket_factory
{
public:
    socket_factory(boost::asio::any_io_executor exe);

public:
    std::unique_ptr<lux::net::base::udp_socket> create_udp_socket(const lux::net::base::udp_socket_config& config,
                                                                  lux::net::base::udp_socket_handler& handler) override;

private:
    boost::asio::any_io_executor executor_;
};

} // namespace lux::net
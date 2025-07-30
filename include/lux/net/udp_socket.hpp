#pragma once

#include <lux/net/base/udp_socket.hpp>
#include <lux/fwd.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ip/udp.hpp>

#include <memory>

namespace lux::net {

class udp_socket : public lux::net::base::udp_socket
{
public:
    udp_socket(boost::asio::any_io_executor exe, lux::net::base::udp_socket_handler& handler, lux::logger& logger);

    ~udp_socket();

    udp_socket(const udp_socket&) = delete;
    udp_socket& operator=(const udp_socket&) = delete;
    udp_socket(udp_socket&&) = default;
    udp_socket& operator=(udp_socket&&) = default;

public:
    // lux::net::base::udp_socket implementation
    bool open() override;
    bool close(bool send_pending_data) override;
    void read() override;
    bool bind(const boost::asio::ip::udp::endpoint& endpoint) override;
    void send(const boost::asio::ip::udp::endpoint& endpoint, const std::span<const std::byte>& data) override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

} // namespace lux::net
#pragma once

#include <lux/fwd.hpp>
#include <lux/io/net/base/tcp_socket.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace lux::net {

class tcp_socket : public lux::net::base::tcp_socket
{
public:
    tcp_socket(boost::asio::any_io_executor exe,
               lux::net::base::tcp_socket_handler& handler,
               const lux::net::base::tcp_socket_config& config);
    ~tcp_socket();

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&) = delete;
    tcp_socket(tcp_socket&&) = default;
    tcp_socket& operator=(tcp_socket&&) = default;

public:
    // lux::net::base::tcp_socket implementation
    std::error_code connect(const lux::net::base::endpoint& endpoint) override;
    std::error_code connect(const lux::net::base::host_endpoint& host_endpoint) override;
    std::error_code disconnect(bool send_pending) override;
    void send(const std::span<const std::byte>& data) override;
    bool is_connected() const override;
    std::optional<lux::net::base::endpoint> local_endpoint() const override;
    std::optional<lux::net::base::endpoint> remote_endpoint() const override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

} // namespace lux::net
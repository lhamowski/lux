#pragma once

#include <lux/fwd.hpp>
#include <lux/io/net/base/tcp_socket.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ssl/context.hpp>

namespace lux::net {

class tcp_socket : public lux::net::base::tcp_socket
{
public:
    tcp_socket(boost::asio::any_io_executor exe,
               lux::net::base::tcp_socket_handler& handler,
               const lux::net::base::tcp_socket_config& config,
               lux::time::base::timer_factory& timer_factory);
    ~tcp_socket();

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&) = delete;
    tcp_socket(tcp_socket&&) = default;
    tcp_socket& operator=(tcp_socket&&) = default;

public:
    // lux::net::base::tcp_socket implementation
    std::error_code connect(const lux::net::base::endpoint& endpoint) override;
    std::error_code connect(const lux::net::base::hostname_endpoint& hostname_endpoint) override;
    std::error_code disconnect(bool send_pending) override;
    std::error_code send(const std::span<const std::byte>& data) override;
    bool is_connected() const override;
    std::optional<lux::net::base::endpoint> local_endpoint() const override;
    std::optional<lux::net::base::endpoint> remote_endpoint() const override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

class ssl_tcp_socket : public lux::net::base::tcp_socket
{
public:
    ssl_tcp_socket(boost::asio::any_io_executor exe,
                   lux::net::base::tcp_socket_handler& handler,
                   const lux::net::base::tcp_socket_config& config,
                   lux::time::base::timer_factory& timer_factory,
                   lux::net::base::ssl_context& ssl_context,
                   lux::net::base::ssl_mode ssl_mode);
    ~ssl_tcp_socket();

    ssl_tcp_socket(const ssl_tcp_socket&) = delete;
    ssl_tcp_socket& operator=(const ssl_tcp_socket&) = delete;
    ssl_tcp_socket(ssl_tcp_socket&&) = default;
    ssl_tcp_socket& operator=(ssl_tcp_socket&&) = default;

public:
    // lux::net::base::tcp_socket implementation
    std::error_code connect(const lux::net::base::endpoint& endpoint) override;
    std::error_code connect(const lux::net::base::hostname_endpoint& hostname_endpoint) override;
    std::error_code disconnect(bool send_pending) override;
    std::error_code send(const std::span<const std::byte>& data) override;
    bool is_connected() const override;
    std::optional<lux::net::base::endpoint> local_endpoint() const override;
    std::optional<lux::net::base::endpoint> remote_endpoint() const override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

} // namespace lux::net
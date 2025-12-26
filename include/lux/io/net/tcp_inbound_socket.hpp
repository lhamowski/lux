#pragma once

#include <lux/fwd.hpp>
#include <lux/io/net/base/tcp_socket.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace lux::net {

class tcp_inbound_socket : public lux::net::base::tcp_inbound_socket
{
public:
    tcp_inbound_socket(boost::asio::ip::tcp::socket&& socket,
                       const lux::net::base::tcp_inbound_socket_config& config);
    ~tcp_inbound_socket();

    tcp_inbound_socket(const tcp_inbound_socket&) = delete;
    tcp_inbound_socket& operator=(const tcp_inbound_socket&) = delete;
    tcp_inbound_socket(tcp_inbound_socket&&) = default;
    tcp_inbound_socket& operator=(tcp_inbound_socket&&) = default;

public:
    // lux::net::base::tcp_inbound_socket implementation
    void set_handler(lux::net::base::tcp_inbound_socket_handler& handler) override;
    std::error_code send(const std::span<const std::byte>& data) override;
    void read() override;

    std::error_code disconnect(bool send_pending) override;
    bool is_connected() const override;
    std::optional<lux::net::base::endpoint> local_endpoint() const override;
    std::optional<lux::net::base::endpoint> remote_endpoint() const override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

class ssl_tcp_inbound_socket : public lux::net::base::tcp_inbound_socket
{
public:
    ssl_tcp_inbound_socket(boost::asio::ssl::stream<boost::asio::ip::tcp::socket>&& stream,
                           const lux::net::base::tcp_inbound_socket_config& config);

    ~ssl_tcp_inbound_socket();

    ssl_tcp_inbound_socket(const ssl_tcp_inbound_socket&) = delete;
    ssl_tcp_inbound_socket& operator=(const ssl_tcp_inbound_socket&) = delete;
    ssl_tcp_inbound_socket(ssl_tcp_inbound_socket&&) = default;
    ssl_tcp_inbound_socket& operator=(ssl_tcp_inbound_socket&&) = default;

public:
    // lux::net::base::tcp_inbound_socket implementation
    void set_handler(lux::net::base::tcp_inbound_socket_handler& handler) override;
    std::error_code send(const std::span<const std::byte>& data) override;
    void read() override;
    
    std::error_code disconnect(bool send_pending) override;
    bool is_connected() const override;
    std::optional<lux::net::base::endpoint> local_endpoint() const override;
    std::optional<lux::net::base::endpoint> remote_endpoint() const override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

} // namespace lux::net
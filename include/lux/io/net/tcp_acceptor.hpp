#pragma once

#include <lux/fwd.hpp>
#include <lux/io/net/base/ssl.hpp>
#include <lux/io/net/base/tcp_acceptor.hpp>

#include <boost/asio/any_io_executor.hpp>

#include <memory>

namespace lux::net {

class tcp_acceptor : public lux::net::base::tcp_acceptor
{
public:
    tcp_acceptor(boost::asio::any_io_executor exe,
                 lux::net::base::tcp_acceptor_handler& handler,
                 const lux::net::base::tcp_acceptor_config& config);
    ~tcp_acceptor();

    tcp_acceptor(const tcp_acceptor&) = delete;
    tcp_acceptor& operator=(const tcp_acceptor&) = delete;
    tcp_acceptor(tcp_acceptor&&) = default;
    tcp_acceptor& operator=(tcp_acceptor&&) = default;

public:
    std::error_code listen(const lux::net::base::endpoint& endpoint) override;
    std::error_code close() override;
    std::optional<lux::net::base::endpoint> local_endpoint() const override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

class ssl_tcp_acceptor : public lux::net::base::tcp_acceptor
{
public:
    ssl_tcp_acceptor(boost::asio::any_io_executor exe,
                     lux::net::base::tcp_acceptor_handler& handler,
                     const lux::net::base::tcp_acceptor_config& config,
                     lux::net::base::ssl_context& ssl_context);
    ~ssl_tcp_acceptor();

    ssl_tcp_acceptor(const ssl_tcp_acceptor&) = delete;
    ssl_tcp_acceptor& operator=(const ssl_tcp_acceptor&) = delete;
    ssl_tcp_acceptor(ssl_tcp_acceptor&&) = default;
    ssl_tcp_acceptor& operator=(ssl_tcp_acceptor&&) = default;

public:
    std::error_code listen(const lux::net::base::endpoint& endpoint) override;
    std::error_code close() override;
    std::optional<lux::net::base::endpoint> local_endpoint() const override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

} // namespace lux::net
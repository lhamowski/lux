#include <lux/io/net/socket_factory.hpp>
#include <lux/io/net/tcp_acceptor.hpp>
#include <lux/io/net/tcp_socket.hpp>
#include <lux/io/net/udp_socket.hpp>

#include <catch2/catch_all.hpp>

namespace {

class test_udp_socket_handler : public lux::net::base::udp_socket_handler
{
public:
    void on_data_read(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data) override
    {
        (void)endpoint;
        (void)data;
    }

    void on_data_sent(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data) override
    {
        (void)endpoint;
        (void)data;
    }

    void on_read_error(const lux::net::base::endpoint& endpoint, const std::error_code& ec) override
    {
        (void)endpoint;
        (void)ec;
    }

    void on_send_error(const lux::net::base::endpoint& endpoint,
                       const std::span<const std::byte>& data,
                       const std::error_code& ec) override
    {
        (void)endpoint;
        (void)data;
        (void)ec;
    }
};

class test_tcp_socket_handler : public lux::net::base::tcp_socket_handler
{
public:
    void on_connected(lux::net::base::tcp_socket& socket) override
    {
        (void)socket;
    }

    void on_disconnected(lux::net::base::tcp_socket& socket, const std::error_code& ec, bool will_reconnect) override
    {
        (void)socket;
        (void)ec;
        (void)will_reconnect;
    }

    void on_data_read(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        (void)socket;
        (void)data;
    }

    void on_data_sent(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        (void)socket;
        (void)data;
    }
};

class test_tcp_acceptor_handler : public lux::net::base::tcp_acceptor_handler
{
public:
    void on_accepted(lux::net::base::tcp_inbound_socket_ptr&& socket_ptr) override
    {
        (void)socket_ptr;
    }
    void on_accept_error(const std::error_code& ec) override
    {
        (void)ec;
    }
};

} // namespace

TEST_CASE("Socket factory creates UDP socket", "[io][net]")
{
    boost::asio::io_context io_context;

    test_udp_socket_handler handler;
    lux::net::base::udp_socket_config config{};
    lux::net::socket_factory factory{io_context.get_executor()};

    auto socket = factory.create_udp_socket(config, handler);
    REQUIRE(socket != nullptr);
}

TEST_CASE("Socket factory creates TCP socket", "[io][net]")
{
    boost::asio::io_context io_context;

    test_tcp_socket_handler handler;
    lux::net::base::tcp_socket_config config{};
    lux::net::socket_factory factory{io_context.get_executor()};

    auto socket = factory.create_tcp_socket(config, handler);
    REQUIRE(socket != nullptr);
}

TEST_CASE("Socket factory creates SSL TCP socket", "[io][net]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;

    lux::net::base::tcp_socket_config config{};
    lux::net::base::ssl_context ssl_context{boost::asio::ssl::context::sslv23};
    lux::net::socket_factory factory{io_context.get_executor()};

    auto socket = factory.create_ssl_tcp_socket(config, ssl_context, handler);
    REQUIRE(socket != nullptr);
}

TEST_CASE("Socket factory creates TCP acceptor", "[io][net]")
{
    boost::asio::io_context io_context;
    test_tcp_acceptor_handler handler;

    lux::net::base::tcp_acceptor_config config{};
    lux::net::socket_factory factory{io_context.get_executor()};

    auto acceptor = factory.create_tcp_acceptor(config, handler);
    REQUIRE(acceptor != nullptr);
}

TEST_CASE("Socket factory creates SSL TCP acceptor", "[io][net]")
{
    boost::asio::io_context io_context;
    test_tcp_acceptor_handler handler;

    lux::net::base::tcp_acceptor_config config{};
    lux::net::base::ssl_context ssl_context{boost::asio::ssl::context::sslv23};
    lux::net::socket_factory factory{io_context.get_executor()};

    auto acceptor = factory.create_ssl_tcp_acceptor(config, ssl_context, handler);
    REQUIRE(acceptor != nullptr);
}
#include "test_utils.hpp"

#include <lux/io/net/tcp_acceptor.hpp>
#include <lux/io/net/tcp_socket.hpp>
#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/address_v4.hpp>
#include <lux/io/time/timer_factory.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <vector>
#include <memory>
#include <chrono>

namespace {

class test_tcp_acceptor_handler : public lux::net::base::tcp_acceptor_handler
{
public:
    void on_accepted(lux::net::base::tcp_inbound_socket_ptr&& socket_ptr) override
    {
        accepted_sockets.push_back(std::move(socket_ptr));
        if (on_accepted_callback)
        {
            on_accepted_callback();
        }
    }

    void on_accept_error(const std::error_code& ec) override
    {
        if (on_accept_error_callback)
        {
            on_accept_error_callback(ec);
        }
    }

    std::vector<lux::net::base::tcp_inbound_socket_ptr> accepted_sockets;
    std::function<void()> on_accepted_callback;
    std::function<void(const std::error_code&)> on_accept_error_callback;
};

class test_tcp_socket_handler : public lux::net::base::tcp_socket_handler
{
public:
    void on_connected(lux::net::base::tcp_socket& socket) override
    {
        std::ignore = socket;
        connected_calls++;
        if (on_connected_callback)
        {
            on_connected_callback();
        }
    }

    void on_disconnected(lux::net::base::tcp_socket& socket, const std::error_code& ec, bool will_reconnect) override
    {
        std::ignore = socket;
        std::ignore = ec;
        std::ignore = will_reconnect;
        disconnected_calls++;
        if (on_disconnected_callback)
        {
            on_disconnected_callback();
        }
    }

    void on_data_read(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket;
        data_read_calls.emplace_back(std::vector<std::byte>{data.begin(), data.end()});
    }

    void on_data_sent(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket;
        std::ignore = data;
    }

    std::size_t connected_calls{0};
    std::size_t disconnected_calls{0};
    std::vector<std::vector<std::byte>> data_read_calls;

    std::function<void()> on_connected_callback;
    std::function<void()> on_disconnected_callback;
};

lux::net::base::tcp_acceptor_config create_default_acceptor_config()
{
    lux::net::base::tcp_acceptor_config config{};
    config.reuse_address = true;
    config.keep_alive = false;
    return config;
}

lux::net::base::tcp_socket_config create_default_socket_config()
{
    lux::net::base::tcp_socket_config config{};
    config.reconnect.enabled = false;
    return config;
}

std::uint16_t get_available_port(boost::asio::io_context& io_context)
{
    boost::asio::ip::tcp::acceptor temp_acceptor{io_context,
                                                 boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto port = temp_acceptor.local_endpoint().port();
    temp_acceptor.close();
    return port;
}

} // namespace

TEST_CASE("Construction succeeds with default config", "[io][net][tcp][acceptor]")
{
    boost::asio::io_context io_context;
    test_tcp_acceptor_handler handler;
    const auto config = create_default_acceptor_config();

    std::optional<lux::net::tcp_acceptor> acceptor;
    REQUIRE_NOTHROW(acceptor.emplace(io_context.get_executor(), handler, config));
}

TEST_CASE("Listen with specific endpoint succeeds", "[io][net][tcp][acceptor]")
{
    boost::asio::io_context io_context;
    test_tcp_acceptor_handler handler;
    const auto config = create_default_acceptor_config();
    lux::net::tcp_acceptor acceptor{io_context.get_executor(), handler, config};

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, 0};
    const auto listen_error = acceptor.listen(endpoint);
    CHECK_FALSE(listen_error);

    acceptor.close();
}

TEST_CASE("Accept single connection", "[io][net][tcp][acceptor]")
{
    boost::asio::io_context io_context;

    test_tcp_acceptor_handler acceptor_handler;
    const auto acceptor_config = create_default_acceptor_config();
    lux::net::tcp_acceptor acceptor{io_context.get_executor(), acceptor_handler, acceptor_config};

    const lux::net::base::endpoint bind_endpoint{lux::net::base::localhost, 0};
    const auto listen_error = acceptor.listen(bind_endpoint);
    REQUIRE_FALSE(listen_error);

    test_tcp_socket_handler socket_handler;
    socket_handler.on_connected_callback = [&]() { io_context.stop(); };
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    const auto socket_config = create_default_socket_config();
    lux::net::tcp_socket client_socket{io_context.get_executor(), socket_handler, socket_config, timer_factory};

    const auto ep = acceptor.local_endpoint();
    REQUIRE(ep.has_value());

    const auto connect_error = client_socket.connect(*ep);
    CHECK_FALSE(connect_error);

    io_context.run_for(std::chrono::seconds{2});

    REQUIRE(!acceptor_handler.accepted_sockets.empty());

    const auto& accepted_socket = acceptor_handler.accepted_sockets[0];
    CHECK(accepted_socket != nullptr);
    CHECK(accepted_socket->is_connected());
    CHECK(accepted_socket->local_endpoint().has_value());
    CHECK(accepted_socket->remote_endpoint().has_value());

    acceptor.close();

    // Try to connect again after acceptor is cancelled
    test_tcp_socket_handler second_socket_handler;
    second_socket_handler.on_disconnected_callback = [&]() { io_context.stop(); };
    lux::net::tcp_socket second_client_socket{io_context.get_executor(),
                                              second_socket_handler,
                                              socket_config,
                                              timer_factory};
    const auto second_connect_error = second_client_socket.connect(*ep);
    CHECK_FALSE(second_connect_error);

    io_context.restart();
    io_context.run_for(std::chrono::seconds{3});
    CHECK(second_socket_handler.connected_calls == 0);
    CHECK(second_socket_handler.disconnected_calls == 1);
}

TEST_CASE("SSL tcp_acceptor construction succeeds", "[io][net][tcp][acceptor][ssl]")
{
    boost::asio::io_context io_context;
    test_tcp_acceptor_handler handler;
    const auto config = create_default_acceptor_config();
    auto ssl_context = lux::test::net::create_ssl_server_context();

    std::optional<lux::net::ssl_tcp_acceptor> acceptor;
    REQUIRE_NOTHROW(acceptor.emplace(io_context.get_executor(), handler, config, ssl_context));
}

TEST_CASE("SSL tcp_acceptor listen succeeds", "[io][net][tcp][acceptor][ssl]")
{
    boost::asio::io_context io_context;
    test_tcp_acceptor_handler handler;
    const auto config = create_default_acceptor_config();
    auto ssl_context = lux::test::net::create_ssl_server_context();
    lux::net::ssl_tcp_acceptor acceptor{io_context.get_executor(), handler, config, ssl_context};

    const auto port = get_available_port(io_context);
    const lux::net::base::endpoint endpoint{lux::net::base::localhost, port};
    const auto listen_error = acceptor.listen(endpoint);
    CHECK_FALSE(listen_error);

    acceptor.close();
}

TEST_CASE("SSL tcp_acceptor accept single connection", "[io][net][tcp][acceptor][ssl]")
{
    boost::asio::io_context io_context;

    test_tcp_acceptor_handler acceptor_handler;
    const auto acceptor_config = create_default_acceptor_config();
    auto server_ssl_context = lux::test::net::create_ssl_server_context();
    lux::net::ssl_tcp_acceptor acceptor{io_context.get_executor(), acceptor_handler, acceptor_config, server_ssl_context};

    const lux::net::base::endpoint bind_endpoint{lux::net::base::localhost, 0};
    const auto listen_error = acceptor.listen(bind_endpoint);
    REQUIRE_FALSE(listen_error);

    bool client_connected = false;
    bool connection_accepted = false;

    test_tcp_socket_handler socket_handler;
    socket_handler.on_connected_callback = [&]() {
        client_connected = true;
        if (connection_accepted)
        {
            io_context.stop();
        }
    };

    acceptor_handler.on_accepted_callback = [&]() {
        connection_accepted = true;
        if (client_connected)
        {
            io_context.stop();
        }
    };

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    const auto socket_config = create_default_socket_config();
    auto client_ssl_context = lux::test::net::create_ssl_client_context();
    lux::net::ssl_tcp_socket client_socket{io_context.get_executor(),
                                           socket_handler,
                                           socket_config,
                                           timer_factory,
                                           client_ssl_context,
                                           lux::net::base::ssl_mode::client};

    const auto ep = acceptor.local_endpoint();
    REQUIRE(ep.has_value());

    const auto connect_error = client_socket.connect(*ep);
    CHECK_FALSE(connect_error);

    io_context.run_for(std::chrono::seconds{5});

    CHECK(client_connected);
    CHECK(connection_accepted);
    REQUIRE(!acceptor_handler.accepted_sockets.empty());

    const auto& accepted_socket = acceptor_handler.accepted_sockets[0];
    CHECK(accepted_socket != nullptr);
    CHECK(accepted_socket->is_connected());
    CHECK(accepted_socket->local_endpoint().has_value());
    CHECK(accepted_socket->remote_endpoint().has_value());
    CHECK(client_socket.is_connected());

    acceptor.close();

    test_tcp_socket_handler second_socket_handler;
    second_socket_handler.on_disconnected_callback = [&]() { io_context.stop(); };
    lux::net::ssl_tcp_socket second_client_socket{io_context.get_executor(),
                                                  second_socket_handler,
                                                  socket_config,
                                                  timer_factory,
                                                  client_ssl_context,
                                                  lux::net::base::ssl_mode::client};

    const auto second_connect_error = second_client_socket.connect(*ep);
    CHECK_FALSE(second_connect_error);

    io_context.restart();
    io_context.run_for(std::chrono::seconds{3});

    CHECK(second_socket_handler.connected_calls == 0);
    CHECK(second_socket_handler.disconnected_calls == 1);
}
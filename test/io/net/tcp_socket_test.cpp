#include <lux/io/net/tcp_socket.hpp>
#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/address_v4.hpp>
#include <lux/io/time/timer_factory.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <vector>
#include <cstring>
#include <thread>
#include <chrono>
#include <functional>

namespace {

class test_tcp_socket_handler : public lux::net::base::tcp_socket_handler
{
public:
    void on_connected(lux::net::base::tcp_socket& socket) override
    {
        (void)socket; // Suppress unused parameter warning
        connected_calls++;
        if (on_connected_callback)
        {
            on_connected_callback();
        }
    }

    void on_disconnected(lux::net::base::tcp_socket& socket, const std::error_code& ec) override
    {
        (void)socket; // Suppress unused parameter warning
        disconnected_calls.emplace_back(ec);
        if (on_disconnected_callback)
        {
            on_disconnected_callback(ec);
        }
    }

    void on_data_read(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        (void)socket; // Suppress unused parameter warning
        data_read_calls.emplace_back(std::vector<std::byte>(data.begin(), data.end()));
        if (on_data_read_callback)
        {
            on_data_read_callback(data);
        }
    }

    void on_data_sent(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        (void)socket; // Suppress unused parameter warning
        data_sent_calls.emplace_back(std::vector<std::byte>(data.begin(), data.end()));
        if (on_data_sent_callback)
        {
            on_data_sent_callback(data);
        }
    }

    std::size_t connected_calls{0};
    std::vector<std::error_code> disconnected_calls;
    std::vector<std::vector<std::byte>> data_read_calls;
    std::vector<std::vector<std::byte>> data_sent_calls;

    std::function<void()> on_connected_callback;
    std::function<void(const std::error_code&)> on_disconnected_callback;
    std::function<void(const std::span<const std::byte>&)> on_data_read_callback;
    std::function<void(const std::span<const std::byte>&)> on_data_sent_callback;
};

lux::net::tcp_socket_config create_default_config()
{
    lux::net::tcp_socket_config config{};
    config.reconnect.enabled = false;
    return config;
}

} // namespace

TEST_CASE("Construction succeeds with default config", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};

    std::optional<lux::net::tcp_socket> socket;
    REQUIRE_NOTHROW(socket.emplace(io_context.get_executor(), handler, config, timer_factory));

    CHECK_FALSE(socket->is_connected());
    CHECK_FALSE(socket->local_endpoint().has_value());
    CHECK_FALSE(socket->remote_endpoint().has_value());
}

TEST_CASE("Connect to invalid endpoint fails", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    bool disconnected_called = false;
    handler.on_disconnected_callback = [&](const std::error_code& ec) {
        CHECK(ec); // Should have an error
        disconnected_called = true;
        io_context.stop();
    };

    const lux::net::base::endpoint invalid_endpoint{lux::net::base::address_v4{"255.255.255.255"}, 1};
    const auto result = socket.connect(invalid_endpoint);

    CHECK_FALSE(result); // connect() should return no error (async operation started)

    io_context.run_for(std::chrono::milliseconds{100});
    CHECK(disconnected_called);
}

TEST_CASE("Connect when already connecting returns error", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, 12345};

    const auto result1 = socket.connect(endpoint);
    CHECK_FALSE(result1); // First connect should succeed

    const auto result2 = socket.connect(endpoint);
    CHECK(result2); // Second connect should fail
    CHECK(result2 == std::make_error_code(std::errc::operation_in_progress));
}

TEST_CASE("Disconnect when disconnected returns success", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    const auto result1 = socket.disconnect(false);
    const auto result2 = socket.disconnect(true);

    CHECK_FALSE(result1);
    CHECK_FALSE(result2);
}

TEST_CASE("Send data when disconnected returns error", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    const std::array<std::byte, 3> data{std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};

    const auto result = socket.send(std::span<const std::byte>{data});
    CHECK(result); // Should return error
    CHECK(result == std::make_error_code(std::errc::not_connected));

    io_context.run_for(std::chrono::milliseconds(50));
    CHECK(handler.data_sent_calls.empty());
}
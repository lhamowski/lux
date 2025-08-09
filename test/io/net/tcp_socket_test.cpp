#include <lux/io/net/tcp_socket.hpp>
#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/address_v4.hpp>
#include <lux/io/time/timer_factory.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

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
        data_read_calls.emplace_back(std::vector<std::byte>{data.begin(), data.end()});
        if (on_data_read_callback)
        {
            on_data_read_callback(data);
        }
    }

    void on_data_sent(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        (void)socket; // Suppress unused parameter warning
        data_sent_calls.emplace_back(std::vector<std::byte>{data.begin(), data.end()});
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

lux::net::base::tcp_socket_config create_default_config()
{
    lux::net::base::tcp_socket_config config{};
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

TEST_CASE("Connect to localhost with existing server succeeds", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    // Create a simple TCP server
    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    bool connected_called = false;
    handler.on_connected_callback = [&]() {
        connected_called = true;
        io_context.stop();
    };

    // Start accepting connections
    boost::asio::ip::tcp::socket server_socket{io_context};
    acceptor.async_accept(server_socket, [](const boost::system::error_code&) {});

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, server_port};
    const auto result = socket.connect(endpoint);
    CHECK_FALSE(result);

    io_context.run_for(std::chrono::milliseconds{100});
    CHECK(connected_called);
    CHECK(socket.is_connected());
    CHECK(socket.local_endpoint().has_value());
    CHECK(socket.remote_endpoint().has_value());

    // Clean up
    socket.disconnect(false);
    acceptor.close();
}

TEST_CASE("Send and receive data with server", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    // Create a simple echo server
    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket server_socket{io_context};
    std::array<char, 1024> server_buffer{};

    bool data_sent = false;
    bool data_received = false;

    handler.on_connected_callback = [&]() {
        // Send data once connected
        const std::array<std::byte, 5> test_data{
            std::byte{'h'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'}, std::byte{'o'}};
        
        const auto ec1 = socket.send(std::span<const std::byte>{test_data});
        CHECK_FALSE(ec1); // Should not return an error

        const auto ec2 = socket.send(std::span<const std::byte>{}); 
        CHECK(ec2); // Sending empty data should return error
    };

    handler.on_data_sent_callback = [&](const std::span<const std::byte>& sent_data) {
        CHECK(sent_data.size() == 5);
        CHECK(sent_data[0] == std::byte{'h'});
        CHECK(sent_data[1] == std::byte{'e'});
        CHECK(sent_data[2] == std::byte{'l'});
        CHECK(sent_data[3] == std::byte{'l'});
        CHECK(sent_data[4] == std::byte{'o'});
        data_sent = true;
    };

    handler.on_data_read_callback = [&](const std::span<const std::byte>& received_data) {
        CHECK(received_data.size() == 5);
        CHECK(received_data[0] == std::byte{'h'});
        CHECK(received_data[1] == std::byte{'e'});
        CHECK(received_data[2] == std::byte{'l'});
        CHECK(received_data[3] == std::byte{'l'});
        CHECK(received_data[4] == std::byte{'o'});
        data_received = true;
        io_context.stop();
    };

    // Simple echo server logic
    acceptor.async_accept(server_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            server_socket.async_read_some(boost::asio::buffer(server_buffer),
                                          [&](const boost::system::error_code& read_ec, std::size_t bytes_read) {
                                              if (!read_ec && bytes_read > 0)
                                              {
                                                  // Echo back the data
                                                  boost::asio::async_write(
                                                      server_socket,
                                                      boost::asio::buffer(server_buffer.data(), bytes_read),
                                                      [](const boost::system::error_code&, std::size_t) {});
                                              }
                                          });
        }
    });

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, server_port};
    const auto result = socket.connect(endpoint);
    CHECK_FALSE(result);

    io_context.run_for(std::chrono::milliseconds{200});

    CHECK(data_sent);
    CHECK(data_received);

    // Clean up
    socket.disconnect(false);
    server_socket.close();
    acceptor.close();
}

TEST_CASE("Multiple sends are queued properly", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    // Create a server that accepts but doesn't read (to test queuing)
    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket server_socket{io_context};

    int send_count = 0;
    handler.on_connected_callback = [&]() {
        // Send multiple messages
        const std::array<std::byte, 3> data1{std::byte{'1'}, std::byte{'2'}, std::byte{'3'}};
        const std::array<std::byte, 3> data2{std::byte{'4'}, std::byte{'5'}, std::byte{'6'}};
        const std::array<std::byte, 3> data3{std::byte{'7'}, std::byte{'8'}, std::byte{'9'}};

        socket.send(std::span<const std::byte>{data1});
        socket.send(std::span<const std::byte>{data2});
        socket.send(std::span<const std::byte>{data3});
    };

    handler.on_data_sent_callback = [&](const std::span<const std::byte>& sent_data) {
        CHECK(sent_data.size() == 3);
        send_count++;

        if (send_count == 3)
        {
            io_context.stop();
        }
    };

    acceptor.async_accept(server_socket, [](const boost::system::error_code&) {});

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, server_port};
    const auto result = socket.connect(endpoint);
    CHECK_FALSE(result);

    io_context.run_for(std::chrono::milliseconds{200});
    CHECK(send_count == 3);

    // Clean up
    socket.disconnect(false);
    server_socket.close();
    acceptor.close();
}

TEST_CASE("Disconnect gracefully sends pending data", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    // Create a server
    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket server_socket{io_context};

    bool data_sent = false;
    bool disconnected = false;

    handler.on_connected_callback = [&]() {
        const std::array<std::byte, 4> data{std::byte{'t'}, std::byte{'e'}, std::byte{'s'}, std::byte{'t'}};
        socket.send(std::span<const std::byte>{data});
        socket.disconnect(true); // Graceful disconnect - should send pending data
    };

    handler.on_data_sent_callback = [&](const std::span<const std::byte>&) { data_sent = true; };

    handler.on_disconnected_callback = [&](const std::error_code& ec) {
        CHECK_FALSE(ec); // Graceful disconnect should not have error
        disconnected = true;
        io_context.stop();
    };

    acceptor.async_accept(server_socket, [](const boost::system::error_code&) {});

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, server_port};
    const auto result = socket.connect(endpoint);
    CHECK_FALSE(result);

    io_context.run_for(std::chrono::milliseconds{200});

    CHECK(data_sent);
    CHECK(disconnected);

    // Clean up
    server_socket.close();
    acceptor.close();
}

TEST_CASE("Disconnect immediately doesn't send pending data", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    // Create a server
    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket server_socket{io_context};

    bool data_sent = false;
    bool disconnected = false;

    handler.on_connected_callback = [&]() {
        const std::array<std::byte, 4> data{std::byte{'t'}, std::byte{'e'}, std::byte{'s'}, std::byte{'t'}};
        socket.send(std::span<const std::byte>{data});
        socket.disconnect(false); // Immediate disconnect - may not send pending data
    };

    handler.on_disconnected_callback = [&](const std::error_code& ec) {
        CHECK_FALSE(ec); // Manual disconnect should not have error
        disconnected = true;
        io_context.stop();
    };

    handler.on_data_sent_callback = [&](const std::span<const std::byte>&) { data_sent = true; };

    acceptor.async_accept(server_socket, [](const boost::system::error_code&) {});

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, server_port};
    const auto result = socket.connect(endpoint);
    CHECK_FALSE(result);

    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(disconnected);
    CHECK_FALSE(data_sent); // Data should not have been sent due to immediate disconnect

    // Clean up
    server_socket.close();
    acceptor.close();
}

TEST_CASE("Complete lifecycle connect send receive disconnect", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    // Create echo server
    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket server_socket{io_context};
    std::array<char, 1024> server_buffer{};

    bool connected = false;
    bool data_sent = false;
    bool data_received = false;
    bool disconnected = false;

    handler.on_connected_callback = [&]() {
        connected = true;
        CHECK(socket.is_connected());
        CHECK(socket.local_endpoint().has_value());
        CHECK(socket.remote_endpoint().has_value());

        const std::array<std::byte, 9> test_data{std::byte{'l'},
                                                 std::byte{'i'},
                                                 std::byte{'f'},
                                                 std::byte{'e'},
                                                 std::byte{'c'},
                                                 std::byte{'y'},
                                                 std::byte{'c'},
                                                 std::byte{'l'},
                                                 std::byte{'e'}};
        socket.send(std::span<const std::byte>{test_data});
    };

    handler.on_data_sent_callback = [&](const std::span<const std::byte>& sent_data) {
        CHECK(sent_data.size() == 9);
        data_sent = true;
    };

    handler.on_data_read_callback = [&](const std::span<const std::byte>& received_data) {
        CHECK(received_data.size() == 9);
        data_received = true;

        // Disconnect after receiving echo
        socket.disconnect(true);
    };

    handler.on_disconnected_callback = [&](const std::error_code& ec) {
        CHECK_FALSE(ec);
        disconnected = true;
        CHECK_FALSE(socket.is_connected());
        io_context.stop();
    };

    // Echo server setup
    acceptor.async_accept(server_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            server_socket.async_read_some(boost::asio::buffer(server_buffer),
                                          [&](const boost::system::error_code& read_ec, std::size_t bytes_read) {
                                              if (!read_ec && bytes_read > 0)
                                              {
                                                  boost::asio::async_write(
                                                      server_socket,
                                                      boost::asio::buffer(server_buffer.data(), bytes_read),
                                                      [](const boost::system::error_code&, std::size_t) {});
                                              }
                                          });
        }
    });

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, server_port};
    const auto connect_result = socket.connect(endpoint);
    CHECK_FALSE(connect_result);

    io_context.run_for(std::chrono::milliseconds{300});

    CHECK(connected);
    CHECK(data_sent);
    CHECK(data_received);
    CHECK(disconnected);

    // Clean up
    server_socket.close();
    acceptor.close();
}

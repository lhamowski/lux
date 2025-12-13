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
#include <utility>

namespace {

class test_tcp_socket_handler : public lux::net::base::tcp_socket_handler
{
public:
    void on_connected(lux::net::base::tcp_socket& socket) override
    {
        std::ignore = socket; // Suppress unused parameter warning
        connected_calls++;
        if (on_connected_callback)
        {
            on_connected_callback();
        }
    }

    void on_disconnected(lux::net::base::tcp_socket& socket, const std::error_code& ec, bool will_reconnect) override
    {
        std::ignore = socket; // Suppress unused parameter warning
        disconnected_calls.emplace_back(ec);
        will_reconnect_flags.emplace_back(will_reconnect);

        if (on_disconnected_callback)
        {
            on_disconnected_callback(ec, will_reconnect);
        }
    }

    void on_data_read(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket; // Suppress unused parameter warning
        data_read_calls.emplace_back(std::vector<std::byte>{data.begin(), data.end()});
        if (on_data_read_callback)
        {
            on_data_read_callback(data);
        }
    }

    void on_data_sent(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket; // Suppress unused parameter warning
        data_sent_calls.emplace_back(std::vector<std::byte>{data.begin(), data.end()});
        if (on_data_sent_callback)
        {
            on_data_sent_callback(data);
        }
    }

    std::size_t connected_calls{0};
    std::vector<std::error_code> disconnected_calls;
    std::vector<bool> will_reconnect_flags;
    std::vector<std::vector<std::byte>> data_read_calls;
    std::vector<std::vector<std::byte>> data_sent_calls;

    std::function<void()> on_connected_callback;
    std::function<void(const std::error_code&, bool)> on_disconnected_callback;
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
    handler.on_disconnected_callback = [&](const std::error_code& ec, bool) {
        CHECK(ec); // Should have an error
        disconnected_called = true;
        io_context.stop();
    };

    const lux::net::base::endpoint invalid_endpoint{*lux::net::base::make_address_v4("255.255.255.255"), 1};
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

TEST_CASE("Connect using host endpoint with echo server", "[io][net][tcp]")
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

    handler.on_connected_callback = [&]() {
        connected = true;

        // Send test data after connection
        const std::array<std::byte, 8> test_data{std::byte{'h'},
                                                 std::byte{'o'},
                                                 std::byte{'s'},
                                                 std::byte{'t'},
                                                 std::byte{'n'},
                                                 std::byte{'a'},
                                                 std::byte{'m'},
                                                 std::byte{'e'}};
        socket.send(std::span<const std::byte>{test_data});
    };

    handler.on_data_sent_callback = [&](const std::span<const std::byte>& sent_data) {
        CHECK(sent_data.size() == 8); // "hostname"
        data_sent = true;
    };

    handler.on_data_read_callback = [&](const std::span<const std::byte>& received_data) {
        CHECK(received_data.size() == 8); // Echo back "hostname"
        CHECK(received_data[0] == std::byte{'h'});
        CHECK(received_data[1] == std::byte{'o'});
        CHECK(received_data[2] == std::byte{'s'});
        CHECK(received_data[3] == std::byte{'t'});
        CHECK(received_data[4] == std::byte{'n'});
        CHECK(received_data[5] == std::byte{'a'});
        CHECK(received_data[6] == std::byte{'m'});
        CHECK(received_data[7] == std::byte{'e'});
        data_received = true;
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

    // Connect using hostname instead of IP
    const lux::net::base::host_endpoint host_ep{"127.0.0.1", server_port}; // Use IP as hostname for testing
    const auto result = socket.connect(host_ep);
    CHECK_FALSE(result);

    io_context.run_for(std::chrono::milliseconds{300});

    CHECK(connected);
    CHECK(data_sent);
    CHECK(data_received);

    // Clean up
    server_socket.close();
    acceptor.close();

    // We don't need to explicitly disconnect the socket here since destructors will handle it
}

TEST_CASE("Connect using host endpoint to invalid hostname fails", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;
    const auto config = create_default_config();
    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    bool disconnected_called = false;
    handler.on_disconnected_callback = [&](const std::error_code& ec, bool) {
        CHECK(ec); // Should have an error (DNS resolution failure)
        disconnected_called = true;
        io_context.stop();
    };

    // Use non-existent hostname
    const lux::net::base::host_endpoint invalid_host{"255.255.255.255", 80};
    const auto result = socket.connect(invalid_host);
    CHECK_FALSE(result); // Should not return error immediately (async operation)

    io_context.run_for(std::chrono::milliseconds{300}); // Give more time for DNS resolution failure
    CHECK(disconnected_called);
    CHECK_FALSE(socket.is_connected());
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
        const std::array<std::byte, 5> test_data{std::byte{'h'},
                                                 std::byte{'e'},
                                                 std::byte{'l'},
                                                 std::byte{'l'},
                                                 std::byte{'o'}};

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

    handler.on_disconnected_callback = [&](const std::error_code& ec, bool) {
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

    handler.on_disconnected_callback = [&](const std::error_code& ec, bool) {
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

    handler.on_disconnected_callback = [&](const std::error_code& ec, bool) {
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

TEST_CASE("Reconnect on connection failure with exponential backoff", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;

    auto config = create_default_config();
    config.reconnect.enabled = true;
    config.reconnect.reconnect_policy.strategy = lux::time::base::retry_policy::backoff_strategy::exponential_backoff;
    config.reconnect.reconnect_policy.max_attempts = 2;
    config.reconnect.reconnect_policy.base_delay = std::chrono::milliseconds{50};
    config.reconnect.reconnect_policy.max_delay = std::chrono::milliseconds{200};

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();
    acceptor.close();

    handler.on_disconnected_callback = [&](const std::error_code& ec, bool will_reconnect) {
        
        CHECK(ec);
        
        if (handler.disconnected_calls.size() < 3) // 1 initial + 2 reconnect attempts
        {
            CHECK(will_reconnect);
        }
        else
        {
            CHECK_FALSE(will_reconnect);
            io_context.stop();
        }

    };

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, server_port};
    const auto ec = socket.connect(endpoint);
    CHECK_FALSE(ec);

    io_context.run_for(std::chrono::seconds{10});

    CHECK(handler.disconnected_calls.size() == 3);
    REQUIRE(handler.will_reconnect_flags.size() == 3);
    CHECK(handler.will_reconnect_flags[0] == true);  // First reconnect attempt
    CHECK(handler.will_reconnect_flags[1] == true);  // Second reconnect attempt
    CHECK(handler.will_reconnect_flags[2] == false); // No more reconnect attempts
}

TEST_CASE("Reconnect succeeds after server becomes available", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;

    auto config = create_default_config();
    config.reconnect.enabled = true;
    config.reconnect.reconnect_policy.max_attempts = 5;
    config.reconnect.reconnect_policy.base_delay = std::chrono::milliseconds{100};

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();
    acceptor.close();

    std::size_t disconnect_with_error = 0;
    boost::asio::ip::tcp::socket server_socket{io_context};

    handler.on_disconnected_callback = [&](const std::error_code& ec, bool) {
        if (ec)
        {
            ++disconnect_with_error;

            if (disconnect_with_error == 1)
            {
                acceptor.open(boost::asio::ip::tcp::v4());
                acceptor.bind(boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), server_port});
                acceptor.listen();
                acceptor.async_accept(server_socket, [](const boost::system::error_code&) {});
            }
        }
    };

    handler.on_connected_callback = [&]() { io_context.stop(); };

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, server_port};
    const auto ec = socket.connect(endpoint);
    CHECK_FALSE(ec);

    io_context.run_for(std::chrono::seconds{10});

    CHECK(handler.connected_calls == 1);
    CHECK(socket.is_connected());

    socket.disconnect(false);
    server_socket.close();
    acceptor.close();
}

TEST_CASE("Manual disconnect stops reconnection attempts", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_socket_handler handler;

    auto config = create_default_config();
    config.reconnect.enabled = true;
    config.reconnect.reconnect_policy.max_attempts = std::nullopt;
    config.reconnect.reconnect_policy.base_delay = std::chrono::milliseconds{50};

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    lux::net::tcp_socket socket{io_context.get_executor(), handler, config, timer_factory};

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();
    acceptor.close();

    std::size_t disconnect_with_error = 0;
    handler.on_disconnected_callback = [&](const std::error_code& ec, bool will_reconnect) {
        if (ec)
        {
            CHECK(will_reconnect);
            ++disconnect_with_error;

            if (disconnect_with_error == 2)
            {
                socket.disconnect(false);
                io_context.stop();
            }
        }
    };

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, server_port};
    const auto result = socket.connect(endpoint);
    CHECK_FALSE(result);

    io_context.run_for(std::chrono::seconds{10});

    CHECK(disconnect_with_error == 2);
    CHECK(handler.disconnected_calls.size() == 2);
    REQUIRE(handler.will_reconnect_flags.size() == 2);
    CHECK(handler.will_reconnect_flags[0] == true); // First reconnect attempt
    CHECK(handler.will_reconnect_flags[1] == true); // Second reconnect attempt but then manual disconnect stops further
                                                    // attempts with no more calls (already disconnected)
}
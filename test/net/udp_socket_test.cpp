#include <lux/net/udp_socket.hpp>
#include <lux/net/base/endpoint.hpp>
#include <lux/net/base/address_v4.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <vector>
#include <cstring>
#include <thread>
#include <chrono>
#include <functional>

namespace {

class test_udp_socket_handler : public lux::net::base::udp_socket_handler
{
public:
    void on_data_read(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data) override
    {
        data_read_calls.emplace_back(endpoint, std::vector<std::byte>(data.begin(), data.end()));
        if (on_data_read_callback)
        {
            on_data_read_callback(endpoint, data);
        }
    }

    void on_data_sent(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data) override
    {
        data_sent_calls.emplace_back(endpoint, std::vector<std::byte>(data.begin(), data.end()));
        if (on_data_sent_callback)
        {
            on_data_sent_callback(endpoint, data);
        }
    }

    void on_read_error(const lux::net::base::endpoint& endpoint, const std::error_code& ec) override
    {
        (void)endpoint; // Suppress unused parameter warning
        (void)ec;       // Suppress unused parameter warning
    }

    void on_send_error(const lux::net::base::endpoint& endpoint,
                       const std::span<const std::byte>& data,
                       const std::error_code& ec) override
    {
        send_error_calls.emplace_back(endpoint, std::vector<std::byte>(data.begin(), data.end()), ec);
        if (on_send_error_callback)
        {
            on_send_error_callback(endpoint, data, ec);
        }
    }

    std::vector<std::pair<lux::net::base::endpoint, std::vector<std::byte>>> data_read_calls;
    std::vector<std::pair<lux::net::base::endpoint, std::vector<std::byte>>> data_sent_calls;
    std::vector<std::pair<lux::net::base::endpoint, std::error_code>> read_error_calls;
    std::vector<std::tuple<lux::net::base::endpoint, std::vector<std::byte>, std::error_code>> send_error_calls;

    std::function<void(const lux::net::base::endpoint&, const std::span<const std::byte>&)> on_data_read_callback;
    std::function<void(const lux::net::base::endpoint&, const std::span<const std::byte>&)> on_data_sent_callback;
    std::function<void(const lux::net::base::endpoint&, const std::span<const std::byte>&, const std::error_code&)>
        on_send_error_callback;
};

} // namespace

TEST_CASE("Construction succeeds with default config", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};

    REQUIRE_NOTHROW(lux::net::udp_socket{io_context.get_executor(), handler, config});
}

TEST_CASE("Construction succeeds with custom config", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    lux::net::udp_socket_config config{};
    config.memory_arena_initial_item_size = 2048;
    config.memory_arena_initial_item_count = 8;

    REQUIRE_NOTHROW(lux::net::udp_socket{io_context.get_executor(), handler, config});
}

TEST_CASE("Socket opens successfully", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    const auto result = socket.open();
    CHECK_FALSE(result);
}

TEST_CASE("Opening already opened socket returns success", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    const auto result1 = socket.open();
    const auto result2 = socket.open();

    CHECK_FALSE(result1);
    CHECK_FALSE(result2);
}

TEST_CASE("Socket closes gracefully with send pending data", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    const auto result1 = socket.open();
    CHECK_FALSE(result1);

    const auto result2 = socket.close(true);
    CHECK_FALSE(result2);

    INFO("Socket closed gracefully with pending data sent");
    const auto err_msg = result2.message();
    INFO(err_msg);
}

TEST_CASE("Socket closes immediately without send pending data", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    socket.open();
    const auto result = socket.close(false);
    CHECK_FALSE(result);
}

TEST_CASE("Closing already closed socket returns success", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    socket.open();
    const auto result1 = socket.close(false);
    const auto result2 = socket.close(false);

    CHECK_FALSE(result1);
    CHECK_FALSE(result2);
}

TEST_CASE("Bind to localhost succeeds", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    socket.open();

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, 0};
    const auto result = socket.bind(endpoint);

    CHECK_FALSE(result);
}

TEST_CASE("Bind to any address", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    socket.open();

    const lux::net::base::endpoint endpoint{lux::net::base::any_address, 0};
    const auto result = socket.bind(endpoint);

    CHECK_FALSE(result);
}

TEST_CASE("Send data to endpoint", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    socket.open();

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, 12346};
    const std::array<std::byte, 3> data{std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};

    bool callback_called = false;
    handler.on_data_sent_callback = [&](const auto& ep, const auto& sent_data) {
        CHECK(ep == endpoint);
        CHECK(sent_data.size() == 3);
        CHECK(sent_data[0] == std::byte{'a'});
        CHECK(sent_data[1] == std::byte{'b'});
        CHECK(sent_data[2] == std::byte{'c'});
        callback_called = true;
        io_context.stop();
    };

    socket.send(endpoint, std::span<const std::byte>{data});
    io_context.run_for(std::chrono::milliseconds(100));
}

TEST_CASE("Multiple sends are queued properly", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    socket.open();

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, 12350};
    const std::array<std::byte, 3> data1{std::byte{'1'}, std::byte{'2'}, std::byte{'3'}};
    const std::array<std::byte, 3> data2{std::byte{'4'}, std::byte{'5'}, std::byte{'6'}};
    const std::array<std::byte, 3> data3{std::byte{'7'}, std::byte{'8'}, std::byte{'9'}};

    int callback_count = 0;
    handler.on_data_sent_callback = [&](const auto& ep, const auto& sent_data) {
        callback_count++;
        CHECK(ep == endpoint);
        CHECK(sent_data.size() == 3);

        if (callback_count == 3)
        {
            io_context.stop();
        }
    };

    socket.send(endpoint, std::span<const std::byte>{data1});
    socket.send(endpoint, std::span<const std::byte>{data2});
    socket.send(endpoint, std::span<const std::byte>{data3});

    io_context.run_for(std::chrono::milliseconds(300));
}

TEST_CASE("Send error callback is called on network error", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    socket.open();

    const lux::net::base::endpoint invalid_endpoint{lux::net::base::address_v4{"255.255.255.255"}, 1};
    const std::array<std::byte, 3> data{std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};

    bool error_callback_called = false;
    handler.on_send_error_callback = [&](const auto& ep, const auto& sent_data, const auto& ec) {
        CHECK(ep == invalid_endpoint);
        CHECK(sent_data.size() == 3);
        CHECK(ec);
        error_callback_called = true;
        io_context.stop();
    };

    socket.send(invalid_endpoint, std::span<const std::byte>{data});
    io_context.run_for(std::chrono::milliseconds(200));

    CHECK(error_callback_called);
}

TEST_CASE("Complete lifecycle open bind send close", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    const auto open_result = socket.open();
    CHECK_FALSE(open_result);

    const lux::net::base::endpoint bind_endpoint{lux::net::base::localhost, 0};
    const auto bind_result = socket.bind(bind_endpoint);
    CHECK_FALSE(bind_result);

    const lux::net::base::endpoint target_endpoint{lux::net::base::localhost, 12351};
    const std::array<std::byte, 5> data{std::byte{'h'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'}, std::byte{'o'}};
    REQUIRE_NOTHROW(socket.send(target_endpoint, std::span<const std::byte>{data}));

    io_context.run_for(std::chrono::milliseconds(50));

    const auto close_result = socket.close(true);
    CHECK_FALSE(close_result);
}

TEST_CASE("Receive data from external sender", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    socket.open();
    const lux::net::base::endpoint bind_endpoint{lux::net::base::localhost, 12400};
    const auto bind_result = socket.bind(bind_endpoint);
    REQUIRE_FALSE(bind_result);

    const std::array<char, 4> test_data{'t', 'e', 's', 't'};
    bool data_received = false;

    handler.on_data_read_callback = [&](const lux::net::base::endpoint& sender_endpoint,
                                        const std::span<const std::byte>& received_data) {
        CHECK(sender_endpoint.address() == lux::net::base::localhost);

        REQUIRE(received_data.size() == 4);
        CHECK(received_data[0] == std::byte{'t'});
        CHECK(received_data[1] == std::byte{'e'});
        CHECK(received_data[2] == std::byte{'s'});
        CHECK(received_data[3] == std::byte{'t'});

        data_received = true;
        io_context.stop();
    };

    boost::asio::ip::udp::socket sender_socket{io_context};
    sender_socket.open(boost::asio::ip::udp::v4());

    const boost::asio::ip::udp::endpoint boost_endpoint{boost::asio::ip::make_address("127.0.0.1"),
                                                        bind_endpoint.port()};

    sender_socket.send_to(boost::asio::buffer(test_data), boost_endpoint);
    io_context.run_for(std::chrono::milliseconds(500));

    CHECK(data_received);

    sender_socket.close();
    socket.close(false);
}

TEST_CASE("Receive multiple packets from external sender", "[udp_socket][net]")
{
    boost::asio::io_context io_context;
    test_udp_socket_handler handler;
    const lux::net::udp_socket_config config{};
    lux::net::udp_socket socket{io_context.get_executor(), handler, config};

    socket.open();
    const lux::net::base::endpoint bind_endpoint{lux::net::base::localhost, 12401};
    const auto bind_result = socket.bind(bind_endpoint);
    REQUIRE_FALSE(bind_result);

    const std::array<char, 3> data1{'a', 'b', 'c'};
    const std::array<char, 3> data2{'x', 'y', 'z'};
    int packets_received = 0;

    handler.on_data_read_callback = [&](const lux::net::base::endpoint& sender_endpoint,
                                        const std::span<const std::byte>& received_data) {
        CHECK(sender_endpoint.address() == lux::net::base::localhost);
        CHECK(received_data.size() == 3);

        packets_received++;
        if (packets_received == 2)
        {
            io_context.stop();
        }
    };

    boost::asio::ip::udp::socket sender_socket{io_context};
    sender_socket.open(boost::asio::ip::udp::v4());

    const boost::asio::ip::udp::endpoint boost_endpoint{boost::asio::ip::make_address("127.0.0.1"),
                                                        bind_endpoint.port()};

    sender_socket.send_to(boost::asio::buffer(data1), boost_endpoint);
    sender_socket.send_to(boost::asio::buffer(data2), boost_endpoint);

    io_context.run_for(std::chrono::milliseconds(200));

    CHECK(packets_received == 2);

    sender_socket.close();
    socket.close(false);
}
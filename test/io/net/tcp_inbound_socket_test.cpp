#include "test_case.hpp"
#include "io/net/test_utils.hpp"

#include <lux/support/move.hpp>

#include <lux/io/net/tcp_inbound_socket.hpp>
#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/address_v4.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ssl.hpp>

#include <vector>
#include <chrono>
#include <functional>
#include <utility>

namespace {

class test_tcp_inbound_socket_handler : public lux::net::base::tcp_inbound_socket_handler
{
public:
    void on_disconnected(lux::net::base::tcp_inbound_socket& socket, const std::error_code& ec) override
    {
        std::ignore = socket;
        disconnected_calls.emplace_back(ec);
        if (on_disconnected_callback)
        {
            on_disconnected_callback(ec);
        }
    }

    void on_data_read(lux::net::base::tcp_inbound_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket;
        data_read_calls.emplace_back(std::vector<std::byte>{data.begin(), data.end()});
        if (on_data_read_callback)
        {
            on_data_read_callback(data);
        }
    }

    void on_data_sent(lux::net::base::tcp_inbound_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket;
        data_sent_calls.emplace_back(std::vector<std::byte>{data.begin(), data.end()});
        if (on_data_sent_callback)
        {
            on_data_sent_callback(data);
        }
    }

    std::vector<std::error_code> disconnected_calls;
    std::vector<std::vector<std::byte>> data_read_calls;
    std::vector<std::vector<std::byte>> data_sent_calls;

    std::function<void(const std::error_code&)> on_disconnected_callback;
    std::function<void(const std::span<const std::byte>&)> on_data_read_callback;
    std::function<void(const std::span<const std::byte>&)> on_data_sent_callback;
};

lux::net::base::tcp_inbound_socket_config create_default_config()
{
    lux::net::base::tcp_inbound_socket_config config{};
    return config;
}

} // namespace

LUX_TEST_CASE("tcp_inbound_socket", "constructs successfully from accepted socket", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    // Create a server and client to get an accepted socket
    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};

    bool accepted = false;
    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
        }
    });

    client_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                [](const boost::system::error_code&) {});

    io_context.run_for(std::chrono::milliseconds{100});
    REQUIRE(accepted);

    std::optional<lux::net::tcp_inbound_socket> inbound_socket;
    REQUIRE_NOTHROW(inbound_socket.emplace(lux::move(accepted_socket), config));
    inbound_socket->set_handler(handler);

    CHECK(inbound_socket->is_connected());
    CHECK(inbound_socket->local_endpoint().has_value());
    CHECK(inbound_socket->remote_endpoint().has_value());

    // Clean up
    client_socket.close();
    acceptor.close();
}

LUX_TEST_CASE("tcp_inbound_socket", "succeeds when disconnecting while connected", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};

    bool accepted = false;
    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
        }
    });

    client_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                [](const boost::system::error_code&) {});

    io_context.run_for(std::chrono::milliseconds{100});
    REQUIRE(accepted);

    lux::net::tcp_inbound_socket inbound_socket{lux::move(accepted_socket), config};
    inbound_socket.set_handler(handler);

    CHECK(inbound_socket.is_connected());

    const auto error1 = inbound_socket.disconnect(false);
    CHECK_FALSE(error1);
    CHECK_FALSE(inbound_socket.is_connected());

    // Disconnect again when already disconnected
    const auto error2 = inbound_socket.disconnect(true);
    CHECK_FALSE(error2);

    // Clean up
    client_socket.close();
    acceptor.close();
}

LUX_TEST_CASE("tcp_inbound_socket", "returns error when sending data while disconnected", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};

    bool accepted = false;
    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
        }
    });

    client_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                [&](const boost::system::error_code&) { io_context.stop(); });

    io_context.run_for(std::chrono::milliseconds{200});
    REQUIRE(accepted);

    lux::net::tcp_inbound_socket inbound_socket{lux::move(accepted_socket), config};
    inbound_socket.set_handler(handler);

    // Disconnect first
    inbound_socket.disconnect(false);

    const std::array<std::byte, 3> data{std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};

    const auto error = inbound_socket.send(std::span<const std::byte>{data});
    CHECK(error); // Should return error
    CHECK(error == std::make_error_code(std::errc::not_connected));
    CHECK(handler.data_sent_calls.empty());

    // Clean up
    client_socket.close();
    acceptor.close();
}

LUX_TEST_CASE("tcp_inbound_socket", "sends and receives data with client", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};
    std::array<char, 1024> client_buffer{};

    bool accepted = false;
    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
        }
    });

    client_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                [&](const boost::system::error_code&) { io_context.stop(); });

    io_context.run_for(std::chrono::milliseconds{200});
    REQUIRE(accepted);

    lux::net::tcp_inbound_socket inbound_socket{lux::move(accepted_socket), config};
    inbound_socket.set_handler(handler);

    bool data_sent = false;
    bool data_received = false;

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

    // Send data from inbound socket to client
    const std::array<std::byte, 5> test_data{std::byte{'h'},
                                             std::byte{'e'},
                                             std::byte{'l'},
                                             std::byte{'l'},
                                             std::byte{'o'}};

    const auto ec1 = inbound_socket.send(std::span<const std::byte>{test_data});
    CHECK_FALSE(ec1);

    const auto ec2 = inbound_socket.send(std::span<const std::byte>{});
    CHECK(ec2); // Sending empty data should return error

    // Start reading on inbound socket
    inbound_socket.read();

    // Client echoes back data
    client_socket.async_read_some(boost::asio::buffer(client_buffer),
                                  [&](const boost::system::error_code& read_ec, std::size_t bytes_read) {
                                      if (!read_ec && bytes_read > 0)
                                      {
                                          boost::asio::async_write(
                                              client_socket,
                                              boost::asio::buffer(client_buffer.data(), bytes_read),
                                              [](const boost::system::error_code&, std::size_t) {});
                                      }
                                  });

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(data_sent);
    CHECK(data_received);

    // Clean up
    inbound_socket.disconnect(false);
    client_socket.close();
    acceptor.close();
}

LUX_TEST_CASE("tcp_inbound_socket", "queues multiple send operations correctly", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};

    bool accepted = false;
    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
        }
    });

    client_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                [&](const boost::system::error_code&) { io_context.stop(); });

    io_context.run_for(std::chrono::milliseconds{200});
    REQUIRE(accepted);

    lux::net::tcp_inbound_socket inbound_socket{lux::move(accepted_socket), config};
    inbound_socket.set_handler(handler);

    int send_count = 0;
    handler.on_data_sent_callback = [&](const std::span<const std::byte>& sent_data) {
        CHECK(sent_data.size() == 3);
        send_count++;

        if (send_count == 3)
        {
            io_context.stop();
        }
    };

    // Send multiple messages
    const std::array<std::byte, 3> data1{std::byte{'1'}, std::byte{'2'}, std::byte{'3'}};
    const std::array<std::byte, 3> data2{std::byte{'4'}, std::byte{'5'}, std::byte{'6'}};
    const std::array<std::byte, 3> data3{std::byte{'7'}, std::byte{'8'}, std::byte{'9'}};

    inbound_socket.send(std::span<const std::byte>{data1});
    inbound_socket.send(std::span<const std::byte>{data2});
    inbound_socket.send(std::span<const std::byte>{data3});

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});
    CHECK(send_count == 3);

    // Clean up
    inbound_socket.disconnect(false);
    client_socket.close();
    acceptor.close();
}

LUX_TEST_CASE("tcp_inbound_socket", "sends pending data when disconnecting gracefully", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};

    bool accepted = false;
    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
        }
    });

    client_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                [&](const boost::system::error_code&) { io_context.stop(); });

    io_context.run_for(std::chrono::milliseconds{200});
    REQUIRE(accepted);

    lux::net::tcp_inbound_socket inbound_socket{lux::move(accepted_socket), config};
    inbound_socket.set_handler(handler);

    bool data_sent = false;
    bool disconnected = false;

    handler.on_data_sent_callback = [&](const std::span<const std::byte>&) { data_sent = true; };

    handler.on_disconnected_callback = [&](const std::error_code& ec) {
        CHECK_FALSE(ec); // Graceful disconnect should not have error
        disconnected = true;
        io_context.stop();
    };

    const std::array<std::byte, 4> data{std::byte{'t'}, std::byte{'e'}, std::byte{'s'}, std::byte{'t'}};
    inbound_socket.send(std::span<const std::byte>{data});
    inbound_socket.disconnect(true); // Graceful disconnect - should send pending data

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(data_sent);
    CHECK(disconnected);

    // Clean up
    client_socket.close();
    acceptor.close();
}

LUX_TEST_CASE("tcp_inbound_socket", "discards pending data when disconnecting immediately", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};

    bool accepted = false;
    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
        }
    });

    client_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                [&](const boost::system::error_code&) { io_context.stop(); });

    io_context.run_for(std::chrono::milliseconds{200});
    REQUIRE(accepted);

    lux::net::tcp_inbound_socket inbound_socket{lux::move(accepted_socket), config};
    inbound_socket.set_handler(handler);

    bool data_sent = false;
    bool disconnected = false;

    handler.on_disconnected_callback = [&](const std::error_code& ec) {
        CHECK_FALSE(ec); // Manual disconnect should not have error
        disconnected = true;
        io_context.stop();
    };

    handler.on_data_sent_callback = [&](const std::span<const std::byte>&) { data_sent = true; };

    const std::array<std::byte, 4> data{std::byte{'t'}, std::byte{'e'}, std::byte{'s'}, std::byte{'t'}};
    inbound_socket.send(std::span<const std::byte>{data});
    inbound_socket.disconnect(false); // Immediate disconnect - may not send pending data

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(disconnected);
    CHECK_FALSE(data_sent); // Data should not have been sent due to immediate disconnect

    // Clean up
    client_socket.close();
    acceptor.close();
}

LUX_TEST_CASE("tcp_inbound_socket", "invokes callback when remote peer disconnects", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};

    bool accepted = false;
    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
        }
    });

    client_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                [&](const boost::system::error_code&) { io_context.stop(); });

    io_context.run_for(std::chrono::milliseconds{200});
    REQUIRE(accepted);

    lux::net::tcp_inbound_socket inbound_socket{lux::move(accepted_socket), config};
    inbound_socket.set_handler(handler);

    bool disconnected = false;
    handler.on_disconnected_callback = [&](const std::error_code&) {
        disconnected = true;
        io_context.stop();
    };

    // Start reading to detect disconnect
    inbound_socket.read();

    // Close client socket to trigger disconnect on server side
    client_socket.close();

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(disconnected);
    CHECK_FALSE(inbound_socket.is_connected());

    // Clean up
    acceptor.close();
}

LUX_TEST_CASE("tcp_inbound_socket", "completes full lifecycle of send, receive, disconnect", "[io][net][tcp]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};
    std::array<char, 1024> client_buffer{};

    bool accepted = false;
    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
        }
    });

    client_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                [&](const boost::system::error_code&) { io_context.stop(); });

    io_context.run_for(std::chrono::milliseconds{200});
    REQUIRE(accepted);

    lux::net::tcp_inbound_socket inbound_socket{lux::move(accepted_socket), config};
    inbound_socket.set_handler(handler);
    inbound_socket.read();

    CHECK(inbound_socket.is_connected());
    CHECK(inbound_socket.local_endpoint().has_value());
    CHECK(inbound_socket.remote_endpoint().has_value());

    bool data_sent = false;
    bool data_received = false;
    bool disconnected = false;

    handler.on_data_sent_callback = [&](const std::span<const std::byte>& sent_data) {
        CHECK(sent_data.size() == 9);
        data_sent = true;
    };

    handler.on_data_read_callback = [&](const std::span<const std::byte>& received_data) {
        CHECK(received_data.size() == 9);
        data_received = true;

        // Disconnect after receiving echo
        inbound_socket.disconnect(true);
    };

    handler.on_disconnected_callback = [&](const std::error_code& ec) {
        CHECK_FALSE(ec);
        disconnected = true;
        CHECK_FALSE(inbound_socket.is_connected());
        io_context.stop();
    };

    const std::array<std::byte, 9> test_data{std::byte{'l'},
                                             std::byte{'i'},
                                             std::byte{'f'},
                                             std::byte{'e'},
                                             std::byte{'c'},
                                             std::byte{'y'},
                                             std::byte{'c'},
                                             std::byte{'l'},
                                             std::byte{'e'}};
    inbound_socket.send(std::span<const std::byte>{test_data});

    bool client_data_received = false;
    bool client_data_sent = false;

    // Client echoes back data
    client_socket.async_read_some(boost::asio::buffer(client_buffer),
                                  [&](const boost::system::error_code& read_ec, std::size_t bytes_read) {
                                      if (!read_ec && bytes_read > 0)
                                      {
                                          client_data_received = true;
                                          boost::asio::async_write(
                                              client_socket,
                                              boost::asio::buffer(client_buffer.data(), bytes_read),
                                              [&](const boost::system::error_code& ec, std::size_t) {
                                                  CHECK_FALSE(ec);
                                                  if (!ec)
                                                  {
                                                      client_data_sent = true;
                                                  }
                                              });
                                      }
                                  });

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(client_data_received);
    CHECK(client_data_sent);

    CHECK(data_received);
    CHECK(data_sent);
    CHECK(disconnected);

    // Clean up
    client_socket.close();
    acceptor.close();
}

LUX_TEST_CASE("ssl_tcp_inbound_socket", "constructs successfully from SSL stream", "[io][net][tcp][ssl]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    auto server_ssl_ctx = lux::test::net::create_ssl_server_context();
    auto client_ssl_ctx = lux::test::net::create_ssl_client_context();

    // Create a server and client to get an accepted socket
    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_tcp_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};

    std::optional<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> client_ssl_stream;
    std::optional<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> server_ssl_stream;

    bool accepted = false;
    bool client_handshake_complete = false;
    bool server_handshake_complete = false;

    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
            server_ssl_stream.emplace(lux::move(accepted_socket), server_ssl_ctx);
            server_ssl_stream->async_handshake(boost::asio::ssl::stream_base::server,
                                               [&](const boost::system::error_code& handshake_ec) {
                                                   if (!handshake_ec)
                                                   {
                                                       server_handshake_complete = true;
                                                   }
                                               });
        }
    });

    client_tcp_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                    [&](const boost::system::error_code& ec) {
                                        if (!ec)
                                        {
                                            client_ssl_stream.emplace(lux::move(client_tcp_socket), client_ssl_ctx);
                                            client_ssl_stream->async_handshake(
                                                boost::asio::ssl::stream_base::client,
                                                [&](const boost::system::error_code& handshake_ec) {
                                                    if (!handshake_ec)
                                                    {
                                                        client_handshake_complete = true;
                                                    }
                                                    io_context.stop();
                                                });
                                        }
                                    });

    io_context.run_for(std::chrono::milliseconds{100});
    REQUIRE(accepted);
    REQUIRE(client_handshake_complete);
    REQUIRE(server_handshake_complete);

    std::optional<lux::net::ssl_tcp_inbound_socket> inbound_socket;
    REQUIRE_NOTHROW(inbound_socket.emplace(lux::move(*server_ssl_stream), config));
    inbound_socket->set_handler(handler);

    CHECK(inbound_socket->is_connected());
    CHECK(inbound_socket->local_endpoint().has_value());
    CHECK(inbound_socket->remote_endpoint().has_value());

    // Clean up
    if (client_ssl_stream)
    {
        boost::system::error_code ignored_ec;
        client_ssl_stream->lowest_layer().close(ignored_ec);
    }
    acceptor.close();
}

LUX_TEST_CASE("ssl_tcp_inbound_socket", "returns error when sending while disconnected", "[io][net][tcp][ssl]")
{
    boost::asio::io_context io_context;
    test_tcp_inbound_socket_handler handler;
    const auto config = create_default_config();

    auto server_ssl_ctx = lux::test::net::create_ssl_server_context();
    auto client_ssl_ctx = lux::test::net::create_ssl_client_context();

    boost::asio::ip::tcp::acceptor acceptor{io_context, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const auto server_port = acceptor.local_endpoint().port();

    boost::asio::ip::tcp::socket client_tcp_socket{io_context};
    boost::asio::ip::tcp::socket accepted_socket{io_context};

    std::optional<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> client_ssl_stream;
    std::optional<boost::asio::ssl::stream<boost::asio::ip::tcp::socket>> server_ssl_stream;

    bool accepted = false;
    bool client_handshake_complete = false;
    bool server_handshake_complete = false;

    acceptor.async_accept(accepted_socket, [&](const boost::system::error_code& ec) {
        if (!ec)
        {
            accepted = true;
            server_ssl_stream.emplace(lux::move(accepted_socket), server_ssl_ctx);
            server_ssl_stream->async_handshake(boost::asio::ssl::stream_base::server,
                                               [&](const boost::system::error_code& handshake_ec) {
                                                   if (!handshake_ec)
                                                   {
                                                       server_handshake_complete = true;
                                                   }
                                               });
        }
    });

    client_tcp_socket.async_connect(lux::test::net::make_localhost_endpoint(server_port),
                                    [&](const boost::system::error_code& ec) {
                                        if (!ec)
                                        {
                                            client_ssl_stream.emplace(lux::move(client_tcp_socket), client_ssl_ctx);
                                            client_ssl_stream->async_handshake(
                                                boost::asio::ssl::stream_base::client,
                                                [&](const boost::system::error_code& handshake_ec) {
                                                    if (!handshake_ec)
                                                    {
                                                        client_handshake_complete = true;
                                                    }
                                                    io_context.stop();
                                                });
                                        }
                                    });

    io_context.run_for(std::chrono::milliseconds{100});
    REQUIRE(accepted);
    REQUIRE(client_handshake_complete);
    REQUIRE(server_handshake_complete);

    lux::net::ssl_tcp_inbound_socket inbound_socket{lux::move(*server_ssl_stream), config};
    inbound_socket.set_handler(handler);

    // Disconnect first
    const auto disconnect_error = inbound_socket.disconnect(false);
    CHECK_FALSE(disconnect_error);

    // Second disconnect does nothing
    const auto second_disconnect_error = inbound_socket.disconnect(true);
    CHECK_FALSE(second_disconnect_error);

    const std::array<std::byte, 3> data{std::byte{'a'}, std::byte{'b'}, std::byte{'c'}};

    const auto send_error = inbound_socket.send(std::span<const std::byte>{data});
    CHECK(send_error);
    CHECK(send_error == std::make_error_code(std::errc::not_connected));

    // Clean up
    if (client_ssl_stream)
    {
        boost::system::error_code ignored_ec;
        client_ssl_stream->lowest_layer().close(ignored_ec);
    }
    acceptor.close();
}

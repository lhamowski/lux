#include "test_utils.hpp"

#include <lux/io/net/http_server.hpp>
#include <lux/io/net/socket_factory.hpp>
#include <lux/io/net/tcp_socket.hpp>
#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/address_v4.hpp>
#include <lux/io/net/base/http_method.hpp>
#include <lux/io/net/base/http_status.hpp>
#include <lux/io/time/timer_factory.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/io_context.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace {

class test_http_server_handler : public lux::net::base::http_server_handler
{
public:
    void on_server_started() override
    {
        started_calls++;
        if (on_server_started_callback)
        {
            on_server_started_callback();
        }
    }

    void on_server_stopped() override
    {
        stopped_calls++;
        if (on_server_stopped_callback)
        {
            on_server_stopped_callback();
        }
    }

    void on_server_error(const std::error_code& ec) override
    {
        error_calls++;
        last_error = ec;
        if (on_server_error_callback)
        {
            on_server_error_callback(ec);
        }
    }

    lux::net::base::http_response handle_request(const lux::net::base::http_request& request) override
    {
        request_calls++;
        last_request = request;

        if (handle_request_callback)
        {
            return handle_request_callback(request);
        }

        return lux::net::base::http_response{lux::net::base::http_status::ok};
    }

    std::size_t started_calls{0};
    std::size_t stopped_calls{0};
    std::size_t error_calls{0};
    std::size_t request_calls{0};
    std::error_code last_error;
    lux::net::base::http_request last_request;

    std::function<void()> on_server_started_callback;
    std::function<void()> on_server_stopped_callback;
    std::function<void(const std::error_code&)> on_server_error_callback;
    std::function<lux::net::base::http_response(const lux::net::base::http_request&)> handle_request_callback;
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
        received_data.insert(received_data.end(), data.begin(), data.end());
        if (on_data_read_callback)
        {
            on_data_read_callback(data);
        }
    }

    void on_data_sent(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) override
    {
        std::ignore = socket;
        std::ignore = data;
    }

    std::atomic<std::size_t> connected_calls{0};
    std::atomic<std::size_t> disconnected_calls{0};
    std::vector<std::byte> received_data;

    std::function<void()> on_connected_callback;
    std::function<void()> on_disconnected_callback;
    std::function<void(const std::span<const std::byte>&)> on_data_read_callback;
};

lux::net::base::http_server_config create_default_http_server_config()
{
    lux::net::base::http_server_config config;
    config.acceptor_config.reuse_address = true;
    config.acceptor_config.keep_alive = false;
    return config;
}

lux::net::base::tcp_socket_config create_default_tcp_socket_config()
{
    return lux::net::base::tcp_socket_config{};
}

std::string create_http_request(const std::string& method,
                                const std::string& target,
                                const std::string& body = "",
                                const std::vector<std::pair<std::string, std::string>>& headers = {})
{
    std::string request = method + " " + target + " HTTP/1.1\r\n";

    for (const auto& [key, value] : headers)
    {
        request += key + ": " + value + "\r\n";
    }

    if (!body.empty())
    {
        request += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    }

    request += "\r\n";

    if (!body.empty())
    {
        request += body;
    }

    return request;
}

std::vector<std::byte> to_bytes(const std::string& str)
{
    std::vector<std::byte> result;
    result.reserve(str.size());
    for (char c : str)
    {
        result.push_back(static_cast<std::byte>(c));
    }
    return result;
}

std::string from_bytes(const std::vector<std::byte>& bytes)
{
    std::string result;
    result.reserve(bytes.size());
    for (std::byte b : bytes)
    {
        result.push_back(static_cast<char>(b));
    }
    return result;
}

} // namespace

TEST_CASE("http_server: constructs successfully with default configuration", "[io][net][http][server]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;
    const auto config = create_default_http_server_config();

    std::unique_ptr<lux::net::http_server> server;
    REQUIRE_NOTHROW(server = std::make_unique<lux::net::http_server>(config, handler, socket_factory));
}

TEST_CASE("http_server: constructs successfully with SSL context", "[io][net][http][server][ssl]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;
    const auto config = create_default_http_server_config();
    auto ssl_context = lux::test::net::create_ssl_server_context();

    std::unique_ptr<lux::net::http_server> server;
    REQUIRE_NOTHROW(server = std::make_unique<lux::net::http_server>(config, handler, socket_factory, ssl_context));
}

TEST_CASE("http_server: starts serving on specified endpoint successfully", "[io][net][http][server]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;
    const auto config = create_default_http_server_config();
    lux::net::http_server server{config, handler, socket_factory};

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, 0};
    const auto serve_error = server.serve(endpoint);
    CHECK_FALSE(serve_error);

    server.stop();
}

TEST_CASE("http_server: stops serving successfully", "[io][net][http][server]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;
    const auto config = create_default_http_server_config();
    lux::net::http_server server{config, handler, socket_factory};

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, 0};
    const auto serve_error = server.serve(endpoint);
    REQUIRE_FALSE(serve_error);

    const auto stop_error = server.stop();
    CHECK_FALSE(stop_error);
}

TEST_CASE("http_server: handles GET request successfully", "[io][net][http][server]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;

    bool request_handled = false;
    handler.handle_request_callback = [&](const lux::net::base::http_request& request) {
        request_handled = true;
        CHECK(request.method() == lux::net::base::http_method::get);
        CHECK(request.target() == "/test");

        lux::net::base::http_response response;
        response.ok("Hello, World!");
        return response;
    };

    const auto config = create_default_http_server_config();
    lux::net::http_server server{config, handler, socket_factory};

    const auto serve_error = server.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);

    test_tcp_socket_handler client_handler;
    bool response_received = false;

    client_handler.on_connected_callback = [&] { io_context.stop(); };
    client_handler.on_data_read_callback = [&](const std::span<const std::byte>& data) {
        std::ignore = data;
        response_received = true;
    };

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    const auto socket_config = create_default_tcp_socket_config();
    lux::net::tcp_socket client_socket{io_context.get_executor(), client_handler, socket_config, timer_factory};

    REQUIRE(server.local_endpoint().has_value());
    const auto endpoint = server.local_endpoint().value();
    const auto connect_error = client_socket.connect(endpoint);
    CHECK_FALSE(connect_error);

    io_context.run_for(std::chrono::milliseconds{100});

    const auto http_request = create_http_request("GET", "/test");
    const auto request_bytes = to_bytes(http_request);
    const auto send_error = client_socket.send(std::span{request_bytes});
    CHECK_FALSE(send_error);

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(request_handled);
    CHECK(response_received);
    CHECK(handler.request_calls > 0);

    const auto response_str = from_bytes(client_handler.received_data);
    CHECK(response_str.find("200 OK") != std::string::npos);
    CHECK(response_str.find("Hello, World!") != std::string::npos);

    server.stop();
}

TEST_CASE("http_server: handles POST request with body successfully", "[io][net][http][server]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;

    const std::string expected_body = R"({"key":"value"})";
    bool request_handled = false;

    handler.handle_request_callback = [&](const lux::net::base::http_request& request) {
        request_handled = true;
        CHECK(request.method() == lux::net::base::http_method::post);
        CHECK(request.target() == "/api/data");
        CHECK(request.body() == expected_body);

        lux::net::base::http_response response;
        response.created("Created");
        return response;
    };

    const auto config = create_default_http_server_config();
    lux::net::http_server server{config, handler, socket_factory};

    const auto serve_error = server.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);

    test_tcp_socket_handler client_handler;
    bool client_connected = false;
    bool response_received = false;

    client_handler.on_connected_callback = [&] {
        client_connected = true;
        io_context.stop();
    };

    client_handler.on_data_read_callback = [&](const std::span<const std::byte>& data) {
        std::ignore = data;
        response_received = true;
    };

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    const auto socket_config = create_default_tcp_socket_config();
    lux::net::tcp_socket client_socket{io_context.get_executor(), client_handler, socket_config, timer_factory};

    REQUIRE(server.local_endpoint().has_value());
    const auto endpoint = server.local_endpoint().value();
    const auto connect_error = client_socket.connect(endpoint);
    CHECK_FALSE(connect_error);

    io_context.run_for(std::chrono::milliseconds{100});
    CHECK(client_connected);

    const auto http_request = create_http_request("POST",
                                                  "/api/data",
                                                  expected_body,
                                                  {{"Content-Type", "application/json"}});
    const auto request_bytes = to_bytes(http_request);
    const auto send_error = client_socket.send(std::span{request_bytes});
    CHECK_FALSE(send_error);

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(request_handled);
    CHECK(response_received);

    const auto response_str = from_bytes(client_handler.received_data);
    CHECK(response_str.find("201 Created") != std::string::npos);
    CHECK(response_str.find("Created") != std::string::npos);

    server.stop();
}

TEST_CASE("http_server: handles multiple requests from same client", "[io][net][http][server]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;

    handler.handle_request_callback = [&](const lux::net::base::http_request& request) {
        lux::net::base::http_response response;
        response.ok("Response for " + request.target());
        return response;
    };

    const auto config = create_default_http_server_config();
    lux::net::http_server server{config, handler, socket_factory};

    const auto serve_error = server.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);

    test_tcp_socket_handler client_handler;

    client_handler.on_connected_callback = [&] { io_context.stop(); };

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    const auto socket_config = create_default_tcp_socket_config();
    lux::net::tcp_socket client_socket{io_context.get_executor(), client_handler, socket_config, timer_factory};

    REQUIRE(server.local_endpoint().has_value());
    const auto endpoint = server.local_endpoint().value();
    const auto connect_error = client_socket.connect(endpoint);
    CHECK_FALSE(connect_error);

    io_context.run_for(std::chrono::milliseconds{100});

    const auto request1 = create_http_request("GET", "/first");
    const auto request1_bytes = to_bytes(request1);
    client_socket.send(std::span{request1_bytes});

    const auto request2 = create_http_request("GET", "/second");
    const auto request2_bytes = to_bytes(request2);
    client_socket.send(std::span{request2_bytes});

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(handler.request_calls >= 2);

    const auto response_str = from_bytes(client_handler.received_data);
    CHECK(response_str.find("Response for /first") != std::string::npos);
    CHECK(response_str.find("Response for /second") != std::string::npos);

    server.stop();
}

TEST_CASE("http_server: handles multiple concurrent client connections", "[io][net][http][server]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;

    std::size_t requests_handled{0};
    handler.handle_request_callback = [&](const lux::net::base::http_request& request) {
        std::ignore = request;
        requests_handled++;
        lux::net::base::http_response response;
        response.ok("Response");
        return response;
    };

    const auto config = create_default_http_server_config();
    lux::net::http_server server{config, handler, socket_factory};

    const auto serve_error = server.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    const auto socket_config = create_default_tcp_socket_config();

    constexpr int num_clients = 3;
    std::vector<std::unique_ptr<test_tcp_socket_handler>> client_handlers;
    std::vector<std::unique_ptr<lux::net::tcp_socket>> client_sockets;

    std::size_t connections_established{0};

    for (int i = 0; i < num_clients; ++i)
    {
        auto client_handler = std::make_unique<test_tcp_socket_handler>();

        client_handler->on_connected_callback = [&] {
            connections_established++;
            if (connections_established >= num_clients)
            {
                io_context.stop();
            }
        };

        client_handlers.push_back(lux::move(client_handler));
        auto socket = std::make_unique<lux::net::tcp_socket>(io_context.get_executor(),
                                                             *client_handlers.back(),
                                                             socket_config,
                                                             timer_factory);
        client_sockets.push_back(lux::move(socket));
    }

    REQUIRE(server.local_endpoint().has_value());
    const auto endpoint = server.local_endpoint().value();
    for (auto& socket : client_sockets)
    {
        socket->connect(endpoint);
    }

    io_context.run_for(std::chrono::milliseconds{200});

    CHECK(connections_established == num_clients);

    for (auto& socket : client_sockets)
    {
        const auto request = create_http_request("GET", "/test");
        const auto request_bytes = to_bytes(request);
        socket->send(std::span{request_bytes});
    }

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(requests_handled >= num_clients);

    for (const auto& client_handler : client_handlers)
    {
        const auto response_str = from_bytes(client_handler->received_data);
        CHECK(response_str.find("Response") != std::string::npos);
    }

    server.stop();
}

TEST_CASE("http_server: handles requests with custom headers", "[io][net][http][server]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;

    bool headers_verified = false;
    handler.handle_request_callback = [&](const lux::net::base::http_request& request) {
        CHECK(request.has_header("User-Agent"));
        CHECK(request.header("User-Agent") == "TestClient/1.0");
        CHECK(request.has_header("Accept"));
        CHECK(request.header("Accept") == "application/json");
        headers_verified = true;

        lux::net::base::http_response response;
        response.ok();
        response.set_header("X-Custom-Header", "CustomValue");
        return response;
    };

    const auto config = create_default_http_server_config();
    lux::net::http_server server{config, handler, socket_factory};

    const auto serve_error = server.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);

    test_tcp_socket_handler client_handler;
    bool response_received = false;

    client_handler.on_connected_callback = [&] { io_context.stop(); };
    client_handler.on_data_read_callback = [&](const std::span<const std::byte>& data) {
        std::ignore = data;
        response_received = true;
    };

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    const auto socket_config = create_default_tcp_socket_config();
    lux::net::tcp_socket client_socket{io_context.get_executor(), client_handler, socket_config, timer_factory};

    REQUIRE(server.local_endpoint().has_value());
    const auto endpoint = server.local_endpoint().value();
    const auto connect_error = client_socket.connect(endpoint);
    CHECK_FALSE(connect_error);

    io_context.run_for(std::chrono::milliseconds{100});

    const auto http_request = create_http_request("GET",
                                                  "/test",
                                                  "",
                                                  {{"User-Agent", "TestClient/1.0"}, {"Accept", "application/json"}});
    const auto request_bytes = to_bytes(http_request);
    const auto send_error = client_socket.send(std::span{request_bytes});
    CHECK_FALSE(send_error);

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(headers_verified);
    CHECK(response_received);

    const auto response_str = from_bytes(client_handler.received_data);
    CHECK(response_str.find("X-Custom-Header: CustomValue") != std::string::npos);

    server.stop();
}

TEST_CASE("http_server: handles different HTTP methods", "[io][net][http][server]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;

    std::vector<lux::net::base::http_method> received_methods;
    handler.handle_request_callback = [&](const lux::net::base::http_request& request) {
        received_methods.push_back(request.method());
        lux::net::base::http_response response;
        response.ok();
        return response;
    };

    const auto config = create_default_http_server_config();
    lux::net::http_server server{config, handler, socket_factory};

    const auto serve_error = server.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);

    test_tcp_socket_handler client_handler;
    std::atomic<int> responses_received{0};

    client_handler.on_connected_callback = [&] { io_context.stop(); };
    client_handler.on_data_read_callback = [&](const std::span<const std::byte>& data) {
        std::ignore = data;
        responses_received++;
        if (responses_received >= 4)
        {
            io_context.stop();
        }
    };

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    const auto socket_config = create_default_tcp_socket_config();
    lux::net::tcp_socket client_socket{io_context.get_executor(), client_handler, socket_config, timer_factory};

    REQUIRE(server.local_endpoint().has_value());
    const auto endpoint = server.local_endpoint().value();
    const auto connect_error = client_socket.connect(endpoint);
    CHECK_FALSE(connect_error);

    io_context.run_for(std::chrono::milliseconds{100});

    const std::vector<std::string> methods = {"GET", "POST", "PUT", "DELETE"};
    for (const auto& method : methods)
    {
        const auto request = create_http_request(method, "/test");
        const auto request_bytes = to_bytes(request);
        client_socket.send(std::span{request_bytes});
    }

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(received_methods.size() >= 4);
    CHECK(std::ranges::contains(received_methods, lux::net::base::http_method::get));
    CHECK(std::ranges::contains(received_methods, lux::net::base::http_method::post));
    CHECK(std::ranges::contains(received_methods, lux::net::base::http_method::put));
    CHECK(std::ranges::contains(received_methods, lux::net::base::http_method::delete_));

    server.stop();
}

TEST_CASE("ssl_http_server: handles HTTPS request successfully", "[io][net][http][server][ssl]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    test_http_server_handler handler;
    auto server_ssl_context = lux::test::net::create_ssl_server_context();

    bool request_handled = false;
    handler.handle_request_callback = [&](const lux::net::base::http_request& request) {
        request_handled = true;
        CHECK(request.method() == lux::net::base::http_method::get);
        CHECK(request.target() == "/secure");

        lux::net::base::http_response response;
        response.ok("Secure Response");
        return response;
    };

    const auto config = create_default_http_server_config();
    lux::net::http_server server{config, handler, socket_factory, server_ssl_context};

    const auto serve_error = server.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);

    test_tcp_socket_handler client_handler;
    bool response_received = false;

    client_handler.on_connected_callback = [&] { io_context.stop(); };
    client_handler.on_data_read_callback = [&](const std::span<const std::byte>& data) {
        std::ignore = data;
        response_received = true;
    };

    lux::time::timer_factory timer_factory{io_context.get_executor()};
    const auto socket_config = create_default_tcp_socket_config();
    auto client_ssl_context = lux::test::net::create_ssl_client_context();
    lux::net::ssl_tcp_socket client_socket{io_context.get_executor(),
                                           client_handler,
                                           socket_config,
                                           timer_factory,
                                           client_ssl_context};

    REQUIRE(server.local_endpoint().has_value());
    const auto endpoint = server.local_endpoint().value();
    const auto connect_error = client_socket.connect(endpoint);
    CHECK_FALSE(connect_error);

    io_context.run_for(std::chrono::milliseconds{500});

    const auto http_request = create_http_request("GET", "/secure");
    const auto request_bytes = to_bytes(http_request);
    const auto send_error = client_socket.send(std::span{request_bytes});
    CHECK_FALSE(send_error);

    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});

    CHECK(request_handled);
    CHECK(response_received);

    const auto response_str = from_bytes(client_handler.received_data);
    CHECK(response_str.find("200 OK") != std::string::npos);
    CHECK(response_str.find("Secure Response") != std::string::npos);

    server.stop();
}

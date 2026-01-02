#include "test_case.hpp"
#include "io/net/test_utils.hpp"

#include <lux/io/net/http_client.hpp>
#include <lux/io/net/http_server_app.hpp>
#include <lux/io/net/http_factory.hpp>
#include <lux/io/net/socket_factory.hpp>
#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/address_v4.hpp>
#include <lux/io/net/base/http_method.hpp>
#include <lux/io/net/base/http_status.hpp>

#include <catch2/catch_all.hpp>

#include <boost/asio/io_context.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace {

lux::net::http_server_app_config create_default_http_server_app_config()
{
    lux::net::http_server_app_config config;
    config.server_config.acceptor_config.reuse_address = true;
    config.server_config.acceptor_config.keep_alive = true;
    return config;
}

lux::net::base::http_client_config create_default_http_client_config()
{
    return lux::net::base::http_client_config{};
}

} // namespace

LUX_TEST_CASE("http_client", "constructs successfully with hostname endpoint", "[io][net][http][client]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    const lux::net::base::hostname_endpoint destination{"localhost", 8080};
    const auto config = create_default_http_client_config();

    std::unique_ptr<lux::net::http_client> client;
    REQUIRE_NOTHROW(client = std::make_unique<lux::net::http_client>(destination, config, socket_factory));
}

LUX_TEST_CASE("http_client", "constructs successfully with SSL context", "[io][net][http][client][ssl]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    const lux::net::base::hostname_endpoint destination{"localhost", 8443};
    const auto config = create_default_http_client_config();
    auto ssl_context = lux::test::net::create_ssl_client_context();

    std::unique_ptr<lux::net::http_client> client;
    REQUIRE_NOTHROW(client = std::make_unique<lux::net::http_client>(destination, config, socket_factory, ssl_context));
}

LUX_TEST_CASE("http_client", "sends GET request successfully", "[io][net][http][client]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    lux::net::http_factory http_factory{socket_factory};

    const auto app_config = create_default_http_server_app_config();
    lux::net::http_server_app app{app_config, http_factory};

    app.get("/test", [](const auto& req, auto& res) {
        CHECK(req.method() == lux::net::base::http_method::get);
        CHECK(req.target() == "/test");
        res.ok("Hello, World!");
    });

    const auto serve_error = app.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);
    REQUIRE(app.local_endpoint().has_value());

    const auto port = app.local_endpoint()->port();
    const lux::net::base::hostname_endpoint destination{"localhost", port};

    const auto client_config = create_default_http_client_config();
    lux::net::http_client client{destination, client_config, socket_factory};

    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::get);
    request.set_target("/test");

    bool response_received = false;
    lux::net::base::http_response received_response;

    client.request(request, [&](const lux::net::base::http_request_result& result) {
        response_received = true;
        REQUIRE(result.has_value());
        received_response = result.value();
        io_context.stop();
    });

    io_context.run_for(std::chrono::seconds{5});

    CHECK(response_received);
    CHECK(received_response.status() == lux::net::base::http_status::ok);
    CHECK(received_response.body() == "Hello, World!");

    app.stop();
    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});
}

LUX_TEST_CASE("http_client", "sends POST request with body successfully", "[io][net][http][client]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    lux::net::http_factory http_factory{socket_factory};

    const auto app_config = create_default_http_server_app_config();
    lux::net::http_server_app app{app_config, http_factory};

    const std::string expected_body = R"({"key":"value"})";
    app.post("/api/data", [&](const auto& req, auto& res) {
        CHECK(req.method() == lux::net::base::http_method::post);
        CHECK(req.target() == "/api/data");
        CHECK(req.body() == expected_body);
        res.created("Data created");
    });

    const auto serve_error = app.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);
    REQUIRE(app.local_endpoint().has_value());

    const auto port = app.local_endpoint()->port();
    const lux::net::base::hostname_endpoint destination{"localhost", port};

    const auto client_config = create_default_http_client_config();
    lux::net::http_client client{destination, client_config, socket_factory};

    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::post);
    request.set_target("/api/data");
    request.set_body(expected_body);
    request.set_header("Content-Type", "application/json");

    bool response_received = false;
    lux::net::base::http_response received_response;

    client.request(request, [&](const lux::net::base::http_request_result& result) {
        response_received = true;
        REQUIRE(result.has_value());
        received_response = result.value();
        io_context.stop();
    });

    io_context.run_for(std::chrono::seconds{5});

    CHECK(response_received);
    CHECK(received_response.status() == lux::net::base::http_status::created);
    CHECK(received_response.body() == "Data created");

    app.stop();
    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});
}

LUX_TEST_CASE("http_client", "sends PUT request successfully", "[io][net][http][client]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    lux::net::http_factory http_factory{socket_factory};

    const auto app_config = create_default_http_server_app_config();
    lux::net::http_server_app app{app_config, http_factory};

    app.put("/resource/123", [](const auto& req, auto& res) {
        CHECK(req.method() == lux::net::base::http_method::put);
        CHECK(req.target() == "/resource/123");
        res.ok("Resource updated");
    });

    const auto serve_error = app.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);
    REQUIRE(app.local_endpoint().has_value());

    const auto port = app.local_endpoint()->port();
    const lux::net::base::hostname_endpoint destination{"localhost", port};

    const auto client_config = create_default_http_client_config();
    lux::net::http_client client{destination, client_config, socket_factory};

    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::put);
    request.set_target("/resource/123");
    request.set_body("updated data");

    bool response_received = false;
    lux::net::base::http_response received_response;

    client.request(request, [&](const lux::net::base::http_request_result& result) {
        response_received = true;
        REQUIRE(result.has_value());
        received_response = result.value();
        io_context.stop();
    });

    io_context.run_for(std::chrono::seconds{5});

    CHECK(response_received);
    CHECK(received_response.status() == lux::net::base::http_status::ok);
    CHECK(received_response.body() == "Resource updated");

    app.stop();
    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});
}

LUX_TEST_CASE("http_client", "sends DELETE request successfully", "[io][net][http][client]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    lux::net::http_factory http_factory{socket_factory};

    const auto app_config = create_default_http_server_app_config();
    lux::net::http_server_app app{app_config, http_factory};

    app.del("/resource/456", [](const auto& req, auto& res) {
        CHECK(req.method() == lux::net::base::http_method::delete_);
        CHECK(req.target() == "/resource/456");
        res.ok("Resource deleted");
    });

    const auto serve_error = app.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);
    REQUIRE(app.local_endpoint().has_value());

    const auto port = app.local_endpoint()->port();
    const lux::net::base::hostname_endpoint destination{"localhost", port};

    const auto client_config = create_default_http_client_config();
    lux::net::http_client client{destination, client_config, socket_factory};

    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::delete_);
    request.set_target("/resource/456");

    bool response_received = false;
    lux::net::base::http_response received_response;

    client.request(request, [&](const lux::net::base::http_request_result& result) {
        response_received = true;
        REQUIRE(result.has_value());
        received_response = result.value();
        io_context.stop();
    });

    io_context.run_for(std::chrono::seconds{5});

    CHECK(response_received);
    CHECK(received_response.status() == lux::net::base::http_status::ok);
    CHECK(received_response.body() == "Resource deleted");

    app.stop();
    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});
}

LUX_TEST_CASE("http_client", "handles request with custom headers", "[io][net][http][client]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    lux::net::http_factory http_factory{socket_factory};

    const auto app_config = create_default_http_server_app_config();
    lux::net::http_server_app app{app_config, http_factory};

    bool headers_verified = false;
    app.get("/test", [&](const auto& req, auto& res) {
        CHECK(req.has_header("User-Agent"));
        CHECK(req.header("User-Agent") == "TestClient/1.0");
        CHECK(req.has_header("X-Custom-Header"));
        CHECK(req.header("X-Custom-Header") == "CustomValue");
        headers_verified = true;

        res.ok("Headers received");
        res.set_header("X-Response-Header", "ResponseValue");
    });

    const auto serve_error = app.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);
    REQUIRE(app.local_endpoint().has_value());

    const auto port = app.local_endpoint()->port();
    const lux::net::base::hostname_endpoint destination{"localhost", port};

    const auto client_config = create_default_http_client_config();
    lux::net::http_client client{destination, client_config, socket_factory};

    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::get);
    request.set_target("/test");
    request.set_header("User-Agent", "TestClient/1.0");
    request.set_header("X-Custom-Header", "CustomValue");

    bool response_received = false;
    lux::net::base::http_response received_response;

    client.request(request, [&](const lux::net::base::http_request_result& result) {
        response_received = true;
        REQUIRE(result.has_value());
        received_response = result.value();
        io_context.stop();
    });

    io_context.run_for(std::chrono::seconds{5});

    CHECK(response_received);
    CHECK(headers_verified);
    CHECK(received_response.status() == lux::net::base::http_status::ok);
    CHECK(received_response.has_header("X-Response-Header"));
    CHECK(received_response.header("X-Response-Header") == "ResponseValue");

    app.stop();
    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});
}

LUX_TEST_CASE("http_client", "queues multiple requests on same connection", "[io][net][http][client]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    lux::net::http_factory http_factory{socket_factory};

    const auto app_config = create_default_http_server_app_config();
    lux::net::http_server_app app{app_config, http_factory};

    std::atomic<int> requests_handled{0};

    app.get("/request0", [&](const auto& req, auto& res) {
        requests_handled++;
        res.ok("Response for " + req.target());
    });

    app.get("/request1", [&](const auto& req, auto& res) {
        requests_handled++;
        res.ok("Response for " + req.target());
    });

    app.get("/request2", [&](const auto& req, auto& res) {
        requests_handled++;
        res.ok("Response for " + req.target());
    });

    const auto serve_error = app.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);
    REQUIRE(app.local_endpoint().has_value());

    const auto port = app.local_endpoint()->port();
    const lux::net::base::hostname_endpoint destination{"localhost", port};

    const auto client_config = create_default_http_client_config();
    lux::net::http_client client{destination, client_config, socket_factory};

    constexpr int num_requests = 3;
    std::atomic<int> responses_received{0};
    std::vector<lux::net::base::http_response> received_responses;
    received_responses.resize(num_requests);

    for (int i = 0; i < num_requests; ++i)
    {
        lux::net::base::http_request request;
        request.set_method(lux::net::base::http_method::get);
        request.set_target("/request" + std::to_string(i));

        client.request(request, [&, i](const lux::net::base::http_request_result& result) {
            REQUIRE(result.has_value());
            received_responses[i] = result.value();
            responses_received++;
            if (responses_received >= num_requests)
            {
                io_context.stop();
            }
        });
    }

    io_context.run_for(std::chrono::seconds{5});

    CHECK(responses_received == num_requests);
    CHECK(requests_handled >= num_requests);

    for (int i = 0; i < num_requests; ++i)
    {
        CHECK(received_responses[i].status() == lux::net::base::http_status::ok);
        CHECK(received_responses[i].body().find("/request" + std::to_string(i)) != std::string::npos);
    }

    app.stop();
    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});
}

LUX_TEST_CASE("http_client", "receives different HTTP status codes", "[io][net][http][client]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    lux::net::http_factory http_factory{socket_factory};

    const auto app_config = create_default_http_server_app_config();
    lux::net::http_server_app app{app_config, http_factory};

    app.get("/ok", [](const auto&, auto& res) { res.ok(); });
    app.get("/created", [](const auto&, auto& res) { res.created(); });
    app.get("/not_found", [](const auto&, auto& res) { res.not_found(); });
    app.get("/bad_request", [](const auto&, auto& res) { res.bad_request(); });
    app.get("/server_error", [](const auto&, auto& res) { res.internal_server_error(); });

    const auto serve_error = app.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);
    REQUIRE(app.local_endpoint().has_value());

    const auto port = app.local_endpoint()->port();
    const lux::net::base::hostname_endpoint destination{"localhost", port};

    const auto client_config = create_default_http_client_config();
    lux::net::http_client client{destination, client_config, socket_factory};

    struct test_case_data
    {
        std::string target;
        lux::net::base::http_status expected_status;
    };

    const std::vector<test_case_data> test_cases = {
        {"/ok", lux::net::base::http_status::ok},
        {"/created", lux::net::base::http_status::created},
        {"/not_found", lux::net::base::http_status::not_found},
        {"/bad_request", lux::net::base::http_status::bad_request},
        {"/server_error", lux::net::base::http_status::internal_server_error}};

    std::atomic<int> responses_received{0};
    std::vector<lux::net::base::http_response> received_responses;
    received_responses.resize(test_cases.size());

    for (std::size_t i = 0; i < test_cases.size(); ++i)
    {
        lux::net::base::http_request request;
        request.set_method(lux::net::base::http_method::get);
        request.set_target(test_cases[i].target);

        client.request(request, [&, i](const lux::net::base::http_request_result& result) {
            REQUIRE(result.has_value());
            received_responses[i] = result.value();
            responses_received++;
            if (responses_received >= static_cast<int>(test_cases.size()))
            {
                io_context.stop();
            }
        });
    }

    io_context.run_for(std::chrono::seconds{5});

    CHECK(responses_received == static_cast<int>(test_cases.size()));

    for (std::size_t i = 0; i < test_cases.size(); ++i)
    {
        CHECK(received_responses[i].status() == test_cases[i].expected_status);
    }

    app.stop();
    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});
}

LUX_TEST_CASE("http_client", "handles connection error gracefully", "[io][net][http][client]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};

    const lux::net::base::hostname_endpoint destination{"localhost", 9999};
    const auto client_config = create_default_http_client_config();
    lux::net::http_client client{destination, client_config, socket_factory};

    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::get);
    request.set_target("/test");

    bool error_received = false;
    std::error_code received_error;

    client.request(request, [&](const lux::net::base::http_request_result& result) {
        error_received = true;
        REQUIRE_FALSE(result.has_value());
        received_error = result.error();
        io_context.stop();
    });

    io_context.run_for(std::chrono::seconds{5});

    CHECK(error_received);
    CHECK(received_error);
}

LUX_TEST_CASE("http_client", "sends HTTPS request successfully", "[io][net][http][client][ssl]")
{
    boost::asio::io_context io_context;
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    lux::net::http_factory http_factory{socket_factory};
    auto server_ssl_context = lux::test::net::create_ssl_server_context();

    const auto app_config = create_default_http_server_app_config();
    lux::net::http_server_app app{app_config, http_factory, server_ssl_context};

    app.get("/secure", [](const auto& req, auto& res) {
        CHECK(req.method() == lux::net::base::http_method::get);
        CHECK(req.target() == "/secure");
        res.ok("Secure Response");
    });

    const auto serve_error = app.serve(lux::net::base::endpoint{lux::net::base::localhost, 0});
    REQUIRE_FALSE(serve_error);
    REQUIRE(app.local_endpoint().has_value());

    const auto port = app.local_endpoint()->port();
    const lux::net::base::hostname_endpoint destination{"localhost", port};

    const auto client_config = create_default_http_client_config();
    auto client_ssl_context = lux::test::net::create_ssl_client_context();
    lux::net::http_client client{destination, client_config, socket_factory, client_ssl_context};

    lux::net::base::http_request request;
    request.set_method(lux::net::base::http_method::get);
    request.set_target("/secure");

    bool response_received = false;
    lux::net::base::http_response received_response;

    client.request(request, [&](const lux::net::base::http_request_result& result) {
        response_received = true;
        REQUIRE(result.has_value());
        received_response = result.value();
        io_context.stop();
    });

    io_context.run_for(std::chrono::seconds{5});

    CHECK(response_received);
    CHECK(received_response.status() == lux::net::base::http_status::ok);
    CHECK(received_response.body() == "Secure Response");

    app.stop();
    io_context.restart();
    io_context.run_for(std::chrono::milliseconds{100});
}

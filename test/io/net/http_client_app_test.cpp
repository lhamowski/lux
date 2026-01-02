#include "test_case.hpp"

#include <lux/io/net/http_client_app.hpp>

#include <lux/io/net/base/http_factory.hpp>
#include <lux/io/net/base/http_client.hpp>
#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/http_method.hpp>
#include <lux/io/net/base/http_status.hpp>
#include <lux/io/net/base/http_response.hpp>

#include <lux/support/move.hpp>

#include <catch2/catch_all.hpp>

#include <memory>
#include <string>
#include <optional>
#include <system_error>

namespace {

class mock_http_client : public lux::net::base::http_client
{
public:
    void request(const lux::net::base::http_request& request, lux::net::base::http_client_handler_type handler) override
    {
        last_request_ = request;
        request_count_++;

        if (handler)
        {
            captured_handler_ = lux::move(handler);
        }
    }

    void simulate_success_response(const lux::net::base::http_response& response)
    {
        if (captured_handler_)
        {
            lux::net::base::http_request_result result{response};
            captured_handler_(result);
        }
    }

    void simulate_error_response(const std::error_code& ec)
    {
        if (captured_handler_)
        {
            lux::net::base::http_request_result result{std::unexpected{ec}};
            captured_handler_(result);
        }
    }

    const lux::net::base::http_request& last_request() const
    {
        return last_request_;
    }

    int request_count() const
    {
        return request_count_;
    }

private:
    lux::net::base::http_request last_request_;
    lux::net::base::http_client_handler_type captured_handler_;
    int request_count_ = 0;
};

class mock_http_factory : public lux::net::base::http_factory
{
public:
    lux::net::base::http_server_ptr create_http_server(const lux::net::base::http_server_config& config,
                                                       lux::net::base::http_server_handler& handler) override
    {
        std::ignore = config;
        std::ignore = handler;

        REQUIRE(false);
        return nullptr;
    }

    lux::net::base::http_server_ptr create_https_server(const lux::net::base::http_server_config& config,
                                                        lux::net::base::http_server_handler& handler,
                                                        lux::net::base::ssl_context& ssl_context) override
    {
        std::ignore = config;
        std::ignore = handler;
        std::ignore = ssl_context;

        REQUIRE(false);
        return nullptr;
    }

    lux::net::base::http_client_ptr create_http_client(const lux::net::base::hostname_endpoint& destination,
                                                       const lux::net::base::http_client_config& config) override
    {
        std::ignore = destination;
        std::ignore = config;

        http_client_created_ = true;
        auto client = std::make_unique<mock_http_client>();
        last_created_client_ = client.get();
        return client;
    }

    lux::net::base::http_client_ptr create_https_client(const lux::net::base::hostname_endpoint& destination,
                                                        const lux::net::base::http_client_config& config,
                                                        lux::net::base::ssl_context& ssl_context) override
    {
        std::ignore = destination;
        std::ignore = config;
        std::ignore = ssl_context;

        https_client_created_ = true;
        auto client = std::make_unique<mock_http_client>();
        last_created_client_ = client.get();
        return client;
    }

    bool http_client_created() const
    {
        return http_client_created_;
    }

    bool https_client_created() const
    {
        return https_client_created_;
    }

    mock_http_client* last_created_client() const
    {
        return last_created_client_;
    }

private:
    bool http_client_created_ = false;
    bool https_client_created_ = false;
    mock_http_client* last_created_client_ = nullptr;
};

lux::net::base::hostname_endpoint create_test_endpoint()
{
    return lux::net::base::hostname_endpoint{"example.com", 80};
}

lux::net::http_client_app_config create_default_http_client_app_config()
{
    lux::net::http_client_app_config config;
    config.client_config.keep_alive = false;
    return config;
}

} // namespace

LUX_TEST_CASE("http_client_app", "constructs successfully with HTTP destination", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();

    std::unique_ptr<lux::net::http_client_app> app;
    REQUIRE_NOTHROW(app = std::make_unique<lux::net::http_client_app>(endpoint, factory, config));
    CHECK(factory.http_client_created());
}

LUX_TEST_CASE("http_client_app", "constructs successfully with HTTPS destination", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    lux::net::base::ssl_context ssl_context{lux::net::base::ssl_context::tls_client};
    const auto config = create_default_http_client_app_config();

    std::unique_ptr<lux::net::http_client_app> app;
    REQUIRE_NOTHROW(app = std::make_unique<lux::net::http_client_app>(endpoint, factory, ssl_context, config));
    CHECK(factory.https_client_created());
}

LUX_TEST_CASE("http_client_app", "sends GET request with correct method and target", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    bool handler_called = false;
    app.get("/api/data", [&](const auto& result) {
        handler_called = true;
        CHECK(result.has_value());
    });

    CHECK(mock_client->request_count() == 1);
    CHECK(mock_client->last_request().method() == lux::net::base::http_method::get);
    CHECK(mock_client->last_request().target() == "/api/data");
    CHECK(mock_client->last_request().body().empty());
}

LUX_TEST_CASE("http_client_app", "sends POST request with body", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    const std::string request_body = R"({"key":"value"})";
    app.post("/api/data", [](const auto&) {}, {}, request_body);

    CHECK(mock_client->request_count() == 1);
    CHECK(mock_client->last_request().method() == lux::net::base::http_method::post);
    CHECK(mock_client->last_request().target() == "/api/data");
    CHECK(mock_client->last_request().body() == request_body);
}

LUX_TEST_CASE("http_client_app", "sends PUT request with body", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    const std::string request_body = R"({"updated":"value"})";
    app.put("/api/resource", [](const auto&) {}, {}, request_body);

    CHECK(mock_client->request_count() == 1);
    CHECK(mock_client->last_request().method() == lux::net::base::http_method::put);
    CHECK(mock_client->last_request().target() == "/api/resource");
    CHECK(mock_client->last_request().body() == request_body);
}

LUX_TEST_CASE("http_client_app", "sends DELETE request with optional body", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    const std::string request_body = R"({"reason":"obsolete"})";
    app.del("/api/resource", [](const auto&) {}, {}, request_body);

    CHECK(mock_client->request_count() == 1);
    CHECK(mock_client->last_request().method() == lux::net::base::http_method::delete_);
    CHECK(mock_client->last_request().target() == "/api/resource");
    CHECK(mock_client->last_request().body() == request_body);
}

LUX_TEST_CASE("http_client_app", "sends GET request with custom headers", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    lux::net::base::http_request::headers_type headers;
    headers["User-Agent"] = "TestClient/1.0";
    headers["Accept"] = "application/json";

    app.get("/api/data", [](const auto&) {}, headers);

    CHECK(mock_client->last_request().has_header("User-Agent"));
    CHECK(mock_client->last_request().header("User-Agent") == "TestClient/1.0");
    CHECK(mock_client->last_request().has_header("Accept"));
    CHECK(mock_client->last_request().header("Accept") == "application/json");
}

LUX_TEST_CASE("http_client_app", "sends POST request with custom headers", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    lux::net::base::http_request::headers_type headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = "Bearer token123";

    app.post("/api/data", [](const auto&) {}, headers, R"({"key":"value"})");

    CHECK(mock_client->last_request().has_header("Content-Type"));
    CHECK(mock_client->last_request().header("Content-Type") == "application/json");
    CHECK(mock_client->last_request().has_header("Authorization"));
    CHECK(mock_client->last_request().header("Authorization") == "Bearer token123");
}

LUX_TEST_CASE("http_client_app", "invokes handler on successful GET response", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    bool handler_called = false;
    lux::net::base::http_response expected_response;
    expected_response.set_status(lux::net::base::http_status::ok);
    expected_response.set_body("Response body");

    app.get("/test", [&](const auto& result) {
        handler_called = true;
        REQUIRE(result.has_value());
        CHECK(result.value().status() == lux::net::base::http_status::ok);
        CHECK(result.value().body() == "Response body");
    });

    mock_client->simulate_success_response(expected_response);
    CHECK(handler_called);
}

LUX_TEST_CASE("http_client_app", "invokes handler on successful POST response", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    bool handler_called = false;
    lux::net::base::http_response expected_response;
    expected_response.set_status(lux::net::base::http_status::created);
    expected_response.set_body("Resource created");

    app.post(
        "/api/resource",
        [&](const auto& result) {
            handler_called = true;
            REQUIRE(result.has_value());
            CHECK(result.value().status() == lux::net::base::http_status::created);
            CHECK(result.value().body() == "Resource created");
        },
        {},
        R"({"data":"value"})");

    mock_client->simulate_success_response(expected_response);
    CHECK(handler_called);
}

LUX_TEST_CASE("http_client_app", "invokes handler on error response", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    bool handler_called = false;
    std::error_code expected_error = std::make_error_code(std::errc::connection_refused);

    app.get("/test", [&](const auto& result) {
        handler_called = true;
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == expected_error);
    });

    mock_client->simulate_error_response(expected_error);
    CHECK(handler_called);
}

LUX_TEST_CASE("http_client_app", "sends multiple GET requests independently", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    app.get("/first", [](const auto&) {});
    CHECK(mock_client->last_request().target() == "/first");

    app.get("/second", [](const auto&) {});
    CHECK(mock_client->last_request().target() == "/second");

    app.get("/third", [](const auto&) {});
    CHECK(mock_client->last_request().target() == "/third");

    CHECK(mock_client->request_count() == 3);
}

LUX_TEST_CASE("http_client_app", "sends requests with different HTTP methods", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    app.get("/resource", [](const auto&) {});
    CHECK(mock_client->last_request().method() == lux::net::base::http_method::get);

    app.post("/resource", [](const auto&) {}, {}, "data");
    CHECK(mock_client->last_request().method() == lux::net::base::http_method::post);

    app.put("/resource", [](const auto&) {}, {}, "data");
    CHECK(mock_client->last_request().method() == lux::net::base::http_method::put);

    app.del("/resource", [](const auto&) {});
    CHECK(mock_client->last_request().method() == lux::net::base::http_method::delete_);

    CHECK(mock_client->request_count() == 4);
}

LUX_TEST_CASE("http_client_app", "sends POST request with empty body", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    app.post("/api/action", [](const auto&) {});

    CHECK(mock_client->last_request().method() == lux::net::base::http_method::post);
    CHECK(mock_client->last_request().body().empty());
}

LUX_TEST_CASE("http_client_app", "sends DELETE request with empty body", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    app.del("/api/resource", [](const auto&) {});

    CHECK(mock_client->last_request().method() == lux::net::base::http_method::delete_);
    CHECK(mock_client->last_request().body().empty());
}

LUX_TEST_CASE("http_client_app", "sends GET request without headers", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    app.get("/test", [](const auto&) {});

    CHECK(mock_client->last_request().headers().empty());
}

LUX_TEST_CASE("http_client_app", "preserves request target with query parameters", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    const std::string target_with_query = "/api/search?query=test&limit=10";
    app.get(target_with_query, [](const auto&) {});

    CHECK(mock_client->last_request().target() == target_with_query);
}

LUX_TEST_CASE("http_client_app", "sends request to root path", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    app.get("/", [](const auto&) {});

    CHECK(mock_client->last_request().target() == "/");
}

LUX_TEST_CASE("http_client_app", "sends PUT request with custom headers and body", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    lux::net::base::http_request::headers_type headers;
    headers["Content-Type"] = "application/xml";
    headers["X-Custom-Header"] = "CustomValue";

    const std::string body = "<data><value>123</value></data>";
    app.put("/api/resource", [](const auto&) {}, headers, body);

    CHECK(mock_client->last_request().method() == lux::net::base::http_method::put);
    CHECK(mock_client->last_request().body() == body);
    CHECK(mock_client->last_request().header("Content-Type") == "application/xml");
    CHECK(mock_client->last_request().header("X-Custom-Header") == "CustomValue");
}

LUX_TEST_CASE("http_client_app", "sends DELETE request with custom headers", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    app.del("/api/resource/123", [](const auto&) {}, {{"Authorization", "Bearer token456"}});

    CHECK(mock_client->last_request().has_header("Authorization"));
    CHECK(mock_client->last_request().header("Authorization") == "Bearer token456");
}

LUX_TEST_CASE("http_client_app", "handler receives response headers", "[io][net][http]")
{
    mock_http_factory factory;
    auto endpoint = create_test_endpoint();
    const auto config = create_default_http_client_app_config();
    lux::net::http_client_app app{endpoint, factory, config};

    auto* mock_client = factory.last_created_client();
    REQUIRE(mock_client != nullptr);

    bool handler_called = false;
    lux::net::base::http_response expected_response;
    expected_response.set_status(lux::net::base::http_status::ok);
    expected_response.set_header("Content-Type", "application/json");
    expected_response.set_header("X-Request-ID", "req-123");

    app.get("/test", [&](const auto& result) {
        handler_called = true;
        REQUIRE(result.has_value());
        CHECK(result.value().has_header("Content-Type"));
        CHECK(result.value().header("Content-Type") == "application/json");
        CHECK(result.value().has_header("X-Request-ID"));
        CHECK(result.value().header("X-Request-ID") == "req-123");
    });

    mock_client->simulate_success_response(expected_response);
    CHECK(handler_called);
}


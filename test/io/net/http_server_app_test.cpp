#include "test_case.hpp"

#include <lux/io/net/http_server_app.hpp>
#include <lux/io/net/base/http_factory.hpp>
#include <lux/io/net/base/http_server.hpp>
#include <lux/io/net/base/endpoint.hpp>
#include <lux/io/net/base/address_v4.hpp>
#include <lux/io/net/base/http_method.hpp>
#include <lux/io/net/base/http_status.hpp>

#include <catch2/catch_all.hpp>

#include <memory>
#include <string>
#include <optional>

namespace {

class mock_http_server : public lux::net::base::http_server
{
public:
    explicit mock_http_server(lux::net::base::http_server_handler& handler, bool& served_flag, bool& stopped_flag)
        : handler_{handler}, served_{served_flag}, stopped_{stopped_flag}
    {
    }

    std::error_code serve(const lux::net::base::endpoint& ep) override
    {
        served_ = true;
        endpoint_ = ep;
        handler_.on_server_started();
        return {};
    }

    std::error_code stop() override
    {
        stopped_ = true;
        handler_.on_server_stopped();
        return {};
    }

    std::optional<lux::net::base::endpoint> local_endpoint() const override
    {
        return endpoint_;
    }

    lux::net::base::http_response simulate_request(const lux::net::base::http_request& request)
    {
        return handler_.handle_request(request);
    }

    void simulate_error(const std::error_code& ec)
    {
        handler_.on_server_error(ec);
    }

    bool served() const
    {
        return served_;
    }

    bool stopped() const
    {
        return stopped_;
    }

private:
    lux::net::base::http_server_handler& handler_;
    std::optional<lux::net::base::endpoint> endpoint_;
    bool& served_;
    bool& stopped_;
};

class mock_http_factory : public lux::net::base::http_factory
{
public:
    lux::net::base::http_server_ptr create_http_server(const lux::net::base::http_server_config& config,
                                                       lux::net::base::http_server_handler& handler) override
    {
        std::ignore = config;
        http_server_created_ = true;
        auto server = std::make_unique<mock_http_server>(handler, server_served_, server_stopped_);
        last_created_server_ = server.get();
        return server;
    }

    lux::net::base::http_server_ptr create_https_server(const lux::net::base::http_server_config& config,
                                                        lux::net::base::http_server_handler& handler,
                                                        lux::net::base::ssl_context& ssl_context) override
    {
        std::ignore = config;
        std::ignore = ssl_context;
        https_server_created_ = true;
        auto server = std::make_unique<mock_http_server>(handler, server_served_, server_stopped_);
        last_created_server_ = server.get();
        return server;
    }

    lux::net::base::http_client_ptr create_http_client(const lux::net::base::hostname_endpoint& destination,
                                                       const lux::net::base::http_client_config& config) override
    {
        std::ignore = destination;
        std::ignore = config;

        REQUIRE(false); // Not used in these tests
        return nullptr;
    }

    lux::net::base::http_client_ptr create_https_client(const lux::net::base::hostname_endpoint& destination,
                                                        const lux::net::base::http_client_config& config,
                                                        lux::net::base::ssl_context& ssl_context) override
    {
        std::ignore = destination;
        std::ignore = config;
        std::ignore = ssl_context;

        REQUIRE(false); // Not used in these tests
        return nullptr;
    }

    bool http_server_created() const
    {
        return http_server_created_;
    }

    bool https_server_created() const
    {
        return https_server_created_;
    }

    mock_http_server* last_created_server() const
    {
        return last_created_server_;
    }

    bool server_served() const
    {
        return server_served_;
    }

    bool server_stopped() const
    {
        return server_stopped_;
    }

private:
    bool http_server_created_ = false;
    bool https_server_created_ = false;
    mock_http_server* last_created_server_ = nullptr;
    bool server_served_ = false;
    bool server_stopped_ = false;
};

lux::net::http_server_app_config create_default_http_server_app_config()
{
    lux::net::http_server_app_config config;
    config.server_config.acceptor_config.reuse_address = true;
    config.server_config.acceptor_config.keep_alive = false;
    return config;
}

} // namespace

LUX_TEST_CASE("http_server_app", "constructs successfully with default configuration", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();

    std::unique_ptr<lux::net::http_server_app> app;
    REQUIRE_NOTHROW(app = std::make_unique<lux::net::http_server_app>(config, factory));
    CHECK(factory.http_server_created());
}

LUX_TEST_CASE("http_server_app", "starts serving on specified endpoint successfully", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, 8080};
    const auto serve_error = app.serve(endpoint);
    CHECK_FALSE(serve_error);

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);
    CHECK(mock_server->served());

    app.stop();
}

LUX_TEST_CASE("http_server_app", "stops serving successfully", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    const lux::net::base::endpoint endpoint{lux::net::base::localhost, 8080};
    const auto serve_error = app.serve(endpoint);
    REQUIRE_FALSE(serve_error);

    const auto stop_error = app.stop();
    CHECK_FALSE(stop_error);

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);
    CHECK(mock_server->stopped());
}

LUX_TEST_CASE("http_server_app", "registers GET route successfully", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    bool handler_called = false;
    REQUIRE_NOTHROW(app.get("/test", [&](const auto&, auto& res) {
        handler_called = true;
        res.ok("Response");
    }));
}

LUX_TEST_CASE("http_server_app", "routes GET request to registered handler", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    bool handler_called = false;
    app.get("/test", [&](const auto& req, auto& res) {
        handler_called = true;
        CHECK(req.method() == lux::net::base::http_method::get);
        CHECK(req.target() == "/test");
        res.ok("GET response");
    });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::get, "/test"};
    auto response = mock_server->simulate_request(request);

    CHECK(handler_called);
    CHECK(response.status() == lux::net::base::http_status::ok);
    CHECK(response.body() == "GET response");
    CHECK(response.has_header("Server"));
}

LUX_TEST_CASE("http_server_app", "registers POST route successfully", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    REQUIRE_NOTHROW(app.post("/api/data", [](const auto&, auto& res) { res.created("Created"); }));
}

LUX_TEST_CASE("http_server_app", "routes POST request to registered handler", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    const std::string expected_body = R"({"key":"value"})";
    bool handler_called = false;

    app.post("/api/data", [&](const auto& req, auto& res) {
        handler_called = true;
        CHECK(req.method() == lux::net::base::http_method::post);
        CHECK(req.target() == "/api/data");
        CHECK(req.body() == expected_body);
        res.created("Resource created");
    });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::post, "/api/data"};
    request.set_body(expected_body);
    request.set_header("Content-Type", "application/json");

    auto response = mock_server->simulate_request(request);

    CHECK(handler_called);
    CHECK(response.status() == lux::net::base::http_status::created);
    CHECK(response.body() == "Resource created");
}

LUX_TEST_CASE("http_server_app", "registers PUT route successfully", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    REQUIRE_NOTHROW(app.put("/api/resource", [](const auto&, auto& res) { res.ok("Updated"); }));
}

LUX_TEST_CASE("http_server_app", "routes PUT request to registered handler", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    bool handler_called = false;
    app.put("/api/resource", [&](const auto& req, auto& res) {
        handler_called = true;
        CHECK(req.method() == lux::net::base::http_method::put);
        res.ok("Resource updated");
    });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::put, "/api/resource"};
    auto response = mock_server->simulate_request(request);

    CHECK(handler_called);
    CHECK(response.status() == lux::net::base::http_status::ok);
    CHECK(response.body() == "Resource updated");
}

LUX_TEST_CASE("http_server_app", "registers DELETE route successfully", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    REQUIRE_NOTHROW(app.del("/api/resource", [](const auto&, auto& res) { res.no_content(); }));
}

LUX_TEST_CASE("http_server_app", "routes DELETE request to registered handler", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    bool handler_called = false;
    app.del("/api/resource", [&](const auto& req, auto& res) {
        handler_called = true;
        CHECK(req.method() == lux::net::base::http_method::delete_);
        res.no_content();
    });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::delete_, "/api/resource"};
    auto response = mock_server->simulate_request(request);

    CHECK(handler_called);
    CHECK(response.status() == lux::net::base::http_status::no_content);
}

LUX_TEST_CASE("http_server_app", "registers multiple routes independently", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    REQUIRE_NOTHROW(app.get("/users", [](const auto&, auto& res) { res.ok("List users"); }));
    REQUIRE_NOTHROW(app.post("/users", [](const auto&, auto& res) { res.created("User created"); }));
    REQUIRE_NOTHROW(app.put("/users", [](const auto&, auto& res) { res.ok("User updated"); }));
    REQUIRE_NOTHROW(app.del("/users", [](const auto&, auto& res) { res.no_content(); }));
}

LUX_TEST_CASE("http_server_app", "routes multiple requests to correct handlers", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    int get_calls = 0;
    int post_calls = 0;
    int put_calls = 0;
    int delete_calls = 0;

    app.get("/users", [&](const auto&, auto& res) {
        get_calls++;
        res.ok("List users");
    });

    app.post("/users", [&](const auto&, auto& res) {
        post_calls++;
        res.created("User created");
    });

    app.put("/users", [&](const auto&, auto& res) {
        put_calls++;
        res.ok("User updated");
    });

    app.del("/users", [&](const auto&, auto& res) {
        delete_calls++;
        res.no_content();
    });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    mock_server->simulate_request(lux::net::base::http_request{lux::net::base::http_method::get, "/users"});
    mock_server->simulate_request(lux::net::base::http_request{lux::net::base::http_method::post, "/users"});
    mock_server->simulate_request(lux::net::base::http_request{lux::net::base::http_method::put, "/users"});
    mock_server->simulate_request(lux::net::base::http_request{lux::net::base::http_method::delete_, "/users"});

    CHECK(get_calls == 1);
    CHECK(post_calls == 1);
    CHECK(put_calls == 1);
    CHECK(delete_calls == 1);
}

LUX_TEST_CASE("http_server_app", "registers routes with different paths", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    REQUIRE_NOTHROW(app.get("/", [](const auto&, auto& res) { res.ok("Root"); }));
    REQUIRE_NOTHROW(app.get("/api/v1/users", [](const auto&, auto& res) { res.ok("Users"); }));
    REQUIRE_NOTHROW(app.get("/api/v1/posts", [](const auto&, auto& res) { res.ok("Posts"); }));
    REQUIRE_NOTHROW(app.post("/api/v1/users", [](const auto&, auto& res) { res.created("User created"); }));
}

LUX_TEST_CASE("http_server_app", "returns 404 for unregistered routes", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    app.get("/existing", [](const auto&, auto& res) { res.ok("Found"); });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::get, "/nonexistent"};
    auto response = mock_server->simulate_request(request);

    CHECK(response.status() == lux::net::base::http_status::not_found);
    CHECK(response.body() == "404 Not Found");
}

LUX_TEST_CASE("http_server_app", "sets custom server name in config", "[io][net][http]")
{
    mock_http_factory factory;
    auto config = create_default_http_server_app_config();
    config.server_name = "CustomServer/2.0";

    lux::net::http_server_app app{config, factory};
    app.get("/test", [](const auto&, auto& res) { res.ok("Test"); });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::get, "/test"};
    auto response = mock_server->simulate_request(request);

    CHECK(response.has_header("Server"));
    CHECK(response.header("Server") == "CustomServer/2.0");
}

LUX_TEST_CASE("http_server_app", "response includes default Server header", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    app.get("/test", [](const auto&, auto& res) { res.ok("Test"); });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::get, "/test"};
    auto response = mock_server->simulate_request(request);

    CHECK(response.has_header("Server"));
    CHECK(response.header("Server") == "LuxHTTPServer");
}

LUX_TEST_CASE("http_server_app", "handler can access request headers", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    bool headers_verified = false;
    app.get("/test", [&](const auto& req, auto& res) {
        CHECK(req.has_header("User-Agent"));
        CHECK(req.header("User-Agent") == "TestClient/1.0");
        CHECK(req.has_header("Accept"));
        CHECK(req.header("Accept") == "application/json");
        headers_verified = true;
        res.ok("OK");
    });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::get, "/test"};
    request.set_header("User-Agent", "TestClient/1.0");
    request.set_header("Accept", "application/json");

    auto response = mock_server->simulate_request(request);

    CHECK(headers_verified);
}

LUX_TEST_CASE("http_server_app", "handler can set custom response headers", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    app.get("/json", [](const auto&, auto& res) { res.ok().json(R"({"status":"success"})"); });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::get, "/json"};
    auto response = mock_server->simulate_request(request);

    CHECK(response.status() == lux::net::base::http_status::ok);
    CHECK(response.body() == R"({"status":"success"})");
    CHECK(response.has_header("Content-Type"));
    CHECK(response.header("Content-Type") == "application/json");
}

LUX_TEST_CASE("http_server_app", "sets custom error handler", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    bool error_handler_called = false;
    std::error_code captured_error;

    REQUIRE_NOTHROW(app.set_on_error_handler([&](const std::error_code& ec) {
        error_handler_called = true;
        captured_error = ec;
    }));

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    std::error_code test_error = std::make_error_code(std::errc::connection_refused);
    mock_server->simulate_error(test_error);

    CHECK(error_handler_called);
    CHECK(captured_error == test_error);
}

LUX_TEST_CASE("http_server_app", "destructor stops server automatically", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();

    {
        lux::net::http_server_app app{config, factory};
        app.get("/test", [](const auto&, auto& res) { res.ok("Test"); });
        app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

        CHECK(factory.server_served());
        CHECK_FALSE(factory.server_stopped());
    }

    CHECK(factory.server_stopped());
}

LUX_TEST_CASE("http_server_app", "supports method chaining for route registration", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    REQUIRE_NOTHROW(app.get("/", [](const auto&, auto& res) { res.ok("Home"); }));
    REQUIRE_NOTHROW(app.get("/about", [](const auto&, auto& res) { res.ok("About"); }));
    REQUIRE_NOTHROW(app.post("/contact", [](const auto&, auto& res) { res.created("Message sent"); }));
}

LUX_TEST_CASE("http_server_app", "handles empty path routes", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    bool handler_called = false;
    app.get("", [&](const auto&, auto& res) {
        handler_called = true;
        res.ok("Empty path");
    });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::get, ""};
    auto response = mock_server->simulate_request(request);

    CHECK(handler_called);
    CHECK(response.body() == "Empty path");
}

LUX_TEST_CASE("http_server_app", "registers routes before serving", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    app.get("/before", [](const auto&, auto& res) { res.ok("Before"); });
    app.post("/before", [](const auto&, auto& res) { res.created("Created before"); });

    const auto serve_error = app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});
    CHECK_FALSE(serve_error);

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);
    CHECK(mock_server->served());
}

LUX_TEST_CASE("http_server_app", "response preserves request HTTP version", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    app.get("/test", [](const auto&, auto& res) { res.ok("Test"); });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::get, "/test"};
    request.set_version(10); // HTTP/1.0

    auto response = mock_server->simulate_request(request);

    CHECK(response.version() == 10);
}

LUX_TEST_CASE("http_server_app", "handler can read request body", "[io][net][http]")
{
    mock_http_factory factory;
    const auto config = create_default_http_server_app_config();
    lux::net::http_server_app app{config, factory};

    const std::string expected_body = "Request payload";
    bool body_verified = false;

    app.post("/echo", [&](const auto& req, auto& res) {
        CHECK(req.body() == expected_body);
        body_verified = true;
        res.ok(req.body());
    });

    app.serve(lux::net::base::endpoint{lux::net::base::localhost, 8080});

    auto* mock_server = factory.last_created_server();
    REQUIRE(mock_server != nullptr);

    lux::net::base::http_request request{lux::net::base::http_method::post, "/echo"};
    request.set_body(expected_body);

    auto response = mock_server->simulate_request(request);

    CHECK(body_verified);
    CHECK(response.body() == expected_body);
}


#include "test_case.hpp"

#include <lux/io/net/http_router.hpp>

#include <lux/io/net/base/http_request.hpp>
#include <lux/io/net/base/http_response.hpp>
#include <lux/io/net/base/http_method.hpp>
#include <lux/io/net/base/http_status.hpp>

#include <catch2/catch_all.hpp>

LUX_TEST_CASE("http_router", "routes request to registered handler successfully", "[io][net][http][router]")
{
    lux::net::http_router router;

    bool handler_called = false;
    router.add_route(lux::net::base::http_method::get, "/test", [&](const auto& req, auto& res) {
        handler_called = true;
        CHECK(req.method() == lux::net::base::http_method::get);
        CHECK(req.target() == "/test");
        res.ok("Test response");
    });

    lux::net::base::http_request request{lux::net::base::http_method::get, "/test"};
    lux::net::base::http_response response;

    router.route(request, response);

    CHECK(handler_called);
    CHECK(response.status() == lux::net::base::http_status::ok);
    CHECK(response.body() == "Test response");
}

LUX_TEST_CASE("http_router", "returns 404 when no route matches", "[io][net][http][router]")
{
    lux::net::http_router router;

    router.add_route(lux::net::base::http_method::get, "/existing", [](const auto&, auto& res) { res.ok("Found"); });

    lux::net::base::http_request request{lux::net::base::http_method::get, "/nonexistent"};
    lux::net::base::http_response response;

    router.route(request, response);

    CHECK(response.status() == lux::net::base::http_status::not_found);
    CHECK(response.body() == "404 Not Found");
}

LUX_TEST_CASE("http_router", "distinguishes routes by HTTP method", "[io][net][http][router]")
{
    lux::net::http_router router;

    bool get_called = false;
    bool post_called = false;

    router.add_route(lux::net::base::http_method::get, "/api/resource", [&](const auto&, auto& res) {
        get_called = true;
        res.ok("GET response");
    });

    router.add_route(lux::net::base::http_method::post, "/api/resource", [&](const auto&, auto& res) {
        post_called = true;
        res.created("POST response");
    });

    SECTION("GET request routes to GET handler")
    {
        lux::net::base::http_request request{lux::net::base::http_method::get, "/api/resource"};
        lux::net::base::http_response response;

        router.route(request, response);

        CHECK(get_called);
        CHECK_FALSE(post_called);
        CHECK(response.status() == lux::net::base::http_status::ok);
        CHECK(response.body() == "GET response");
    }

    SECTION("POST request routes to POST handler")
    {
        lux::net::base::http_request request{lux::net::base::http_method::post, "/api/resource"};
        lux::net::base::http_response response;

        router.route(request, response);

        CHECK_FALSE(get_called);
        CHECK(post_called);
        CHECK(response.status() == lux::net::base::http_status::created);
        CHECK(response.body() == "POST response");
    }
}

LUX_TEST_CASE("http_router", "distinguishes routes by target path", "[io][net][http][router]")
{
    lux::net::http_router router;

    bool handler1_called = false;
    bool handler2_called = false;

    router.add_route(lux::net::base::http_method::get, "/path1", [&](const auto&, auto& res) {
        handler1_called = true;
        res.ok("Path 1");
    });

    router.add_route(lux::net::base::http_method::get, "/path2", [&](const auto&, auto& res) {
        handler2_called = true;
        res.ok("Path 2");
    });

    SECTION("Routes to first handler")
    {
        lux::net::base::http_request request{lux::net::base::http_method::get, "/path1"};
        lux::net::base::http_response response;

        router.route(request, response);

        CHECK(handler1_called);
        CHECK_FALSE(handler2_called);
        CHECK(response.body() == "Path 1");
    }

    SECTION("Routes to second handler")
    {
        lux::net::base::http_request request{lux::net::base::http_method::get, "/path2"};
        lux::net::base::http_response response;

        router.route(request, response);

        CHECK_FALSE(handler1_called);
        CHECK(handler2_called);
        CHECK(response.body() == "Path 2");
    }
}

LUX_TEST_CASE("http_router", "handles multiple route registrations", "[io][net][http][router]")
{
    lux::net::http_router router;

    router.add_route(lux::net::base::http_method::get, "/users", [](const auto&, auto& res) { res.ok("List users"); });

    router.add_route(lux::net::base::http_method::post, "/users", [](const auto&, auto& res) {
        res.created("Create user");
    });

    router.add_route(lux::net::base::http_method::put, "/users", [](const auto&, auto& res) { res.ok("Update user"); });

    router.add_route(lux::net::base::http_method::delete_, "/users", [](const auto&, auto& res) { res.no_content(); });

    SECTION("GET /users")
    {
        lux::net::base::http_request request{lux::net::base::http_method::get, "/users"};
        lux::net::base::http_response response;
        router.route(request, response);
        CHECK(response.status() == lux::net::base::http_status::ok);
        CHECK(response.body() == "List users");
    }

    SECTION("POST /users")
    {
        lux::net::base::http_request request{lux::net::base::http_method::post, "/users"};
        lux::net::base::http_response response;
        router.route(request, response);
        CHECK(response.status() == lux::net::base::http_status::created);
        CHECK(response.body() == "Create user");
    }

    SECTION("PUT /users")
    {
        lux::net::base::http_request request{lux::net::base::http_method::put, "/users"};
        lux::net::base::http_response response;
        router.route(request, response);
        CHECK(response.status() == lux::net::base::http_status::ok);
        CHECK(response.body() == "Update user");
    }

    SECTION("DELETE /users")
    {
        lux::net::base::http_request request{lux::net::base::http_method::delete_, "/users"};
        lux::net::base::http_response response;
        router.route(request, response);
        CHECK(response.status() == lux::net::base::http_status::no_content);
    }
}

LUX_TEST_CASE("http_router", "handler can access request data", "[io][net][http][router]")
{
    lux::net::http_router router;

    router.add_route(lux::net::base::http_method::post, "/echo", [](const auto& req, auto& res) {
        res.ok(req.body());
        for (const auto& [key, value] : req.headers())
        {
            res.set_header(key, value);
        }
    });

    lux::net::base::http_request request{lux::net::base::http_method::post, "/echo"};
    request.set_body("Request body content");
    request.set_header("X-Custom-Header", "CustomValue");
    request.set_header("Content-Type", "text/plain");

    lux::net::base::http_response response;

    router.route(request, response);

    CHECK(response.status() == lux::net::base::http_status::ok);
    CHECK(response.body() == "Request body content");
    CHECK(response.has_header("X-Custom-Header"));
    CHECK(response.header("X-Custom-Header") == "CustomValue");
    CHECK(response.has_header("Content-Type"));
    CHECK(response.header("Content-Type") == "text/plain");
}

LUX_TEST_CASE("http_router", "handler can modify response headers", "[io][net][http][router]")
{
    lux::net::http_router router;

    router.add_route(lux::net::base::http_method::get, "/json", [](const auto&, auto& res) {
        res.ok().json(R"({"status":"ok"})");
    });

    lux::net::base::http_request request{lux::net::base::http_method::get, "/json"};
    lux::net::base::http_response response;

    router.route(request, response);

    CHECK(response.status() == lux::net::base::http_status::ok);
    CHECK(response.body() == R"({"status":"ok"})");
    CHECK(response.has_header("Content-Type"));
    CHECK(response.header("Content-Type") == "application/json");
}

LUX_TEST_CASE("http_router", "handler can set different status codes", "[io][net][http][router]")
{
    lux::net::http_router router;

    SECTION("Handler sets 201 Created")
    {
        router.add_route(lux::net::base::http_method::post, "/resource", [](const auto&, auto& res) {
            res.created("Resource created");
        });

        lux::net::base::http_request request{lux::net::base::http_method::post, "/resource"};
        lux::net::base::http_response response;
        router.route(request, response);

        CHECK(response.status() == lux::net::base::http_status::created);
        CHECK(response.body() == "Resource created");
    }

    SECTION("Handler sets 204 No Content")
    {
        router.add_route(lux::net::base::http_method::delete_, "/resource", [](const auto&, auto& res) {
            res.no_content();
        });

        lux::net::base::http_request request{lux::net::base::http_method::delete_, "/resource"};
        lux::net::base::http_response response;
        router.route(request, response);

        CHECK(response.status() == lux::net::base::http_status::no_content);
    }

    SECTION("Handler sets 400 Bad Request")
    {
        router.add_route(lux::net::base::http_method::post, "/validate", [](const auto&, auto& res) {
            res.bad_request("Invalid input");
        });

        lux::net::base::http_request request{lux::net::base::http_method::post, "/validate"};
        lux::net::base::http_response response;
        router.route(request, response);

        CHECK(response.status() == lux::net::base::http_status::bad_request);
        CHECK(response.body() == "Invalid input");
    }

    SECTION("Handler sets 500 Internal Server Error")
    {
        router.add_route(lux::net::base::http_method::get, "/error", [](const auto&, auto& res) {
            res.internal_server_error("Server error");
        });

        lux::net::base::http_request request{lux::net::base::http_method::get, "/error"};
        lux::net::base::http_response response;
        router.route(request, response);

        CHECK(response.status() == lux::net::base::http_status::internal_server_error);
        CHECK(response.body() == "Server error");
    }
}

LUX_TEST_CASE("http_router", "handles same path with different methods independently", "[io][net][http][router]")
{
    lux::net::http_router router;

    int get_calls = 0;
    int post_calls = 0;
    int put_calls = 0;

    router.add_route(lux::net::base::http_method::get, "/item", [&](const auto&, auto& res) {
        get_calls++;
        res.ok("GET item");
    });

    router.add_route(lux::net::base::http_method::post, "/item", [&](const auto&, auto& res) {
        post_calls++;
        res.created("POST item");
    });

    router.add_route(lux::net::base::http_method::put, "/item", [&](const auto&, auto& res) {
        put_calls++;
        res.ok("PUT item");
    });

    lux::net::base::http_response response1;
    router.route(lux::net::base::http_request{lux::net::base::http_method::get, "/item"}, response1);

    lux::net::base::http_response response2;
    router.route(lux::net::base::http_request{lux::net::base::http_method::post, "/item"}, response2);

    lux::net::base::http_response response3;
    router.route(lux::net::base::http_request{lux::net::base::http_method::put, "/item"}, response3);

    CHECK(get_calls == 1);
    CHECK(post_calls == 1);
    CHECK(put_calls == 1);
    CHECK(response1.body() == "GET item");
    CHECK(response2.body() == "POST item");
    CHECK(response3.body() == "PUT item");
}

LUX_TEST_CASE("http_router", "unregistered method on registered path returns 404", "[io][net][http][router]")
{
    lux::net::http_router router;

    router.add_route(lux::net::base::http_method::get, "/api", [](const auto&, auto& res) { res.ok("GET handler"); });

    lux::net::base::http_request request{lux::net::base::http_method::post, "/api"};
    lux::net::base::http_response response;

    router.route(request, response);

    CHECK(response.status() == lux::net::base::http_status::not_found);
    CHECK(response.body() == "404 Not Found");
}

LUX_TEST_CASE("http_router", "routes request with unknown method", "[io][net][http][router]")
{
    lux::net::http_router router;

    router.add_route(lux::net::base::http_method::get, "/test", [](const auto&, auto& res) { res.ok("Found"); });

    lux::net::base::http_request request{lux::net::base::http_method::unknown, "/test"};
    lux::net::base::http_response response;

    router.route(request, response);

    CHECK(response.status() == lux::net::base::http_status::not_found);
}

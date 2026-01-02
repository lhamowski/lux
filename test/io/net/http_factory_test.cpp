#include <lux/io/net/http_factory.hpp>
#include <lux/io/net/socket_factory.hpp>
#include <lux/io/net/base/http_server.hpp>
#include <lux/io/net/base/ssl.hpp>

#include <catch2/catch_all.hpp>

namespace {

class test_http_server_handler : public lux::net::base::http_server_handler
{
public:
    void on_server_started() override
    {
    }

    void on_server_stopped() override
    {
    }

    void on_server_error(const std::error_code& ec) override
    {
        (void)ec;
    }

    lux::net::base::http_response handle_request(const lux::net::base::http_request& request) override
    {
        (void)request;
        lux::net::base::http_response response;
        response.set_status(lux::net::base::http_status::ok);
        response.set_body("Test response");
        return response;
    }
};

} // namespace

TEST_CASE("http_factory: creates HTTP server successfully", "[io][net][http]")
{
    boost::asio::io_context io_context;

    test_http_server_handler handler;
    lux::net::base::http_server_config config{};
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    lux::net::http_factory factory{socket_factory};

    auto server = factory.create_http_server(config, handler);
    REQUIRE(server != nullptr);
}

TEST_CASE("http_factory: creates HTTPS server successfully", "[io][net][http]")
{
    boost::asio::io_context io_context;

    test_http_server_handler handler;
    lux::net::base::http_server_config config{};
    lux::net::base::ssl_context ssl_context{boost::asio::ssl::context::sslv23};
    lux::net::socket_factory socket_factory{io_context.get_executor()};
    lux::net::http_factory factory{socket_factory};

    auto server = factory.create_https_server(config, handler, ssl_context);
    REQUIRE(server != nullptr);
}

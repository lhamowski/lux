#include <lux/io/net/socket_factory.hpp>
#include <lux/io/net/udp_socket.hpp>

#include <catch2/catch_all.hpp>

namespace {
class test_udp_socket_handler : public lux::net::base::udp_socket_handler
{
public:
    void on_data_read(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data) override
    {
        (void)endpoint;
        (void)data;
    }

    void on_data_sent(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data) override
    {
        (void)endpoint;
        (void)data;
    }

    void on_read_error(const lux::net::base::endpoint& endpoint, const std::error_code& ec) override
    {
        (void)endpoint;
        (void)ec;
    }

    void on_send_error(const lux::net::base::endpoint& endpoint,
                       const std::span<const std::byte>& data,
                       const std::error_code& ec) override
    {
        (void)endpoint;
        (void)data;
        (void)ec;
    }
};

} // namespace

TEST_CASE("Socket factory creates UDP socket", "[io][net]")
{
    boost::asio::io_context io_context;

    test_udp_socket_handler handler;
    lux::net::base::udp_socket_config config{};
    lux::net::socket_factory factory{io_context.get_executor()};

    auto socket = factory.create_udp_socket(config, handler);
    REQUIRE(socket != nullptr);
}
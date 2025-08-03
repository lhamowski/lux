#pragma once

#include <lux/net/base/udp_socket.hpp>
#include <lux/net/base/endpoint.hpp>
#include <lux/fwd.hpp>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/ip/udp.hpp>

#include <memory>

namespace lux::net {

struct udp_socket_config
{
    std::size_t memory_arena_initial_item_size = 1024; // Initial size of each item in the memory arena
    std::size_t memory_arena_initial_item_count = 4;   // Initial number of items in the memory arena
};

class udp_socket : public lux::net::base::udp_socket
{
public:
    udp_socket(boost::asio::any_io_executor exe,
               lux::net::base::udp_socket_handler& handler,
               const udp_socket_config& config);

    ~udp_socket();

    udp_socket(const udp_socket&) = delete;
    udp_socket& operator=(const udp_socket&) = delete;
    udp_socket(udp_socket&&) = default;
    udp_socket& operator=(udp_socket&&) = default;

public:
    // lux::net::base::udp_socket implementation
    std::error_code open() override;
    std::error_code close(bool send_pending_data) override;
    std::error_code bind(const lux::net::base::endpoint& endpoint) override;
    void send(const lux::net::base::endpoint& endpoint, const std::span<const std::byte>& data) override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

} // namespace lux::net
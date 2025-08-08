#pragma once

#include <lux/fwd.hpp>

#include <lux/io/net/base/tcp_socket.hpp>
#include <lux/io/time/delayed_retry_executor.hpp>

#include <boost/asio/any_io_executor.hpp>

namespace lux::net {

struct tcp_socket_config
{
    /**
     * Keep-alive settings
     * If true, enables TCP keep-alive to maintain the connection.
     */
    bool keep_alive{false};

    struct reconnect_config
    {
        /**
         * If true, enables automatic reconnection attempts.
         * If false, the socket will not attempt to reconnect after disconnection.
         */
        bool enabled{true};

        /**
         * Configuration for the retry mechanism used during reconnection attempts.
         * This defines how the retry mechanism will behave in terms of delay between attempts.
         */
        lux::time::delayed_retry_config retry_config{
            .strategy = lux::time::delayed_retry_config::backoff_strategy::exponential_backoff,
            .max_attempts = 5,
            .base_delay = std::chrono::milliseconds{1000},
            .max_delay = std::chrono::milliseconds{30000}};

    } reconnect{};

    struct buffer_config
    {
        /**
         * Size of each allocated buffer chunk in bytes.
         */
        std::size_t initial_send_chunk_size{1024};

        /**
         * Number of buffer chunks to preallocate.
         */
        std::size_t initial_send_chunk_count{4};

        /**
         * Size of read buffer to preallocate for reading data.
         */
        std::size_t read_buffer_size{8 * 1024}; // 8 KB

    } buffer{};
};

class tcp_socket : public lux::net::base::tcp_socket
{
public:
    tcp_socket(boost::asio::any_io_executor exe,
               lux::net::base::tcp_socket_handler& handler,
               const lux::net::tcp_socket_config& config,
               lux::time::base::timer_factory& timer_factory);
    ~tcp_socket();

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&) = delete;
    tcp_socket(tcp_socket&&) = default;
    tcp_socket& operator=(tcp_socket&&) = default;

public:
    // lux::net::base::tcp_socket implementation
    std::error_code connect(const lux::net::base::endpoint& endpoint) override;
    std::error_code connect(const lux::net::base::host_endpoint& host_endpoint) override;
    std::error_code disconnect(bool send_pending) override;
    std::error_code send(const std::span<const std::byte>& data) override;
    bool is_connected() const override;
    std::optional<lux::net::base::endpoint> local_endpoint() const override;
    std::optional<lux::net::base::endpoint> remote_endpoint() const override;

private:
    class impl;
    std::shared_ptr<impl> impl_;
};

} // namespace lux::net
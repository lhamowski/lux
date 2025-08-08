#pragma once

#include <lux/io/net/base/endpoint.hpp>

#include <optional>
#include <span>
#include <string_view>
#include <system_error>

namespace lux::net::base {

struct tcp_socket_config
{
    /**
     * Keep-alive settings
     * If true, enables TCP keep-alive to maintain the connection.
     */
    bool keep_alive{false};

    struct reconnect_config
    {
        enum class strategy
        {
            disabled,
            linear_backoff,
            exponential_backoff,
        };

        /**
         * Reconnection strategy.
         * If set to `disabled`, no reconnection attempts will be made.
         */
        strategy backoff_strategy{strategy::exponential_backoff};

        /**
         * Maximum number of reconnection attempts.
         * Set to 0 for infinite attempts.
         */
        std::size_t max_attempts{5};

        /**
         * Initial delay between first reconnection attempt.
         */
        std::chrono::milliseconds base_delay{1000};

        /**
         * Maximum delay between reconnection attempts.
         * Used as ceiling for both linear and exponential backoff.
         */
        std::chrono::milliseconds max_delay{30000};

        /**
         * Helper function to check if reconnection is enabled.
         */
        bool enabled() const
        {
            return backoff_strategy != strategy::disabled && max_attempts > 0;
        }

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

class tcp_socket
{
public:
    /**
     * Opens the TCP socket.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code connect(const lux::net::base::endpoint& endpoint) = 0;

    /**
     * Opens the TCP socket using a host and port.
     * @param host The host endpoint to connect to.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code connect(const lux::net::base::host_endpoint& host_endpoint) = 0;

    /**
     * Closes the TCP socket.
     * @param send_pending If true, sends any pending data before closing.
     * @return An error code indicating success or failure.
     */
    virtual std::error_code disconnect(bool send_pending) = 0;

    /**
     * Sends data to the connected endpoint.
     * @param data The data to send, represented as a span of bytes.
     */
    virtual void send(const std::span<const std::byte>& data) = 0;

    /**
     * Checks if the socket is currently connected.
     * @return true if the socket is connected, false otherwise.
     */
    virtual bool is_connected() const = 0;

    /**
     * Gets the local endpoint of the socket.
     * @return The local endpoint if connected, otherwise std::nullopt.
     */
    virtual std::optional<lux::net::base::endpoint> local_endpoint() const = 0;

    /**
     * Gets the remote endpoint of the socket.
     * @return The remote endpoint if connected, otherwise std::nullopt.
     */
    virtual std::optional<lux::net::base::endpoint> remote_endpoint() const = 0;

protected:
    virtual ~tcp_socket() = default;
};

class tcp_socket_handler
{
public:
    /**
     * Called when a connection is established with a specific endpoint.
     * @param socket The socket that is now connected.
     */
    virtual void on_connected(lux::net::base::tcp_socket& socket) = 0;

    /**
     * Called when a connection is disconnected from a specific endpoint.
     * @param socket The socket that was disconnected.
     * @param ec The error code indicating the reason for disconnection.
     */
    virtual void on_disconnected(lux::net::base::tcp_socket& socket, const std::error_code& ec) = 0;

    /**
     * Called when data is received from a specific endpoint.
     * @param socket The socket that received the data.
     * @param data The received data, represented as a span of bytes.
     */
    virtual void on_data_read(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) = 0;

    /**
     * Called when data is successfully sent to a specific endpoint.
     * @param socket The socket that sent the data.
     * @param data The data that was sent, represented as a span of bytes.
     */
    virtual void on_data_sent(lux::net::base::tcp_socket& socket, const std::span<const std::byte>& data) = 0;

protected:
    virtual ~tcp_socket_handler() = default;
};

} // namespace lux::net::base
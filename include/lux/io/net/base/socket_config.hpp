#pragma once

#include <cstddef>

namespace lux::net::base {

struct socket_buffer_config
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
};

} // namespace lux::net::base
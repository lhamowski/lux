#pragma once

#include <lux/support/concepts.hpp>
#include <lux/support/exception.hpp>

#include <array>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace lux {

/**
 * @brief Binary input reader that reads directly from an externally provided buffer.
 *
 * The buffer_reader class provides a fluent interface for reading binary data
 * from a std::span<const std::byte>. It tracks the current read position and handles
 * buffer underflow by throwing exceptions.
 */
class buffer_reader
{
public:
    /**
     * @brief Constructs a buffer_reader with the given buffer.
     * @param buffer The external buffer to read from.
     */
    explicit buffer_reader(std::span<const std::byte> buffer) noexcept : buffer_{buffer}, position_{0}
    {
    }

    /**
     * @brief Gets the current read position.
     * @return The number of bytes read so far.
     */
    std::size_t position() const noexcept
    {
        return position_;
    }

    /**
     * @brief Gets the total buffer size.
     * @return The total size of the buffer in bytes.
     */
    std::size_t size() const noexcept
    {
        return buffer_.size();
    }

    /**
     * @brief Gets the remaining buffer space.
     * @return The number of bytes remaining in the buffer.
     */
    std::size_t remaining() const noexcept
    {
        return size() - position();
    }

    /**
     * @brief Gets a span containing the remaining unread data.
     * @return A span of const bytes representing the unread data.
     */
    std::span<const std::byte> remaining_data() const noexcept
    {
        return buffer_.subspan(position_);
    }

    /**
     * @brief Advances the read position by the specified number of bytes without reading data.
     * @param bytes Number of bytes to skip.
     * @return Reference to this buffer_reader for chaining.
     * @throws lux::formatted_exception if there's insufficient buffer data.
     */
    buffer_reader& skip(std::size_t bytes)
    {
        if (remaining() < bytes)
        {
            throw lux::formatted_exception(
                "Buffer underflow: attempting to skip {} bytes, but only {} bytes remaining", bytes, remaining());
        }

        position_ += bytes;
        return *this;
    }

    /**
     * @brief Reads binary data from the buffer.
     * @param value Reference to store the read value.
     * @return Reference to this buffer_reader for chaining.
     * @throws lux::formatted_exception if there's insufficient buffer data.
     */
    template <lux::trivially_copyable T>
    buffer_reader& read(T& value)
    {
        if (remaining() < sizeof(T))
        {
            throw lux::formatted_exception(
                "Buffer underflow: attempting to read {} bytes, but only {} bytes remaining", sizeof(T), remaining());
        }

        std::memcpy(&value, buffer_.data() + position_, sizeof(T));
        position_ += sizeof(T);
        return *this;
    }

    /**
     * @brief Reads data into a C-style array.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Array size.
     * @param arr C-style array to read into.
     * @return Reference to this buffer_reader for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_reader& read(T (&arr)[N])
    {
        const auto bytes_needed = sizeof(T) * N;
        if (remaining() < bytes_needed)
        {
            throw lux::formatted_exception(
                "Buffer underflow: attempting to read {} bytes for array, but only {} bytes remaining",
                bytes_needed,
                remaining());
        }

        std::memcpy(arr, buffer_.data() + position_, bytes_needed);
        position_ += bytes_needed;
        return *this;
    }

    /**
     * @brief Reads data into a std::array.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Array size.
     * @param arr std::array to read into.
     * @return Reference to this buffer_reader for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_reader& read(std::array<T, N>& arr)
    {
        const auto bytes_needed = sizeof(T) * N;
        if (remaining() < bytes_needed)
        {
            throw lux::formatted_exception(
                "Buffer underflow: attempting to read {} bytes for std::array, but only {} bytes remaining",
                bytes_needed,
                remaining());
        }

        std::memcpy(arr.data(), buffer_.data() + position_, bytes_needed);
        position_ += bytes_needed;
        return *this;
    }

    /**
     * @brief Reads a trivially copyable type from the buffer and returns it.
     * @tparam T The type to read (must be trivially copyable and default constructible).
     * @return The read value of type T.
     * @throws lux::formatted_exception if there's insufficient buffer data.
     */
    template <lux::trivially_readable T>
    T read()
    {
        T value{};
        read(value);
        return value;
    }

    /**
     * @brief Reads a std::array from the buffer and returns it.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Array size.
     * @return The read std::array of type T.
     * @throws lux::formatted_exception if there's insufficient buffer data.
     */
    template <lux::trivially_copyable T, std::size_t N>
    std::array<T, N> read()
    {
        std::array<T, N> arr{};
        read(arr);
        return arr;
    }

    /**
     * @brief Stream operator for reading trivially copyable types.
     * @tparam T The type to read (must be trivially copyable).
     * @param value Reference to the value to read into.
     * @return Reference to this buffer_reader for chaining.
     */
    template <lux::trivially_copyable T>
    buffer_reader& operator>>(T& value)
    {
        return read(value);
    }

    /**
     * @brief Stream operator for reading std::array.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Array size.
     * @param arr std::array to read into.
     * @return Reference to this buffer_reader for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_reader& operator>>(std::array<T, N>& arr)
    {
        return read(arr);
    }

private:
    std::span<const std::byte> buffer_;
    std::size_t position_;
};

} // namespace lux
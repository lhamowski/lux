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
 * @brief Binary output writer that writes directly into an externally provided buffer.
 *
 * The buffer_writer class provides a fluent interface for writing binary data
 * into a std::span<std::byte>. It tracks the current write position and handles
 * buffer overflow by throwing exceptions.
 */
class buffer_writer
{
public:
    /**
     * @brief Constructs a buffer_writer with the given buffer.
     * @param buffer The external buffer to write into.
     */
    explicit buffer_writer(std::span<std::byte> buffer) noexcept : buffer_{buffer}, position_{0}
    {
    }

    /**
     * @brief Gets the current write position.
     * @return The number of bytes written so far.
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
     * @brief Gets a span containing the data written so far.
     * @return A span of const bytes representing the written data.
     */
    std::span<const std::byte> written_data() const noexcept
    {
        return std::span<const std::byte>{buffer_.data(), position_};
    }

    /**
     * @brief Advances the write position by the specified number of bytes without writing data.
     * @param bytes Number of bytes to skip.
     * @return Reference to this buffer_writer for chaining.
     * @throws lux::formatted_exception if there's insufficient buffer space.
     */
    buffer_writer& skip(std::size_t bytes)
    {
        if (remaining() < bytes)
        {
            throw lux::formatted_exception("Buffer overflow: attempting to skip {} bytes, but only {} bytes remaining",
                                           bytes,
                                           remaining());
        }

        position_ += bytes;
        return *this;
    }

    /**
     * @brief Writes binary data to the buffer.
     * @tparam N Size of the span.
     * @param data Span of bytes to write.
     * @return Reference to this buffer_writer for chaining.
     * @throws lux::formatted_exception if there's insufficient buffer space.
     */
    template <std::size_t N>
    buffer_writer& write(std::span<const std::byte, N> data)
    {
        if (remaining() < data.size())
        {
            throw lux::formatted_exception("Buffer overflow: attempting to write {} bytes, but only {} bytes remaining",
                                           data.size(),
                                           remaining());
        }

        std::memcpy(buffer_.data() + position_, data.data(), data.size());
        position_ += data.size();
        return *this;
    }

    /**
     * @brief Writes a std::span to the buffer.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Size of the span.
     * @param span The std::span to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_writer& write(std::span<const T, N> span)
    {
        if (span.empty())
        {
            return *this;
        }
        return write(std::as_bytes(span));
    }

    /**
     * @brief Writes a mutable std::span to the buffer.
     * @tparam N Size of the span.
     * @tparam T Element type (must be trivially copyable).
     * @param span The std::span to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_writer& write(std::span<T, N> span)
    {
        return write(std::span<const T>{span});
    }

    /**
     * @brief Writes a trivially copyable type to the buffer.
     * @tparam T The type to write (must be trivially copyable).
     * @param value The value to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T>
    buffer_writer& write(const T& value)
    {
        return write(std::as_bytes(std::span{&value, 1}));
    }

    /**
     * @brief Writes a C-style array to the buffer.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Array size.
     * @param arr The C-style array to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_writer& write(const T (&arr)[N])
    {
        return write(std::as_bytes(std::span{arr}));
    }

    /**
     * @brief Writes a std::array to the buffer.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Array size.
     * @param arr The std::array to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_writer& write(const std::array<T, N>& arr)
    {
        return write(std::as_bytes(std::span{arr}));
    }

    /**
     * @brief Writes a std::vector to the buffer.
     * @tparam T Element type (must be trivially copyable).
     * @param vec The std::vector to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T>
    buffer_writer& write(const std::vector<T>& vec)
    {
        if (vec.empty())
        {
            return *this;
        }
        return write(std::as_bytes(std::span{vec}));
    }

    /**
     * @brief Writes a std::string to the buffer.
     * @param str The std::string to write.
     * @return Reference to this buffer_writer for chaining.
     */
    buffer_writer& write(const std::string& str)
    {
        if (!str.empty())
        {
            return write(std::as_bytes(std::span{str.data(), str.size()}));
        }
        return *this;
    }

    /**
     * @brief Writes a std::string_view to the buffer.
     * @param str The std::string_view to write.
     * @return Reference to this buffer_writer for chaining.
     */
    buffer_writer& write(std::string_view str)
    {
        if (!str.empty())
        {
            return write(std::as_bytes(std::span{str.data(), str.size()}));
        }
        return *this;
    }

    /**
     * @brief Writes a C-style string to the buffer.
     * @param str The C-style string to write.
     * @return Reference to this buffer_writer for chaining.
     */
    buffer_writer& write(const char* str)
    {
        return write(std::string_view{str});
    }

    /**
     * @brief Stream operator for writing trivially copyable types.
     * @tparam T The type to write (must be trivially copyable).
     * @param value The value to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T>
    buffer_writer& operator<<(const T& value)
    {
        return write(value);
    }

    /**
     * @brief Stream operator for writing std::string without length prefix.
     * @param str The string to write (raw string data only).
     * @return Reference to this buffer_writer for chaining.
     */
    buffer_writer& operator<<(const std::string& str)
    {
        return write(str);
    }

    /**
     * @brief Stream operator for writing std::string_view without length prefix.
     * @param str The string_view to write (raw string data only).
     * @return Reference to this buffer_writer for chaining.
     */
    buffer_writer& operator<<(std::string_view str)
    {
        return write(str);
    }

    /**
     * @brief Stream operator for writing C-style string literals without length prefix.
     * @param str The C-style string to write (raw string data only).
     * @return Reference to this buffer_writer for chaining.
     */
    buffer_writer& operator<<(const char* str)
    {
        return write(str);
    }

    /**
     * @brief Stream operator for writing std::array of trivially copyable types.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Array size.
     * @param arr The array to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_writer& operator<<(const std::array<T, N>& arr)
    {
        return write(arr);
    }

    /**
     * @brief Stream operator for writing std::vector of trivially copyable types.
     * @tparam T Element type (must be trivially copyable).
     * @param vec The vector to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T>
    buffer_writer& operator<<(const std::vector<T>& vec)
    {
        return write(vec);
    }

    /**
     * @brief Stream operator for writing std::span of trivially copyable types.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Size of the span.
     * @param span The span to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_writer& operator<<(std::span<const T> span)
    {
        return write(span);
    }

    /**
     * @brief Stream operator for writing mutable std::span of trivially copyable types.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Size of the span.
     * @param span The span to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_writer& operator<<(std::span<T> span)
    {
        return write(span);
    }

    /**
     * @brief Stream operator for writing C-style arrays of trivially copyable types.
     * @tparam T Element type (must be trivially copyable).
     * @tparam N Array size.
     * @param arr The C-style array to write.
     * @return Reference to this buffer_writer for chaining.
     */
    template <lux::trivially_copyable T, std::size_t N>
    buffer_writer& operator<<(const T (&arr)[N])
    {
        return write(arr);
    }

private:
    std::span<std::byte> buffer_;
    std::size_t position_;
};

} // namespace lux
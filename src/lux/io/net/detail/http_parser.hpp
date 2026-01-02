#pragma once

#include <lux/support/assert.hpp>

#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/beast/http/string_body.hpp>

#include <cstring>
#include <optional>
#include <span>
#include <system_error>

namespace lux::net::detail {

template <typename BoostMessageType>
class http_parser_handler
{
public:
    virtual void on_message_parsed(BoostMessageType&& message) = 0;
    virtual void on_parse_error(const std::error_code& ec) = 0;

protected:
    virtual ~http_parser_handler() = default;
};

template <bool IsRequest>
class http_parser
{
private:
    using boost_message_type = boost::beast::http::message<IsRequest, boost::beast::http::string_body>;
    using boost_parser_type = boost::beast::http::parser<IsRequest, boost::beast::http::string_body>;

public:
    explicit http_parser(http_parser_handler<boost_message_type>& handler) : handler_{handler}
    {
        parser_.emplace();
    }

public:
    void parse(const std::span<const std::byte>& data)
    {
        LUX_ASSERT(parser_, "Parser must be initialized at this point");

        const auto buf = buffer_.prepare(data.size());
        std::memcpy(buf.data(), data.data(), data.size());
        buffer_.commit(data.size());

        while (buffer_.size() > 0)
        {
            boost::beast::error_code ec;
            const std::size_t bytes_consumed = parser_->put(buffer_.data(), ec);

            if (ec == boost::beast::http::error::need_more)
            {
                buffer_.consume(bytes_consumed);
                return;
            }
            else if (ec)
            {
                buffer_.consume(buffer_.size());
                parser_.emplace();
                handler_.on_parse_error(ec);
                return;
            }

            if (parser_->is_done())
            {
                boost_message_type message = parser_->release();
                parser_.emplace();
                handler_.on_message_parsed(std::move(message));
                buffer_.consume(bytes_consumed);
            }
            else
            {
                buffer_.consume(bytes_consumed);
            }
        }
    }

private:
    http_parser_handler<boost_message_type>& handler_;

private:
    boost::beast::flat_buffer buffer_;
    std::optional<boost_parser_type> parser_;
};

using http_request_parser = http_parser<true>;
using http_response_parser = http_parser<false>;

using boost_http_request_type = boost::beast::http::request<boost::beast::http::string_body>;
using boost_http_response_type = boost::beast::http::response<boost::beast::http::string_body>;

using http_request_parser_handler = http_parser_handler<boost_http_request_type>;
using http_response_parser_handler = http_parser_handler<boost_http_response_type>;

} // namespace lux::net::detail

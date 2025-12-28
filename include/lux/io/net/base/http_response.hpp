#pragma once

#include <lux/io/net/base/http_status.hpp>
#include <lux/support/move.hpp>
#include <lux/support/container.hpp>

#include <string>
#include <string_view>

namespace lux::net::base {

class http_response
{
public:
    http_response() = default;

    explicit http_response(http_status status) : status_{status}
    {
    }

    http_response(http_status status, std::string_view body) : status_{status}, body_{body}
    {
    }

    http_status status() const noexcept
    {
        return status_;
    }

    void set_status(http_status status) noexcept
    {
        status_ = status;
    }

    unsigned version() const noexcept
    {
        return version_;
    }

    void set_version(unsigned version) noexcept
    {
        version_ = version;
    }

    std::string& body() noexcept
    {
        return body_;
    }

    const std::string& body() const noexcept
    {
        return body_;
    }

    void set_body(std::string body)
    {
        body_ = lux::move(body);
    }

    std::string_view header(std::string_view key) const
    {
        if (auto it = headers_.find(key); it != headers_.end())
        {
            return it->second;
        }
        return {};
    }

    void set_header(std::string key, std::string value)
    {
        headers_[lux::move(key)] = lux::move(value);
    }

    bool has_header(std::string_view key) const
    {
        return headers_.contains(key);
    }

    void remove_header(std::string_view key)
    {
        // NOTE: some compilers do not support erase with string_view directly yet
        auto eq_range = headers_.equal_range(key);
        headers_.erase(eq_range.first, eq_range.second);
    }

    const lux::string_unordered_map<std::string>& headers() const noexcept
    {
        return headers_;
    }

    http_response& ok(std::string_view body = {})
    {
        status_ = http_status::ok;
        if (!body.empty())
        {
            body_ = body;
        }
        return *this;
    }

    http_response& created(std::string_view body = {})
    {
        status_ = http_status::created;
        if (!body.empty())
        {
            body_ = body;
        }
        return *this;
    }

    http_response& no_content()
    {
        status_ = http_status::no_content;
        body_.clear();
        return *this;
    }

    http_response& bad_request(std::string_view body = {})
    {
        status_ = http_status::bad_request;
        if (!body.empty())
        {
            body_ = body;
        }
        return *this;
    }

    http_response& unauthorized(std::string_view body = {})
    {
        status_ = http_status::unauthorized;
        if (!body.empty())
        {
            body_ = body;
        }
        return *this;
    }

    http_response& forbidden(std::string_view body = {})
    {
        status_ = http_status::forbidden;
        if (!body.empty())
        {
            body_ = body;
        }
        return *this;
    }

    http_response& not_found(std::string_view body = {})
    {
        status_ = http_status::not_found;
        if (!body.empty())
        {
            body_ = body;
        }
        return *this;
    }

    http_response& internal_server_error(std::string_view body = {})
    {
        status_ = http_status::internal_server_error;
        if (!body.empty())
        {
            body_ = body;
        }
        return *this;
    }

    http_response& json(std::string_view json_body)
    {
        body_ = json_body;
        set_header("Content-Type", "application/json");
        return *this;
    }

    http_response& text(std::string_view text_body)
    {
        body_ = text_body;
        set_header("Content-Type", "text/plain");
        return *this;
    }

    http_response& html(std::string_view html_body)
    {
        body_ = html_body;
        set_header("Content-Type", "text/html");
        return *this;
    }

private:
    http_status status_ = http_status::ok;
    unsigned version_ = 11; // HTTP/1.1 by default
    lux::string_unordered_map<std::string> headers_;
    std::string body_;
};

} // namespace lux::net::base

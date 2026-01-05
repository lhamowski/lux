#pragma once

#include <lux/io/net/base/http_method.hpp>
#include <lux/support/assert.hpp>
#include <lux/support/container.hpp>
#include <lux/support/move.hpp>

#include <boost/url/parse.hpp>

#include <unordered_map>
#include <string>
#include <string_view>
#include <optional>

namespace lux::net::base {

class http_request
{
public:
    using headers_type = lux::string_unordered_map<std::string>;
    using query_params_type = lux::string_unordered_map<std::string>;

public:
    http_request() = default;

    http_request(http_method method, std::string_view target) : method_{method}, target_{target}
    {
    }

    http_method method() const noexcept
    {
        return method_;
    }

    void set_method(http_method method) noexcept
    {
        method_ = method;
    }

    const std::string& target() const noexcept
    {
        return target_;
    }

    void set_target(std::string target)
    {
        query_params_.reset(); // Invalidate existing query parameters
        target_ = lux::move(target);
    }

    unsigned version() const noexcept
    {
        return version_;
    }

    void set_version(unsigned version) noexcept
    {
        version_ = version;
    }

    const std::string& body() const noexcept
    {
        return body_;
    }

    void set_body(const std::string& body)
    {
        body_ = body;
    }

    void set_body(std::string&& body)
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

    void set_headers(const headers_type& headers)
    {
        headers_ = headers;
    }

    void set_headers(headers_type&& headers)
    {
        headers_ = lux::move(headers);
    }

    void set_header(std::string key, std::string value)
    {
        headers_[std::move(key)] = std::move(value);
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

    const headers_type& headers() const noexcept
    {
        return headers_;
    }

    const query_params_type& query_params() const
    {
        if (!query_params_)
        {
            // Lazy initialization
            construct_query_params();
        }

        LUX_ASSERT(query_params_, "Query parameters should be initialized");
        return *query_params_;
    }

private:
    void construct_query_params() const
    {
        LUX_ASSERT(!query_params_, "Query parameters already constructed");

        auto result = boost::urls::parse_origin_form(target_);
        if (!result)
        {
            query_params_ = query_params_type{};
            return;
        }

        const boost::urls::url_view url = result.value();
        const auto& boost_params = url.params();

        query_params_.emplace();
        query_params_->reserve(boost_params.size());
        for (const auto& param : boost_params)
        {
            query_params_->emplace(param.key, param.value);
        }
    }

private:
    http_method method_ = http_method::unknown;
    std::string target_;
    unsigned version_ = 11; // HTTP/1.1 by default
    headers_type headers_;
    mutable std::optional<query_params_type> query_params_; // Lazy initialized
    std::string body_;
};

} // namespace lux::net::base
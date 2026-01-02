#pragma once

#include <lux/support/move.hpp>

#include <memory>

namespace lux {

/**
 * @brief A RAII guard that tracks object lifetime and provides tokens for safe access from potentially detached
 * contexts.
 *
 * The lifetime_guard class provides a mechanism to safely check if an object is still alive
 * from callbacks, lambdas, or other potentially detached contexts. When the guard is destroyed,
 * all associated tokens will report the object as invalid.
 *
 * This is particularly useful for preventing dangling pointers in asynchronous operations,
 * event handlers, or when passing callbacks that might outlive the original object.
 *
 * @note The lifetime_guard cannot be copied or moved to ensure proper lifetime tracking.
 *
 * Example usage:
 * @code
 * class my_class
 * {
 * private:
 *     lifetime_guard guard_;
 *
 * public:
 *     void start_async_operation() {
 *         auto token = guard_.create_token();
 *
 *         // Start some async operation that captures the token
 *         async_operation([this, token](const auto& result) {
 *             if (token.is_valid())
 *             {
 *                 // Safe to access the my_class instance
 *             }
 *             else
 *             {
 *                 // Object may have been destroyed, skip processing
 *             }
 *         });
 *     }
 * };
 * @endcode
 */
class lifetime_guard
{
public:
    lifetime_guard() : valid_{std::make_shared<bool>(true)}
    {
    }

    ~lifetime_guard()
    {
        *valid_ = false;
    }

    lifetime_guard(const lifetime_guard&) = delete;
    lifetime_guard& operator=(const lifetime_guard&) = delete;
    lifetime_guard(lifetime_guard&&) = delete;
    lifetime_guard& operator=(lifetime_guard&&) = delete;

public:
    class token
    {
    private:
        friend class lifetime_guard;

    public:
        token(const token&) = default;
        token& operator=(const token&) = default;
        token(token&&) = default;
        token& operator=(token&&) = default;

    public:
        bool is_valid() const
        {
            if (const auto val = valid.lock())
            {
                return *val;
            }
            return false;
        }

    private:
        token(std::weak_ptr<bool> valid) : valid{lux::move(valid)}
        {
        }

        std::weak_ptr<bool> valid;
    };

public:
    /**
     * @brief Creates a token that can be used to check if the object is still valid.
     * @return A token that can be checked for validity.
     */
    token create_token() const
    {
        return token{valid_};
    }

private:
    std::shared_ptr<bool> valid_;
};
} // namespace lux
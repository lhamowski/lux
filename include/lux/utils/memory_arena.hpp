#pragma once

#include <lux/support/assert.hpp>
#include <lux/support/move.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stack>
#include <utility>
#include <vector>

namespace lux {

template <typename T>
class growable_memory_arena : public std::enable_shared_from_this<growable_memory_arena<T>>
{
private:
    class deleter
    {
    public:
        deleter(std::weak_ptr<growable_memory_arena<T>> arena) : arena_{lux::move(arena)}
        {
        }

        void operator()(T* mem) const
        {
            if (auto arena = arena_.lock())
            {
                arena->push(std::unique_ptr<T>(mem));
            }
            else
            {
                std::default_delete<T>{}(mem);
            }
        }

    private:
        std::weak_ptr<growable_memory_arena<T>> arena_;
    };

public:
    using element_type = std::unique_ptr<T, deleter>;

public:
    growable_memory_arena() = delete;
    growable_memory_arena(const growable_memory_arena&) = delete;
    growable_memory_arena& operator=(const growable_memory_arena&) = delete;
    growable_memory_arena(growable_memory_arena&&) = delete;
    growable_memory_arena& operator=(growable_memory_arena&&) = delete;

    static auto make(std::size_t init_size, std::size_t reserve_size)
    {
        return std::shared_ptr<growable_memory_arena<T>>(new growable_memory_arena<T>(init_size, reserve_size));
    }

public:
    auto get(std::size_t size)
    {
        if (arena_.empty())
        {
            push(std::make_unique<T>());
        }

        element_type mem{arena_.top().release(), deleter{this->weak_from_this()}};
        arena_.pop();
        mem->resize(size);
        return mem;
    }

private:
    explicit growable_memory_arena(std::size_t init_size, std::size_t reserve_size) : reserve_size_{reserve_size}
    {
        for (std::size_t i{}; i < init_size; ++i)
        {
            push(std::make_unique<T>());
        }
    }

    void push(std::unique_ptr<T> val)
    {
        val->reserve(reserve_size_);
        arena_.push(lux::move(val));
    }

private:
    std::stack<std::unique_ptr<T>> arena_;
    std::size_t reserve_size_{};
};

template <typename T = std::vector<std::byte>>
using growable_memory_arena_ptr = std::shared_ptr<growable_memory_arena<T>>;

template <typename T = std::vector<std::byte>>
auto make_growable_memory_arena(std::size_t init_size, std::size_t reserve_size)
{
    return growable_memory_arena<T>::make(init_size, reserve_size);
}

} // namespace lux
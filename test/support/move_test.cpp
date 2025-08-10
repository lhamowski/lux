#include <lux/support/move.hpp>

#include <catch2/catch_all.hpp>
#include <type_traits>

namespace {
class non_copyable
{
public:
    non_copyable() = default;

    non_copyable(const non_copyable&) = delete;
    non_copyable& operator=(const non_copyable&) = delete;

    non_copyable(non_copyable&&) noexcept
    {
        move_ctor_called = true;
    }

    non_copyable& operator=(non_copyable&&) noexcept = default;

    bool move_ctor_called = false;
};
} // namespace

TEST_CASE("Move utility", "[support][move]")
{
    SECTION("Move a non-const lvalue")
    {
        int x = 42;
        auto&& moved_x = lux::move(x);
        CHECK(moved_x == 42);
        STATIC_CHECK(std::is_same_v<decltype(moved_x), int&&>);
    }

    SECTION("Move a non-const rvalue")
    {
        int moved_x = lux::move(42);
        CHECK(moved_x == 42);
        STATIC_CHECK(std::is_same_v<decltype(lux::move(42)), int&&>);
    }

    SECTION("Move a non-copyable type")
    {
        non_copyable obj;
        non_copyable obj2{lux::move(obj)};
        CHECK(obj2.move_ctor_called);
        STATIC_CHECK(std::is_same_v<decltype(lux::move(obj)), non_copyable&&>);
    }
}

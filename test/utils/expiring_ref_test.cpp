#include <lux/utils/expiring_ref.hpp>

#include <catch2/catch_all.hpp>

#include <string>
#include <thread>
#include <vector>

namespace {

class test_object
{
public:
    test_object() = default;
    explicit test_object(int value) : value_{value}
    {
    }

    int get_value() const
    {
        return value_;
    }

    void set_value(int value)
    {
        value_ = value;
    }

    void increment()
    {
        ++value_;
    }

private:
    int value_{0};
};

} // namespace

TEST_CASE("expiring_ref: constructs and provides access to referenced object", "[expiring_ref][utils]")
{
    test_object obj{42};
    lux::expiring_ref<test_object> ref{obj};

    SECTION("Reference is valid after construction")
    {
        REQUIRE(ref.is_valid());
    }

    SECTION("Provides access to referenced object")
    {
        REQUIRE(ref.get().get_value() == 42);
    }

    SECTION("Allows modification of referenced object")
    {
        ref.get().set_value(100);
        REQUIRE(obj.get_value() == 100);
    }
}

TEST_CASE("expiring_ref: becomes invalid when invalidated", "[expiring_ref][utils]")
{
    test_object obj{10};
    lux::expiring_ref<test_object> ref{obj};

    SECTION("Transitions to invalid state after calling invalidate()")
    {
        REQUIRE(ref.is_valid());
        ref.invalidate();
        REQUIRE_FALSE(ref.is_valid());
    }

    SECTION("Multiple invalidate calls have no additional effect")
    {
        ref.invalidate();
        ref.invalidate();
        REQUIRE_FALSE(ref.is_valid());
    }
}

TEST_CASE("expiring_ref: shares validity state across copies", "[expiring_ref][utils]")
{
    test_object obj{5};
    lux::expiring_ref<test_object> ref1{obj};

    SECTION("Copies share validity state via copy constructor")
    {
        lux::expiring_ref<test_object> ref2{ref1};

        REQUIRE(ref1.is_valid());
        REQUIRE(ref2.is_valid());

        ref1.invalidate();

        REQUIRE_FALSE(ref1.is_valid());
        REQUIRE_FALSE(ref2.is_valid());
    }

    SECTION("Copies share validity state via copy assignment")
    {
        test_object another_obj{99};
        lux::expiring_ref<test_object> ref2{another_obj};

        ref2 = ref1;

        REQUIRE(ref1.is_valid());
        REQUIRE(ref2.is_valid());

        ref2.invalidate();

        REQUIRE_FALSE(ref1.is_valid());
        REQUIRE_FALSE(ref2.is_valid());
    }

    SECTION("All copies share the same validity state")
    {
        lux::expiring_ref<test_object> ref2{ref1};
        lux::expiring_ref<test_object> ref3{ref2};
        lux::expiring_ref<test_object> ref4{ref3};

        REQUIRE(ref1.is_valid());
        REQUIRE(ref2.is_valid());
        REQUIRE(ref3.is_valid());
        REQUIRE(ref4.is_valid());

        ref3.invalidate();

        REQUIRE_FALSE(ref1.is_valid());
        REQUIRE_FALSE(ref2.is_valid());
        REQUIRE_FALSE(ref3.is_valid());
        REQUIRE_FALSE(ref4.is_valid());
    }
}

TEST_CASE("expiring_ref: transfers validity state when moved", "[expiring_ref][utils]")
{
    test_object obj{7};
    lux::expiring_ref<test_object> ref1{obj};

    SECTION("Move constructor transfers validity state")
    {
        lux::expiring_ref<test_object> ref2{std::move(ref1)};

        REQUIRE(ref2.is_valid());
        ref2.invalidate();
        REQUIRE_FALSE(ref2.is_valid());
    }

    SECTION("Move assignment transfers validity state")
    {
        test_object another_obj{88};
        lux::expiring_ref<test_object> ref2{another_obj};

        ref2 = std::move(ref1);

        REQUIRE(ref2.is_valid());
        ref2.invalidate();
        REQUIRE_FALSE(ref2.is_valid());
    }
}

TEST_CASE("expiring_ref: provides consistent access across multiple copies", "[expiring_ref][utils]")
{
    test_object obj{0};
    lux::expiring_ref<test_object> ref1{obj};
    lux::expiring_ref<test_object> ref2{ref1};

    SECTION("Modifications are visible through all copies")
    {
        ref1.get().set_value(50);
        REQUIRE(ref2.get().get_value() == 50);

        ref2.get().increment();
        REQUIRE(ref1.get().get_value() == 51);
        REQUIRE(obj.get_value() == 51);
    }
}

TEST_CASE("expiring_ref: maintains const-correctness", "[expiring_ref][utils]")
{
    test_object obj{15};
    const lux::expiring_ref<test_object> ref{obj};

    SECTION("Const reference can check validity")
    {
        REQUIRE(ref.is_valid());
    }

    SECTION("Const reference can access object")
    {
        REQUIRE(ref.get().get_value() == 15);
    }
}

TEST_CASE("expiring_ref: supports various types", "[expiring_ref][utils]")
{
    SECTION("Works with std::string")
    {
        std::string str = "hello";
        lux::expiring_ref<std::string> ref{str};

        REQUIRE(ref.is_valid());
        REQUIRE(ref.get() == "hello");

        ref.get() += " world";
        REQUIRE(str == "hello world");

        ref.invalidate();
        REQUIRE_FALSE(ref.is_valid());
    }

    SECTION("Works with primitive types")
    {
        int value = 42;
        lux::expiring_ref<int> ref{value};

        REQUIRE(ref.is_valid());
        REQUIRE(ref.get() == 42);

        ref.get() = 100;
        REQUIRE(value == 100);

        ref.invalidate();
        REQUIRE_FALSE(ref.is_valid());
    }

    SECTION("Works with const types")
    {
        const int value = 42;
        lux::expiring_ref<const int> ref{value};

        REQUIRE(ref.is_valid());
        REQUIRE(ref.get() == 42);

        ref.invalidate();
        REQUIRE_FALSE(ref.is_valid());
    }
}

TEST_CASE("expiring_ref: supports thread-safe validity checking", "[expiring_ref][threading][utils]")
{
    test_object obj{0};
    lux::expiring_ref<test_object> ref{obj};

    SECTION("Concurrent validity checks are thread-safe")
    {
        std::vector<std::thread> threads;
        std::atomic<int> valid_count{0};

        for (int i = 0; i < 10; ++i)
        {
            threads.emplace_back([&ref, &valid_count]() {
                for (int j = 0; j < 1000; ++j)
                {
                    if (ref.is_valid())
                    {
                        ++valid_count;
                    }
                }
            });
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        ref.invalidate();

        for (auto& t : threads)
        {
            t.join();
        }

        REQUIRE_FALSE(ref.is_valid());
    }

    SECTION("Invalidation from multiple threads is thread-safe")
    {
        std::vector<std::thread> threads;
        std::vector<lux::expiring_ref<test_object>> refs;

        for (int i = 0; i < 5; ++i)
        {
            refs.push_back(ref);
        }

        for (size_t i = 0; i < refs.size(); ++i)
        {
            threads.emplace_back([&refs, i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(i * 2));
                refs[i].invalidate();
            });
        }

        for (auto& t : threads)
        {
            t.join();
        }

        REQUIRE_FALSE(ref.is_valid());
        for (const auto& r : refs)
        {
            REQUIRE_FALSE(r.is_valid());
        }
    }
}

TEST_CASE("expiring_ref: demonstrates common usage patterns", "[expiring_ref][utils]")
{
    SECTION("Check-before-use pattern prevents access to invalid reference")
    {
        test_object obj{42};
        lux::expiring_ref<test_object> ref{obj};

        if (ref.is_valid())
        {
            ref.get().set_value(100);
        }
        REQUIRE(obj.get_value() == 100);

        ref.invalidate();

        if (ref.is_valid())
        {
            ref.get().set_value(200);
        }
        REQUIRE(obj.get_value() == 100); // Should not have changed
    }

    SECTION("Sharing in callbacks allows controlled lifetime management")
    {
        test_object obj{0};
        lux::expiring_ref<test_object> ref{obj};

        auto callback1 = [ref]() mutable {
            if (ref.is_valid())
            {
                ref.get().increment();
            }
        };

        auto callback2 = [ref]() mutable {
            if (ref.is_valid())
            {
                ref.get().increment();
            }
        };

        callback1();
        callback2();
        REQUIRE(obj.get_value() == 2);

        ref.invalidate();

        callback1();
        callback2();
        REQUIRE(obj.get_value() == 2); // Should not have changed
    }
}

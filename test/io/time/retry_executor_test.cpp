#include "test_case.hpp"

#include <lux/io/time/retry_executor.hpp>

#include "mocks/timer_factory_mock.hpp"
#include "mocks/interval_timer_mock.hpp"

#include <catch2/catch_all.hpp>

#include <chrono>

using namespace lux::time;
using namespace lux::time::base;
using namespace test;

namespace {

retry_policy create_exponential_backoff_policy()
{
    retry_policy policy;
    policy.strategy = retry_policy::backoff_strategy::exponential_backoff;
    policy.max_attempts = 3;
    policy.base_delay = std::chrono::milliseconds{100};
    policy.max_delay = std::chrono::milliseconds{5000};
    return policy;
}

retry_policy create_fixed_delay_policy()
{
    retry_policy policy;
    policy.strategy = retry_policy::backoff_strategy::fixed_delay;
    policy.max_attempts = 3;
    policy.base_delay = std::chrono::milliseconds{100};
    policy.max_delay = std::chrono::milliseconds{5000};
    return policy;
}

retry_policy create_linear_backoff_policy()
{
    retry_policy policy;
    policy.strategy = retry_policy::backoff_strategy::linear_backoff;
    policy.max_attempts = 3;
    policy.base_delay = std::chrono::milliseconds{100};
    policy.max_delay = std::chrono::milliseconds{5000};
    return policy;
}

} // namespace

LUX_TEST_CASE("retry_executor", "creates timer and schedules retries with delays", "[io][time]")
{
    SECTION("Should create timer on construction")
    {
        timer_factory_mock factory;
        auto policy = create_exponential_backoff_policy();

        retry_executor executor{factory, policy};

        CHECK(factory.created_timers_.size() == 1);
        auto* timer_mock = factory.created_timers_[0];
        CHECK(timer_mock->set_handler_call_count() == 1);
    }

    SECTION("Should schedule timer with correct delay on first retry")
    {
        timer_factory_mock factory;
        auto policy = create_exponential_backoff_policy();
        retry_executor executor{factory, policy};

        bool retry_called = false;
        bool exhausted_called = false;

        executor.set_retry_action([&retry_called]() { retry_called = true; });
        executor.set_exhausted_callback([&exhausted_called]() { exhausted_called = true; });

        auto* timer_mock = factory.created_timers_[0];

        executor.retry();

        CHECK(timer_mock->schedule_call_count() == 1);
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{100});

        // Callbacks should be set but not called yet
        CHECK_FALSE(retry_called);
        CHECK_FALSE(exhausted_called);
    }
}

LUX_TEST_CASE("retry_executor", "applies fixed delay between retry attempts", "[io][time]")
{
    SECTION("Should use same delay for all attempts")
    {
        timer_factory_mock factory;
        auto policy = create_fixed_delay_policy();
        retry_executor executor{factory, policy};

        auto* timer_mock = factory.created_timers_[0];

        // First retry
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{100});

        // Simulate timer expiration - trigger retry action
        timer_mock->execute_handler();

        // Second retry
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{100});

        // Third retry
        timer_mock->execute_handler();
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{100});
    }
}

LUX_TEST_CASE("retry_executor", "applies linear backoff between retry attempts", "[io][time]")
{
    SECTION("Should increase delay linearly")
    {
        timer_factory_mock factory;
        auto policy = create_linear_backoff_policy();
        retry_executor executor{factory, policy};

        auto* timer_mock = factory.created_timers_[0];

        // First retry (attempts = 0)
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{100});

        // Simulate timer expiration
        timer_mock->execute_handler();

        // Second retry (attempts = 1)
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{100}); // 100 * 1

        // Simulate timer expiration
        timer_mock->execute_handler();

        // Third retry (attempts = 2)
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{200}); // 100 * 2
    }

    SECTION("Should cap delay at max_delay")
    {
        timer_factory_mock factory;
        auto policy = create_linear_backoff_policy();
        policy.base_delay = std::chrono::milliseconds{1000};
        policy.max_delay = std::chrono::milliseconds{1500};
        policy.max_attempts = std::nullopt; // Infinite attempts
        retry_executor executor{factory, policy};

        auto* timer_mock = factory.created_timers_[0];

        // Simulate many attempts to reach max_delay
        for (std::size_t i{}; i < 5; ++i)
        {
            timer_mock->execute_handler();
        }

        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{1500}); // Capped at max_delay
    }
}

LUX_TEST_CASE("retry_executor", "applies exponential backoff between retry attempts", "[io][time]")
{
    SECTION("Should increase delay exponentially")
    {
        timer_factory_mock factory;
        auto policy = create_exponential_backoff_policy();
        policy.base_delay = std::chrono::milliseconds{100};
        retry_executor executor{factory, policy};

        auto* timer_mock = factory.created_timers_[0];

        // First retry (attempts_ = 0)
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{100});

        timer_mock->execute_handler();

        // Second retry (attempts_ = 1, multiplier = 2^1 = 2)
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{200}); // 100 * 2

        timer_mock->execute_handler();

        // Third retry (attempts_ = 2, multiplier = 2^2 = 4)
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{400}); // 100 * 4

        timer_mock->execute_handler();
    }

    SECTION("Should cap delay at max_delay")
    {
        timer_factory_mock factory;
        auto policy = create_exponential_backoff_policy();
        policy.base_delay = std::chrono::milliseconds{1000};
        policy.max_delay = std::chrono::milliseconds{2500};
        policy.max_attempts = std::nullopt; // Infinite attempts
        retry_executor executor{factory, policy};

        auto* timer_mock = factory.created_timers_[0];

        // Simulate enough attempts to exceed max_delay
        for (int i = 0; i < 5; ++i)
        {
            timer_mock->execute_handler();
        }

        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{2500}); // Capped at max_delay
    }
}

LUX_TEST_CASE("retry_executor", "respects maximum retry attempts limit", "[io][time]")
{
    SECTION("Should call retry action up to max_attempts")
    {
        timer_factory_mock factory;
        auto policy = create_exponential_backoff_policy();
        policy.max_attempts = 3;
        retry_executor executor{factory, policy};

        auto* timer_mock = factory.created_timers_[0];

        std::size_t retry_call_count = 0;
        std::size_t exhausted_call_count = 0;

        executor.set_retry_action([&retry_call_count]() { ++retry_call_count; });
        executor.set_exhausted_callback([&exhausted_call_count]() { ++exhausted_call_count; });

        // First attempt
        executor.retry();
        timer_mock->execute_handler();
        CHECK(retry_call_count == 1);
        CHECK(exhausted_call_count == 0);

        // Second attempt
        executor.retry();
        timer_mock->execute_handler();
        CHECK(retry_call_count == 2);
        CHECK(exhausted_call_count == 0);

        // Third attempt
        executor.retry();
        timer_mock->execute_handler();
        CHECK(retry_call_count == 3);
        CHECK(exhausted_call_count == 1); // Exhausted callback should be called after max attempts
    }

    SECTION("Should handle max_attempts = std::nullopt (infinite retries)")
    {
        timer_factory_mock factory;
        auto policy = create_exponential_backoff_policy();
        policy.max_attempts = std::nullopt; // Infinite retries
        retry_executor executor{factory, policy};

        auto* timer_mock = factory.created_timers_[0];

        std::size_t retry_call_count = 0;
        bool exhausted_called = false;

        executor.set_retry_action([&retry_call_count]() { ++retry_call_count; });
        executor.set_exhausted_callback([&exhausted_called]() { exhausted_called = true; });

        // Simulate many attempts
        for (int i = 0; i < 1000; ++i)
        {
            executor.retry();
            timer_mock->execute_handler();
        }

        CHECK(retry_call_count == 1000);
        CHECK_FALSE(exhausted_called); // Should never be exhausted with max_attempts = 0
    }
}

LUX_TEST_CASE("retry_executor", "resets attempt counter and cancels timer", "[io][time]")
{
    SECTION("Should reset attempts counter and cancel timer")
    {
        timer_factory_mock factory;
        auto policy = create_exponential_backoff_policy();
        retry_executor executor{factory, policy};

        auto* timer_mock = factory.created_timers_[0];

        // Make some attempts
        executor.retry();
        timer_mock->execute_handler();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{100});

        executor.retry();
        timer_mock->execute_handler();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{200}); // 100 * 2

        // Reset should cancel timer and reset attempts
        executor.reset();
        CHECK(timer_mock->cancel_call_count() == 1);

        // After reset, delay should be back to base_delay
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{100});
    }
}

LUX_TEST_CASE("retry_executor", "cancels ongoing retry attempts", "[io][time]")
{
    SECTION("Should cancel ongoing retries")
    {
        timer_factory_mock factory;
        auto policy = create_exponential_backoff_policy();
        retry_executor executor{factory, policy};
        auto* timer_mock = factory.created_timers_[0];

        executor.retry();
        CHECK(timer_mock->schedule_call_count() == 1);

        executor.cancel();
        CHECK(timer_mock->cancel_call_count() == 1);
        // Further retries should not schedule the timer

        executor.retry();
        CHECK(timer_mock->schedule_call_count() == 1); // No new schedules

        executor.reset(); // Reset should allow retries again
        executor.retry();
        CHECK(timer_mock->schedule_call_count() == 2); // Now it should schedule again
    }
}

LUX_TEST_CASE("retry_executor", "edge cases", "[io][time]")
{
    SECTION("Should handle zero base delay")
    {
        timer_factory_mock factory;
        auto policy = create_exponential_backoff_policy();
        policy.base_delay = std::chrono::milliseconds{0};
        retry_executor executor{factory, policy};

        std::size_t retry_call_count = 0;
        bool exhausted_called = false;

        executor.set_retry_action([&retry_call_count]() { ++retry_call_count; });
        executor.set_exhausted_callback([&exhausted_called]() { exhausted_called = true; });

        auto* timer_mock = factory.created_timers_[0];

        executor.retry();

        // Should immediately call retry action without delay
        CHECK(retry_call_count == 1);
        CHECK(exhausted_called == false);

        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{0});
    }

    SECTION("Should handle base_delay greater than max_delay")
    {
        timer_factory_mock factory;
        auto policy = create_exponential_backoff_policy();
        policy.base_delay = std::chrono::milliseconds{1000};
        policy.max_delay = std::chrono::milliseconds{500};
        retry_executor executor{factory, policy};

        auto* timer_mock = factory.created_timers_[0];

        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{500}); // Capped at max_delay
    }
}

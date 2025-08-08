#include <lux/io/time/delayed_retry_executor.hpp>

#include "mocks/timer_factory_mock.hpp"
#include "mocks/interval_timer_mock.hpp"

#include <catch2/catch_all.hpp>

#include <chrono>

using namespace lux::time;
using namespace test;

namespace {

delayed_retry_config create_exponential_backoff_config()
{
    delayed_retry_config config;
    config.strategy = delayed_retry_config::backoff_strategy::exponential_backoff;
    config.max_attempts = 3;
    config.base_delay = std::chrono::milliseconds{100};
    config.max_delay = std::chrono::milliseconds{5000};
    return config;
}

delayed_retry_config create_fixed_delay_config()
{
    delayed_retry_config config;
    config.strategy = delayed_retry_config::backoff_strategy::fixed_delay;
    config.max_attempts = 3;
    config.base_delay = std::chrono::milliseconds{100};
    config.max_delay = std::chrono::milliseconds{5000};
    return config;
}

delayed_retry_config create_linear_backoff_config()
{
    delayed_retry_config config;
    config.strategy = delayed_retry_config::backoff_strategy::linear_backoff;
    config.max_attempts = 3;
    config.base_delay = std::chrono::milliseconds{100};
    config.max_delay = std::chrono::milliseconds{5000};
    return config;
}

} // namespace

TEST_CASE("Delayed retry executor - Basic functionality", "[io][time]")
{
    SECTION("Should create timer on construction")
    {
        timer_factory_mock factory;
        auto config = create_exponential_backoff_config();

        delayed_retry_executor executor{factory, config};

        CHECK(factory.created_timers_.size() == 1);
        auto* timer_mock = factory.created_timers_[0];
        CHECK(timer_mock->set_handler_call_count() == 1);
    }

    SECTION("Should schedule timer with correct delay on first retry")
    {
        timer_factory_mock factory;
        auto config = create_exponential_backoff_config();
        delayed_retry_executor executor{factory, config};

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

TEST_CASE("Delayed retry executor - Fixed delay strategy", "[io][time]")
{
    SECTION("Should use same delay for all attempts")
    {
        timer_factory_mock factory;
        auto config = create_fixed_delay_config();
        delayed_retry_executor executor{factory, config};

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

TEST_CASE("Delayed retry executor - Linear backoff strategy", "[io][time]")
{
    SECTION("Should increase delay linearly")
    {
        timer_factory_mock factory;
        auto config = create_linear_backoff_config();
        delayed_retry_executor executor{factory, config};

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
        auto config = create_linear_backoff_config();
        config.base_delay = std::chrono::milliseconds{1000};
        config.max_delay = std::chrono::milliseconds{1500};
        delayed_retry_executor executor{factory, config};

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

TEST_CASE("Delayed retry executor - Exponential backoff strategy", "[io][time]")
{
    SECTION("Should increase delay exponentially")
    {
        timer_factory_mock factory;
        auto config = create_exponential_backoff_config();
        config.base_delay = std::chrono::milliseconds{100};
        delayed_retry_executor executor{factory, config};

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

        // Fourth retry (attempts_ = 3, multiplier = 2^3 = 8)
        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{800}); // 100 * 8
    }

    SECTION("Should cap delay at max_delay")
    {
        timer_factory_mock factory;
        auto config = create_exponential_backoff_config();
        config.base_delay = std::chrono::milliseconds{1000};
        config.max_delay = std::chrono::milliseconds{2500};
        delayed_retry_executor executor{factory, config};

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

TEST_CASE("Delayed retry executor - Max attempts behavior", "[io][time]")
{
    SECTION("Should call retry action up to max_attempts")
    {
        timer_factory_mock factory;
        auto config = create_exponential_backoff_config();
        config.max_attempts = 3;
        delayed_retry_executor executor{factory, config};

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
        CHECK(exhausted_call_count == 0);

        // Fourth attempt should trigger exhausted callback
        executor.retry();
        timer_mock->execute_handler();
        CHECK(retry_call_count == 3); // No more retry calls
        CHECK(exhausted_call_count == 1);
    }

    SECTION("Should handle max_attempts = 0 (infinite retries)")
    {
        timer_factory_mock factory;
        auto config = create_exponential_backoff_config();
        config.max_attempts = 0;
        delayed_retry_executor executor{factory, config};

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

TEST_CASE("Delayed retry executor - Reset functionality", "[io][time]")
{
    SECTION("Should reset attempts counter and cancel timer")
    {
        timer_factory_mock factory;
        auto config = create_exponential_backoff_config();
        delayed_retry_executor executor{factory, config};

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

TEST_CASE("Delayed retry executor - Edge cases", "[io][time]")
{
    SECTION("Should handle zero base delay")
    {
        timer_factory_mock factory;
        auto config = create_exponential_backoff_config();
        config.base_delay = std::chrono::milliseconds{0};
        delayed_retry_executor executor{factory, config};

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
        auto config = create_exponential_backoff_config();
        config.base_delay = std::chrono::milliseconds{1000};
        config.max_delay = std::chrono::milliseconds{500};
        delayed_retry_executor executor{factory, config};

        auto* timer_mock = factory.created_timers_[0];

        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{500}); // Capped at max_delay
    }

    SECTION("Should handle overflow in exponential backoff")
    {
        timer_factory_mock factory;
        auto config = create_exponential_backoff_config();
        config.base_delay = std::chrono::milliseconds{1000};
        config.max_delay = std::chrono::milliseconds{5000};
        delayed_retry_executor executor{factory, config};

        auto* timer_mock = factory.created_timers_[0];

        // Simulate many attempts to trigger overflow protection
        for (int i = 0; i < 20; ++i)
        {
            timer_mock->execute_handler();
        }

        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{5000}); // Should be capped
    }

    SECTION("Should handle overflow in linear backoff")
    {
        timer_factory_mock factory;
        auto config = create_linear_backoff_config();
        config.base_delay = std::chrono::milliseconds{1000};
        config.max_delay = std::chrono::milliseconds{3000};
        delayed_retry_executor executor{factory, config};

        auto* timer_mock = factory.created_timers_[0];

        // Simulate many attempts
        for (int i = 0; i < 10000; ++i)
        {
            timer_mock->execute_handler();
        }

        executor.retry();
        CHECK(timer_mock->scheduled_delay() == std::chrono::milliseconds{3000}); // Should be capped
    }
}
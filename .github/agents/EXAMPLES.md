# Test Generator Agent - Usage Examples

This document provides practical examples of using the Test Generator agent to create tests for components in the Lux library.

## Example 1: Testing a Simple Value Type

### Component: `circular_buffer` (hypothetical)

Given a component header like this:

```cpp
// include/lux/utils/circular_buffer.hpp
namespace lux {

template<typename T, std::size_t Capacity>
class circular_buffer
{
public:
    circular_buffer() = default;
    
    void push(const T& value);
    std::optional<T> pop();
    
    bool empty() const;
    bool full() const;
    std::size_t size() const;
    std::size_t capacity() const;
    void clear();
};

} // namespace lux
```

### Generated Test (following test-generator conventions):

```cpp
// test/utils/circular_buffer_test.cpp
#include "test_case.hpp"

#include <lux/utils/circular_buffer.hpp>

#include <catch2/catch_all.hpp>

LUX_TEST_CASE("circular_buffer", "constructs with zero size", "[utils][circular_buffer]")
{
    SECTION("Buffer is empty after default construction")
    {
        lux::circular_buffer<int, 10> buffer;
        
        CHECK(buffer.empty());
        CHECK_FALSE(buffer.full());
        CHECK(buffer.size() == 0);
        CHECK(buffer.capacity() == 10);
    }
}

LUX_TEST_CASE("circular_buffer", "stores and retrieves elements in FIFO order", "[utils][circular_buffer]")
{
    SECTION("Single element push and pop")
    {
        lux::circular_buffer<int, 5> buffer;
        
        buffer.push(42);
        
        CHECK(buffer.size() == 1);
        CHECK_FALSE(buffer.empty());
        
        const auto value = buffer.pop();
        REQUIRE(value.has_value());
        CHECK(*value == 42);
        CHECK(buffer.empty());
    }
    
    SECTION("Multiple elements maintain FIFO order")
    {
        lux::circular_buffer<int, 5> buffer;
        
        buffer.push(1);
        buffer.push(2);
        buffer.push(3);
        
        CHECK(buffer.size() == 3);
        CHECK(*buffer.pop() == 1);
        CHECK(*buffer.pop() == 2);
        CHECK(*buffer.pop() == 3);
    }
}

LUX_TEST_CASE("circular_buffer", "handles capacity limits correctly", "[utils][circular_buffer]")
{
    SECTION("Buffer becomes full at capacity")
    {
        lux::circular_buffer<int, 3> buffer;
        
        buffer.push(1);
        buffer.push(2);
        buffer.push(3);
        
        CHECK(buffer.full());
        CHECK(buffer.size() == 3);
    }
    
    SECTION("Overwrites oldest element when pushing to full buffer")
    {
        lux::circular_buffer<int, 3> buffer;
        
        buffer.push(1);
        buffer.push(2);
        buffer.push(3);
        buffer.push(4); // Overwrites 1
        
        CHECK(*buffer.pop() == 2);
        CHECK(*buffer.pop() == 3);
        CHECK(*buffer.pop() == 4);
        CHECK(buffer.empty());
    }
}

LUX_TEST_CASE("circular_buffer", "returns empty optional when popping from empty buffer", "[utils][circular_buffer]")
{
    SECTION("Pop on empty buffer returns nullopt")
    {
        lux::circular_buffer<int, 5> buffer;
        
        const auto value = buffer.pop();
        
        CHECK_FALSE(value.has_value());
    }
}

LUX_TEST_CASE("circular_buffer", "clears all elements successfully", "[utils][circular_buffer]")
{
    SECTION("Clear empties buffer with elements")
    {
        lux::circular_buffer<int, 5> buffer;
        
        buffer.push(1);
        buffer.push(2);
        buffer.push(3);
        
        REQUIRE(buffer.size() == 3);
        
        buffer.clear();
        
        CHECK(buffer.empty());
        CHECK(buffer.size() == 0);
    }
    
    SECTION("Clear on empty buffer has no effect")
    {
        lux::circular_buffer<int, 5> buffer;
        
        buffer.clear();
        
        CHECK(buffer.empty());
    }
}
```

### Key Patterns Demonstrated:

1. **Test Naming**: Each TEST_CASE clearly describes a specific behavior
   - "constructs with zero size"
   - "stores and retrieves elements in FIFO order"
   - "handles capacity limits correctly"

2. **Section Organization**: Logical grouping within each TEST_CASE
   - Construction and initial state
   - Normal operations
   - Edge cases (capacity limits)
   - Error conditions (empty buffer)
   - State management (clear)

3. **Code Style**:
   - Uniform initialization: `lux::circular_buffer<int, 5> buffer;`
   - Const correctness: `const auto value = buffer.pop();`
   - Clear variable names: `buffer`, `value`, not `buf` or `val`

4. **Assertions**:
   - REQUIRE for preconditions: `REQUIRE(buffer.size() == 3);`
   - CHECK for test assertions: `CHECK(buffer.empty());`
   - CHECK_FALSE for negative assertions: `CHECK_FALSE(buffer.full());`

## Example 2: Testing an Async I/O Component

### Component: `deadline_timer` (hypothetical)

```cpp
// include/lux/io/time/deadline_timer.hpp
namespace lux::time {

class deadline_timer
{
public:
    explicit deadline_timer(boost::asio::any_io_executor executor);
    
    void set_handler(std::function<void()> handler);
    void expires_after(std::chrono::milliseconds duration);
    void cancel();
    bool is_active() const;
};

} // namespace lux::time
```

### Generated Test:

```cpp
// test/io/time/deadline_timer_test.cpp
#include "test_case.hpp"

#include <lux/io/time/deadline_timer.hpp>

#include <catch2/catch_all.hpp>

#include <chrono>

LUX_TEST_CASE("deadline_timer", "constructs successfully with executor", "[io][time]")
{
    SECTION("Timer is inactive after construction")
    {
        boost::asio::io_context io_context;
        lux::time::deadline_timer timer{io_context.get_executor()};
        
        CHECK_FALSE(timer.is_active());
    }
}

LUX_TEST_CASE("deadline_timer", "executes handler after specified duration", "[io][time]")
{
    SECTION("Handler called once after timeout")
    {
        boost::asio::io_context io_context;
        lux::time::deadline_timer timer{io_context.get_executor()};
        
        bool handler_called{false};
        timer.set_handler([&handler_called, &io_context] {
            handler_called = true;
            io_context.stop();
        });
        
        timer.expires_after(std::chrono::milliseconds{10});
        CHECK(timer.is_active());
        
        io_context.run_for(std::chrono::milliseconds{50});
        
        CHECK(handler_called);
        CHECK_FALSE(timer.is_active());
    }
}

LUX_TEST_CASE("deadline_timer", "cancels pending timeout successfully", "[io][time]")
{
    SECTION("Handler not called after cancellation")
    {
        boost::asio::io_context io_context;
        lux::time::deadline_timer timer{io_context.get_executor()};
        
        bool handler_called{false};
        timer.set_handler([&handler_called] {
            handler_called = true;
        });
        
        timer.expires_after(std::chrono::milliseconds{50});
        timer.cancel();
        
        CHECK_FALSE(timer.is_active());
        
        io_context.run_for(std::chrono::milliseconds{100});
        
        CHECK_FALSE(handler_called);
    }
    
    SECTION("Multiple cancellations have no adverse effect")
    {
        boost::asio::io_context io_context;
        lux::time::deadline_timer timer{io_context.get_executor()};
        
        timer.expires_after(std::chrono::milliseconds{50});
        
        REQUIRE_NOTHROW(timer.cancel());
        REQUIRE_NOTHROW(timer.cancel());
        
        CHECK_FALSE(timer.is_active());
    }
}
```

### Key Patterns for Async Components:

1. **Setup**: Create `io_context` and component instances
2. **Callbacks**: Use lambdas with capture for handler testing
3. **Control Flow**: Use `io_context.stop()` to control async execution
4. **Timeouts**: Use `run_for()` with generous timeouts for test stability
5. **State Verification**: Check component state before and after operations

## Example 3: Testing with Mock Dependencies

### Component Using Interfaces:

```cpp
// include/lux/io/net/connection_pool.hpp
namespace lux::net {

class connection_factory
{
public:
    virtual ~connection_factory() = default;
    virtual std::unique_ptr<connection> create() = 0;
};

class connection_pool
{
public:
    explicit connection_pool(connection_factory& factory, std::size_t max_size);
    
    lux::result<connection*> acquire();
    void release(connection* conn);
    std::size_t available() const;
    std::size_t in_use() const;
};

} // namespace lux::net
```

### Generated Test with Mock:

```cpp
// test/io/net/connection_pool_test.cpp
#include "test_case.hpp"

#include <lux/io/net/connection_pool.hpp>

#include <catch2/catch_all.hpp>

#include <memory>
#include <vector>

namespace {

class mock_connection : public lux::net::connection
{
public:
    explicit mock_connection(int id) : id_{id}
    {
    }
    
    int id() const { return id_; }
    
private:
    int id_{0};
};

class mock_connection_factory : public lux::net::connection_factory
{
public:
    std::unique_ptr<lux::net::connection> create() override
    {
        auto conn = std::make_unique<mock_connection>(next_id_++);
        created_connections_.push_back(conn.get());
        return conn;
    }
    
    std::vector<lux::net::connection*> created_connections_;
    int next_id_{0};
};

} // namespace

LUX_TEST_CASE("connection_pool", "creates connections using factory", "[io][net]")
{
    SECTION("Acquires new connection from factory on first request")
    {
        mock_connection_factory factory;
        lux::net::connection_pool pool{factory, 5};
        
        const auto result = pool.acquire();
        
        REQUIRE(result.has_value());
        CHECK(factory.created_connections_.size() == 1);
        CHECK(pool.in_use() == 1);
        CHECK(pool.available() == 0);
    }
}

LUX_TEST_CASE("connection_pool", "reuses released connections", "[io][net]")
{
    SECTION("Released connection available for reuse")
    {
        mock_connection_factory factory;
        lux::net::connection_pool pool{factory, 5};
        
        auto result1 = pool.acquire();
        REQUIRE(result1.has_value());
        lux::net::connection* conn1 = *result1;
        
        pool.release(conn1);
        CHECK(pool.available() == 1);
        CHECK(pool.in_use() == 0);
        
        auto result2 = pool.acquire();
        REQUIRE(result2.has_value());
        
        // Same connection should be reused
        CHECK(*result2 == conn1);
        CHECK(factory.created_connections_.size() == 1); // No new connection created
    }
}

LUX_TEST_CASE("connection_pool", "enforces maximum pool size", "[io][net]")
{
    SECTION("Returns error when pool exhausted")
    {
        mock_connection_factory factory;
        lux::net::connection_pool pool{factory, 2};
        
        auto result1 = pool.acquire();
        auto result2 = pool.acquire();
        
        REQUIRE(result1.has_value());
        REQUIRE(result2.has_value());
        
        auto result3 = pool.acquire();
        
        CHECK_FALSE(result3.has_value());
        CHECK(result3.error().str() == "Connection pool exhausted (max_size=2)\n");
    }
}
```

## Usage Tips

1. **Study Existing Tests First**: Before generating tests for a new component, look at tests for similar components in the codebase

2. **Match the Module Structure**: 
   - `include/lux/utils/component.hpp` → `test/utils/component_test.cpp`
   - Use matching tags: `[utils]` for utils module, `[io][time]` for io/time module

3. **One Behavior Per TEST_CASE**: Each TEST_CASE should test one specific aspect or behavior of the component

4. **Progressive Complexity**: Start with construction, then basic operations, then edge cases, then error conditions

5. **Clear, Specific Names**: Test names should be specific enough that you can understand what failed just from the test name

6. **Use Helper Functions**: When setup is complex, extract to helper functions in anonymous namespace

## Common Mistakes to Avoid

❌ **Don't**: Generic test names
```cpp
LUX_TEST_CASE("buffer", "works correctly", "[utils]")  // Too vague
```

✅ **Do**: Specific test names
```cpp
LUX_TEST_CASE("buffer_reader", "reads primitive types in sequence", "[utils][buffer_reader]")
```

❌ **Don't**: Mix multiple unrelated behaviors
```cpp
SECTION("Buffer operations")  // Tests both read and write
```

✅ **Do**: Separate behaviors
```cpp
SECTION("Reads uint32 values successfully")
SECTION("Writes uint32 values successfully")
```

❌ **Don't**: Use copy initialization when uniform initialization is appropriate
```cpp
int count = 0;  // Old style
```

✅ **Do**: Use uniform initialization
```cpp
int count{0};  // Modern C++
```

## Integrating with GitHub Copilot

When using GitHub Copilot in VS Code or other IDEs:

1. Open the component header you want to test
2. Create a new test file in the appropriate directory
3. Start typing and let Copilot suggest tests based on the test-generator agent patterns
4. Reference the agent explicitly in chat: "Generate tests following the test-generator agent conventions"

The agent ensures consistency and reduces manual corrections, making test writing faster and more reliable.

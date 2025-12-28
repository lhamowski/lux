#pragma once

#include <lux/support/assert.hpp>

#include <atomic>
#include <functional>
#include <memory>

namespace lux {

/**
 * @brief A reference wrapper with lifetime tracking that provides safe access to a resource.
 *
 * The expiring_ref
 * class encapsulates a reference to an object along with a shared validity state.
 * It ensures that the referenced
 * object is only accessed when valid, preventing use-after-free errors
 * in asynchronous or multi-threaded contexts.
 * The reference can "expire" when invalidated, after which
 * all access attempts will fail.
 *
 * This is particularly
 * useful when multiple components need to share access to a resource that may
 * be invalidated at any time, such as
 * handlers in asynchronous servers or callbacks that outlive
 * the original object.
 *
 * @tparam T The type of the
 * referenced object.
 *
 * @note The expiring_ref uses thread-safe atomic operations for validity tracking.
 * @note
 * The expiring_ref can be copied to share the same validity state across multiple instances.
 *
 * Example usage:
 *
 * @code
 * class handler
 * {
 * public:
 *     void on_event() { }
 * };
 *
 * handler h;
 * expiring_ref<handler>
 * ref{h};
 *
 * // Share the expiring reference
 * auto copy = ref;
 *
 * // Later, expire the reference
 *
 * ref.invalidate();
 *
 * // Check before use
 * if (copy.is_valid())
 * {
 *     copy.get().on_event();
 * }
 *
 * @endcode
 */
template <typename T>
class expiring_ref
{
public:
    explicit expiring_ref(T& ref) : ref_{ref}, is_valid_{std::make_shared<std::atomic_bool>(true)}
    {
    }

    expiring_ref(const expiring_ref&) = default;
    expiring_ref& operator=(const expiring_ref&) = default;
    expiring_ref(expiring_ref&&) = default;
    expiring_ref& operator=(expiring_ref&&) = default;

public:
    /**
     * @brief Invalidates the reference, preventing further access.
     *
     * After calling this method, all copies of this expiring_ref will report as invalid.
     *
     * @note This operation is thread-safe.
     */
    void invalidate() noexcept
    {
        is_valid_->store(false, std::memory_order_release);
    }

    /**
     * @brief Checks if the reference is still valid.
     *
     * @return true if the reference is valid, false otherwise.
     *
     * @note This operation is thread-safe.
     */
    bool is_valid() const noexcept
    {
        return is_valid_->load(std::memory_order_acquire);
    }

    /**
     * @brief Returns a reference to the guarded object.
     *
     * @return A reference to the object.
     *
     * @pre is_valid() must return true. This is checked with assertions.
     */
    T& get() const noexcept
    {
        LUX_ASSERT(is_valid(), "Expiring reference must be valid");
        return ref_.get();
    }

private:
    std::reference_wrapper<T> ref_;
    std::shared_ptr<std::atomic_bool> is_valid_;
};

} // namespace lux

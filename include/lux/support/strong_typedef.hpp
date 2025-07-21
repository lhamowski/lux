#pragma once

namespace lux {

template <typename T, typename Tag>
class strong_typedef
{
public:
    using value_type = T;

public:
    explicit strong_typedef(T value) : value_(value) {}

public:
    operator T&() const { return value_; }
    operator const T&() const { return value_; }
    auto operator<=>(const strong_typedef&) const = default;

public:
    T& get() { return value_; }
    const T& get() const { return value_; }

private:
    T value_;
};

} // namespace lux

// Macro for easy typedef creation
#define LUX_STRONG_TYPEDEF(name, type)                                                                                           \
    struct name##_lux_tag                                                                                                        \
    {                                                                                                                            \
    };                                                                                                                           \
    using name = ::lux::strong_typedef<type, name##_lux_tag>;
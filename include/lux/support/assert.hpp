#pragma once

#include <cassert>

#define LUX_ASSERT(condition, message) assert((condition) && (message))

#if defined(__GNUC__) || defined(__clang__)
    #define LUX_BUILTIN_UNREACHABLE() __builtin_unreachable()
#elif defined(_MSC_VER)
    #define LUX_BUILTIN_UNREACHABLE() __assume(false)
#else
    #define LUX_BUILTIN_UNREACHABLE() ((void)0)
#endif

#define LUX_UNREACHABLE()                                                                                                        \
    do                                                                                                                           \
    {                                                                                                                            \
        LUX_BUILTIN_UNREACHABLE();                                                                                               \
    } while (false)

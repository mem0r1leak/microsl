#pragma once
#include "types.hpp"

#if defined(__clang__) || defined(__GNUC__)
#define MSL_BUILTIN_MEMCPY(dest, src, count) __builtin_memcpy(dest, src, count)
#elif defined(_MSC_VER)
#define MSL_BUILTIN_MEMCPY(dest, src, count) __builtin_memcpy(dest, src, count)
#else
#define MSL_BUILTIN_MEMCPY(dest, src, count) \
    do { \
    auto* d = static_cast<unsigned char*>(dest); \
    const auto* s = static_cast<const unsigned char*>(src); \
    for (msl::types::usize i = 0; i < (count); ++i) d[i] = s[i]; \
    } while(0)
#endif

namespace msl::builtin {
#if defined(_MSC_VER)
    extern "C" unsigned char _BitScanForward(unsigned long *_Index, unsigned long _Mask);
#endif

    [[noreturn]] inline void unreachable() noexcept {
#if defined(__clang__) || defined(__GNUC__)
        __builtin_unreachable();
#elif defined(_MSC_VER)
        __assume(0);
#endif
    }

    template<typename To, typename From>
        requires (sizeof(To) == sizeof(From)) &&
                 types::concepts::trivially_copyable<To> &&
                 types::concepts::trivially_copyable<From>
    [[nodiscard]] constexpr To bit_cast(const From &src) noexcept {
        static_assert(sizeof(To) == sizeof(From), "bit_cast source and destination sizes must be equal");

        To dst;
        __builtin_memcpy(&dst, &src, sizeof(To));
        return dst;
    }

    inline unsigned int ctz(const unsigned int value) noexcept {
        if (value == 0) return 32;

#if defined(__clang__) || defined(__GNUC__)
        return static_cast<unsigned int>(__builtin_ctz(value));
#elif defined(_MSC_VER)
        unsigned long index = 0;
        _BitScanForward(&index, value);
        return static_cast<unsigned int>(index);
#else
#error Unsupported compiler
#endif
    }
}

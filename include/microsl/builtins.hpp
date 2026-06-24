#pragma once

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

    inline unsigned int ctz(const unsigned int value) noexcept {
        if (value == 0) return 32;

#if defined(__clang__) || defined(__GNUC__)
        return static_cast<unsigned int>(__builtin_ctz(value));
#elif defined(_MSC_VER)
        unsigned long index = 0;
        // _BitScanForward повертає 0, якщо маска порожня (ми це вже перевірили вище)
        _BitScanForward(&index, value);
        return static_cast<unsigned int>(index);
#else
#error Unsupported compiler
#endif
    }
}

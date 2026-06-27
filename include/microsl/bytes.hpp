#pragma once
#include "types.hpp"

using namespace msl::types;

namespace msl::bytes {
    enum class Endian : u8 {
        Little,
        Big
    };

    constexpr Endian get_native_endian() noexcept {
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
        return Endian::Big;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        return Endian::Little;
#elif defined(__LITTLE_ENDIAN__) || defined(_LITTLE_ENDIAN)
        return Endian::Little;
#elif defined(__BIG_ENDIAN__) || defined(_BIG_ENDIAN)
        return Endian::Big;
#elif defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86) || defined(__ARMEL__) || defined(__AARCH64EL__)
        return Endian::Little;
#elif defined(_WIN32)
        return Endian::Little;
#else
#error "microsl: Unknown target platform architecture (endianness)"
#endif
    }

    static constexpr Endian NativeEndian = get_native_endian();

    template<typename T>
    constexpr T swap(T value) {
        if constexpr (sizeof(T) == 2) {
            return static_cast<T>(__builtin_bswap16(static_cast<u16>(value)));
        } else if constexpr (sizeof(T) == 4) {
            return static_cast<T>(__builtin_bswap32(static_cast<u32>(value)));
        } else if constexpr (sizeof(T) == 8) {
            return static_cast<T>(__builtin_bswap64(static_cast<u64>(value)));
        } else {
            static_assert(false);
        }
        return value;
    }

    template<typename T, Endian endian>
    constexpr bool need_swap() noexcept {
        return endian != NativeEndian and sizeof(T) > 1;
    }
}

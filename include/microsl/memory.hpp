#pragma once
#include "types.hpp"

namespace msl::mem {
    template<typename T, typename... Args>
    constexpr T *construct(T *ptr, Args &&... args) noexcept {
        if (!ptr) return nullptr;
        return ::new(static_cast<void *>(ptr)) T(static_cast<Args &&>(args)...);
    }

    template<typename T>
    constexpr T &&move(T &val) noexcept {
        return static_cast<T &&>(val);
    }

    template<typename T>
    constexpr void destruct(T *ptr) noexcept {
        if (ptr) {
            ptr->~T();
        }
    }

    template<typename T>
    constexpr void destruct(T *ptr, const types::usize count) noexcept {
        if (!ptr) return;
        for (types::usize i = 0; i < count; ++i) {
            ptr[i].~T();
        }
    }
}

// Doesn't do any allocation. Just to silence the compiler.
inline void* operator new(msl::types::usize, void* __p) noexcept {
    return __p;
}

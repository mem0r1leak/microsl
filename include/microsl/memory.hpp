#pragma once
#include "builtins.hpp"
#include "types.hpp"

namespace msl::mem {
    /**
     * @brief Constructs an object of type T at the specified memory location (Placement New).
     * @note Does not allocate memory; it only invokes the constructor on an already existing buffer.
     * The static_cast<void*> bypasses any class-specific operator new overloads.
     */
    template<typename T, typename... Args>
    constexpr T *construct(T *ptr, Args &&... args) noexcept {
        if (!ptr) return nullptr;
        return ::new(static_cast<void *>(ptr)) T(static_cast<Args &&>(args)...);
    }

    /**
     * @brief Casts an lvalue reference to an rvalue reference to enable move semantics.
     * @note This is a pure compile-time type cast. It carries zero runtime overhead
     * and generates no CPU instructions.
     */
    template<typename T>
    constexpr T &&move(T &val) noexcept {
        return static_cast<T &&>(val);
    }

    /**
     * @brief Explicitly invokes the destructor for a single object.
     * @note Used for manual cleanup of objects initialized via mem::construct.
     */
    template<typename T>
    constexpr void destruct(T *ptr) noexcept {
        if (ptr) {
            ptr->~T();
        }
    }

    /**
     * @brief Explicitly invokes destructors for a contiguous array of objects.
     * @note Destruction is performed sequentially in forward index order.
     */
    template<typename T>
    constexpr void destruct(T *ptr, const types::usize count) noexcept {
        if (!ptr) return;
        for (types::usize i = 0; i < count; ++i) {
            ptr[i].~T();
        }
    }

    /**
     * @brief Runtime bulk byte copy between non-overlapping memory blocks.
     * @param dst Pointer to the destination memory buffer.
     * @param src Pointer to the source data buffer.
     * @param count Total number of bytes to copy.
     * @note Strictly for trivially copyable types (pods, primitives, raw blocks).
     */
    inline void copy(void* dst, const void* src, const types::usize count) noexcept {
        MSL_BUILTIN_MEMCPY(dst, src, count);
    }

    /**
     * @brief Compile-time fixed-size byte copy.
     * @tparam N Exact number of bytes to copy known at compile time.
     * @param dst Pointer to the destination memory buffer.
     * @param src Pointer to the source data buffer.
     * @note Maximizes performance via compiler intrinsics, enabling loop unrolling and SIMD vectorization.
     */
    template<types::usize N>
    void copy(void* dst, const void* src) noexcept {
        MSL_BUILTIN_MEMCPY(dst, src, N);
    }

    /**
     * @brief Compile-time fixed-size copy for complex, non-trivial objects.
     * @tparam T Type of elements being copied.
     * @tparam N Static count of objects in the contiguous sequence.
     * @param dst Pointer to the destination array storage.
     * @param src Pointer to the source array data.
     * @note Invokes copy constructor via placement new at destination layout.
     * Triggers a compilation error if used on trivial types to enforce optimization. @see copy
     */
    template<typename T, types::usize N>
    constexpr void obj_copy(T* dst, const T* src) noexcept {
        static_assert(!msl::types::concepts::trivially_copyable<T> &&
                      "It's better to use copy function for copying trivial types");
        for (types::usize i = 0; i < N; ++i) {
            mem::construct(dst + i, src[i]);
        }
    }

    /**
     * @brief Runtime variable-size copy for complex, non-trivial objects.
     * @tparam T Type of elements being copied.
     * @param dst Pointer to the destination array storage.
     * @param src Pointer to the source array data.
     * @param count Runtime total count of objects to be copied.
     * @note Invokes copy constructor via placement new at destination layout.
     * Triggers a compilation error if used on trivial types to enforce optimization. @see copy
     */
    template<typename T>
    void obj_copy(T* dst, const T* src, const types::usize count) noexcept {
        static_assert(!msl::types::concepts::trivially_copyable<T> &&
                      "It's better to use copy function for copying trivial types");
        for (types::usize i = 0; i < count; ++i) {
            mem::construct(dst + i, src[i]);
        }
    }

    /**
     * @brief Unified copying engine that automatically selects the most efficient strategy.
     * @details If the type is trivially copyable, it executes a lightning-fast bulk block copy.
     * Otherwise, it safely constructs complex objects sequentially using copy constructors.
    */
    template<typename T>
    constexpr void generic_copy(T *dst, const T *src, const types::usize count) noexcept {
        if constexpr (msl::types::concepts::trivially_copyable<T>) {
            mem::copy(dst, src, count * sizeof(T));
        } else {
            mem::obj_copy(dst, src, count);
        }
    }

    /**
     * @brief Unified copying engine that automatically selects the most efficient strategy.
     * @details If the type is trivially copyable, it executes a lightning-fast bulk block copy.
     * Otherwise, it safely constructs complex objects sequentially using copy constructors.
    */
    template<typename T, types::usize N>
    constexpr void generic_copy(T *dst, const T *src) noexcept {
        if constexpr (msl::types::concepts::trivially_copyable<T>) {
            mem::copy<N * sizeof(T)>(dst, src);
        } else {
            mem::obj_copy<T, N>(dst, src);
        }
    }
}

/**
 * @brief Global placement new operator overload.
 * @details Required by the language syntax to make `::new(ptr) T(...)` work.
 * Per the C++ standard, it simply returns the provided pointer without allocating anything.
 */
inline void *operator new(msl::types::usize, void *__p) noexcept {
    return __p;
}

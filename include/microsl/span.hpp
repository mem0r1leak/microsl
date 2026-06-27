#pragma once

#include "assert.hpp"
#include "types.hpp"
#include "memory.hpp"

namespace msl {
    using namespace msl::types;

    constexpr usize DynamicExtent = max_size;

    /**
     *
     * @tparam T type of elements
     * @tparam N count of elements
     */
    template<typename T, usize N = DynamicExtent>
    struct Span;

    /**
     * @brief Span of runtime known size. Just like slices in other languages (pointer + length)
     * @tparam T type of elements
     */
    template<typename T>
    struct Span<T, DynamicExtent> {
        T *ptr;
        usize len;

        constexpr Span(T *ptr, const usize len) noexcept : ptr(ptr), len(len) {
        }

        template<typename U>
            requires std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U> > &&
                     std::is_const_v<T>
        constexpr Span(const Span<U, DynamicExtent> &other) noexcept
            : ptr(other.ptr), len(other.len) {
        }

        struct Iterator {
            T *ptr;

            constexpr Iterator &operator++() noexcept {
                ++ptr;
                return *this;
            }

            constexpr T &operator *() const noexcept {
                return *ptr;
            }

            constexpr bool operator !=(const Iterator &other) const noexcept {
                return ptr != other.ptr;
            }
        };

        constexpr Iterator begin() const noexcept {
            return Iterator(ptr);
        }

        constexpr Iterator end() const noexcept {
            return Iterator(ptr + len);
        }

        /**
         * @brief Provides subscript access to the element at the specified index.
         * Returns a reference to the element, allowing both retrieval and modification
         * of the underlying data (unless `T` is const).
         * @param i The zero-based index of the element to access.
         * @return A reference (`T&`) to the element at the given index.
         * @note Asserts that `i < len` in debug mode.
         * Out-of-bounds access (`i >= len`) in release mode results in Undefined Behaviour.
         */
        constexpr T &operator [](usize i) const noexcept {
            assert(i < len);
            return ptr[i];
        }

        /**
         * @brief Creates a new sub-span (slice) from the current span.
         * Extracts a view of the memory within the half-open interval `[start, end)`.
         * Equivalent to the `[start..end]` or `[start:end]` slice operators in other languages.
         * This operation is non-destructive and does not modify the current span object.
         * @param start The inclusive starting index of the new sub-span.
         * @param end The exclusive ending index of the new sub-span.
         * @return A new `Span` instance starting at `ptr + start` with length equal to `end - start`.
         * @note Asserts that `start <= end` and `end <= len` in debug mode.
         * Out-of-bounds access in release mode results in Undefined Behaviour.
         */
        [[nodiscard]] constexpr Span slice(usize start, const usize end) const noexcept {
            assert(start <= end && end <= len);
            return Span(ptr + start, end - start);
        }

        /**
         * @brief Shrinks the span by truncating elements from the beginning.
         * Creates a new `Span` that starts at the shifted memory address and has its length
         * reduced accordingly. Equivalent to the `[start...]` slice operator in other languages.
         * This operation is non-destructive and does not modify the current span object.
         * @param start The number of elements to remove from the left side of the span.
         * @return A new `Span` instance starting at `ptr + start` with length equal to `len - start`.
         * @note Asserts that the offset does not exceed the current length of the span in debug mode.
         * Exceeding the bounds in release mode results in Undefined Behaviour.
         */
        [[nodiscard]] constexpr Span shift_start(usize start) const noexcept {
            assert(start <= len);
            return Span(ptr + start, len - start);
        }

        /**
          * @brief Shrinks the span by truncating elements from the end.
          * Creates a new `Span` that starts at the same memory address but has its length reduced.
          * This operation is non-destructive and does not modify the current span object.
          * @param offset The number of elements to remove from the right side of the span.
          * @return A new `Span` instance with length equal to `len - offset`.
          * @note Asserts that the offset does not exceed the current length of the span to prevent underflow.
          */
        [[nodiscard]] constexpr Span shift_end(usize offset) const noexcept {
            assert(offset <= len);
            return Span(ptr, len - offset);
        }

        [[nodiscard]] constexpr usize size_bytes() const noexcept {
            return sizeof(T) * len;
        }

        /**
         *
         * @param src Source span
         * @details Copies objects from one span to another. Effectively memcpy trivial objects @see generic_copy.
         */
        constexpr void copy(const Span src) const noexcept {
            const usize count = len < src.len ? len : src.len;
            mem::generic_copy<T>(ptr, src.ptr, count);
        }

        /**
         *
         * @tparam Size Size of new Span
         * @return New slice of fixed size.
         * @details Out-of-bounds access (`i >= Size`) in release mode results in Undefined Behaviour.
         */
        template<usize Size>
        [[nodiscard]] constexpr Span<T, Size> fixed() const noexcept {
            assert(len >= Size);
            return Span<T, Size>(ptr);
        }
    };

    /**
     *
     * @tparam T type of elements
     * @tparam N count of elements
     * @brief Fixed length Span. For example it can be used for arrays. (Doesn't waste memory for length).
     */
    template<typename T, usize N>
    struct Span {
        T *ptr;
        static constexpr usize len = N;

        explicit constexpr Span(T *arr) noexcept : ptr(arr) {
        }

        template<typename U>
            requires std::is_same_v<std::remove_const_t<T>, std::remove_const_t<U> > &&
                     std::is_const_v<T>
        constexpr Span(const Span<U, N> &other) noexcept
            : ptr(other.ptr) {
        }

        struct Iterator {
            T *ptr;

            constexpr Iterator &operator++() noexcept {
                ++ptr;
                return *this;
            }

            constexpr T &operator *() const noexcept {
                return *ptr;
            }

            constexpr bool operator !=(const Iterator &other) const noexcept {
                return ptr != other.ptr;
            }
        };

        constexpr Iterator begin() const noexcept {
            return Iterator(ptr);
        }

        constexpr Iterator end() const noexcept {
            return Iterator(ptr + len);
        }

        /**
         * @brief Provides subscript access to the element at the specified index.
         * Returns a reference to the element, allowing both retrieval and modification
         * of the underlying data (unless `T` is const).
         * @param i The zero-based index of the element to access.
         * @return A reference (`T&`) to the element at the given index.
         * @note Asserts that `i < len` in debug mode.
         * Out-of-bounds access (`i >= len`) in release mode results in Undefined Behaviour.
         */
        constexpr T &operator [](usize i) const noexcept {
            assert(i < len);
            return ptr[i];
        }

        /**
         *
         * @param src Source span
         * @details Copies objects from one span to another. Effectively memcpy trivial objects @see generic_copy.
         */
        template<usize SourceN>
        constexpr void copy(const Span<T, SourceN> src) const noexcept {
            constexpr usize count = len < SourceN ? len : SourceN;
            mem::generic_copy<T, count>(this->ptr, src.ptr);
        }

        template<usize Start>
        [[nodiscard]] constexpr Span<T, len - Start> shift_start() const noexcept {
            static_assert(Start <= len, "Start offset exceeds fixed span bounds");
            return Span<T, len - Start>(this->ptr + Start);
        }

        template<usize Offset>
        [[nodiscard]] constexpr Span<T, len - Offset> shift_end() const noexcept {
            static_assert(Offset <= len, "End offset exceeds fixed span bounds");
            return Span<T, len - Offset>(this->ptr);
        }

        [[nodiscard]] static constexpr usize size_bytes() noexcept {
            return sizeof(T) * len;
        }

        /**
         *
         * @tparam Start Index of first item
         * @tparam End Index of last item
         * @return New span with different bounds. [start...end] in other languages.
         */
        template<usize Start, usize End>
        [[nodiscard]] constexpr Span<T, End - Start> slice() const noexcept {
            static_assert(Start <= End && End <= len, "Index out of compile-time bounds");
            return Span<T, End - Start>(this->ptr + Start);
        }

        [[nodiscard]] constexpr Span<T> make_dynamic() {
            return Span<T>(ptr, len);
        }
    };

    template<typename T, usize Size>
    Span(T (&)[Size]) -> Span<T, Size>;
}

#pragma once

#include "assert.hpp"
#include "types.hpp"
#include "memory.hpp"

namespace msl {
    using namespace msl::types;

    /**
     *
     * @tparam T type of elements
     * @tparam N is count of elements
     */
    template<typename T, usize N = max_size>
    struct Span;

    /**
     * @brief Span of runtime known size. Just like slices in other languages (pointer + length)
     * @tparam T type of elements
     */
    template<typename T>
    struct Span<T, max_size> {
        T *ptr;
        usize len;

        constexpr Span(T *ptr, const usize len) noexcept : ptr(ptr), len(len) {
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
         *
         * @param i Index of item
         * @return item
         * @details It is undefined behaviour if i is out of bounds in release mode. And in debug it will trigger assert.
         */
        constexpr T &operator [](usize i) const noexcept {
            assert(i < len);
            return ptr[i];
        }

        /**
         *
         * @param start Index of new first item
         * @param end Index of new last item
         * @return New span sliced from start to end. [start...end] in other languages.
         * @details It is undefined behaviour if start or end is out of bounds in release mode. And in debug it will trigger assert.
         */
        [[nodiscard]] constexpr Span slice(usize start, const usize end) const noexcept {
            assert(start <= end && end <= len);
            return Span(ptr + start, end - start);
        }

        /**
         *
         * @param start Index of new first item
         * @return New span with shifted start. [start...] in other languages.
         * @details It is undefined behaviour if start is out of bounds in release mode. And in debug it will trigger assert.
         */
        [[nodiscard]] constexpr Span shift_start(usize start) const noexcept {
            assert(start <= len);
            return Span(ptr + start, len - start);
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
         * @details It is undefined behaviour if size is out of bounds in release mode. And in debug it will trigger assert with runtime check.
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
     * @brief Fixed length Span. Can be used for arrays for example. (Doesn't waste memory for length).
     */
    template<typename T, usize N>
    struct Span : Span<T, max_size> {
        static constexpr usize fixed_len = N;

        constexpr explicit Span(T *ptr) : Span<T, max_size>(ptr, N) {
        }

        /**
         *
         * @param src Source span
         * @details Copies objects from one span to another. Effectively memcpy trivial objects @see generic_copy.
         */
        template<usize SourceN>
        constexpr void copy(const Span<T, SourceN> src) const noexcept {
            constexpr usize count = N < SourceN ? N : SourceN;
            mem::generic_copy<T, count>(this->ptr, src.ptr);
        }

        /**
         *
         * @tparam Start Index of first item
         * @tparam End Index of last item
         * @return New span with different bounds. [start...end] in other languages.
         */
        template<usize Start, usize End>
        [[nodiscard]] constexpr Span<T, End - Start> slice() const noexcept {
            static_assert(Start <= End && End <= N, "Index out of compile-time bounds");
            return Span<T, End - Start>(this->ptr + Start);
        }
    };
}

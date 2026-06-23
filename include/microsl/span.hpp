#pragma once
#include "types.hpp"

namespace msl {
    using namespace msl::types;

    template<typename T, usize N = max_size>
    struct Span;

    template<typename T>
    struct Span<T, max_size> {
        T *ptr;
        usize len;

        Span(T *ptr, const usize len) : ptr(ptr), len(len) {
        }

        struct Iterator {
            T* ptr;

            void operator++() {
                ++ptr;
            }

            T operator *() {
                return *ptr;
            }
        };

        Iterator begin() {
            return Iterator(ptr);
        }

        Iterator end() {
            return Iterator(ptr + len);
        }

        T operator [](usize i) const {
            assert(i < len);
            return ptr[i];
        }

        [[nodiscard]] Span slice(usize start, const usize end) const {
            assert(start <= end && end <= len);
            return Span(ptr + start, end - start);
        }

        [[nodiscard]] Span shift_start(usize start) const {
            assert(start <= len);
            return Span(ptr + start, len - start);
        }

        [[nodiscard]] usize size_bytes() const {
            return sizeof(T) * len;
        }

        void copy(Span<const T, max_size> src) {
            const usize count = min(len, src.len);
            if constexpr (concepts::trivially_copyable<T>) {
                memcpy(this->ptr, src.ptr, count * sizeof(T));
            } else {
                obj_copy(src.ptr, this->ptr, count, count);
            }
        }

        template<usize Size>
        void copy(Span<const T, Size> src) {
            const usize count = min(len, src.len);
            if constexpr (concepts::trivially_copyable<T>) {
                memcpy(this->ptr, src.ptr, count * sizeof(T));
            } else {
                obj_copy(src.ptr, this->ptr, count, count);
            }
        }

        template<usize Size>
        [[nodiscard]] Span<T, Size> fixed() const {
            assert(len >= Size);
            return Span<T, Size>(ptr);
        }
    };

    template<typename T, usize N>
    struct Span : Span<T, max_size> {
        static constexpr usize fixed_len = N;

        explicit Span(T *ptr) : Span<T, max_size>(ptr, N) {
        }

        template<usize Start, usize End>
        [[nodiscard]] Span<T, End - Start> slice() const {
            static_assert(Start <= End && End <= N, "Index out of compile-time bounds");
            return Span<T, End - Start>(this->ptr + Start);
        }
    };
}

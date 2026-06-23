#pragma once

#include "span.hpp"
#include "types.hpp"

namespace msl {
    class Allocator {
        using address = void *;

        enum class AllocationError {
            OutOfMemory,
        };

    public:
        virtual ~Allocator() = default;

        template<typename T>
        [[nodiscard]] Result<Span<T>, AllocationError> alloc(usize size) {
            auto memory = allocate(size * sizeof(T), alignof(T));
            if (!memory.has_value()) {
                return memory.error();
            }

            auto items = static_cast<T*>(memory);
            for (usize i = 0; i < size; ++i) {
                ::new(&items[i]) T();
            }
            return Ok{Span<T>(items, size)};
        }

        template<typename T, typename... Args>
        [[nodiscard]] Result<T*, AllocationError> create(Args &&... args) {
            auto memory = allocate(sizeof(T), alignof(T));
            if (!memory.has_value()) return memory.error();
            return Ok{::new(memory) T(static_cast<Args &&>(args)...)};
        }

        template<typename T>
        void free(Span<T> items) {
            for (auto& i: items) {
                i.~T();
            }
            const auto memory = static_cast<address>(items.ptr);
            deallocate(memory, items.size * sizeof(T), alignof(T));
        }

        template<typename T>
        Option<AllocationError> destroy(T *item) {
            item->~T();
            const auto memory = static_cast<address>(item);
            return deallocate(memory, sizeof(T), alignof(T));
        }

    protected:
        virtual Result<address, AllocationError> allocate(usize count, usize alignment) = 0;
        virtual Option<AllocationError> deallocate(address ptr, usize size, usize alignment) = 0;
    };
}

#pragma once

#include "result.hpp"
#include "span.hpp"
#include "types.hpp"

namespace msl {
    class Allocator {
        using address = void *;

        enum class AllocationError {
            OutOfMemory,
            InvalidArgument,
            PermissionDenied,
            Unsupported,
            FragmentationFailure,
        };

    public:
        virtual ~Allocator() = default;

        Allocator(const Allocator&) = delete;
        Allocator& operator=(const Allocator&) = delete;
        Allocator(Allocator&&) = delete;
        Allocator& operator=(Allocator&&) = delete;

        template<typename T>
        [[nodiscard]] Result<Span<T>, AllocationError> alloc(usize size) {
            const auto memory = allocate(size * sizeof(T), alignof(T));
            if (!memory.ok()) {
                return memory.error();
            }

            auto items = static_cast<T*>(memory.value());
            for (usize i = 0; i < size; ++i) {
                ::new(&items[i]) T();
            }
            return Ok{Span<T>(items, size)};
        }

        template<typename T, typename... Args>
        [[nodiscard]] Result<T*, AllocationError> create(Args &&... args) {
            const auto memory = allocate(sizeof(T), alignof(T));
            if (!memory.ok()) return memory.error();
            auto object = ::new(memory.value()) T(static_cast<Args &&>(args)...);
            return Ok{object};
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
        void destroy(T *item) {
            item->~T();
            const auto memory = static_cast<address>(item);
            deallocate(memory, sizeof(T), alignof(T));
        }

    protected:
        virtual Result<address, AllocationError> allocate(usize count, usize alignment) = 0;
        virtual void deallocate(address ptr, usize size, usize alignment) = 0;
    };
}

#pragma once
#include "memory.hpp"
#include "assert.hpp"

namespace msl {
    template<typename T>
    struct Value {
        T value;
    };

    struct None {};

    inline constexpr None none{};

    template<typename T>
    class Option {
        union {
            Value<T> value;
        };

        bool has_value;

    public:
        // ReSharper disable once CppPossiblyUninitializedMember
        constexpr Option() : has_value(false) {
        }

        // ReSharper disable once CppPossiblyUninitializedMember
        constexpr Option(None) noexcept : has_value(false) {
        }

        constexpr Option(Option &&other) noexcept : has_value(other.has_value) {
            if (other.has_value) {
                mem::construct(&value, mem::move(other.value));
            }
        }

        // ReSharper disable once CppNonExplicitConvertingConstructor
        constexpr Option(const Value<T>& val) noexcept : has_value(true) {
            mem::construct(&value, val.value);
        }

        // ReSharper disable once CppNonExplicitConvertingConstructor
        constexpr Option(Value<T> &&val) : has_value(true) {
            mem::construct(&value, mem::move(val.value));
        }

        [[nodiscard]] bool with_value() const noexcept { return has_value; }

        [[nodiscard]] T get_value() const {
            if (!has_value)
                assert(false && "Has no value");
            return value.value;
        }

        ~Option() {
            if (has_value) {
                mem::destruct(&value);
            }
        }
    };
}
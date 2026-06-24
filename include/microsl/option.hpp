#pragma once
#include "memory.hpp"
#include "assert.hpp"

template<typename T>
struct Value {
    T value;
};

namespace msl {
    using namespace msl;

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

        [[nodiscard]] bool with_value() const noexcept { return has_value; }
        [[nodiscard]] T get_value() { if (!has_value) assert(false && "Has no value"); return value.value; }

        ~Option() {
            if (has_value) {
                mem::destruct(&value);
            }
        }

        constexpr Option(Option &&other) noexcept : has_value(other.has_value) {
            if (other.has_value) {
                mem::construct(&value, mem::move(other.value));
            }
        }

        // ReSharper disable once CppNonExplicitConvertingConstructor
        constexpr Option(Value<T> &&val) : has_value(true) {
            mem::construct(&value, mem::move(val.value));
        }

        static constexpr Option none() noexcept {
            return Option();
        }
    };
}

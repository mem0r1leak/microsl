#pragma once
#include "memory.hpp"

namespace msl {
    template<typename OkType, typename ErrType>
    class Result;

    template<typename T>
    struct Ok {
        T value;
    };

    template<typename T>
    struct Err {
        T value;
    };

    template<typename OkType, typename ErrType>
    class Result {
        bool is_ok;

        union {
            Err<ErrType> err_val;
            Ok<OkType> ok_val;
        };

    public:
        Result(Ok<OkType> &&ok) : is_ok(true) {
            mem::construct(&ok_val, mem::move(ok));
        }

        Result(Err<ErrType> &&err) : is_ok(false) {
            mem::construct(&err_val, mem::move(err));
        }

        ~Result() {
            if (is_ok) {
                ok_val.~Ok();
            } else {
                err_val.~Err();
            }
        }

        template<typename Function> requires msl::types::concepts::invocable<Function, Ok<OkType> >
        const Result *if_ok(Function callback) const {
            if (is_ok) callback(ok_val);
            return this;
        }

        template<typename Function> requires msl::types::concepts::invocable<Function, Ok<OkType> >
        const Result *if_err(Function callback) const {
            if (!is_ok) callback(err_val);
            return this;
        }

        template<typename OkFunc, typename ErrFunc>
            requires msl::types::concepts::invocable<OkFunc, const Ok<OkType> &> &&
                     msl::types::concepts::invocable<ErrFunc, const Err<ErrType> &>
        void match(OkFunc ok_fn, ErrFunc err_fn) const {
            if (is_ok) [[likely]] {
                ok_fn(ok_val);
            } else [[unlikely]] {
                err_fn(err_val);
            }
        }

        [[nodiscard]] bool has_value() const { return is_ok; }
        OkType value() const { return move(ok_val.value); }
        ErrType error() const { return move(err_val.value); }
    };
}

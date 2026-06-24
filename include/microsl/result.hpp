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
            mem::construct(&ok_val, move(ok));
        }

        Result(Err<ErrType> &&err) : is_ok(false) {
            mem::construct(&err_val, move(err));
        }

        ~Result() {
            if (is_ok) {
                ok_val.~Ok();
            } else {
                err_val.~Err();
            }
        }

        Result *if_ok(types::Func<void(const Ok<OkType> &)> ok_fn) {
            if (is_ok) ok_fn(ok_val);
            return this;
        }

        Result *if_err(types::Func<void(const Err<ErrType> &)> err_fn) {
            if (!is_ok) err_fn(err_val);
            return this;
        }

        void unwrap(types::Func<void(const Ok<OkType> &)> ok_fn, types::Func<void(const Err<ErrType> &)> err_fn) {
            is_ok ? ok_fn(ok_val) : err_fn(err_val);
        }

        [[nodiscard]] bool has_value() const { return is_ok; }
        Ok<OkType> value() { return move(ok_val); }
        Err<ErrType> error() { return move(err_val); }
    };
}

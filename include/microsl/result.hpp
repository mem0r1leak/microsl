#pragma once
#include "memory.hpp"

namespace msl {

    // Forward declaration of the monadic Result container.
    template<typename OkType, typename ErrType>
    class Result;

    /**
     * @brief Wrapper representing a successful operation result.
     */
    template<typename T>
    struct Ok {
        T value;
    };

    /**
     * @brief Wrapper representing a failed operation result/error.
     */
    template<typename T>
    struct Err {
        T value;
    };

    /**
     * @brief A monadic container that holds either a success value (Ok) or an error value (Err).
     * @details Implements explicit Rust-like error handling using a memory-efficient anonymous union.
     * Prevents overhead associated with C++ exceptions.
     */
    template<typename OkType, typename ErrType>
    class Result {
        bool is_ok_;

        // Memory allocated dynamically fits only the largest type size, minimizing footprint.
        union {
            Err<ErrType> err_val;
            Ok<OkType> ok_val;
        };

    public:
        /**
         * @brief Constructs a successful Result via move semantics.
         */
        Result(Ok<OkType>&& ok) : is_ok_(true) {
            mem::construct(&ok_val, mem::move(ok));
        }

        /**
         * @brief Constructs an error Result via move semantics.
         * @note Marked implicit to allow cleaner syntax like: `Result<int, const char*> res = Err("Error");`
         */
        Result(Err<ErrType>&& err) : is_ok_(false) {
            mem::construct(&err_val, mem::move(err));
        }

        /**
         * @brief Manually destroys the active member of the union depending on the state.
         */
        ~Result() {
            if (is_ok_) {
                ok_val.~Ok();
            } else {
                err_val.~Err();
            }
        }

        /**
         * @brief Implicit conversion to bool for quick validity checks.
         * @code if (result) { // handles ok } else { // handles err }
         */
        operator bool() const { return is_ok_; }

        /**
         * @brief Executes a callback if the operation was successful.
         * @return Fluent-style pointer to this Result instance.
         */
        template<typename Function>
            requires msl::types::concepts::invocable<Function, Ok<OkType>>
        const Result* if_ok(Function callback) const {
            if (is_ok_) callback(ok_val);
            return this;
        }

        /**
         * @brief Executes a callback if the operation failed.
         * @return Fluent-style pointer to this Result instance.
         */
        template<typename Function>
            requires msl::types::concepts::invocable<Function, Err<ErrType>> // Fixed constraint type here
        const Result* if_err(Function callback) const {
            if (!is_ok_) callback(err_val);
            return this;
        }

        /**
         * @brief Evaluates both execution paths using specialized code branch attributes.
         * @param ok_fn Executed if the result is Ok.
         * @param err_fn Executed if the result is Err.
         */
        template<typename OkFunc, typename ErrFunc>
            requires msl::types::concepts::invocable<OkFunc, const Ok<OkType>&> &&
                     msl::types::concepts::invocable<ErrFunc, const Err<ErrType>&>
        void match(OkFunc ok_fn, ErrFunc err_fn) const {
            if (is_ok_) [[likely]] {
                ok_fn(ok_val);
            } else [[unlikely]] {
                err_fn(err_val);
            }
        }

        /**
         * @brief Checks whether the container holds a successful value.
         */
        [[nodiscard]] bool is_ok() const { return is_ok_; }

        /**
         * @brief Unwraps and returns the successful value using move semantics.
         */
        OkType value() const { return mem::move(ok_val.value); }

        /**
         * @brief Unwraps and returns the error value using move semantics.
         */
        ErrType error() const { return mem::move(err_val.value); }
    };
}

#pragma once

namespace msl::types::concepts {
    template<typename T, T v>
    struct integral_constant {
        static constexpr T value = v;
    };

    using true_type = integral_constant<bool, true>;
    using false_type = integral_constant<bool, false>;

    template<typename T, typename U>
    struct is_same : false_type {
    };

    template<typename T>
    struct is_same<T, T> : true_type {
    };

    template<typename T, typename U>
    inline constexpr bool is_same_v = is_same<T, U>::value;

    template<typename T>
    struct remove_cv {
        using type = T;
    };

    template<typename T>
    struct remove_cv<const T> {
        using type = T;
    };

    template<typename T>
    struct remove_cv<volatile T> {
        using type = T;
    };

    template<typename T>
    struct remove_cv<const volatile T> {
        using type = T;
    };

    template<typename T>
    using remove_cv_t = remove_cv<T>::type;

    // Перевірка, чи тип є точно інтегральним (int, char, short, long тощо)
    template<typename T>
    concept integral =
            is_same_v<T, bool> ||
            is_same_v<T, char> ||
            is_same_v<T, signed char> ||
            is_same_v<T, unsigned char> ||
            is_same_v<T, short> ||
            is_same_v<T, unsigned short> ||
            is_same_v<T, int> ||
            is_same_v<T, unsigned int> ||
            is_same_v<T, long> ||
            is_same_v<T, unsigned long> ||
            is_same_v<T, long long> ||
            is_same_v<T, unsigned long long> ||

            is_same_v<remove_cv_t<T>, bool> ||
            is_same_v<remove_cv_t<T>, char> ||
            is_same_v<remove_cv_t<T>, signed char> ||
            is_same_v<remove_cv_t<T>, unsigned char> ||
            is_same_v<remove_cv_t<T>, short> ||
            is_same_v<remove_cv_t<T>, unsigned short> ||
            is_same_v<remove_cv_t<T>, int> ||
            is_same_v<remove_cv_t<T>, unsigned int> ||
            is_same_v<remove_cv_t<T>, long> ||
            is_same_v<remove_cv_t<T>, unsigned long> ||
            is_same_v<remove_cv_t<T>, long long> ||
            is_same_v<remove_cv_t<T>, unsigned long long>;

    template<typename T>
    concept floating_point =
            is_same_v<T, float> ||
            is_same_v<T, double> ||
            is_same_v<T, long double>;

    template<typename T>
    concept number = integral<T> || floating_point<T>;

    template<typename T>
    struct is_pointer : false_type {
    };

    template<typename T>
    struct is_pointer<T *> : true_type {
    };

    template<typename T>
    struct remove_reference {
        using type = T;
    };

    template<typename T>
    struct remove_reference<T &> {
        using type = T;
    };

    template<typename T>
    struct remove_reference<T &&> {
        using type = T;
    };

    template<typename T>
    using remove_reference_t = remove_reference<T>::type;

    template<typename T>
    concept pointer = is_pointer<T>::value;

    // Чи можна тип копіювати просто через memcpy (Trivially Copyable)?
    template<typename T>
    concept trivially_copyable = __is_trivially_copyable(T);

    // Чи має конструктор
    template<typename T>
    concept trivially_constructible = __is_trivially_constructible(T);

    // Чи має тип тривіальний деструктор (тобто його не треба викликати, як у int)?
    template<typename T>
    concept trivially_destructible = __is_trivially_destructible(T);

    // Чи можна тип викликати як функцію тобто чи є тип функтором
    template<typename F, typename... Args>
    concept invocable = requires(F &&f, Args &&... args)
    {
        f(static_cast<Args &&>(args)...);
    };
}

namespace msl::types {
    using i8 = signed char;
    using i16 = short;
    using i32 = int;
    using i64 = long long;

    using u8 = unsigned char;
    using u16 = unsigned short;
    using u32 = unsigned int;
    using u64 = unsigned long long;

    using usize = decltype(sizeof(0));

    using size_t = usize;

    inline static constexpr usize max_size = ~static_cast<usize>(0);
    inline static constexpr usize min_size = 0;

    inline static constexpr u8 max_u8 = 0xff;
    inline static constexpr u8 min_u8 = 0;

    inline static constexpr u16 max_u16 = 0xffff;
    inline static constexpr u16 min_u16 = 0;

    inline static constexpr u32 max_u32 = 0xffffffff;
    inline static constexpr u32 min_u32 = 0;

    inline static constexpr u64 max_u64 = 0xffffffffffffffffULL;
    inline static constexpr u64 min_u64 = 0;

    inline static constexpr i8 max_i8 = 0x7f;
    inline static constexpr i8 min_i8 = static_cast<i8>(0x80);

    inline static constexpr i16 max_i16 = 0x7fff;
    inline static constexpr i16 min_i16 = static_cast<i16>(0x8000);

    inline static constexpr i32 max_i32 = 0x7fffffff;
    inline static constexpr i32 min_i32 = static_cast<i32>(0x80000000);

    inline static constexpr i64 max_i64 = 0x7fffffffffffffffLL;
    inline static constexpr i64 min_i64 = static_cast<i64>(0x8000000000000000ULL);

    template<typename Signature>
    class Func;

    /**
     * @brief Casts an lvalue reference to an rvalue reference to enable move semantics.
     * @note This is a pure compile-time type cast. It carries zero runtime overhead
     * and generates no CPU instructions.
     */
    template<typename T>
    [[nodiscard]] constexpr concepts::remove_reference_t<T>&& move(T&& val) noexcept {
        return static_cast<concepts::remove_reference_t<T>&&>(val);
    }

    /**
     * @brief Implements perfect forwarding for a function argument.
     * Preserves the original value category (lvalue or rvalue) of the passed object.
     * The use of `remove_reference_t` creates a non-deduced context, forcing the
     * compiler to use the explicitly provided template parameter `T`.
     * @tparam T The original type of the argument captured via universal reference.
     * @param param The reference to the object whose value category needs to be restored.
     * @return T&& The original reference (collapses to either lvalue or rvalue reference).
     */
    template<typename T>
    [[nodiscard]] constexpr T&& forward(concepts::remove_reference_t<T>& param) noexcept {
        return static_cast<T&&>(param);
    }

    /**
     * @brief Something like a trivial implementation of std::function. Maden to simplify function pointer syntax.
     */
    template<typename Ret, typename... Args>
    class Func<Ret(Args...)> {
    public:
        using ptr_type = Ret(*)(Args...);

    private:
        ptr_type m_func = nullptr;

    public:
        Func() = default;

        Func(const ptr_type func) : m_func(func) {
        }

        Ret operator()(Args &&... args) const {
            return m_func(static_cast<Args &&>(args)...);
        }

        explicit operator bool() const { return m_func != nullptr; }
    };
}

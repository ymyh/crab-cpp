export module crab_cpp:match;

import std;
import :option;
import :result;

export namespace crab_cpp::internal
{
    template<typename... Fs>
    struct overload : Fs...
    {
        using Fs::operator()...;
    };

    template<typename... Fs>
    overload(Fs...) -> overload<Fs...>;

    /**
     * @brief True when T is a specialization of std::variant
     */
    template<typename T>
    concept variant_specialization = requires
    {
        std::variant_size<std::remove_cvref_t<T>>::value;
    };

    /**
     * @brief Visits one Option, Result, or std::variant
     */
    template<typename F, typename V>
    constexpr auto visit_one(F&& f, V&& value) -> decltype(auto)
    {
        using raw = std::remove_cvref_t<V>;

        if constexpr (
            requires { std::declval<const raw&>().is_none(); } &&
            !requires { typename raw::err_type; }
        )
        {
            return std::forward<V>(value).visit(std::forward<F>(f));
        }
        else if constexpr (requires { typename raw::err_type; std::declval<const raw&>().is_ok(); })
        {
            return std::forward<V>(value).visit(std::forward<F>(f));
        }
        else
        {
            return std::visit(std::forward<F>(f), std::forward<V>(value));
        }
    }

    template<typename F, typename V, typename... Vs>
    constexpr auto visit_all(F&& f, V&& value, Vs&&... rest) -> decltype(auto);

    /**
     * @brief Visits the first value, then the rest, and invokes f with every alternative
     */
    template<typename F, typename V, typename... Vs>
    constexpr auto visit_rest(F&& f, V&& value, Vs&&... rest) -> decltype(auto)
    {
        return visit_one(
            [&]<typename First>(First&& first) -> decltype(auto)
            {
                if constexpr (sizeof...(Vs) == 0)
                {
                    return std::invoke(std::forward<F>(f), std::forward<First>(first));
                }
                else
                {
                    return visit_all(
                        [&]<typename... Rest>(Rest&&... tails) -> decltype(auto)
                        {
                            return std::invoke(
                                std::forward<F>(f),
                                std::forward<First>(first),
                                std::forward<Rest>(tails)...
                            );
                        },
                        std::forward<Vs>(rest)...
                    );
                }
            },
            std::forward<V>(value)
        );
    }

    /**
     * @brief Visits every argument. A pack of only std::variant uses one std::visit.
     */
    template<typename F, typename V, typename... Vs>
    constexpr auto visit_all(F&& f, V&& value, Vs&&... rest) -> decltype(auto)
    {
        if constexpr ((variant_specialization<V> && ... && variant_specialization<Vs>))
        {
            return std::visit(std::forward<F>(f), std::forward<V>(value), std::forward<Vs>(rest)...);
        }
        else
        {
            return visit_rest(std::forward<F>(f), std::forward<V>(value), std::forward<Vs>(rest)...);
        }
    }

    template<typename... Ts>
    struct matcher
    {
    public:
        std::tuple<Ts...> vs;

        template<typename... Vs>
        constexpr matcher(Vs&&... values) : vs(std::forward<Vs>(values)...)
        {
        }

        template<typename Fs>
        constexpr auto operator->*(Fs&& f) const -> decltype(auto)
        {
            return std::apply(
                [&](auto&&... values) -> decltype(auto)
                {
                    return visit_all(std::forward<Fs>(f), std::forward<decltype(values)>(values)...);
                },
                std::move(vs)
            );
        }
    };

    template<typename... Ts>
    matcher(Ts&&...) -> matcher<Ts&&...>;

    template<typename T>
    struct is_option_type : std::false_type
    {
    };

    template<typename T>
    struct is_option_type<Option<T>> : std::true_type
    {
    };

    /**
     * @brief True when T is a specialization of Option
     */
    template<typename T>
    concept option_specialization = is_option_type<std::remove_cvref_t<T>>::value;

    template<typename T>
    struct is_result_type : std::false_type
    {
    };

    template<typename T, typename E>
    struct is_result_type<Result<T, E>> : std::true_type
    {
    };

    /**
     * @brief True when T is a specialization of Result
     */
    template<typename T>
    concept result_specialization = is_result_type<std::remove_cvref_t<T>>::value;

    /**
     * @brief Proxy returned by CRAB_TRY / CRAB_TRY_VOID when propagating None
     */
    struct try_none_carrier
    {
        using crab_cpp_try_carrier_tag = void;

        try_none_carrier() = default;
        try_none_carrier(const try_none_carrier&) = delete;
        auto operator=(const try_none_carrier&) -> try_none_carrier& = delete;
        try_none_carrier(try_none_carrier&&) = default;
        auto operator=(try_none_carrier&&) -> try_none_carrier& = delete;

        template<typename U>
        constexpr operator Option<U>() && noexcept
        {
            return None{};
        }
    };

    /**
     * @brief Proxy returned by CRAB_TRY / CRAB_TRY_VOID when propagating Err
     * @tparam Err The extracted error type (value or lvalue reference)
     */
    template<typename Err>
    struct try_err_carrier
    {
        using crab_cpp_try_carrier_tag = void;

        Err m_err;

        constexpr explicit try_err_carrier(Err err)
            noexcept(std::is_lvalue_reference_v<Err> || std::is_nothrow_move_constructible_v<Err>)
            : m_err(static_cast<Err&&>(err))
        {
        }

        try_err_carrier(const try_err_carrier&) = delete;
        auto operator=(const try_err_carrier&) -> try_err_carrier& = delete;
        try_err_carrier(try_err_carrier&&) = default;
        auto operator=(try_err_carrier&&) -> try_err_carrier& = delete;

        template<typename OutT, typename OutE>
            requires (
                (
                    std::is_lvalue_reference_v<Err> &&
                    std::convertible_to<Err, OutE>
                ) ||
                (
                    !std::is_lvalue_reference_v<Err> &&
                    !std::is_lvalue_reference_v<OutE> &&
                    std::convertible_to<Err&&, OutE>
                )
            )
        constexpr operator Result<OutT, OutE>() &&
            noexcept(std::is_nothrow_constructible_v<OutE, Err&&>)
        {
            return Result<OutT, OutE>(std::in_place_index<1>, static_cast<Err&&>(m_err));
        }
    };

    /**
     * @brief Returns true when an Option is None or a Result is Err
     */
    template<typename V>
        requires (option_specialization<V> || result_specialization<V>)
    [[nodiscard]] constexpr auto try_is_err(const V& value) noexcept -> bool
    {
        if constexpr (option_specialization<V>)
        {
            return value.is_none();
        }
        else
        {
            return value.is_err();
        }
    }

    /**
     * @brief Extracts the Some or Ok value after try_is_err has returned false
     */
    template<typename V>
        requires (option_specialization<V> || result_specialization<V>)
    [[nodiscard]] constexpr auto try_unwrap_unchecked(V&& value)
        noexcept(noexcept(std::forward<V>(value).unwrap_unchecked())) -> decltype(auto)
    {
        return std::forward<V>(value).unwrap_unchecked();
    }

    /**
     * @brief Builds a carrier that propagates None or Err to the enclosing function's return type
     */
    template<typename V>
        requires (option_specialization<V> || result_specialization<V>)
    [[nodiscard]] constexpr auto try_propagate_err(V&& value)
    {
        if constexpr (option_specialization<V>)
        {
            return try_none_carrier{};
        }
        else
        {
            using err_ret_t = decltype(std::forward<V>(value).unwrap_err_unchecked());

            return try_err_carrier<err_ret_t>(std::forward<V>(value).unwrap_err_unchecked());
        }
    }
}

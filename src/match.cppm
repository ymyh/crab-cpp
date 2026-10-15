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
}

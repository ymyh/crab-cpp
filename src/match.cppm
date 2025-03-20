export module crab_cpp:match;

import std;

export namespace crab_cpp::internal
{
    template<typename... Fs> struct overload : Fs... { using Fs::operator()...; };
    template<typename... Fs> overload(Fs...) -> overload<Fs...>;

    template<typename... Ts>
    struct matcher
    {
    public:
        std::tuple<Ts...> vs;

        template<typename... Vs>
        constexpr matcher(Vs &&...vs) : vs(std::forward<Vs>(vs)...) {}

        template<typename Fs>
        constexpr auto operator->*(Fs&& f) const
        {
            auto curry = [&](auto&&... vss) { return std::visit(std::forward<Fs>(f), vss...); };
    	    return std::apply(curry, std::move(vs));
        }
    };

    template<typename ...Ts> matcher(Ts&&...) -> matcher<Ts&&...>;
}


export module crab_cpp:result;

import std;
import :panic;

export namespace crab_cpp
{

template<typename R, typename E>
struct Result : std::variant<R, E>
{
    using std::variant<R, E>::variant;

    using std::variant<R, E>::operator=;

    using type = R;

    using err_type = E;

    /**
     * Returns true if Ok.
     */
    auto is_ok() const -> bool
    {
        return this->index() == 0;
    }

    /**
     * Returns true if Err.
     */
    auto is_err() const -> bool
    {
        return !this->is_ok();
    }

    /**
     * Returns reference if Ok.
     * Calls std::terminate if Err.
     */
    template<typename Self>
    auto unwrap(this Self &&self) noexcept -> auto&&
    {
        if (self.is_ok())
        {
            return std::get<0>(self);
        }

        panic("Calling Result<T, E>::unwrap() on an Err value");
    }

    /**
     * Returns reference if Err.
     * Calls std::terminate if Ok.
     */
    template<typename Self>
    auto unwrap_err(this Self &&self) noexcept -> auto&&
    {
        if (self.is_err())
        {
            return std::get<1>(self);
        }

        panic("Calling Result<T, E>::unwrap_err() on an Ok value");
    }

    /**
     * Calls a function with the contained result if Ok.
     */
    template<typename F, typename Self>
        requires std::invocable<F, R&>
    auto inspect(this Self &&self, F &&f) -> Self&&
    {
        if (self.is_ok())
        {
            f(std::get<0>(self));
        }

        return self;
    }

    /**
     * Maps an Result<T, E> to Result<U, E> by applying f to the contained value if Ok, otherwise does nothing.
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, R&>::type>
        requires requires (F f, R &r)
        {
            { f(r) } -> std::same_as<Result<U, E>>;
        }
    auto map(this Self &&self, F &&f) -> Result<U, E>
    {
        if (self.is_ok())
        {
            return f(std::get<0>(self));
        }

        return std::get<1>(self);
    }
};

}

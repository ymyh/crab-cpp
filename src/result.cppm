export module crab_cpp:result;

import std;
import :panic;
import :option;

export namespace crab_cpp
{

template<typename T, typename E>
struct Result : std::variant<T, E>
{
    using std::variant<T, E>::variant;

    using std::variant<T, E>::operator=;

    using type = T;

    using err_type = E;

    /**
     * Calls f if the result is Ok, otherwise returns the Err value of self.
     */
    template<typename F, typename Self, typename U = typename std::invoke_result_t<F, T&>>
        requires requires (F f, T& r)
        {
            { f(r) } -> std::same_as<Result<typename U::type, typename U::err_type>>;
        }
    auto and_then(this Self&& self, F&& f) -> Result<typename U::type, typename U::err_type>
    {
        if (self.is_ok())
        {
            return f(std::get<0>(self));
        }

        return std::get<1>(self);
    }

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
     * Converts from Result<T, E> to Option<T>.
     * Converts self into an Option<T>, invalidate self, and discarding the error, if any.
     */
    auto ok() noexcept -> Option<T>
    {
        if (this->is_ok())
        {
            auto temp = std::move(std::get<0>(*this));
            return temp;
        }

        return None{};
    }

    /**
     * Converts from Result<T, E> to Option<E>.
     * Converts self into an Option<E>, invalidate self, and discarding the success value, if any.
     */
    auto err() noexcept -> Option<E>
    {
        if (this->is_err())
        {
            auto temp = std::move(std::get<1>(*this));
            return temp;
        }

        return None{};
    }

    /**
     * Calls a function with a reference to the contained value if Ok.
     * Returns the original result.
     */
    template<typename F, typename Self>
        requires std::invocable<F, T&>
    auto inspect(this Self&& self, F&& f) -> Self&&
    {
        if (self.is_ok())
        {
            f(std::get<0>(self));
        }

        return self;
    }

    /**
     * Calls a function with a reference to the contained value if Err.
     * Returns the original result.
    */
    template<typename F, typename Self>
        requires std::invocable<F, E&>
    auto inspect_err(this Self&& self, F&& f) -> Self&&
    {
        if (self.is_err())
        {
            f(std::get<1>(self));
        }

        return self;
    }

    /**
     * Maps a Result<T, E> to Result<U, E> by applying a function to a contained Ok value, leaving an Err value untouched.
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, T&>>
        requires requires (F f, T& r)
        {
            { f(r) } -> std::same_as<U>;
        }
    auto map(this Self&& self, F&& f) -> Result<U, E>
    {
        if (self.is_ok())
        {
            return f(std::get<0>(self));
        }

        return std::get<1>(self);
    }

    /**
     * Maps a Result<T, E> to Result<T, F> by applying a function to a contained Err value, leaving an Ok value untouched.
     */
    template<typename Self, typename M, typename F = typename std::invoke_result_t<M, E&>>
        requires requires (M m, E& e)
        {
            { m(e) } -> std::same_as<F>;
        }
    auto map_err(this Self&& self, M&& m) -> Result<T, F>
    {
        if (self.is_err())
        {
            return m(std::get<1>(self));
        }

        return std::get<0>(self);
    }

    /**
     * Returns reference if Ok.
     * Panics if Err with the given message.
     */
    template<typename Self>
    auto expect(this Self&& self, std::string_view msg) noexcept -> auto&&
    {
        if (self.is_ok())
        {
            return std::get<0>(self);
        }

        panic(msg);
    }

    /**
     * Returns Err reference.
     * Panics if Ok with the given message.
     */
    template<typename Self>
    auto expect_err(this Self&& self, std::string_view msg) noexcept -> auto&&
    {
        if (self.is_err())
        {
            return std::get<1>(self);
        }

        panic(msg);
    }

    /**
     * Returns reference if Ok.
     * Panics if Err.
     */
    template<typename Self>
    auto unwrap(this Self&& self) noexcept -> auto&&
    {
        if (self.is_ok())
        {
            return std::get<0>(self);
        }

        panic("Calling Result<T, E>::unwrap() on an Err value");
    }

    /**
     * Returns Err reference.
     * Panics if Ok.
     */
    template<typename Self>
    auto unwrap_err(this Self&& self) noexcept -> auto&&
    {
        if (self.is_err())
        {
            return std::get<1>(self);
        }

        panic("Calling Result<T, E>::unwrap_err() on an Ok value");
    }
};

}

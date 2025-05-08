export module crab_cpp:result;

import std;
import :panic;
import :option;

export namespace crab_cpp
{

/**
 * @brief Result type that can hold either a value of type T or an error of type E
 * @tparam T The type of value that can be held in the Result
 * @tparam E The type of error that can be held in the Result
 */
template<typename T, typename E>
struct Result : std::variant<T, E>
{
    using std::variant<T, E>::variant;

    using std::variant<T, E>::operator=;

    using type = T;

    using err_type = E;

    /**
     * @brief Calls f if the result is Ok, otherwise returns the Err value of self
     * @tparam F The type of the function to call
     * @tparam Self The type of self
     * @tparam U The return type of the function
     * @param f The function to call
     * @return Result containing the result of f if Ok, Err otherwise
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
     * @brief Returns true if the result is Ok
     * @return true if the result is Ok, false otherwise
     */
    [[nodiscard]] auto is_ok() const -> bool
    {
        return this->index() == 0;
    }

    /**
     * @brief Returns true if the result is Err
     * @return true if the result is Err, false otherwise
     */
    [[nodiscard]] auto is_err() const -> bool
    {
        return !this->is_ok();
    }

    /**
     * @brief Converts from Result<T, E> to Option<T>
     * @return Option containing the value if Ok, None otherwise
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
     * @brief Converts from Result<T, E> to Option<E>
     * @return Option containing the error if Err, None otherwise
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
     * @brief Calls a function with a reference to the contained value if Ok
     * @tparam F The type of the function to call
     * @tparam Self The type of self
     * @param f The function to call
     * @return The original result
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
     * @brief Calls a function with a reference to the contained value if Err
     * @tparam F The type of the function to call
     * @tparam Self The type of self
     * @param f The function to call
     * @return The original result
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
     * @brief Maps a Result<T, E> to Result<U, E> by applying a function to a contained Ok value
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @tparam U The return type of the function
     * @param f The function to call
     * @return Result containing the result of f if Ok, Err otherwise
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
     * @brief Maps a Result<T, E> to Result<T, F> by applying a function to a contained Err value
     * @tparam Self The type of self
     * @tparam M The type of the function to call
     * @tparam F The return type of the function
     * @param m The function to call
     * @return Result containing the value if Ok, result of m if Err
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
     * @brief Returns reference if Ok
     * @tparam Self The type of self
     * @param msg The panic message to display if Err
     * @return Reference to the contained value
     * @panics if Err with the given message
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
     * @brief Returns Err reference
     * @tparam Self The type of self
     * @param msg The panic message to display if Ok
     * @return Reference to the contained error
     * @panics if Ok with the given message
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
     * @brief Returns reference if Ok
     * @tparam Self The type of self
     * @return Reference to the contained value
     * @panics if Err
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
     * @brief Returns Err reference
     * @tparam Self The type of self
     * @return Reference to the contained error
     * @panics if Ok
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

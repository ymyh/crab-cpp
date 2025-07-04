export module crab_cpp:option;

import std;
import :panic;

export namespace crab_cpp
{

using None = std::monostate;

template<typename T>
struct Option;

/**
 * @brief Option type that can hold either a value of type T or None
 * @tparam T The type of value that can be held in the Option
 * @requires std::move_constructible<T> && !std::is_pointer_v<T> && !std::is_reference_v<T>
 */
template<typename T>
    requires (std::move_constructible<T> && !std::is_pointer_v<T> && !std::is_reference_v<T>)
struct Option<T> : std::variant<None, T>
{
    using std::variant<None, T>::variant;

    using std::variant<None, T>::operator=;

    using type = T;

    /**
     * @brief Returns None if the option is None, otherwise calls f with the wrapped value and returns the result
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @tparam U The return type of the function
     * @param f The function to call
     * @return Option containing the result of f if Some, None otherwise
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, T&>>
        requires requires (F f, T &t)
        {
            { f(t) } -> std::same_as<Option<typename U::type>>;
        }
    auto and_then(this Self&& self, F&& f) -> Option<typename U::type>
    {
        if (self.is_some())
        {
            return f(std::get<1>(self));
        }

        return None{};
    }

    /**
     * @brief Returns the contained Some value, leaving None in its place
     * @param msg The panic message to display if the value is None
     * @return The contained value
     * @panics if the value is None
     */
    auto expect_take(std::string_view msg) -> T
    {
        if (this->index() == 1)
        {
            T tmp = std::move(std::get<1>(*this));
            *this = None{};

            return tmp;
        }

        panic(msg);
    }

    /**
     * @brief Returns the reference of contained Some value
     * @tparam Self The type of self
     * @param msg The panic message to display if the value is None
     * @return Reference to the contained value
     * @panics if the value is None
     */
    template<typename Self>
    auto expect(this Self&& self, std::string_view msg) -> decltype(auto)
    {
        if (self.index() == 1)
        {
            return (std::get<1>(self));
        }

        panic(msg);
    }

    /**
     * @brief Returns true if the option is a None value
     * @return true if the option is None, false otherwise
     */
    [[nodiscard]] auto is_none() const noexcept -> bool
    {
        return this->index() == 0;
    }

    /**
     * @brief Returns true if the option is a Some value
     * @return true if the option is Some, false otherwise
     */
    [[nodiscard]] auto is_some() const noexcept -> bool
    {
        return !this->is_none();
    }

    /**
     * @brief Calls a function with a reference to the contained value if Some
     * @tparam F The type of the function to call
     * @tparam Self The type of self
     * @param f The function to call
     * @return The original option
     */
    template<typename F, typename Self>
        requires std::invocable<F, T&>
    auto inspect(this Self&& self, F&& f) -> Self
    {
        if (self.is_some())
        {
            f(std::get<1>(self));
        }

        return self;
    }

    /**
     * @brief Maps an Option<T> to Option<U> by applying a function to a contained value
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @tparam U The return type of the function
     * @param f The function to call
     * @return Option containing the result of f if Some, None otherwise
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, T&>>
        requires requires (F f, T& t)
        {
            { f(t) } -> std::same_as<U>;
        }
    auto map(this Self&& self, F&& f) -> Option<U>
    {
        if (self.is_some())
        {
            return f(std::get<1>(self));
        }

        return None{};
    }

    /**
     * @brief Computes a default function result (if none), or applies a different function to the contained value (if Some)
     * @tparam Self The type of self
     * @tparam F The type of the function to call if Some
     * @tparam D The type of the function to call if None
     * @tparam U The return type of both functions
     * @param f The function to call if Some
     * @param d The function to call if None
     * @return The result of the appropriate function
     */
    template<typename Self, typename F, typename D, typename U = typename std::invoke_result_t<F, T&>>
        requires requires (F f, T& t, D d)
        {
            { f(t) } -> std::same_as<U>;
            { d() } -> std::same_as<U>;
        }
    auto map_or_else(this Self&& self, F&& f, D&& d) -> Option<U>
    {
        if (self.is_some())
        {
            return f(std::get<1>(self));
        }

        return d();
    }

    /**
     * @brief Replaces the actual value in the option by the value given in parameter
     * @param t The new value to store
     * @return The old value if present, None otherwise
     */
    auto replace(T&& t) noexcept -> Option<T>
    {
        if (this->is_some())
        {
            auto temp = std::move(std::get<1>(*this));
            *this = std::move(t);

            return temp;
        }

        *this = std::move(t);
        return None{};
    }

    /**
     * @brief Takes the value out of the option, leaving a None in its place
     * @return The contained value
     * @panics if the value is None
     */
    auto take() noexcept -> T
    {
        return this->expect_take("Calling Option<T>::take() on a None value");
    }

    /**
     * @brief Takes the value out of the option if the predicate evaluates to true
     * @tparam P The type of the predicate function
     * @param pred The predicate function
     * @return The value if predicate returns true, None otherwise
     */
    template<typename P>
        requires (std::invocable<P, T&> && std::is_same_v<std::invoke_result_t<P, T&>, bool>)
    auto take_if(P&& pred) -> Option<T>
    {
        if (this->is_some() && std::invoke(pred, std::get<1>(*this)))
        {
            T tmp = std::move(std::get<1>(*this));
            *this = None{};
            return tmp;
        }

        return None{};
    }

    /**
     * @brief Takes the value out of the option, or returns default value if None
     * @tparam Self The type of self
     * @return The contained value or default constructed value
     */
    template<typename Self>
        requires std::is_default_constructible_v<T>
    [[nodiscard]] auto take_or_default(this Self&& self) noexcept -> T
    {
        if (self.index() == 1)
        {
            T tmp = std::move(std::get<1>(self));
            self = None{};

            return tmp;
        }

        return T{};
    }

    /**
     * @brief Takes the value out of the option, or returns result of calling f if None
     * @tparam F The type of the function to call if None
     * @param f The function to call if None
     * @return The contained value or result of f
     */
    template<typename F>
        requires requires(F f)
        {
            { f() } -> std::same_as<T>;
        }
    [[nodiscard]] auto take_or_else(F&& f) -> T
    {
        if (this->index() == 1)
        {
            T tmp = std::move(std::get<1>(*this));
            *this = None{};

            return tmp;
        }

        return f();
    }

    /**
     * @brief Returns the contained Some value reference
     * @tparam Self The type of self
     * @return Reference to the contained value
     * @panics if the value is None
     */
    template<typename Self>
    [[nodiscard]] auto unwrap(this Self&& self) noexcept -> decltype(auto)
    {
        return self.expect("Calling Option<T>::unwrap() on a None value");
    }

    /**
     * @brief Returns the contained Some value reference without checking
     * @tparam Self The type of self
     * @return Reference to the contained value
     * @warning This function is unsafe and may cause undefined behavior if called on None
     */
    template<typename Self>
    [[nodiscard]] auto unwrap_unchecked(this Self&& self) noexcept -> decltype(auto)
    {
        return (std::get<1>(self));
    }
};

/**
 * @brief Specialization for pointer type
 * @tparam T The pointer type
 */
template<typename T>
    requires std::is_pointer_v<T>
struct Option<T>
{
    using type = T;

private:
    using raw_type = std::remove_pointer_t<T>;

    T ptr;

public:
    /**
     * @brief Constructs an Option from a pointer
     * @param p The pointer to store
     */
    Option(T p) noexcept : ptr(p)
    {

    }

    /**
     * @brief Constructs a None Option
     * @param none The None value
     */
    Option(const None&) noexcept : ptr(nullptr)
    {

    }

public:
    auto operator=(const Option<T>&) noexcept -> Option<T>& = default;

    /**
     * @brief Returns true if pointer is not null
     * @return true if pointer is null, false otherwise
     */
    auto is_none() const noexcept -> bool
    {
        return this->ptr == nullptr;
    }

    /**
     * @brief Returns true if pointer is null
     * @return true if pointer is not null, false otherwise
     */
    auto is_some() const noexcept -> bool
    {
        return !this->is_none();
    }

    /**
     * @brief Returns pointer if pointer is not null, otherwise panics
     * @tparam Self The type of self
     * @param msg The panic message to display if pointer is null
     * @return The pointer
     * @panics if pointer is null
     */
    template<typename Self>
    auto expect(this Self&& self, std::string_view msg) -> decltype(auto)
    {
        if (self.is_some())
        {
            return self.ptr;
        }

        panic(msg);
    }

    /**
     * @brief Returns pointer if pointer is not null, otherwise panics
     * @param msg The panic message to display if pointer is null
     * @return The pointer
     * @panics if pointer is null
     */
    auto expect_take(std::string_view msg) -> T
    {
        if (this->is_some())
        {
            auto temp = this->ptr;
            this->ptr = nullptr;

            return temp;
        }

        panic(msg);
    }

    /**
     * @brief Calls a function with a reference to the contained value if Some
     * @tparam F The type of the function to call
     * @tparam Self The type of self
     * @param f The function to call
     * @return The original option
     */
    template<typename F, typename Self>
        requires std::invocable<F, T>
    auto inspect(this Self&& self, F&& f) -> Self
    {
        if (self.is_some())
        {
            f(self.ptr);
        }

        return self;
    }

    /**
     * @brief Maps an Option<T> to Option<U> by applying a function to a contained value
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @tparam U The return type of the function
     * @param f The function to call
     * @return Option containing the result of f if Some, None otherwise
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, T>>
        requires requires (F f, T t)
        {
            { f(t) } -> std::same_as<U>;
        }
    auto map(this Self&& self, F&& f) -> Option<U>
    {
        if (self.is_some())
        {
            return f(self.ptr);
        }

        return None{};
    }

    /**
     * @brief Replaces the actual value in the option by the value given in parameter
     * @param t The new pointer to store
     * @return The old pointer if present, None otherwise
     */
    auto replace(T t) noexcept -> Option<T>
    {
        if (this->is_some())
        {
            auto temp = this->ptr;
            this->ptr = t;

            return temp;
        }

        this->ptr = t;
        return None{};
    }

    /**
     * @brief Takes the pointer out of the option, leaving a None in its place
     * @return The contained pointer
     * @panics if the pointer is null
     */
    auto take() noexcept -> T
    {
        return this->expect_take("Calling Option<T>::take() on a None value");
    }

    /**
     * @brief Returns pointer if pointer is not null, otherwise panics
     * @tparam Self The type of self
     * @return The pointer
     * @panics if pointer is null
     */
    template<typename Self>
    auto unwrap(this Self&& self) noexcept -> decltype(auto)
    {
        return self.expect("Calling Option<T>::unwrap() on a None value");
    }

    /**
     * @brief Returns pointer if pointer is not null, otherwise panics
     * @tparam Self The type of self
     * @return The pointer
     * @warning This function is unsafe and may cause undefined behavior if called on null
     */
    template<typename Self>
    auto unwrap_unchecked(this Self&& self) noexcept -> decltype(auto)
    {
        return self.ptr;
    }
};

/**
 * @brief Specialization for lvalue references
 * @tparam T The reference type
 */
template<typename T>
    requires std::is_lvalue_reference_v<T>
struct Option<T>
{
    using type = T;

private:
    using raw_type = std::remove_reference_t<T>;

    raw_type* ptr;

public:
    /**
     * @brief Constructs an Option from a reference
     * @param t The reference to store
     */
    Option(T t) noexcept : ptr(&t)
    {

    }

    /**
     * @brief Constructs a None Option
     * @param none The None value
     */
    Option(const None&) noexcept : ptr(nullptr)
    {

    }

public:
    auto operator=(const Option<T>&) noexcept -> Option<T>& = default;

    /**
     * @brief Returns reference if pointer is not null, otherwise panics
     * @tparam Self The type of self
     * @param msg The panic message to display if pointer is null
     * @return The reference
     * @panics if pointer is null
     */
    template<typename Self>
    auto expect(this Self&& self, std::string_view msg) -> T
    {
        if (self.is_some())
        {
            return *self.ptr;
        }

        panic(msg);
    }

    /**
     * @brief Returns reference if pointer is not null, otherwise panics
     * @param msg The panic message to display if pointer is null
     * @return The reference
     * @panics if pointer is null
     */
    auto expect_take(std::string_view msg) -> T
    {
        if (this->is_some())
        {
            auto& temp = *this->ptr;
            this->ptr = nullptr;

            return temp;
        }

        panic(msg);
    }

    /**
     * @brief Returns true if the option is a None value
     * @return true if the option is None, false otherwise
     */
    auto is_none() const noexcept -> bool
    {
        return this->ptr == nullptr;
    }

    /**
     * @brief Returns true if the option is a Some value
     * @return true if the option is Some, false otherwise
     */
    auto is_some() const noexcept -> bool
    {
       return !this->is_none();
    }

    /**
     * @brief Calls a function with a reference to the contained value if Some
     * @tparam F The type of the function to call
     * @tparam Self The type of self
     * @param f The function to call
     * @return The original option
     */
    template<typename F, typename Self>
        requires std::invocable<F, T>
    auto inspect(this Self&& self, F&& f) -> Self
    {
        if (self.is_some())
        {
            f(*self.ptr);
        }

        return self;
    }

    /**
     * @brief Maps an Option<T> to Option<U> by applying a function to a contained value
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @tparam U The return type of the function
     * @param f The function to call
     * @return Option containing the result of f if Some, None otherwise
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, T>>
        requires requires (F f, T t)
        {
            { f(t) } -> std::same_as<U>;
        }
    auto map(this Self&& self, F&& f) -> Option<U>
    {
        if (self.is_some())
        {
            return f(*self.ptr);
        }

        return None{};
    }

    /**
     * @brief Replaces the actual value in the option by the value given in parameter
     * @param t The new reference to store
     * @return The old reference if present, None otherwise
     */
    auto replace(T t) noexcept -> Option<T>
    {
        if (this->is_some())
        {
            auto temp = this->ptr;
            this->ptr = std::addressof(t);

            return *temp;
        }

        this->ptr = std::addressof(t);
        return None{};
    }

    /**
     * @brief Takes the reference out of the option, leaving a None in its place
     * @return The contained reference
     * @panics if the reference is null
     */
    auto take() noexcept -> T
    {
        return this->expect_take("Calling Option<T>::take() on a None value");
    }

    /**
     * @brief Returns reference if pointer is not null, otherwise panics
     * @tparam Self The type of self
     * @return The reference
     * @panics if pointer is null
     */
    template<typename Self>
    constexpr auto unwrap(this Self&& self) noexcept -> decltype(auto)
    {
        if (self.is_some())
        {
            return (*self.ptr);
        }

        panic("Calling Option<T>::unwrap() on a None value");
    }

    /**
     * @brief Returns reference if pointer is not null, otherwise panics
     * @tparam Self The type of self
     * @return The reference
     * @warning This function is unsafe and may cause undefined behavior if called on null
     */
    template<typename Self>
    constexpr auto unwrap_unchecked(this Self&& self) noexcept -> decltype(auto)
    {
        return (*self.ptr);
    }
};

/**
 * @brief Deduction guide for Option
 * @tparam T The type of value to store in the Option
 */
template<typename T>
Option(T) -> Option<T>;

}

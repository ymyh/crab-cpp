export module crab_cpp:option;

import std;
import :panic;

export namespace crab_cpp
{

using None = std::monostate;

template<typename T>
struct Option;

template<typename T>
    requires (std::move_constructible<T> && !std::is_pointer_v<T> && !std::is_reference_v<T>)
struct Option<T> : std::variant<None, T>
{
    using std::variant<None, T>::variant;

    using std::variant<None, T>::operator=;

    using type = T;

    /**
     * Returns None if the option is None, otherwise calls f with the wrapped value and returns the result.
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, T&>::type>
        requires requires (F f, T &t)
        {
            { f(t) } -> std::same_as<Option<U>>;
        }
    auto and_then(this Self&& self, F&& f) -> Option<U>
    {
        if (self.is_some())
        {
            return f(std::get<1>(self));
        }

        return None{};
    }

    /**
     * Returns the contained Some value, leaving None in its place.
     * Panics if the value is a None with a custom panic message provided by msg.
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
     * Returns the reference of contained Some value.
     * Panics if the value is a None with a custom panic message provided by msg.
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
     * Returns true if the option is a None value.
     */
    [[nodiscard]] auto is_none() const noexcept -> bool
    {
        return this->index() == 0;
    }

    /**
     * Returns true if the option is a Some value.
     */
    [[nodiscard]] auto is_some() const noexcept -> bool
    {
        return !this->is_none();
    }

    /**
     * Calls a function with a reference to the contained value if Some.
     * Returns the original option.
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
     * Maps an Option<T> to Option<U> by applying a function to a contained value (if Some) or returns None (if None).
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, T&>::type>
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
     * Computes a default function result (if none), or applies a different function to the contained value (if Some).
     */
    template<typename Self, typename F, typename D, typename U = typename std::invoke_result_t<F, T&>::type>
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
     * Replaces the actual value in the option by the value given in parameter, returning the old value if present, leaving a Some in its place without deinitializing either one.
     */
    auto replace(T&& t) noexcept -> Option<T>
    {
        if (this->is_some())
        {
            auto temp = std::move(std::get<1>(*this));
            (*this) = std::move(t);

            return temp;
        }

        return None{};
    }

    /**
     * Takes the value out of the option, leaving a None in its place.
     */
    auto take() noexcept -> T
    {
        return this->expect_take("Calling Option<T>::take() on a None value");
    }

    /**
     * Takes the value out of the option, but only if the predicate evaluates to true on a mutable reference to the value.
     * In other words, replaces self with None if the predicate returns true. This method operates similar to Option::take but conditional.
     */
    template<typename P>
        requires (std::invocable<P, T&> && std::is_same_v<std::invoke_result_t<P, T&>, bool>)
    auto take_if(P&& pred) -> Option<T>
    {
        if (this->is_some() && std::invoke(pred, std::get<1>(*this)))
        {
            T tmp = std::move(std::get<1>(*this));
            return tmp;
        }

        return None{};
    }

    /**
     * If value present, moves the contained value, left None behind.
     * If no value present, returns default value.
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
     * If value present, moves the contained value, left None behind.
     * If no value present, the result of calling to f is returned.
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
     * Returns the contained Some value reference.
     * Because this function may panic, its use is generally discouraged. Panics are meant for unrecoverable errors, and abort the entire program.
     * Instead, prefer to use pattern matching and handle the None case explicitly.
     * Panics if the self value equals None.
     */
    template<typename Self>
    [[nodiscard]] auto unwrap(this Self&& self) noexcept -> decltype(auto)
    {
        return self.expect("Calling Option<T>::unwrap() on a None value");
    }

    template<typename Self>
    [[nodiscard]] auto unwrap_unchecked(this Self&& self) noexcept -> decltype(auto)
    {
        return (std::get<1>(self));
    }
};

/**
 * Specialization for pointer type, which has same size of size_t.
 * nullptr will be None, otherwise Some.
 */
template<typename T>
    requires std::is_pointer_v<T>
struct Option<T>
{
    using type = T;

private:
    using raw_type = std::remove_reference_t<T>;

    T ptr;

public:
    Option(T p) noexcept : ptr(p)
    {

    }

    Option(const None&) noexcept : ptr(nullptr)
    {

    }

public:
    auto operator=(const Option<T>&) noexcept -> Option<T>& = default;

    /**
     * Returns true if pointer is not null.
     */
    auto is_none() const noexcept -> bool
    {
        return this->ptr == nullptr;
    }

    /**
     * Returns true if pointer is null.
     */
    auto is_some() const noexcept -> bool
    {
        return !this->is_none();
    }

    /**
     * Returns pointer if pointer is not null, otherwise panics.
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
     * Calls a function with a reference to the contained value if Some.
     * Returns the original option.
     */
    template<typename F, typename Self>
        requires std::invocable<F, std::add_lvalue_reference_t<std::remove_pointer_t<T>>>
    auto inspect(this Self&& self, F&& f) -> Self
    {
        if (self.is_some())
        {
            f(*self.ptr);
        }

        return self;
    }

    /**
     * Maps an Option<T> to Option<U> by applying a function to a contained value (if Some) or returns None (if None).
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, raw_type&>::type>
        requires requires (F f, T& t)
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
     * Replaces the actual value in the option by the value given in parameter, returning the old value if present, leaving a Some in its place.
     */
    auto replace(T t) noexcept -> Option<T>
    {
        if (this->is_some())
        {
            auto temp = this->ptr;
            this->ptr = t;

            return temp;
        }

        return None{};
    }

    /**
     * Returns pointer if pointer is not null, otherwise panics.
     */
    template<typename Self>
    auto unwrap(this Self&& self) noexcept -> decltype(auto)
    {
        return self.expect("Calling Option<T>::unwrap() on a None value");
    }

    template<typename Self>
    auto unwrap_unchecked(this Self&& self) noexcept -> decltype(auto)
    {
        return self.ptr;
    }
};

/**
 * Specialization for lvalue references, which has same size of size_t.
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
    Option(T t) noexcept : ptr(&t)
    {

    }

    Option(const None&) noexcept : ptr(nullptr)
    {

    }

public:
    auto operator=(const Option<T>&) noexcept -> Option<T>& = default;

    /**
     * Returns true if the option is a None value.
     */
    auto is_none() const noexcept -> bool
    {
        return this->ptr == nullptr;
    }

    /**
     * Returns true if the option is a Some value.
     */
    auto is_some() const noexcept -> bool
    {
       return !this->is_none();
    }

    /**
     * Calls a function with a reference to the contained value if Some.
     * Returns the original option.
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
     * Maps an Option<T> to Option<U> by applying a function to a contained value (if Some) or returns None (if None).
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, T>::type>
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
     * Replaces the actual value in the option by the value given in parameter, returning the old value if present, leaving a Some in its place.
     */
    auto replace(T t) noexcept -> Option<T>
    {
        if (this->is_some())
        {
            auto temp = this->ptr;
            this->ptr = &t;

            return *temp;
        }

        return None{};
    }

    template<typename Self>
    constexpr auto unwrap(this Self&& self) noexcept -> decltype(auto)
    {
        if (self.is_some())
        {
            return (*self.ptr);
        }

        panic("Calling Option<T>::unwrap() on a None value");
    }

    template<typename Self>
    constexpr auto unwrap_unchecked(this Self&& self) noexcept -> decltype(auto)
    {
        return (*self.ptr);
    }
};

template<typename T>
Option(T) -> Option<T>;

template<typename T1, typename T2>
    requires requires(T1 a, T2 b) { { a + b }; }
constexpr auto operator+(const Option<T1>& lhs, const Option<T2>& rhs)
    -> Option<decltype(std::declval<T1>() + std::declval<T2>())>
{
    if (lhs.is_some() && rhs.is_some())
    {
        return lhs.unwrap_unchecked() + rhs.unwrap_unchecked();
    }
    return None{};
}

template<typename T1, typename T2>
    requires requires(T1 a, T2 b) { { a - b }; }
constexpr auto operator-(const Option<T1>& lhs, const Option<T2>& rhs)
    -> Option<decltype(std::declval<T1>() - std::declval<T2>())>
{
    if (lhs.is_some() && rhs.is_some())
    {
        return lhs.unwrap_unchecked() - rhs.unwrap_unchecked();
    }
    return None{};
}

template<typename T1, typename T2>
    requires requires(T1 a, T2 b) { { a * b }; }
constexpr auto operator*(const Option<T1>& lhs, const Option<T2>& rhs)
    -> Option<decltype(std::declval<T1>() * std::declval<T2>())>
{
    if (lhs.is_some() && rhs.is_some())
    {
        return lhs.unwrap_unchecked() * rhs.unwrap_unchecked();
    }
    return None{};
}

template<typename T1, typename T2>
    requires requires(T1 a, T2 b) { { a / b }; }
constexpr auto operator/(const Option<T1>& lhs, const Option<T2>& rhs)
    -> Option<decltype(std::declval<T1>() / std::declval<T2>())>
{
    if (lhs.is_some() && rhs.is_some())
    {
        return lhs.unwrap_unchecked() / rhs.unwrap_unchecked();
    }
    return None{};
}

template<typename T1, typename T2>
    requires requires(T1 a, T2 b) { { a & b }; }
constexpr auto operator&(const Option<T1>& lhs, const Option<T2>& rhs)
    -> Option<decltype(std::declval<T1>() & std::declval<T2>())>
{
    if (lhs.is_some() && rhs.is_some())
    {
        return lhs.unwrap_unchecked() & rhs.unwrap_unchecked();
    }
    return None{};
}


template<typename T1, typename T2>
    requires requires(T1 a, T2 b) { { a | b }; }
constexpr auto operator|(const Option<T1>& lhs, const Option<T2>& rhs)
    -> Option<decltype(std::declval<T1>() | std::declval<T2>())>
{
    if (lhs.is_some() && rhs.is_some())
    {
        return lhs.unwrap_unchecked() | rhs.unwrap_unchecked();
    }
    return None{};
}

template<typename T1, typename T2>
    requires requires(T1 a, T2 b) { { a ^ b }; }
constexpr auto operator^(const Option<T1>& lhs, const Option<T2>& rhs)
    -> Option<decltype(std::declval<T1>() ^ std::declval<T2>())>
{
    if (lhs.is_some() && rhs.is_some())
    {
        return lhs.unwrap_unchecked() ^ rhs.unwrap_unchecked();
    }
    return None{};
}

}

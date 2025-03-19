export module crab_cpp:option;

import :panic;
import std;

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
     * If value present, moves the contained value, left None behind, otherwise panics.
     */
    auto take() noexcept -> T
    {
        return this->expect_take("Calling Option<T>::take() on a None value");
    }

    /**
     * If value present, moves the contained value, left None behind.
     * If no value present, returns default value.
     */
    template<typename Self>
        requires std::is_default_constructible_v<T>
    auto take_or_default(this Self&& self) noexcept -> T
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
    auto take_or_else(F &&f) -> T
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
     * Returns reference if value present, otherwise panics.
     */
    template<typename Self>
    auto unwrap(this Self&& self) noexcept -> decltype(auto)
    {
        return self.expect("Calling Option<T>::unwrap() on a None value");
    }

    /**
     * Returns true if no value present.
     */
    [[nodiscard]] auto is_none() const noexcept -> bool
    {
        return this->index() == 0;
    }

    /**
     * Returns true if it has value present.
     */
    [[nodiscard]] auto is_some() const noexcept -> bool
    {
        return !this->is_none();
    }

    /**
     * Calls a function with the contained value if there is one.
     */
    template<typename F, typename Self>
        requires std::invocable<F, T&>
    auto inspect(this Self &&self, F &&f) -> Self
    {
        if (self.is_some())
        {
            f(std::get<1>(self));
        }

        return self;
    }

    /**
     * Maps an Option<T> to Option<U> by applying f to the contained value if present, otherwise returns None.
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, T&>::type>
        requires requires (F f, T &t)
        {
            { f(t) } -> std::same_as<Option<U>>;
        }
    auto map(this Self &&self, F &&f) -> Option<U>
    {
        if (self.is_some())
        {
            return f(std::get<1>(self));
        }

        return None{};
    }

    /**
     * Maps an Option<T> to Option<U> by applying f to the contained value if present, otherwise returns None.
     */
    template<typename Self, typename F, typename D, typename U = typename std::invoke_result_t<F, T&>::type>
        requires requires (F f, T &t, D d)
        {
            { f(t) } -> std::same_as<Option<U>>;
            { d() } -> std::same_as<Option<U>>;
        }
    auto map_or_else(this Self &&self, F &&f) -> Option<U>
    {
        if (self.is_some())
        {
            return f(std::get<1>(self));
        }

        return d();
    }
};

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
    auto unwrap(this Self&& self) noexcept -> decltype(auto)
    {
       if (self.ptr != nullptr)
       {
           return self.ptr;
       }

       panic("Calling Option<T>::unwrap() on a None value");
    }

    /**
     * Calls a function with the contained value if there is one.
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
     * Maps an Option<T> to Option<U> by applying f to the contained value if present, otherwise returns None.
     */
    template<typename Self, typename F, typename U = typename std::invoke_result_t<F, raw_type&>::type>
        requires requires (F f, T &t)
        {
            { f(t) } -> std::same_as<Option<U>>;
        }
    auto map(this Self &&self, F &&f) -> Option<U>
    {
        if (self.is_some())
        {
            return f(*self.ptr);
        }

        return None{};
    }
};

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
    /**
     * Returns true if value persent.
     */
    auto is_none() const noexcept -> bool
    {
        return this->ptr == nullptr;
    }

    /**
     * Returns true if value absent.
     */
    auto is_some() const noexcept -> bool
    {
       return !this->is_none();
    }

    template<typename Self>
    auto unwrap(this Self&& self) noexcept -> decltype(auto)
    {
        if (self.is_some())
        {
            return (*self.ptr);
        }

        panic("Calling Option<T>::unwrap() on a None value");
    }

    /**
     * Calls a function with the contained value if there is one.
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
};

template<typename T>
Option(T) -> Option<T>;

}

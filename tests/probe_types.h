#pragma once

// Included after `import std;`. Probe types for the Option/Result matrix.

namespace crab_test
{

/**
 * @brief True when an lvalue Option/Result can hand out T without moving it
 * @details Pointers copy the address. References rebind. Values need a copy constructor.
 *          int& is not std::copy_constructible, but Option<int&> still rebinds.
 */
template<typename T>
constexpr bool can_duplicate =
    std::is_pointer_v<T> ||
    std::is_lvalue_reference_v<T> ||
    std::is_copy_constructible_v<T>;

template<typename T>
constexpr bool is_indirect = std::is_pointer_v<T> || std::is_lvalue_reference_v<T>;

template<bool Default>
struct CopyProbe
{
    int m_id;

    constexpr CopyProbe() requires Default : m_id(0)
    {
    }

    constexpr explicit CopyProbe(int id) : m_id(id)
    {
    }

    constexpr CopyProbe(const CopyProbe& other) : m_id(other.m_id)
    {
    }

    constexpr CopyProbe(CopyProbe&& other) noexcept : m_id(other.m_id)
    {
        other.m_id = -1;
    }

    constexpr auto operator=(const CopyProbe& other) -> CopyProbe&
    {
        this->m_id = other.m_id;

        return *this;
    }

    constexpr auto operator=(CopyProbe&& other) noexcept -> CopyProbe&
    {
        this->m_id = other.m_id;
        other.m_id = -1;

        return *this;
    }

    [[nodiscard]] constexpr auto value() const noexcept -> int
    {
        return this->m_id;
    }

    constexpr auto operator==(const CopyProbe&) const -> bool = default;

    constexpr auto operator<=>(const CopyProbe&) const -> std::strong_ordering = default;
};

template<bool Default>
struct MoveProbe
{
    std::unique_ptr<int> m_id;

    MoveProbe() requires Default : m_id(std::make_unique<int>(0))
    {
    }

    explicit MoveProbe(int id) : m_id(std::make_unique<int>(id))
    {
    }

    MoveProbe(const MoveProbe&) = delete;

    auto operator=(const MoveProbe&) -> MoveProbe& = delete;

    MoveProbe(MoveProbe&&) noexcept = default;

    auto operator=(MoveProbe&&) noexcept -> MoveProbe& = default;

    [[nodiscard]] auto value() const noexcept -> int
    {
        if (this->m_id == nullptr)
        {
            return -1;
        }

        return *this->m_id;
    }

    auto operator==(const MoveProbe& other) const -> bool
    {
        return this->value() == other.value();
    }

    auto operator<=>(const MoveProbe& other) const -> std::strong_ordering
    {
        return this->value() <=> other.value();
    }
};

using CopyDefault = CopyProbe<true>;
using CopyNoDefault = CopyProbe<false>;
using MoveDefault = MoveProbe<true>;
using MoveNoDefault = MoveProbe<false>;

struct MapTag
{
    int m_id;

    explicit MapTag(int id) : m_id(id)
    {
    }
};

/**
 * @brief Object whose destructor updates a caller-owned flag
 */
struct Tracked
{
    int* m_flag;
    int m_id;

    Tracked(int* flag, int id) : m_flag(flag), m_id(id)
    {
        *this->m_flag += 1;
    }

    Tracked(const Tracked& other) : m_flag(other.m_flag), m_id(other.m_id)
    {
        *this->m_flag += 1;
    }

    Tracked(Tracked&& other) noexcept : m_flag(other.m_flag), m_id(other.m_id)
    {
        other.m_flag = nullptr;
    }

    auto operator=(const Tracked& other) -> Tracked&
    {
        if (this != &other)
        {
            if (this->m_flag != nullptr)
            {
                *this->m_flag -= 1;
            }

            this->m_flag = other.m_flag;
            this->m_id = other.m_id;
            *this->m_flag += 1;
        }

        return *this;
    }

    auto operator=(Tracked&& other) noexcept -> Tracked&
    {
        if (this != &other)
        {
            if (this->m_flag != nullptr)
            {
                *this->m_flag -= 1;
            }

            this->m_flag = other.m_flag;
            this->m_id = other.m_id;
            other.m_flag = nullptr;
        }

        return *this;
    }

    ~Tracked()
    {
        if (this->m_flag != nullptr)
        {
            *this->m_flag -= 1;
        }
    }

    [[nodiscard]] auto value() const noexcept -> int
    {
        return this->m_id;
    }
};

template<typename T>
[[nodiscard]] auto read_id(const std::remove_reference_t<T>& value) -> int
{
    if constexpr (std::is_pointer_v<T>)
    {
        if (value == nullptr)
        {
            return -1;
        }

        return *value;
    }
    else if constexpr (std::is_arithmetic_v<std::remove_cv_t<std::remove_reference_t<T>>>)
    {
        return static_cast<int>(value);
    }
    else
    {
        return value.value();
    }
}

template<typename Prototype, typename Object>
[[nodiscard]] auto forward_like(Object& object) -> decltype(auto)
{
    using proto = Prototype;
    using object_type = std::remove_reference_t<Object>;

    if constexpr (std::is_rvalue_reference_v<proto> && std::is_const_v<std::remove_reference_t<proto>>)
    {
        return static_cast<const object_type&&>(object);
    }
    else if constexpr (std::is_rvalue_reference_v<proto>)
    {
        return static_cast<object_type&&>(object);
    }
    else if constexpr (std::is_const_v<std::remove_reference_t<proto>>)
    {
        return static_cast<const object_type&>(object);
    }
    else
    {
        return static_cast<object_type&>(object);
    }
}

template<typename Opt>
struct access
{
    static constexpr bool is_lvalue = std::is_lvalue_reference_v<Opt>;

    static constexpr bool is_const = std::is_const_v<std::remove_reference_t<Opt>>;

    // Non-const rvalue: Option resets to None, Result moves the active value out.
    static constexpr bool consume = !is_lvalue && !is_const;
};

template<typename Make, typename F>
void for_categories(Make&& make, F&& fn)
{
    {
        auto object = make();
        SCOPED_TRACE("lvalue");
        fn(object);
    }
    {
        auto object = make();
        SCOPED_TRACE("const lvalue");
        fn(std::as_const(object));
    }
    {
        auto object = make();
        SCOPED_TRACE("rvalue");
        fn(std::move(object));
    }
    {
        auto object = make();
        SCOPED_TRACE("const rvalue");
        fn(static_cast<const std::remove_reference_t<decltype(object)>&&>(object));
    }
}

}

namespace std
{

template<bool Default>
struct hash<crab_test::CopyProbe<Default>>
{
    [[nodiscard]] auto operator()(const crab_test::CopyProbe<Default>& value) const noexcept -> std::size_t
    {
        return std::hash<int>{}(value.value());
    }
};

template<bool Default>
struct hash<crab_test::MoveProbe<Default>>
{
    [[nodiscard]] auto operator()(const crab_test::MoveProbe<Default>& value) const noexcept -> std::size_t
    {
        return std::hash<int>{}(value.value());
    }
};

}

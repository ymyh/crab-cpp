export module crab_cpp:option;

import std;
import :panic;

export namespace crab_cpp
{

using None = std::monostate;

template<typename T>
struct Option;

// Forward declaration; defined in :result (avoids a circular module dependency).
// ok_or / ok_or_else name Result in an explicit return type. The function body
// is instantiated at the call site, where Result is complete.
template<typename T, typename E>
    requires (!std::is_rvalue_reference_v<T> && !std::is_rvalue_reference_v<E>)
struct Result;

namespace internal
{
    /**
     * @brief Checks whether T is a tuple-like type with at least one element
     */
    template<typename T>
    concept tuple_like = requires
    {
        typename std::tuple_element_t<0, std::remove_cvref_t<T>>;
    };

    /**
     * @brief Checks whether T is a tuple-like type with exactly two elements
     */
    template<typename T>
    concept tuple_of_size_2 = tuple_like<T> && (std::tuple_size_v<std::remove_cvref_t<T>> == 2);

    /**
     * @brief Option type of the first element of a two-element tuple-like T
     */
    template<typename T>
    using unzip_option_first = Option<std::tuple_element_t<0, std::remove_cvref_t<T>>>;

    /**
     * @brief Option type of the second element of a two-element tuple-like T
     */
    template<typename T>
    using unzip_option_second = Option<std::tuple_element_t<1, std::remove_cvref_t<T>>>;

    /**
     * @brief Result type of Option<T>::unzip
     */
    template<typename T>
    using unzip_result_t = std::tuple<unzip_option_first<T>, unzip_option_second<T>>;

    /**
     * @brief Return type of unzip. Non-tuples use a placeholder so merely naming Option<T> does not
     *        instantiate tuple_element on T. The method itself is constrained to tuple_of_size_2.
     */
    struct unzip_unavailable
    {
    };

    template<typename T>
    struct unzip_result
    {
        using type = unzip_unavailable;
    };

    template<tuple_of_size_2 T>
    struct unzip_result<T>
    {
        using type = unzip_result_t<T>;
    };

    template<typename T>
    constexpr bool unzip_nothrow_move_v = true;

    template<tuple_of_size_2 T>
    constexpr bool unzip_nothrow_move_v<T> =
        std::is_nothrow_move_constructible_v<std::tuple_element_t<0, std::remove_cvref_t<T>>> &&
        std::is_nothrow_move_constructible_v<std::tuple_element_t<1, std::remove_cvref_t<T>>>;

    template<typename T>
    constexpr bool unzip_nothrow_copy_v = true;

    template<tuple_of_size_2 T>
    constexpr bool unzip_nothrow_copy_v<T> =
        std::is_nothrow_copy_constructible_v<std::tuple_element_t<0, std::remove_cvref_t<T>>> &&
        std::is_nothrow_copy_constructible_v<std::tuple_element_t<1, std::remove_cvref_t<T>>>;

    /**
     * @brief compare_three_way_result when T is three-way comparable, otherwise strong_ordering
     * @details The fallback keeps Option/Result instantiable for types that do not support <=>.
     *          The comparison operators stay constrained on three_way_comparable.
     */
    template<typename T>
    struct compare_category
    {
        using type = std::strong_ordering;
    };

    template<std::three_way_comparable T>
    struct compare_category<T>
    {
        using type = std::compare_three_way_result_t<T>;
    };

    template<typename T>
    using compare_category_t = typename compare_category<T>::type;

    template<typename T>
    constexpr bool nothrow_three_way_v = true;

    template<std::three_way_comparable T>
    constexpr bool nothrow_three_way_v<T> = noexcept(std::declval<const T&>() <=> std::declval<const T&>());

    /**
     * @brief Noexcept for by-value unwrap-family accessors
     * @tparam T The value type
     * @details Both copy (lvalue self) and move (rvalue self) must be noexcept.
     *          Pointers and references satisfy this because only the pointer is copied.
     */
    template<typename T>
    constexpr bool unwrap_by_value_noexcept_v =
        std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_move_constructible_v<T>;

    /**
     * @brief True when an rvalue self can be reset to None
     * @details Const rvalues cannot be assigned, so they are treated as a copy.
     */
    template<typename Self>
    constexpr bool resets_on_rvalue_v =
        !std::is_lvalue_reference_v<Self> && !std::is_const_v<std::remove_reference_t<Self>>;

    /**
     * @brief Storage for Option<T>
     * @details Values are a variant<None, T>. Pointers and lvalue references store one pointer;
     *          nullptr is None. References store the address of the referent.
     */
    template<typename T>
    struct option_storage
    {
        static constexpr bool is_pointer = std::is_pointer_v<T>;

        static constexpr bool is_reference = std::is_lvalue_reference_v<T>;

        static constexpr bool is_indirect = is_pointer || is_reference;

        using repr_t = std::conditional_t<
            is_pointer,
            T,
            std::conditional_t<
                is_reference,
                std::remove_reference_t<T>*,
                std::variant<None, T>
            >
        >;

        repr_t m_repr{};

        [[nodiscard]] constexpr auto is_some() const noexcept -> bool
        {
            if constexpr (is_indirect)
            {
                return this->m_repr != nullptr;
            }
            else
            {
                return this->m_repr.index() == 1;
            }
        }

        constexpr auto reset() noexcept -> void
        {
            if constexpr (is_indirect)
            {
                this->m_repr = nullptr;
            }
            else
            {
                this->m_repr = None{};
            }
        }

        template<typename U>
        constexpr auto emplace(U&& value)
            noexcept(is_indirect || std::is_nothrow_constructible_v<T, U&&>) -> void
        {
            if constexpr (is_pointer)
            {
                this->m_repr = static_cast<T>(std::forward<U>(value));
            }
            else if constexpr (is_reference)
            {
                static_assert(std::is_lvalue_reference_v<U>, "Option reference alternatives bind an lvalue");
                this->m_repr = std::addressof(value);
            }
            else
            {
                this->m_repr.template emplace<1>(std::forward<U>(value));
            }
        }

        template<typename Self>
        [[nodiscard]] constexpr auto get(this Self&& self) -> decltype(auto)
        {
            if constexpr (is_reference)
            {
                return *self.m_repr;
            }
            else if constexpr (is_pointer)
            {
                return std::forward<Self>(self).m_repr;
            }
            else
            {
                return std::get<1>(std::forward<Self>(self).m_repr);
            }
        }
    };

    /**
     * @brief cv-ref of Storage matching Self's value category
     */
    template<typename Storage, typename Self>
    using storage_cvref_t = std::conditional_t<
        std::is_lvalue_reference_v<Self>,
        std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const Storage&, Storage&>,
        std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const Storage&&, Storage&&>
    >;

    /**
     * @brief Argument type passed to a consuming callback for this Self
     * @details Values and pointers forward the value category. References always pass an lvalue,
     *          because the referent is borrowed and only the stored pointer is cleared.
     */
    template<typename Self, typename T>
    using contained_arg_t = decltype(std::declval<storage_cvref_t<option_storage<T>, Self>>().get());

    /**
     * @brief True when std::hash can hash the value Option<T> contains
     * @details References hash the referent. Pointers hash the pointer.
     */
    template<typename T>
    concept option_hashable =
        (
            std::is_lvalue_reference_v<T> &&
            requires (const std::remove_reference_t<T>& value)
            {
                { std::hash<std::remove_cv_t<std::remove_reference_t<T>>>{}(value) } -> std::convertible_to<std::size_t>;
            }
        ) ||
        (
            !std::is_lvalue_reference_v<T> &&
            requires (const T& value)
            {
                { std::hash<T>{}(value) } -> std::convertible_to<std::size_t>;
            }
        );
}

/**
 * @brief Option type that can hold either a value of type T or None
 * @tparam T The type of value that can be held in the Option
 * @details One implementation covers values, pointers, and lvalue references.
 *          Values live in a variant. Pointers and references store a single pointer
 *          (nullptr is None), so the object stays pointer-sized.
 *          A null pointer is None: Option cannot represent Some(nullptr).
 */
template<typename T>
    requires (
        !std::is_rvalue_reference_v<T> &&
        (
            std::is_pointer_v<T> ||
            std::is_lvalue_reference_v<T> ||
            std::move_constructible<T>
        )
    )
struct Option<T>
{
    using type = T;

private:
    using storage = internal::option_storage<T>;

    static constexpr bool stores_pointer = storage::is_pointer;

    static constexpr bool stores_reference = storage::is_reference;

    static constexpr bool stores_indirect = storage::is_indirect;

    storage m_data{};

    // zip takes the value out of another Option specialization.
    template<typename>
    friend struct Option;

    /**
     * @brief Forwards the contained value with the category consuming callbacks should see
     */
    template<typename Self>
    [[nodiscard]] constexpr auto forward_contained(this Self&& self) -> decltype(auto)
    {
        if constexpr (stores_pointer)
        {
            // Copy the address. A reference to the stored pointer would dangle if the
            // callback outlived a later reset, and a prvalue keeps the bits stable.
            return T(self.m_data.get());
        }
        else if constexpr (stores_reference || std::is_lvalue_reference_v<Self>)
        {
            return self.m_data.get();
        }
        else
        {
            return std::move(self.m_data.get());
        }
    }

    /**
     * @brief Copies or moves the contained value out, resetting a non-const rvalue self
     */
    template<typename Self>
    [[nodiscard]] constexpr auto take_out(this Self&& self)
        noexcept(internal::unwrap_by_value_noexcept_v<T>) -> T
    {
        if constexpr (stores_reference)
        {
            auto& ref = self.m_data.get();

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            return ref;
        }
        else if constexpr (internal::resets_on_rvalue_v<Self>)
        {
            T tmp(std::move(self.m_data.get()));
            self = None{};

            return tmp;
        }
        else
        {
            return T(self.m_data.get());
        }
    }

public:
    constexpr Option() noexcept = default;

    constexpr Option(const Option&) = default;

    constexpr Option(Option&&) = default;

    constexpr auto operator=(const Option&) -> Option& = default;

    constexpr auto operator=(Option&&) -> Option& = default;

    /**
     * @brief Constructs a None Option
     */
    constexpr Option(None) noexcept
    {
    }

    /**
     * @brief Constructs a Some Option from a value, pointer, or lvalue reference
     * @param value The value to store. For a reference option this must be an lvalue.
     */
    template<typename U>
        requires (
            !std::same_as<std::remove_cvref_t<U>, Option> &&
            !std::same_as<std::remove_cvref_t<U>, None> &&
            (
                (stores_reference && std::is_lvalue_reference_v<U> && std::is_convertible_v<U&&, T>) ||
                (!stores_reference && std::is_constructible_v<T, U&&>)
            )
        )
    constexpr Option(U&& value)
        noexcept(stores_indirect || std::is_nothrow_constructible_v<T, U&&>)
    {
        this->m_data.emplace(std::forward<U>(value));
    }

    /**
     * @brief Assigns None
     */
    constexpr auto operator=(None) noexcept -> Option&
    {
        this->m_data.reset();

        return *this;
    }

    /**
     * @brief Assigns a new contained value
     */
    template<typename U>
        requires (
            !std::same_as<std::remove_cvref_t<U>, Option> &&
            !std::same_as<std::remove_cvref_t<U>, None> &&
            (
                (stores_reference && std::is_lvalue_reference_v<U> && std::is_convertible_v<U&&, T>) ||
                (!stores_reference && std::is_constructible_v<T, U&&>)
            )
        )
    constexpr auto operator=(U&& value)
        noexcept(stores_indirect || std::is_nothrow_constructible_v<T, U&&>) -> Option&
    {
        this->m_data.emplace(std::forward<U>(value));

        return *this;
    }

    /**
     * @brief Returns optb if the option is Some, otherwise returns None
     * @tparam Self The type of self
     * @tparam Opt The type of optb
     * @param optb The option to return if this option is Some
     * @return optb if Some, otherwise None
     * @note Lvalue operands are copied and left unchanged. A non-const rvalue is moved from and set to None.
     *       An operand that is not returned is discarded. The returned value is taken before either operand is reset.
     *       Pointer options copy the pointer. Reference options copy the reference.
     */
    template<typename Self, typename Opt, typename U = typename std::remove_cvref_t<Opt>::type>
        requires std::same_as<std::remove_cvref_t<Opt>, Option<U>>
    [[nodiscard]] constexpr auto and_(this Self&& self, Opt&& optb)
        noexcept(internal::unwrap_by_value_noexcept_v<U>) -> Option<U>
    {
        if (self.is_some())
        {
            Option<U> result(std::forward<Opt>(optb));

            if constexpr (internal::resets_on_rvalue_v<Opt>)
            {
                optb = None{};
            }

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            return result;
        }

        if constexpr (internal::resets_on_rvalue_v<Self>)
        {
            self = None{};
        }

        if constexpr (internal::resets_on_rvalue_v<Opt>)
        {
            optb = None{};
        }

        return None{};
    }

    /**
     * @brief Returns None if the option is None, otherwise calls f with the wrapped value and returns the result
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @param f The function to call
     * @return Option containing the result of f if Some, None otherwise
     * @note Lvalue self passes an lvalue and is left unchanged. Non-const rvalue self passes an rvalue
     *       (a reference option still passes an lvalue to the referent) and is set to None.
     */
    template<
        typename Self,
        typename F,
        typename Arg = internal::contained_arg_t<Self, T>,
        typename Ret = std::remove_cvref_t<std::invoke_result_t<F, Arg>>
    >
        requires (
            std::invocable<F, Arg> &&
            requires { typename Ret::type; } &&
            std::same_as<Ret, Option<typename Ret::type>>
        )
    [[nodiscard]] constexpr auto and_then(this Self&& self, F&& f) -> Option<typename Ret::type>
    {
        if (self.is_some())
        {
            auto result = f(std::forward<Self>(self).forward_contained());

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            return result;
        }

        return None{};
    }

    /**
     * @brief Returns true if the option is Some and the contained value equals the given value
     * @tparam U The type of the value to compare
     * @param value The value to compare
     * @return true if Some and the contained value equals value, false otherwise
     * @note Pointer options compare addresses. Reference options compare the referents.
     */
    template<typename U>
        requires std::equality_comparable_with<std::remove_reference_t<T>, std::remove_cvref_t<U>>
    [[nodiscard]] constexpr auto contains(U&& value) const
        noexcept(noexcept(
            std::declval<const std::remove_reference_t<T>&>() ==
            std::declval<const std::remove_cvref_t<U>&>()
        )) -> bool
    {
        return this->is_some() && this->m_data.get() == std::forward<U>(value);
    }

    /**
     * @brief Returns the contained Some value by value, or panics if None
     * @tparam Self The type of self
     * @param msg The panic message to display if the value is None
     * @return The contained value. Lvalue self copies. Non-const rvalue self moves out and becomes None.
     *         Pointer options copy the pointer. Reference options return the reference. An rvalue of
     *         either becomes None.
     * @note Panics if the value is None
     */
    template<typename Self>
    [[nodiscard]] constexpr auto expect(this Self&& self, std::string_view msg)
        noexcept(internal::unwrap_by_value_noexcept_v<T>) -> T
    {
        if (self.is_some())
        {
            return std::forward<Self>(self).take_out();
        }

        panic(msg);
    }

    /**
     * @brief Returns true if the option is a None value
     * @return true if the option is None, false otherwise
     */
    [[nodiscard]] constexpr auto is_none() const noexcept -> bool
    {
        return !this->m_data.is_some();
    }

    /**
     * @brief Returns true if the option is a Some value
     * @return true if the option is Some, false otherwise
     */
    [[nodiscard]] constexpr auto is_some() const noexcept -> bool
    {
        return this->m_data.is_some();
    }

    /**
     * @brief Returns None if the option is None, otherwise returns Some if the predicate is true
     * @tparam Self The type of self
     * @tparam F The type of the predicate function
     * @param pred The predicate function. It borrows the value.
     * @return The option if Some and pred(value) is true, otherwise None
     * @note Lvalue self is left unchanged. Non-const rvalue self is set to None whether or not the predicate holds.
     */
    template<
        typename Self,
        typename F,
        typename Arg = internal::contained_arg_t<Self&, T>
    >
        requires (
            std::invocable<F, Arg> &&
            std::same_as<std::invoke_result_t<F, Arg>, bool>
        )
    [[nodiscard]] constexpr auto filter(this Self&& self, F&& pred)
        noexcept(
            std::is_nothrow_invocable_v<F, Arg> &&
            (
                stores_indirect ||
                (
                    internal::resets_on_rvalue_v<Self>
                        ? std::is_nothrow_move_constructible_v<T>
                        : std::is_nothrow_copy_constructible_v<T>
                )
            )
        ) -> Option
    {
        if (self.is_some() && pred(self.m_data.get()))
        {
            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                Option result(std::move(self));
                self = None{};

                return result;
            }
            else
            {
                return self;
            }
        }

        if constexpr (internal::resets_on_rvalue_v<Self>)
        {
            self = None{};
        }

        return None{};
    }

    template<typename Self>
        requires std::same_as<T, Option<typename T::type>>
    [[nodiscard]] constexpr auto flatten(this Self&& self) -> T
    {
        if (self.is_none())
        {
            return None{};
        }

        if constexpr (internal::resets_on_rvalue_v<Self>)
        {
            T inner(std::move(self.m_data.get()));
            self = None{};
            return inner;
        }
        else
        {
            return self.m_data.get();
        }
    }

    /**
     * @brief Inserts a value, dropping the previous one if Some, and returns a reference to it
     * @tparam U A type from which T can be constructed
     * @param value The value to insert
     * @return A reference to the contained value
     * @note Not provided for pointer or reference options; those do not own a T.
     *       The new value is constructed before the old one is removed. If that constructor throws,
     *       this option is left unchanged.
     */
    template<typename U>
        requires (!stores_indirect && std::is_constructible_v<T, U&&>)
    [[nodiscard]] constexpr auto insert(U&& value) &
        noexcept(
            std::is_nothrow_constructible_v<T, U&&> &&
            std::is_nothrow_move_constructible_v<T>
        ) -> T&
    {
        T created(std::forward<U>(value));
        this->m_data.emplace(std::move(created));

        return this->m_data.get();
    }

    /**
     * @brief Returns the contained value, inserting value first when None
     * @tparam U A type from which T can be constructed
     * @param value The value to insert if None
     * @return A reference to the contained value
     * @note Not provided for pointer or reference options; those do not own a T.
     *       value is constructed into T only when this option is None, so a Some option does not copy it.
     */
    template<typename U>
        requires (!stores_indirect && std::is_constructible_v<T, U&&>)
    [[nodiscard]] constexpr auto get_or_insert(U&& value) &
        noexcept(
            std::is_nothrow_constructible_v<T, U&&> &&
            std::is_nothrow_move_constructible_v<T>
        ) -> T&
    {
        if (this->is_none())
        {
            T created(std::forward<U>(value));
            this->m_data.emplace(std::move(created));
        }

        return this->m_data.get();
    }

    /**
     * @brief Returns the contained value, default-constructing T first when None
     * @return A reference to the contained value
     * @note Not provided for pointer or reference options, or when T is not default constructible.
     */
    [[nodiscard]] constexpr auto get_or_insert_default() &
        noexcept(
            std::is_nothrow_default_constructible_v<T> &&
            std::is_nothrow_move_constructible_v<T>
        ) -> T&
        requires (!stores_indirect && std::is_default_constructible_v<T>)
    {
        if (this->is_none())
        {
            T created{};
            this->m_data.emplace(std::move(created));
        }

        return this->m_data.get();
    }

    /**
     * @brief Returns the contained value, calling f to produce one when None
     * @tparam F The type of the function to call
     * @param f The function to call if None. It is not called when this option is already Some.
     * @return A reference to the contained value
     * @note Not provided for pointer or reference options; those do not own a T.
     *       f's result only needs to be constructible into T.
     */
    template<typename F>
        requires (
            !stores_indirect &&
            std::invocable<F> &&
            std::is_constructible_v<T, std::invoke_result_t<F>>
        )
    [[nodiscard]] constexpr auto get_or_insert_with(F&& f) &
        noexcept(
            std::is_nothrow_invocable_v<F> &&
            std::is_nothrow_constructible_v<T, std::invoke_result_t<F>> &&
            std::is_nothrow_move_constructible_v<T>
        ) -> T&
    {
        if (this->is_none())
        {
            T created(f());
            this->m_data.emplace(std::move(created));
        }

        return this->m_data.get();
    }

    template<
        typename Self,
        typename F,
        typename Arg = internal::contained_arg_t<Self&, T>
    >
        requires (
            std::invocable<F, Arg> &&
            std::same_as<std::invoke_result_t<F, Arg>, bool>
        )
    [[nodiscard]] constexpr auto is_none_or(this Self&& self, F&& f) -> bool
    {
        return self.is_none() || f(self.m_data.get());
    }

    /**
     * @brief Returns true if the option is Some and the predicate returns true for the contained value
     * @tparam Self The type of self
     * @tparam F The type of the predicate function
     * @param f The predicate function
     * @return true if Some and f(value) is true, false otherwise
     * @note The predicate borrows the value. self is left unchanged.
     */
    template<
        typename Self,
        typename F,
        typename Arg = internal::contained_arg_t<Self&, T>
    >
        requires (
            std::invocable<F, Arg> &&
            std::same_as<std::invoke_result_t<F, Arg>, bool>
        )
    [[nodiscard]] constexpr auto is_some_and(this Self&& self, F&& f) -> bool
    {
        return self.is_some() && f(self.m_data.get());
    }

    /**
     * @brief Calls a function with a reference to the contained value if Some
     * @tparam F The type of the function to call
     * @tparam Self The type of self
     * @param f The function to call
     * @return Lvalue self is returned by reference. Rvalue self is returned by value, so a chain
     *         such as make().inspect(f).unwrap() still consumes, and binding the result extends
     *         the returned temporary.
     * @note The callback always borrows. inspect does not set self to None.
     */
    template<
        typename F,
        typename Self,
        typename Arg = internal::contained_arg_t<Self&, T>
    >
        requires std::invocable<F, Arg>
    constexpr auto inspect(this Self&& self, F&& f)
        -> std::conditional_t<
            std::is_lvalue_reference_v<Self>,
            Self&,
            std::remove_cvref_t<Self>
        >
    {
        if (self.is_some())
        {
            f(self.m_data.get());
        }

        if constexpr (std::is_lvalue_reference_v<Self>)
        {
            return self;
        }
        else
        {
            return std::move(self);
        }
    }

    /**
     * @brief Maps an Option<T> to Option<U> by applying a function to a contained value
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @param f The function to call
     * @return Option containing the result of f if Some, None otherwise
     * @note Lvalue self passes an lvalue and is left unchanged. Non-const rvalue self passes an rvalue
     *       (a reference option still passes an lvalue to the referent) and is set to None.
     */
    template<
        typename Self,
        typename F,
        typename Arg = internal::contained_arg_t<Self, T>,
        typename U = std::invoke_result_t<F, Arg>
    >
        requires std::invocable<F, Arg>
    [[nodiscard]] constexpr auto map(this Self&& self, F&& f) -> Option<U>
    {
        if (self.is_some())
        {
            auto result = f(std::forward<Self>(self).forward_contained());

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            return result;
        }

        return None{};
    }

    template<
        typename Self,
        typename F,
        typename Arg = internal::contained_arg_t<Self, T>,
        typename U = std::invoke_result_t<F, Arg>
    >
        requires (
            std::is_default_constructible_v<U> &&
            std::invocable<F, Arg>
        )
    [[nodiscard]] constexpr auto map_or_default(this Self&& self, F&& f) -> U
    {
        if (self.is_some())
        {
            auto result = f(std::forward<Self>(self).forward_contained());

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            return result;
        }

        return U{};
    }

    /**
     * @brief Computes a default function result (if none), or applies a different function to the contained value (if Some)
     * @tparam Self The type of self
     * @tparam F The type of the function to call if Some
     * @tparam D The type of the function to call if None
     * @param f The function to call if Some
     * @param d The function to call if None
     * @return The result of the appropriate function
     * @note Lvalue self passes an lvalue and is left unchanged. Non-const rvalue self passes an rvalue and is set to None.
     */
    template<
        typename Self,
        typename F,
        typename D,
        typename Arg = internal::contained_arg_t<Self, T>,
        typename U = std::invoke_result_t<F, Arg>
    >
        requires (
            std::invocable<F, Arg> &&
            std::invocable<D> &&
            std::same_as<std::invoke_result_t<D>, U>
        )
    [[nodiscard]] constexpr auto map_or_else(this Self&& self, D&& d, F&& f) -> U
    {
        if (self.is_some())
        {
            auto result = f(std::forward<Self>(self).forward_contained());

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            return result;
        }

        return d();
    }

    /**
     * @brief Maps an Option<T> to U by applying a function to a contained value, otherwise returns the provided default
     * @tparam Self The type of self
     * @tparam U The return type of the function
     * @tparam F The type of the function to call
     * @param default_value The default value to return if None
     * @param f The function to call if Some
     * @return The result of f if Some, otherwise default_value
     * @note Lvalue self is left unchanged. Non-const rvalue self is set to None.
     */
    template<
        typename Self,
        typename U,
        typename F,
        typename Arg = internal::contained_arg_t<Self, T>
    >
        requires (
            std::invocable<F, Arg> &&
            std::convertible_to<std::invoke_result_t<F, Arg>, U>
        )
    [[nodiscard]] constexpr auto map_or(this Self&& self, U&& default_value, F&& f) -> U
    {
        if (self.is_some())
        {
            auto result = f(std::forward<Self>(self).forward_contained());

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            return result;
        }

        return std::forward<U>(default_value);
    }

    /**
     * @brief Returns the option if it contains a value, otherwise returns the provided option
     * @tparam Self The type of self
     * @tparam Opt The type of optb
     * @param optb The option to return if this option is None
     * @return The original option if it contains a value, otherwise optb
     * @note Lvalue operands are copied and left unchanged. A non-const rvalue is moved from and set to None.
     *       When self is Some, an rvalue optb is discarded. The returned value is taken before either operand is reset.
     *       Pointer options copy the pointer. Reference options copy the reference.
     */
    template<typename Self, typename Opt>
        requires std::same_as<std::remove_cvref_t<Opt>, Option>
    [[nodiscard]] constexpr auto or_(this Self&& self, Opt&& optb)
        noexcept(internal::unwrap_by_value_noexcept_v<T>) -> Option
    {
        if (self.is_some())
        {
            Option result(std::forward<Self>(self));

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            if constexpr (internal::resets_on_rvalue_v<Opt>)
            {
                optb = None{};
            }

            return result;
        }

        Option result(std::forward<Opt>(optb));

        if constexpr (internal::resets_on_rvalue_v<Opt>)
        {
            optb = None{};
        }

        return result;
    }

    /**
     * @brief Returns the option if it contains a value, otherwise calls f and returns the result
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @param f The function to call if None
     * @return The original option if Some, otherwise the result of f
     * @note Lvalue self is copied and left unchanged. Non-const rvalue self is moved out and set to None.
     */
    template<typename Self, typename F>
        requires (
            std::invocable<F> &&
            std::same_as<std::remove_cvref_t<std::invoke_result_t<F>>, Option>
        )
    [[nodiscard]] constexpr auto or_else(this Self&& self, F&& f)
        noexcept(
            std::is_nothrow_invocable_v<F> &&
            internal::unwrap_by_value_noexcept_v<T>
        ) -> Option
    {
        if (self.is_some())
        {
            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                Option result(std::move(self));
                self = None{};

                return result;
            }
            else
            {
                return self;
            }
        }

        return f();
    }

    /**
     * @brief Replaces the actual value in the option by the value given in parameter
     * @tparam U The type of the new value
     * @param value The new value to store
     * @return The old value if present, None otherwise
     * @note The new value is constructed before the old one is removed. If that constructor throws,
     *       this option is left unchanged.
     */
    template<typename U>
        requires (
            (stores_reference && std::is_lvalue_reference_v<U> && std::is_convertible_v<U&&, T>) ||
            (!stores_reference && std::is_constructible_v<T, U&&>)
        )
    [[nodiscard]] constexpr auto replace(U&& value)
        noexcept(
            stores_indirect ||
            (std::is_nothrow_constructible_v<T, U&&> && std::is_nothrow_move_constructible_v<T>)
        ) -> Option
    {
        if constexpr (stores_indirect)
        {
            Option old(None{});

            if (this->is_some())
            {
                old = *this;
            }

            this->m_data.emplace(std::forward<U>(value));

            return old;
        }
        else
        {
            T created(std::forward<U>(value));
            Option old(None{});

            if (this->is_some())
            {
                old.m_data.emplace(std::move(this->m_data.get()));
            }

            this->m_data.emplace(std::move(created));

            return old;
        }
    }

    /**
     * @brief Returns the contained Some value by value, or panics if None
     * @tparam Self The type of self
     * @return The contained value. Lvalue self copies. Non-const rvalue self moves out and becomes None.
     * @note Panics if the value is None
     */
    template<typename Self>
    [[nodiscard]] constexpr auto unwrap(this Self&& self)
        noexcept(internal::unwrap_by_value_noexcept_v<T>) -> T
    {
        return std::forward<Self>(self).expect("Calling Option<T>::unwrap() on a None value");
    }

    /**
     * @brief Returns the contained Some value, or the provided default if None
     * @tparam Self The type of self
     * @tparam U The type of the default value
     * @param default_value The default value to return if None
     * @return The contained value if Some, otherwise default_value
     * @note Non-const rvalue self moves the value out and is set to None.
     *       A reference option only accepts an lvalue default, so the result cannot bind a temporary.
     */
    template<typename Self, typename U>
        requires (
            std::convertible_to<U, T> &&
            (!stores_reference || std::is_lvalue_reference_v<U>)
        )
    [[nodiscard]] constexpr auto unwrap_or(this Self&& self, U&& default_value)
        noexcept(
            internal::unwrap_by_value_noexcept_v<T> &&
            std::is_nothrow_constructible_v<T, U&&>
        ) -> T
    {
        if (self.is_some())
        {
            return std::forward<Self>(self).take_out();
        }

        return T(std::forward<U>(default_value));
    }

    /**
     * @brief Returns the contained Some value, or computes it from a closure if None
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @param f The function to call if None
     * @return The contained value if Some, otherwise the result of f
     * @note Non-const rvalue self moves the value out and is set to None.
     *       A reference option only accepts a function that returns an lvalue, so the result cannot bind a temporary.
     */
    template<typename Self, typename F>
        requires (
            std::invocable<F> &&
            std::convertible_to<std::invoke_result_t<F>, T> &&
            (!stores_reference || std::is_lvalue_reference_v<std::invoke_result_t<F>>)
        )
    [[nodiscard]] constexpr auto unwrap_or_else(this Self&& self, F&& f)
        noexcept(
            internal::unwrap_by_value_noexcept_v<T> &&
            std::is_nothrow_invocable_v<F> &&
            std::is_nothrow_constructible_v<T, std::invoke_result_t<F>>
        ) -> T
    {
        if (self.is_some())
        {
            return std::forward<Self>(self).take_out();
        }

        return T(f());
    }

    /**
     * @brief Returns the contained Some value, or the default-constructed value of T if None
     * @tparam Self The type of self
     * @return The contained value if Some, otherwise T{}
     * @note Reference options have no default value, so this method is not provided for them.
     *       Non-const rvalue self moves the value out and is set to None.
     */
    template<typename Self>
        requires (!stores_reference && std::is_default_constructible_v<T>)
    [[nodiscard]] constexpr auto unwrap_or_default(this Self&& self)
        noexcept(
            internal::unwrap_by_value_noexcept_v<T> &&
            std::is_nothrow_default_constructible_v<T>
        ) -> T
    {
        if (self.is_some())
        {
            return std::forward<Self>(self).take_out();
        }

        return T{};
    }

    /**
     * @brief Transforms the Option<T> into a Result<T, E>, mapping Some to Ok and None to Err
     * @tparam Self The type of self
     * @tparam E The type of the error
     * @param err The error to return if None
     * @return Ok(value) if Some, otherwise Err(err)
     * @note Non-const rvalue self moves the value out and is set to None.
     *       Reference options name Result of a reference; that return type is not instantiated until called.
     */
    template<typename Self, typename E>
    [[nodiscard]] constexpr auto ok_or(this Self&& self, E&& err)
        noexcept(
            internal::unwrap_by_value_noexcept_v<T> &&
            std::is_nothrow_constructible_v<std::remove_cvref_t<E>, E>
        ) -> Result<T, std::remove_cvref_t<E>>
    {
        using err_type = std::remove_cvref_t<E>;

        if (self.is_some())
        {
            return Result<T, err_type>(std::in_place_index<0>, std::forward<Self>(self).take_out());
        }

        return Result<T, err_type>(std::in_place_index<1>, std::forward<E>(err));
    }

    /**
     * @brief Transforms the Option<T> into a Result<T, E>, mapping Some to Ok and None to Err(f())
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @tparam E The return type of the function
     * @param f The function to call if None
     * @return Ok(value) if Some, otherwise Err(f())
     * @note Non-const rvalue self moves the value out and is set to None.
     */
    template<typename Self, typename F, typename E = std::remove_cvref_t<std::invoke_result_t<F>>>
        requires std::invocable<F>
    [[nodiscard]] constexpr auto ok_or_else(this Self&& self, F&& f)
        noexcept(
            internal::unwrap_by_value_noexcept_v<T> &&
            std::is_nothrow_invocable_v<F> &&
            std::is_nothrow_constructible_v<E, std::invoke_result_t<F>>
        ) -> Result<T, E>
    {
        if (self.is_some())
        {
            return Result<T, E>(std::in_place_index<0>, std::forward<Self>(self).take_out());
        }

        return Result<T, E>(std::in_place_index<1>, f());
    }

    /**
     * @brief Converts from Option<T> to Option<const T&>, borrowing the contained value if Some
     * @return Option<const T&> referencing the contained value if Some, otherwise None
     * @note Not provided for pointer or reference options.
     */
    [[nodiscard]] constexpr auto as_ref() const && = delete;

    [[nodiscard]] constexpr auto as_ref() const & noexcept -> Option<const T&>
        requires (!stores_indirect)
    {
        if (this->is_some())
        {
            return Option<const T&>(this->m_data.get());
        }

        return None{};
    }

    /**
     * @brief Converts from Option<T> to Option<T&>, borrowing the contained value if Some
     * @return Option<T&> referencing the contained value if Some, otherwise None
     * @note Not provided for pointer or reference options.
     */
    [[nodiscard]] constexpr auto as_mut() && = delete;

    [[nodiscard]] constexpr auto as_mut() & noexcept -> Option<T&>
        requires (!stores_indirect)
    {
        if (this->is_some())
        {
            return Option<T&>(this->m_data.get());
        }

        return None{};
    }

    /**
     * @brief Returns the contained Some value by value without checking
     * @tparam Self The type of self
     * @return The contained value. Lvalue self copies. Non-const rvalue self moves out and becomes None.
     * @warning This function is unsafe and may cause undefined behavior if called on None
     */
    template<typename Self>
    [[nodiscard]] constexpr auto unwrap_unchecked(this Self&& self)
        noexcept(internal::unwrap_by_value_noexcept_v<T>) -> T
    {
        if constexpr (stores_reference)
        {
            auto& ref = *self.m_data.m_repr;

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            return ref;
        }
        else if constexpr (stores_pointer)
        {
            T ptr = self.m_data.m_repr;

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            return ptr;
        }
        else if constexpr (internal::resets_on_rvalue_v<Self>)
        {
            T tmp(std::move(*std::get_if<1>(&self.m_data.m_repr)));
            self = None{};

            return tmp;
        }
        else
        {
            return T(*std::get_if<1>(&self.m_data.m_repr));
        }
    }

    /**
     * @brief Unzips an option containing a two-element tuple
     * If self is Some((a, b)) this method returns (Some(a), Some(b)). Otherwise, (None, None) is returned.
     * @tparam Self The type of self
     * @return A tuple holding an Option for each element
     * @note Lvalue self copies both elements and is left unchanged. Non-const rvalue self moves both out and becomes None.
     *       Not provided for pointer or reference options; those would borrow the pointee rather than move a value.
     */
    template<typename Self>
        requires (!stores_indirect && internal::tuple_of_size_2<T>)
    [[nodiscard]] constexpr auto unzip(this Self&& self)
        noexcept(
            internal::resets_on_rvalue_v<Self>
                ? internal::unzip_nothrow_move_v<T>
                : internal::unzip_nothrow_copy_v<T>
        ) -> typename internal::unzip_result<T>::type
    {
        using first_type = std::tuple_element_t<0, std::remove_cvref_t<T>>;
        using second_type = std::tuple_element_t<1, std::remove_cvref_t<T>>;

        if (!self.is_some())
        {
            return internal::unzip_result_t<T>{ Option<first_type>(None{}), Option<second_type>(None{}) };
        }

        if constexpr (internal::resets_on_rvalue_v<Self>)
        {
            auto result = internal::unzip_result_t<T>{
                Option<first_type>(std::forward<first_type>(std::get<0>(std::move(self.m_data.get())))),
                Option<second_type>(std::forward<second_type>(std::get<1>(std::move(self.m_data.get()))))
            };
            self = None{};

            return result;
        }
        else
        {
            return internal::unzip_result_t<T>{
                Option<first_type>(std::get<0>(self.m_data.get())),
                Option<second_type>(std::get<1>(self.m_data.get()))
            };
        }
    }

    /**
     * @brief Invokes f with None, or with the contained value
     * @tparam Self The type of self
     * @tparam F The visitor, invocable with None and with the contained value
     * @param f The visitor
     * @return Whatever f returns. Both arms must return the same type.
     * @note Lvalue self passes T& (const T& when self is const). Rvalue self passes T&&.
     *       Pointer options pass the pointer. Reference options pass an lvalue to the referent.
     *       The option is not reset; this is a visit, not a consuming combinator.
     */
    template<typename Self, typename F>
    [[nodiscard]] constexpr auto visit(this Self&& self, F&& f) -> decltype(auto)
    {
        if (self.is_none())
        {
            return std::invoke(std::forward<F>(f), None{});
        }

        return std::invoke(std::forward<F>(f), std::forward<Self>(self).forward_contained());
    }

    /**
     * @brief Returns Some when exactly one of self and optb is Some, otherwise None
     * @tparam Self The type of self
     * @tparam Opt The type of optb
     * @param optb The option to compare with
     * @return The unique Some, or None when both are Some or both are None
     * @note Lvalue operands are copied and left unchanged. A non-const rvalue is moved from and set to None.
     *       When both are Some, both rvalues are discarded.
     *       Pointer options copy the pointer. Reference options copy the reference.
     */
    template<typename Self, typename Opt>
        requires std::same_as<std::remove_cvref_t<Opt>, Option>
    [[nodiscard]] constexpr auto xor_(this Self&& self, Opt&& optb)
        noexcept(internal::unwrap_by_value_noexcept_v<T>) -> Option
    {
        if (self.is_some() && optb.is_some())
        {
            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            if constexpr (internal::resets_on_rvalue_v<Opt>)
            {
                optb = None{};
            }

            return None{};
        }

        if (self.is_some())
        {
            Option result(std::forward<Self>(self));

            if constexpr (internal::resets_on_rvalue_v<Self>)
            {
                self = None{};
            }

            return result;
        }

        Option result(std::forward<Opt>(optb));

        if constexpr (internal::resets_on_rvalue_v<Opt>)
        {
            optb = None{};
        }

        return result;
    }

    /**
     * @brief Returns Some of both values when both options are Some, otherwise None
     * @tparam Self The type of self
     * @tparam Opt The type of the other option
     * @param optb The option to zip with
     * @return An option of (self's value, optb's value) when both are Some, otherwise None
     * @note Lvalue operands are copied and left unchanged. A non-const rvalue is moved from and set to None,
     *       including when the result is None and the value is discarded.
     *       Pointer options copy the pointer into the tuple. Reference options store a reference to the referent.
     *       A non-const rvalue must not refer to the same object as the other operand.
     */
    template<
        typename Self,
        typename Opt,
        typename U = typename std::remove_cvref_t<Opt>::type
    >
        requires std::same_as<std::remove_cvref_t<Opt>, Option<U>>
    [[nodiscard]] constexpr auto zip(this Self&& self, Opt&& optb)
        noexcept(
            internal::unwrap_by_value_noexcept_v<T> &&
            internal::unwrap_by_value_noexcept_v<U>
        ) -> Option<std::tuple<T, U>>
    {
        using zipped = std::tuple<T, U>;

        if (self.is_some() && optb.is_some())
        {
            T first = std::forward<Self>(self).take_out();
            U second = std::forward<Opt>(optb).take_out();

            return Option<zipped>(zipped(std::forward<T>(first), std::forward<U>(second)));
        }

        if constexpr (internal::resets_on_rvalue_v<Self>)
        {
            self = None{};
        }

        if constexpr (internal::resets_on_rvalue_v<Opt>)
        {
            optb = None{};
        }

        return None{};
    }

    /**
     * @brief Compares two Options for equality
     * @param lhs The left option
     * @param rhs The right option
     * @return true if both are None, or both are Some with equal values
     * @note Pointer options compare addresses. Reference options compare the referents.
     */
    friend constexpr auto operator==(const Option& lhs, const Option& rhs)
        noexcept(noexcept(
            std::declval<const std::remove_reference_t<T>&>() ==
            std::declval<const std::remove_reference_t<T>&>()
        )) -> bool
        requires std::equality_comparable<std::remove_reference_t<T>>
    {
        if (lhs.is_none() || rhs.is_none())
        {
            return lhs.is_none() && rhs.is_none();
        }

        return lhs.m_data.get() == rhs.m_data.get();
    }

    /**
     * @brief Compares two pointer Options for ordering
     * @param lhs The left option
     * @param rhs The right option
     * @return std::strong_ordering over None < Some, then a total order on addresses
     * @note Built-in pointer <=> is unspecified for pointers that do not address the same array.
     *       This order uses std::less, which is a strict total order on pointer values.
     */
    friend constexpr auto operator<=>(const Option& lhs, const Option& rhs) noexcept -> std::strong_ordering
        requires stores_pointer
    {
        if (lhs.is_none() && rhs.is_none())
        {
            return std::strong_ordering::equal;
        }

        if (lhs.is_none())
        {
            return std::strong_ordering::less;
        }

        if (rhs.is_none())
        {
            return std::strong_ordering::greater;
        }

        const auto lhs_ptr = lhs.m_data.get();
        const auto rhs_ptr = rhs.m_data.get();

        if (std::less<>{}(lhs_ptr, rhs_ptr))
        {
            return std::strong_ordering::less;
        }

        if (std::less<>{}(rhs_ptr, lhs_ptr))
        {
            return std::strong_ordering::greater;
        }

        return std::strong_ordering::equal;
    }

    /**
     * @brief Compares two Options for ordering
     * @param lhs The left option
     * @param rhs The right option
     * @return Comparison of None < Some, then the contained values. The category follows the values,
     *         so Option<double> yields std::partial_ordering.
     * @note Reference options compare the referents.
     */
    friend constexpr auto operator<=>(const Option& lhs, const Option& rhs)
        noexcept(internal::nothrow_three_way_v<std::remove_reference_t<T>>)
        -> std::common_comparison_category_t<
            std::strong_ordering,
            internal::compare_category_t<const std::remove_reference_t<T>&>
        >
        requires (!stores_pointer && std::three_way_comparable<std::remove_reference_t<T>>)
    {
        if (lhs.is_none() && rhs.is_none())
        {
            return std::strong_ordering::equal;
        }

        if (lhs.is_none())
        {
            return std::strong_ordering::less;
        }

        if (rhs.is_none())
        {
            return std::strong_ordering::greater;
        }

        return lhs.m_data.get() <=> rhs.m_data.get();
    }

    /**
     * @brief Hash of an Option, mixing a Some/None tag with the value hash
     * @param opt The option to hash
     * @return Hash of None, or of Some mixed with the contained value
     * @note References hash the referent. Pointers hash the address.
     */
    friend constexpr auto hash_value(const Option& opt) -> std::size_t
        requires internal::option_hashable<T>
    {
        if (opt.is_none())
        {
            return std::hash<std::size_t>{}(0);
        }

        std::size_t seed = std::hash<std::size_t>{}(1);

        if constexpr (stores_reference)
        {
            using hashed = std::remove_cv_t<std::remove_reference_t<T>>;
            seed ^= std::hash<hashed>{}(opt.m_data.get()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        else
        {
            seed ^= std::hash<T>{}(opt.m_data.get()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        return seed;
    }
};

/**
 * @brief Deduction guide for Option
 * @tparam T The type of value to store in the Option
 */
template<typename T>
Option(T) -> Option<T>;

}

namespace std
{
    /**
     * @brief Hash support for Option<T>
     * @tparam T The value type
     */
    template<typename T>
        requires crab_cpp::internal::option_hashable<T>
    struct hash<crab_cpp::Option<T>>
    {
        auto operator()(const crab_cpp::Option<T>& opt) const -> size_t
        {
            return hash_value(opt);
        }
    };
}

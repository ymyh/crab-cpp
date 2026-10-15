export module crab_cpp:result;

import std;
import :panic;
import :option;

export namespace crab_cpp
{

namespace internal
{
    /**
     * @brief Value category of V as seen through Self
     * @details Lvalue self passes V& / const V&. Rvalue self passes V&& / const V&&.
     */
    template<typename Self, typename V>
    using forward_arg_t = std::conditional_t<
        std::is_lvalue_reference_v<Self>,
        std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const V&, V&>,
        std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const V&&, V&&>
    >;

    /**
     * @brief Argument passed to a consuming Result callback
     * @details Reference alternatives always pass an lvalue to the referent. Values forward Self's category.
     */
    template<typename Self, typename Alt>
    using result_forward_t = std::conditional_t<
        std::is_lvalue_reference_v<Alt>,
        Alt,
        forward_arg_t<Self, Alt>
    >;

    /**
     * @brief Argument passed to a borrowing Result callback
     * @details Forces an lvalue, and keeps the referent's reference type when Alt is already a reference.
     */
    template<typename Self, typename Alt>
    using result_borrow_t = std::conditional_t<
        std::is_lvalue_reference_v<Alt>,
        Alt,
        forward_arg_t<Self&, Alt>
    >;

    template<std::size_t I>
    struct result_alt_index
    {
        static constexpr std::size_t value = I;
    };

    /**
     * @brief Overload set that picks Ok or Err the way std::variant's converting constructor does
     * @details Identical alternatives are excluded: construction would be ambiguous.
     */
    template<typename T, typename E>
        requires (!std::same_as<T, E>)
    struct result_init_overloads
    {
        static auto test(T) -> result_alt_index<0>;

        static auto test(E) -> result_alt_index<1>;
    };

    template<typename T, typename E, typename U>
    concept result_alt_usable = !std::same_as<T, E> && requires
    {
        result_init_overloads<T, E>::test(std::declval<U>());
    };

    template<typename T, typename E, typename U>
    constexpr std::size_t result_alt_index_v =
        decltype(result_init_overloads<T, E>::test(std::declval<U>()))::value;

    /**
     * @brief A reference alternative accepts an lvalue argument, never a temporary
     */
    template<typename Alt, typename Arg>
    concept result_ref_convert_ok =
        !std::is_lvalue_reference_v<Alt> || std::is_lvalue_reference_v<Arg>;

    template<typename T, typename E, typename Arg>
    concept result_selected_ref_ok =
        (result_alt_index_v<T, E, Arg> == 0 && result_ref_convert_ok<T, Arg>) ||
        (result_alt_index_v<T, E, Arg> == 1 && result_ref_convert_ok<E, Arg>);

    /**
     * @brief Converting constructor accepts Arg only when the selected alternative can bind it
     */
    template<typename T, typename E, typename Arg>
    concept result_convertible_from =
        result_alt_usable<T, E, Arg> && result_selected_ref_ok<T, E, Arg>;

    template<typename Alt, typename... Args>
    concept result_ref_arg_ok =
        !std::is_lvalue_reference_v<Alt> ||
        (sizeof...(Args) == 1 && (std::is_lvalue_reference_v<Args> && ...));

    /**
     * @brief Storage for Result<T, E>
     * @details Values live in variant<T, E>. If either alternative is an lvalue reference, that side is a
     *          pointer and the other side is a pointer or an optional. A reference is never null while active.
     *          Pointer values stay inside the variant: Ok(nullptr) is a real value.
     */
    template<typename T, typename E>
    struct result_storage
    {
        static constexpr bool t_is_ref = std::is_lvalue_reference_v<T>;

        static constexpr bool e_is_ref = std::is_lvalue_reference_v<E>;

        static constexpr bool any_ref = t_is_ref || e_is_ref;

        using ok_store = std::conditional_t<
            t_is_ref,
            std::remove_reference_t<T>*,
            std::optional<T>
        >;

        using err_store = std::conditional_t<
            e_is_ref,
            std::remove_reference_t<E>*,
            std::optional<E>
        >;

        struct value_repr
        {
            std::variant<T, E> data;

            constexpr value_repr() = default;

            template<typename... Args>
            constexpr value_repr(std::in_place_index_t<0>, Args&&... args)
                : data(std::in_place_index<0>, std::forward<Args>(args)...)
            {
            }

            template<typename... Args>
            constexpr value_repr(std::in_place_index_t<1>, Args&&... args)
                : data(std::in_place_index<1>, std::forward<Args>(args)...)
            {
            }

            constexpr value_repr(const value_repr&) = default;

            constexpr value_repr(value_repr&&) = default;

            constexpr auto operator=(const value_repr&) -> value_repr& = default;

            constexpr auto operator=(value_repr&&) -> value_repr& = default;
        };

        struct ref_repr
        {
            bool ok = false;

            ok_store ok_slot{};

            err_store err_slot{};
        };

        using repr_t = std::conditional_t<any_ref, ref_repr, value_repr>;

        repr_t m_repr;

        constexpr result_storage(const result_storage&) = default;

        constexpr result_storage(result_storage&&) = default;

        constexpr auto operator=(const result_storage&) -> result_storage& = default;

        constexpr auto operator=(result_storage&&) -> result_storage& = default;

        template<std::size_t I, typename... Args>
        constexpr result_storage(std::in_place_index_t<I>, Args&&... args)
            : m_repr(make_repr(std::in_place_index<I>, std::forward<Args>(args)...))
        {
        }

        [[nodiscard]] constexpr auto is_ok() const noexcept -> bool
        {
            if constexpr (any_ref)
            {
                return this->m_repr.ok;
            }
            else
            {
                return this->m_repr.data.index() == 0;
            }
        }

        template<typename Self>
        [[nodiscard]] constexpr auto get_ok(this Self&& self) -> decltype(auto)
        {
            if constexpr (any_ref)
            {
                if constexpr (t_is_ref)
                {
                    return *self.m_repr.ok_slot;
                }
                else
                {
                    return *std::forward<Self>(self).m_repr.ok_slot;
                }
            }
            else
            {
                return std::get<0>(std::forward<Self>(self).m_repr.data);
            }
        }

        template<typename Self>
        [[nodiscard]] constexpr auto get_err(this Self&& self) -> decltype(auto)
        {
            if constexpr (any_ref)
            {
                if constexpr (e_is_ref)
                {
                    return *self.m_repr.err_slot;
                }
                else
                {
                    return *std::forward<Self>(self).m_repr.err_slot;
                }
            }
            else
            {
                return std::get<1>(std::forward<Self>(self).m_repr.data);
            }
        }

    private:
        template<typename U>
        static constexpr auto bind_ref(U&& value) -> std::remove_reference_t<U>*
        {
            static_assert(std::is_lvalue_reference_v<U>, "Result reference alternatives bind an lvalue");
            return std::addressof(value);
        }

        template<std::size_t I, typename... Args>
        static constexpr auto make_repr(std::in_place_index_t<I>, Args&&... args) -> repr_t
        {
            if constexpr (any_ref)
            {
                ref_repr repr{};

                if constexpr (I == 0)
                {
                    repr.ok = true;

                    if constexpr (t_is_ref)
                    {
                        static_assert(sizeof...(Args) == 1);
                        repr.ok_slot = bind_ref(std::forward<Args>(args)...);
                    }
                    else
                    {
                        repr.ok_slot.emplace(std::forward<Args>(args)...);
                    }
                }
                else
                {
                    repr.ok = false;

                    if constexpr (e_is_ref)
                    {
                        static_assert(sizeof...(Args) == 1);
                        repr.err_slot = bind_ref(std::forward<Args>(args)...);
                    }
                    else
                    {
                        repr.err_slot.emplace(std::forward<Args>(args)...);
                    }
                }

                return repr;
            }
            else
            {
                return value_repr(std::in_place_index<I>, std::forward<Args>(args)...);
            }
        }
    };

    /**
     * @brief True when the Ok or Err alternative can be hashed
     * @details References hash the referent.
     */
    template<typename V>
    concept result_hash_alt =
        (
            std::is_lvalue_reference_v<V> &&
            requires (const std::remove_reference_t<V>& value)
            {
                { std::hash<std::remove_cv_t<std::remove_reference_t<V>>>{}(value) } -> std::convertible_to<std::size_t>;
            }
        ) ||
        (
            !std::is_lvalue_reference_v<V> &&
            requires (const V& value)
            {
                { std::hash<V>{}(value) } -> std::convertible_to<std::size_t>;
            }
        );
}

/**
 * @brief Result type that can hold either a value of type T or an error of type E
 * @tparam T The type of value that can be held in the Result
 * @tparam E The type of error that can be held in the Result
 * @details Values are stored in a variant. An lvalue-reference alternative is stored as a pointer,
 *          so Result<T&, E>, Result<T, E&>, and Result<T&, E&> are valid. Rvalue references are not.
 *          There is no empty state: a moved-from Result still holds its alternative.
 */
template<typename T, typename E>
    requires (!std::is_rvalue_reference_v<T> && !std::is_rvalue_reference_v<E>)
struct Result
{
    using type = T;

    using err_type = E;

private:
    using storage = internal::result_storage<T, E>;

    static constexpr bool t_is_ref = storage::t_is_ref;

    static constexpr bool e_is_ref = storage::e_is_ref;

    storage m_data;

    /**
     * @brief Ok value with the category a consuming callback should see
     */
    template<typename Self>
    [[nodiscard]] constexpr auto forward_ok(this Self&& self) -> decltype(auto)
    {
        if constexpr (t_is_ref || std::is_lvalue_reference_v<Self>)
        {
            return self.m_data.get_ok();
        }
        else
        {
            return std::move(self.m_data.get_ok());
        }
    }

    /**
     * @brief Err value with the category a consuming callback should see
     */
    template<typename Self>
    [[nodiscard]] constexpr auto forward_err(this Self&& self) -> decltype(auto)
    {
        if constexpr (e_is_ref || std::is_lvalue_reference_v<Self>)
        {
            return self.m_data.get_err();
        }
        else
        {
            return std::move(self.m_data.get_err());
        }
    }

public:
    /**
     * @brief Default-constructs the Ok alternative
     */
    constexpr Result()
        noexcept(std::is_nothrow_default_constructible_v<T>)
        requires (!t_is_ref && std::is_default_constructible_v<T>)
        : m_data(std::in_place_index<0>)
    {
    }

    constexpr Result(const Result&) = default;

    constexpr Result(Result&&) = default;

    constexpr auto operator=(const Result&) -> Result& = default;

    constexpr auto operator=(Result&&) -> Result& = default;

    /**
     * @brief Constructs the unique alternative that can be built from value
     * @param value A value convertible to exactly one of T or E
     */
    template<typename U>
        requires (
            !std::same_as<std::remove_cvref_t<U>, Result> &&
            internal::result_convertible_from<T, E, U&&>
        )
    constexpr Result(U&& value)
        : m_data(
            std::in_place_index<internal::result_alt_index_v<T, E, U&&>>,
            std::forward<U>(value)
        )
    {
    }

    /**
     * @brief Constructs alternative I in place
     * @tparam I 0 for Ok, 1 for Err
     */
    template<std::size_t I, typename... Args>
        requires (
            I < 2 &&
            (
                (I == 0 && internal::result_ref_arg_ok<T, Args...>) ||
                (I == 1 && internal::result_ref_arg_ok<E, Args...>)
            )
        )
    explicit constexpr Result(std::in_place_index_t<I>, Args&&... args)
        : m_data(std::in_place_index<I>, std::forward<Args>(args)...)
    {
    }

    /**
     * @brief Calls f if the result is Ok, otherwise returns the Err value of self
     * @tparam F The type of the function to call
     * @tparam Self The type of self
     * @param f The function to call
     * @return Result containing the result of f if Ok, Err otherwise
     * @note Lvalue self passes T& to f and copies E on the Err path. Rvalue self passes T&& and moves E.
     *       A reference alternative is always passed as an lvalue to its referent.
     */
    template<
        typename F,
        typename Self,
        typename Arg = internal::result_forward_t<Self, T>,
        typename Ret = std::remove_cvref_t<std::invoke_result_t<F, Arg>>
    >
        requires (
            std::invocable<F, Arg> &&
            requires { typename Ret::type; typename Ret::err_type; } &&
            std::same_as<Ret, Result<typename Ret::type, typename Ret::err_type>>
        )
    [[nodiscard]] constexpr auto and_then(this Self&& self, F&& f) -> Result<typename Ret::type, typename Ret::err_type>
    {
        using result_t = Result<typename Ret::type, typename Ret::err_type>;

        if (self.is_ok())
        {
            return f(std::forward<Self>(self).forward_ok());
        }

        return result_t(std::in_place_index<1>, std::forward<Self>(self).forward_err());
    }

    /**
     * @brief Returns res if the result is Ok, otherwise returns the Err value of self in res's type
     * @tparam Self The type of self
     * @tparam U The Ok type of res
     * @param res The result to return if self is Ok
     * @return res if Ok, otherwise Err converted into Result<U, E>
     * @note Lvalue self copies E. Rvalue self moves E. The Ok types need not be convertible.
     */
    template<typename Self, typename U>
    [[nodiscard]] constexpr auto and_(this Self&& self, Result<U, E> res)
        noexcept(
            std::is_nothrow_move_constructible_v<U> &&
            std::is_nothrow_copy_constructible_v<E> &&
            std::is_nothrow_move_constructible_v<E>
        ) -> Result<U, E>
    {
        if (self.is_ok())
        {
            return res;
        }

        return Result<U, E>(std::in_place_index<1>, std::forward<Self>(self).forward_err());
    }

    /**
     * @brief Converts from Result<T, E> to Result<const T&, const E&>
     * @return A result that borrows whichever alternative this result holds
     */
    [[nodiscard]] constexpr auto as_ref() const && = delete;

    [[nodiscard]] constexpr auto as_ref() const & noexcept -> Result<const T&, const E&>
    {
        if (this->is_ok())
        {
            return Result<const T&, const E&>(std::in_place_index<0>, this->m_data.get_ok());
        }

        return Result<const T&, const E&>(std::in_place_index<1>, this->m_data.get_err());
    }

    /**
     * @brief Converts from Result<T, E> to Result<T&, E&>
     * @return A result that borrows whichever alternative this result holds
     */
    [[nodiscard]] constexpr auto as_mut() && = delete;

    [[nodiscard]] constexpr auto as_mut() & noexcept -> Result<T&, E&>
    {
        if (this->is_ok())
        {
            return Result<T&, E&>(std::in_place_index<0>, this->m_data.get_ok());
        }

        return Result<T&, E&>(std::in_place_index<1>, this->m_data.get_err());
    }

    /**
     * @brief Returns true if the result is Ok and the contained value equals the given value
     * @tparam U The type of the value to compare
     * @param x The value to compare
     * @return true if Ok and the contained value equals x, false otherwise
     */
    template<typename U>
        requires std::equality_comparable_with<std::remove_reference_t<T>, std::remove_cvref_t<U>>
    [[nodiscard]] constexpr auto contains(U&& x) const
        noexcept(noexcept(
            std::declval<const std::remove_reference_t<T>&>() ==
            std::declval<const std::remove_cvref_t<U>&>()
        )) -> bool
    {
        return this->is_ok() && this->m_data.get_ok() == std::forward<U>(x);
    }

    /**
     * @brief Returns true if the result is Err and the contained error equals the given error
     * @tparam G The type of the error to compare
     * @param x The error to compare
     * @return true if Err and the contained error equals x, false otherwise
     */
    template<typename G>
        requires std::equality_comparable_with<std::remove_reference_t<E>, std::remove_cvref_t<G>>
    [[nodiscard]] constexpr auto contains_err(G&& x) const
        noexcept(noexcept(
            std::declval<const std::remove_reference_t<E>&>() ==
            std::declval<const std::remove_cvref_t<G>&>()
        )) -> bool
    {
        return this->is_err() && this->m_data.get_err() == std::forward<G>(x);
    }

    /**
     * @brief Returns true if the result is Ok
     * @return true if the result is Ok, false otherwise
     */
    [[nodiscard]] constexpr auto is_ok() const noexcept -> bool
    {
        return this->m_data.is_ok();
    }

    /**
     * @brief Returns true if the result is Err
     * @return true if the result is Err, false otherwise
     */
    [[nodiscard]] constexpr auto is_err() const noexcept -> bool
    {
        return !this->is_ok();
    }

    /**
     * @brief Converts from Result<T, E> to Option<T>
     * @tparam Self The type of self
     * @return Option containing the value if Ok, None otherwise; lvalue self -> copies, rvalue self -> moves out (self becomes moved-from)
     * @note Rvalue self moves the value out; self is left in a moved-from state. Reference alternatives are not emptied.
     */
    template<typename Self>
    [[nodiscard]] constexpr auto ok(this Self&& self)
        noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_move_constructible_v<T>) -> Option<T>
    {
        if (self.is_ok())
        {
            return Option<T>(std::forward<Self>(self).forward_ok());
        }

        return None{};
    }

    /**
     * @brief Converts from Result<T, E> to Option<E>
     * @tparam Self The type of self
     * @return Option containing the error if Err, None otherwise; lvalue self -> copies, rvalue self -> moves out (self becomes moved-from)
     * @note Rvalue self moves the error out; self is left in a moved-from state. Reference alternatives are not emptied.
     */
    template<typename Self>
    [[nodiscard]] constexpr auto err(this Self&& self)
        noexcept(std::is_nothrow_copy_constructible_v<E> && std::is_nothrow_move_constructible_v<E>) -> Option<E>
    {
        if (self.is_err())
        {
            return Option<E>(std::forward<Self>(self).forward_err());
        }

        return None{};
    }

    /**
     * @brief Returns the contained Ok value by value, or panics if Err
     * @tparam Self The type of self
     * @param msg The panic message to display if Err
     * @return The contained value by value; lvalue self -> copies, rvalue self -> moves out (self becomes moved-from)
     * @note Panics if Err with the given message. A reference alternative returns that reference.
     */
    template<typename Self>
    [[nodiscard]] constexpr auto expect(this Self&& self, std::string_view msg)
        noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_move_constructible_v<T>) -> T
    {
        if (self.is_ok())
        {
            return T(std::forward<Self>(self).forward_ok());
        }

        panic(msg);
    }

    /**
     * @brief Returns the contained Err value by value, or panics if Ok
     * @tparam Self The type of self
     * @param msg The panic message to display if Ok
     * @return The contained error by value; lvalue self -> copies, rvalue self -> moves out (self becomes moved-from)
     * @note Panics if Ok with the given message. A reference alternative returns that reference.
     */
    template<typename Self>
    [[nodiscard]] constexpr auto expect_err(this Self&& self, std::string_view msg)
        noexcept(std::is_nothrow_copy_constructible_v<E> && std::is_nothrow_move_constructible_v<E>) -> E
    {
        if (self.is_err())
        {
            return E(std::forward<Self>(self).forward_err());
        }

        panic(msg);
    }

    /**
     * @brief Calls a function with a reference to the contained value if Ok
     * @tparam F The type of the function to call
     * @tparam Self The type of self
     * @param f The function to call
     * @return Lvalue self by reference. Rvalue self by value, so chaining stays a consuming rvalue.
     * @note The callback borrows. inspect does not change which alternative is held.
     */
    template<
        typename F,
        typename Self,
        typename Arg = internal::result_borrow_t<Self, T>
    >
        requires std::invocable<F, Arg>
    [[nodiscard]] constexpr auto inspect(this Self&& self, F&& f)
        -> std::conditional_t<
            std::is_lvalue_reference_v<Self>,
            Self&,
            std::remove_cvref_t<Self>
        >
    {
        if (self.is_ok())
        {
            f(self.m_data.get_ok());
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
     * @brief Calls a function with a reference to the contained value if Err
     * @tparam F The type of the function to call
     * @tparam Self The type of self
     * @param f The function to call
     * @return Lvalue self by reference. Rvalue self by value, so chaining stays a consuming rvalue.
     * @note The callback borrows. inspect_err does not change which alternative is held.
     */
    template<
        typename F,
        typename Self,
        typename Arg = internal::result_borrow_t<Self, E>
    >
        requires std::invocable<F, Arg>
    [[nodiscard]] constexpr auto inspect_err(this Self&& self, F&& f)
        -> std::conditional_t<
            std::is_lvalue_reference_v<Self>,
            Self&,
            std::remove_cvref_t<Self>
        >
    {
        if (self.is_err())
        {
            f(self.m_data.get_err());
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
     * @brief Maps a Result<T, E> to Result<U, E> by applying a function to a contained Ok value
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @tparam U The return type of the function
     * @param f The function to call
     * @return Result containing the result of f if Ok, Err otherwise
     * @note Lvalue self passes T&. Rvalue self passes T&& and moves E on the Err path.
     *       A reference alternative is passed as an lvalue.
     */
    template<
        typename Self,
        typename F,
        typename Arg = internal::result_forward_t<Self, T>,
        typename U = std::invoke_result_t<F, Arg>
    >
        requires std::invocable<F, Arg>
    [[nodiscard]] constexpr auto map(this Self&& self, F&& f) -> Result<U, E>
    {
        if (self.is_ok())
        {
            return f(std::forward<Self>(self).forward_ok());
        }

        return Result<U, E>(std::in_place_index<1>, std::forward<Self>(self).forward_err());
    }

    /**
     * @brief Maps a Result<T, E> to Result<T, F> by applying a function to a contained Err value
     * @tparam Self The type of self
     * @tparam M The type of the function to call
     * @tparam F The return type of the function
     * @param m The function to call
     * @return Result containing the value if Ok, result of m if Err
     * @note Lvalue self passes E&. Rvalue self passes E&& and moves T on the Ok path.
     *       A reference alternative is passed as an lvalue.
     *       The new error is placed with in_place, so it may be the same type as T.
     */
    template<
        typename Self,
        typename M,
        typename Arg = internal::result_forward_t<Self, E>,
        typename F = std::invoke_result_t<M, Arg>
    >
        requires std::invocable<M, Arg>
    [[nodiscard]] constexpr auto map_err(this Self&& self, M&& m) -> Result<T, F>
    {
        if (self.is_err())
        {
            return Result<T, F>(std::in_place_index<1>, m(std::forward<Self>(self).forward_err()));
        }

        return Result<T, F>(std::in_place_index<0>, std::forward<Self>(self).forward_ok());
    }

    /**
     * @brief Converts Result<Result<U, E>, E> into Result<U, E>
     * @tparam Self The type of self
     * @return The inner result if Ok, otherwise the outer Err
     * @note Lvalue self copies. Rvalue self moves and is left moved-from; a Result has no empty state.
     *       Reference alternatives are copied as references.
     */
    template<typename Self>
        requires std::same_as<T, Result<typename T::type, E>>
    [[nodiscard]] constexpr auto flatten(this Self&& self) -> T
    {
        if (self.is_ok())
        {
            return T(std::forward<Self>(self).forward_ok());
        }

        return T(std::in_place_index<1>, std::forward<Self>(self).forward_err());
    }

    /**
     * @brief Returns self if Ok, otherwise returns res
     * @tparam Self The type of self
     * @tparam G The error type of res. The Ok type stays T.
     * @param res The result to return if self is Err
     * @return self's Ok value in Result<T, G> if Ok, otherwise res
     * @note Lvalue self copies T. Rvalue self moves T out and is left moved-from.
     */
    template<typename Self, typename G>
    [[nodiscard]] constexpr auto or_(this Self&& self, Result<T, G> res)
        noexcept(
            std::is_nothrow_copy_constructible_v<T> &&
            std::is_nothrow_move_constructible_v<T> &&
            std::is_nothrow_move_constructible_v<G>
        ) -> Result<T, G>
    {
        if (self.is_ok())
        {
            return Result<T, G>(std::in_place_index<0>, std::forward<Self>(self).forward_ok());
        }

        return res;
    }

    /**
     * @brief Calls f if the result is Err, otherwise returns the Ok value of self
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @param f The function to call with the error. It returns Result<T, G> for some error type G.
     * @return self if Ok, otherwise the result of f
     * @note Lvalue self passes E& and copies T. Rvalue self passes E&& and moves T. The Ok type stays T.
     */
    template<
        typename Self,
        typename F,
        typename Arg = internal::result_forward_t<Self, E>,
        typename Ret = std::remove_cvref_t<std::invoke_result_t<F, Arg>>
    >
        requires (
            std::invocable<F, Arg> &&
            requires { typename Ret::err_type; } &&
            std::same_as<Ret, Result<T, typename Ret::err_type>>
        )
    [[nodiscard]] constexpr auto or_else(this Self&& self, F&& f) -> Result<T, typename Ret::err_type>
    {
        using result_t = Result<T, typename Ret::err_type>;

        if (self.is_ok())
        {
            return result_t(std::in_place_index<0>, std::forward<Self>(self).forward_ok());
        }

        return f(std::forward<Self>(self).forward_err());
    }

    /**
     * @brief Maps a Result<T, E> to U by applying a function to a contained Ok value, otherwise returns the provided default
     * @tparam Self The type of self
     * @tparam U The return type of the function
     * @tparam F The type of the function to call
     * @param default_value The default value to return if Err
     * @param f The function to call if Ok
     * @return The result of f if Ok, otherwise default_value; lvalue self -> passes T&, rvalue self -> passes T&&
     */
    template<
        typename Self,
        typename U,
        typename F,
        typename Arg = internal::result_forward_t<Self, T>
    >
        requires (
            std::invocable<F, Arg> &&
            std::convertible_to<std::invoke_result_t<F, Arg>, U>
        )
    [[nodiscard]] constexpr auto map_or(this Self&& self, U&& default_value, F&& f) -> U
    {
        if (self.is_ok())
        {
            return f(std::forward<Self>(self).forward_ok());
        }

        return std::forward<U>(default_value);
    }

    /**
     * @brief Returns true if the result is Ok and the predicate returns true for the contained value
     * @tparam Self The type of self
     * @tparam F The type of the predicate function
     * @param f The predicate function
     * @return true if Ok and f(value) is true, false otherwise
     */
    template<
        typename Self,
        typename F,
        typename Arg = internal::result_borrow_t<Self, T>
    >
        requires (
            std::invocable<F, Arg> &&
            std::same_as<std::invoke_result_t<F, Arg>, bool>
        )
    [[nodiscard]] constexpr auto is_ok_and(this Self&& self, F&& f) -> bool
    {
        return self.is_ok() && f(self.m_data.get_ok());
    }

    /**
     * @brief Returns true if the result is Err and the predicate returns true for the contained error
     * @tparam Self The type of self
     * @tparam F The type of the predicate function
     * @param f The predicate function
     * @return true if Err and f(error) is true, false otherwise
     */
    template<
        typename Self,
        typename F,
        typename Arg = internal::result_borrow_t<Self, E>
    >
        requires (
            std::invocable<F, Arg> &&
            std::same_as<std::invoke_result_t<F, Arg>, bool>
        )
    [[nodiscard]] constexpr auto is_err_and(this Self&& self, F&& f) -> bool
    {
        return self.is_err() && f(self.m_data.get_err());
    }

    /**
     * @brief Returns the contained Ok value by value, or panics if Err
     * @tparam Self The type of self
     * @return The contained value by value; lvalue self -> copies, rvalue self -> moves out (self becomes moved-from)
     * @note Panics if Err
     */
    template<typename Self>
    [[nodiscard]] constexpr auto unwrap(this Self&& self)
        noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_move_constructible_v<T>) -> T
    {
        return std::forward<Self>(self).expect("Calling Result<T, E>::unwrap() on an Err value");
    }

    /**
     * @brief Returns the contained Err value by value, or panics if Ok
     * @tparam Self The type of self
     * @return The contained error by value; lvalue self -> copies, rvalue self -> moves out (self becomes moved-from)
     * @note Panics if Ok
     */
    template<typename Self>
    [[nodiscard]] constexpr auto unwrap_err(this Self&& self)
        noexcept(std::is_nothrow_copy_constructible_v<E> && std::is_nothrow_move_constructible_v<E>) -> E
    {
        return std::forward<Self>(self).expect_err("Calling Result<T, E>::unwrap_err() on an Ok value");
    }

    /**
     * @brief Returns the contained Ok value, or the provided default if Err
     * @tparam Self The type of self
     * @tparam U The type of the default value
     * @param default_value The default value to return if Err
     * @return The contained value if Ok, otherwise default_value; lvalue self -> copies, rvalue self -> moves out (self becomes moved-from)
     * @note Rvalue self moves the value out; self is left in a moved-from state
     */
    template<typename Self, typename U>
        requires std::convertible_to<U, T>
    [[nodiscard]] constexpr auto unwrap_or(this Self&& self, U&& default_value)
        noexcept(
            std::is_nothrow_copy_constructible_v<T> &&
            std::is_nothrow_move_constructible_v<T> &&
            std::is_nothrow_constructible_v<T, U&&>
        ) -> T
    {
        if (self.is_ok())
        {
            return T(std::forward<Self>(self).forward_ok());
        }

        return T(std::forward<U>(default_value));
    }

    /**
     * @brief Returns the contained Ok value, or computes it from a closure if Err
     * @tparam Self The type of self
     * @tparam F The type of the function to call
     * @param f The function to call if Err
     * @return The contained value if Ok, otherwise the result of f; lvalue self -> copies, rvalue self -> moves out (self becomes moved-from)
     * @note Rvalue self moves the value out; self is left in a moved-from state
     */
    template<typename Self, typename F>
        requires (std::invocable<F> && std::convertible_to<std::invoke_result_t<F>, T>)
    [[nodiscard]] constexpr auto unwrap_or_else(this Self&& self, F&& f)
        noexcept(
            std::is_nothrow_copy_constructible_v<T> &&
            std::is_nothrow_move_constructible_v<T> &&
            std::is_nothrow_invocable_v<F> &&
            std::is_nothrow_constructible_v<T, std::invoke_result_t<F>>
        ) -> T
    {
        if (self.is_ok())
        {
            return T(std::forward<Self>(self).forward_ok());
        }

        return T(f());
    }

    /**
     * @brief Returns the contained Ok value, or the default-constructed value of T if Err
     * @tparam Self The type of self
     * @return The contained value if Ok, otherwise T{}; lvalue self -> copies, rvalue self -> moves out (self becomes moved-from)
     * @note Rvalue self moves the value out; self is left in a moved-from state.
     *       Not provided when T is a reference.
     */
    template<typename Self>
        requires (!t_is_ref && std::is_default_constructible_v<T>)
    [[nodiscard]] constexpr auto unwrap_or_default(this Self&& self)
        noexcept(
            std::is_nothrow_copy_constructible_v<T> &&
            std::is_nothrow_move_constructible_v<T> &&
            std::is_nothrow_default_constructible_v<T>
        ) -> T
    {
        if (self.is_ok())
        {
            return T(std::forward<Self>(self).forward_ok());
        }

        return T{};
    }

    /**
     * @brief Returns the contained Ok value by value without checking
     * @tparam Self The type of self
     * @return The contained value. Lvalue self copies. Rvalue self moves out and is left moved-from.
     *         A reference alternative returns that reference.
     * @warning Undefined behavior if this result is Err
     */
    template<typename Self>
    [[nodiscard]] constexpr auto unwrap_unchecked(this Self&& self)
        noexcept(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_move_constructible_v<T>) -> T
    {
        if constexpr (t_is_ref)
        {
            return self.m_data.get_ok();
        }
        else if constexpr (std::is_lvalue_reference_v<Self> || std::is_const_v<std::remove_reference_t<Self>>)
        {
            if constexpr (storage::any_ref)
            {
                return T(*self.m_data.m_repr.ok_slot);
            }
            else
            {
                return T(*std::get_if<0>(&self.m_data.m_repr.data));
            }
        }
        else if constexpr (storage::any_ref)
        {
            return T(std::move(*self.m_data.m_repr.ok_slot));
        }
        else
        {
            return T(std::move(*std::get_if<0>(&self.m_data.m_repr.data)));
        }
    }

    /**
     * @brief Invokes f with the contained Ok value or the contained Err value
     * @tparam Self The type of self
     * @tparam F The visitor, invocable with T and with E
     * @param f The visitor
     * @return Whatever f returns. Both arms must return the same type.
     * @note Lvalue self passes T& or E&. Rvalue self passes T&& or E&&.
     *       A reference alternative is passed as an lvalue to the referent.
     *       The result is not emptied; this is a visit, not a consuming combinator.
     */
    template<typename Self, typename F>
    [[nodiscard]] constexpr auto visit(this Self&& self, F&& f) -> decltype(auto)
    {
        if (self.is_ok())
        {
            return std::invoke(std::forward<F>(f), std::forward<Self>(self).forward_ok());
        }

        return std::invoke(std::forward<F>(f), std::forward<Self>(self).forward_err());
    }

    /**
     * @brief Compares two Results for equality
     * @param lhs The left result
     * @param rhs The right result
     * @return true if both are Ok with equal values, or both are Err with equal errors
     * @note Reference alternatives compare the referents.
     */
    friend constexpr auto operator==(const Result& lhs, const Result& rhs)
        noexcept(
            noexcept(std::declval<const std::remove_reference_t<T>&>() == std::declval<const std::remove_reference_t<T>&>()) &&
            noexcept(std::declval<const std::remove_reference_t<E>&>() == std::declval<const std::remove_reference_t<E>&>())
        ) -> bool
        requires (
            std::equality_comparable<std::remove_reference_t<T>> &&
            std::equality_comparable<std::remove_reference_t<E>>
        )
    {
        if (lhs.is_ok() != rhs.is_ok())
        {
            return false;
        }

        if (lhs.is_ok())
        {
            return lhs.m_data.get_ok() == rhs.m_data.get_ok();
        }

        return lhs.m_data.get_err() == rhs.m_data.get_err();
    }

    /**
     * @brief Compares two Results for ordering
     * @param lhs The left result
     * @param rhs The right result
     * @return Ok before Err, then the contained values. The category follows the values.
     * @note Reference alternatives compare the referents.
     */
    friend constexpr auto operator<=>(const Result& lhs, const Result& rhs)
        noexcept(
            internal::nothrow_three_way_v<std::remove_reference_t<T>> &&
            internal::nothrow_three_way_v<std::remove_reference_t<E>>
        )
        -> std::common_comparison_category_t<
            std::strong_ordering,
            internal::compare_category_t<std::remove_reference_t<T>>,
            internal::compare_category_t<std::remove_reference_t<E>>
        >
        requires (
            std::three_way_comparable<std::remove_reference_t<T>> &&
            std::three_way_comparable<std::remove_reference_t<E>>
        )
    {
        if (lhs.is_ok() != rhs.is_ok())
        {
            return lhs.is_ok() ? std::strong_ordering::less : std::strong_ordering::greater;
        }

        if (lhs.is_ok())
        {
            return lhs.m_data.get_ok() <=> rhs.m_data.get_ok();
        }

        return lhs.m_data.get_err() <=> rhs.m_data.get_err();
    }

    /**
     * @brief Hash of a Result, mixing an Ok/Err tag with the value hash
     * @param res The result to hash
     * @return Hash of the active alternative
     * @note References hash the referent.
     */
    friend constexpr auto hash_value(const Result& res) -> std::size_t
        requires (internal::result_hash_alt<T> && internal::result_hash_alt<E>)
    {
        std::size_t seed = std::hash<std::size_t>{}(res.is_ok() ? 0 : 1);

        if (res.is_ok())
        {
            if constexpr (t_is_ref)
            {
                using hashed = std::remove_cv_t<std::remove_reference_t<T>>;
                seed ^= std::hash<hashed>{}(res.m_data.get_ok()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            else
            {
                seed ^= std::hash<T>{}(res.m_data.get_ok()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
        }
        else
        {
            if constexpr (e_is_ref)
            {
                using hashed = std::remove_cv_t<std::remove_reference_t<E>>;
                seed ^= std::hash<hashed>{}(res.m_data.get_err()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            else
            {
                seed ^= std::hash<E>{}(res.m_data.get_err()) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
        }

        return seed;
    }
};

}

namespace std
{
    /**
     * @brief Hash support for Result<T, E>
     * @tparam T The value type
     * @tparam E The error type
     */
    template<typename T, typename E>
        requires (crab_cpp::internal::result_hash_alt<T> && crab_cpp::internal::result_hash_alt<E>)
    struct hash<crab_cpp::Result<T, E>>
    {
        auto operator()(const crab_cpp::Result<T, E>& res) const -> size_t
        {
            return hash_value(res);
        }
    };
}

// Result x (T kind, E kind) x value-category x Ok/Err. Included from tests/result.cpp.

using crab_test::CopyDefault;
using crab_test::CopyNoDefault;
using crab_test::MoveDefault;
using crab_test::MoveNoDefault;
using crab_test::MapTag;

template<typename T, typename E>
auto peek_ok(Result<T, E>& res) -> int
{
    return crab_test::read_id<T>(res.as_mut().unwrap());
}

template<typename T, typename E>
auto peek_ok(const Result<T, E>& res) -> int
{
    return crab_test::read_id<T>(res.as_ref().unwrap());
}

template<typename T, typename E>
auto peek_err(Result<T, E>& res) -> int
{
    return crab_test::read_id<E>(res.as_mut().unwrap_err());
}

template<typename T, typename E>
auto peek_err(const Result<T, E>& res) -> int
{
    return crab_test::read_id<E>(res.as_ref().unwrap_err());
}

template<typename U>
auto death_value() -> U
{
    static int slot = 4;

    if constexpr (std::is_pointer_v<U>)
    {
        slot = 4;

        return &slot;
    }
    else if constexpr (std::is_lvalue_reference_v<U>)
    {
        slot = 4;

        return slot;
    }
    else
    {
        return U{ 4 };
    }
}

template<typename T, typename E>
void exercise_result()
{
    struct host
    {
        int anchor_ok = 0;
        int anchor_err = 0;

        auto ok(int id) -> Result<T, E>
        {
            if constexpr (std::is_pointer_v<T>)
            {
                this->anchor_ok = id;

                return Result<T, E>(std::in_place_index<0>, &this->anchor_ok);
            }
            else if constexpr (std::is_lvalue_reference_v<T>)
            {
                this->anchor_ok = id;

                return Result<T, E>(std::in_place_index<0>, this->anchor_ok);
            }
            else
            {
                return Result<T, E>(std::in_place_index<0>, T{ id });
            }
        }

        auto err(int id) -> Result<T, E>
        {
            if constexpr (std::is_pointer_v<E>)
            {
                this->anchor_err = id;

                return Result<T, E>(std::in_place_index<1>, &this->anchor_err);
            }
            else if constexpr (std::is_lvalue_reference_v<E>)
            {
                this->anchor_err = id;

                return Result<T, E>(std::in_place_index<1>, this->anchor_err);
            }
            else
            {
                return Result<T, E>(std::in_place_index<1>, E{ id });
            }
        }
    };

    host h;
    constexpr bool result_copyable = std::is_copy_constructible_v<Result<T, E>>;

    if constexpr (!std::is_lvalue_reference_v<T> && std::is_default_constructible_v<T>)
    {
        Result<T, E> res;
        EXPECT_TRUE(res.is_ok());

        if constexpr (std::is_pointer_v<T>)
        {
            EXPECT_EQ(res.as_mut().unwrap(), nullptr);
        }
        else
        {
            EXPECT_EQ(peek_ok(res), 0);
        }
    }

    {
        SCOPED_TRACE("ctors");
        auto ok = h.ok(42);
        auto err = h.err(3);
        EXPECT_TRUE(ok.is_ok());
        EXPECT_FALSE(ok.is_err());
        EXPECT_TRUE(err.is_err());
        EXPECT_EQ(peek_ok(ok), 42);
        EXPECT_EQ(peek_err(err), 3);

        if constexpr (result_copyable)
        {
            auto copied = ok;
            EXPECT_EQ(peek_ok(ok), 42);
            EXPECT_EQ(peek_ok(copied), 42);
            copied = err;
            EXPECT_TRUE(copied.is_err());
            EXPECT_EQ(peek_err(copied), 3);
            EXPECT_EQ(peek_err(err), 3);
        }

        auto source = h.ok(42);
        auto moved = std::move(source);
        EXPECT_EQ(peek_ok(moved), 42);

        if constexpr (crab_test::is_indirect<T>)
        {
            EXPECT_TRUE(source.is_ok());
            EXPECT_EQ(peek_ok(source), 42);
        }
        else if (source.is_ok())
        {
            EXPECT_EQ(peek_ok(source), -1);
        }

        source = h.err(3);
        auto moved_err = std::move(source);
        EXPECT_EQ(peek_err(moved_err), 3);
    }

    if constexpr (std::is_lvalue_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>)
    {
        auto ok = h.ok(1);
        ok.unwrap() = 6;
        EXPECT_EQ(h.anchor_ok, 6);
    }

    if constexpr (std::is_pointer_v<T>)
    {
        auto ok = h.ok(1);
        *ok.as_mut().unwrap() = 6;
        EXPECT_EQ(h.anchor_ok, 6);
    }

    {
        auto ok = h.ok(42);
        auto ref = ok.as_ref();
        auto mut = ok.as_mut();
        EXPECT_TRUE(ref.is_ok());
        EXPECT_EQ(crab_test::read_id<T>(ref.unwrap()), 42);
        EXPECT_EQ(crab_test::read_id<T>(mut.unwrap()), 42);

        if constexpr (!crab_test::is_indirect<T>)
        {
            EXPECT_EQ(std::addressof(mut.unwrap()), std::addressof(ref.unwrap()));
        }
        else if constexpr (std::is_lvalue_reference_v<T>)
        {
            EXPECT_EQ(std::addressof(mut.unwrap()), &h.anchor_ok);
        }
        else
        {
            EXPECT_EQ(mut.unwrap(), &h.anchor_ok);
        }

        auto err = h.err(3);
        EXPECT_EQ(crab_test::read_id<E>(err.as_ref().unwrap_err()), 3);
        EXPECT_EQ(crab_test::read_id<E>(err.as_mut().unwrap_err()), 3);
    }

    for (bool is_ok : { true, false })
    {
        SCOPED_TRACE(is_ok ? "ok" : "err");
        crab_test::for_categories(
            [&] { return is_ok ? h.ok(42) : h.err(3); },
            [&](auto&& res) {
                using acc = crab_test::access<decltype(res)>;
                EXPECT_EQ(res.is_ok(), is_ok);
                EXPECT_EQ(res.is_err(), !is_ok);

                if constexpr (acc::consume || crab_test::can_duplicate<T>)
                {
                    if (is_ok)
                    {
                        decltype(auto) taken = std::forward<decltype(res)>(res).unwrap();
                        EXPECT_EQ(crab_test::read_id<T>(taken), 42);

                        if constexpr (std::is_lvalue_reference_v<T>)
                        {
                            EXPECT_EQ(std::addressof(taken), &h.anchor_ok);
                        }
                        else if constexpr (std::is_pointer_v<T>)
                        {
                            EXPECT_EQ(taken, &h.anchor_ok);
                        }

                        EXPECT_TRUE(res.is_ok());

                        if (acc::consume && !crab_test::is_indirect<T>)
                        {
                            EXPECT_EQ(peek_ok(res), -1);
                        }
                        else
                        {
                            EXPECT_EQ(peek_ok(res), 42);
                        }
                    }
                }

                if constexpr (acc::consume || crab_test::can_duplicate<E>)
                {
                    auto fresh = is_ok ? h.ok(42) : h.err(3);

                    if (!is_ok)
                    {
                        decltype(auto) taken = crab_test::forward_like<decltype(res)>(fresh).unwrap_err();
                        EXPECT_EQ(crab_test::read_id<E>(taken), 3);
                        EXPECT_TRUE(fresh.is_err());

                        if constexpr (std::is_lvalue_reference_v<E>)
                        {
                            EXPECT_EQ(std::addressof(taken), &h.anchor_err);
                        }
                    }
                }

                if constexpr (acc::consume || crab_test::can_duplicate<T>)
                {
                    if (is_ok)
                    {
                        auto fresh = h.ok(42);
                        EXPECT_EQ(crab_test::read_id<T>(crab_test::forward_like<decltype(res)>(fresh).expect("boom")), 42);
                        auto unchecked = h.ok(42);
                        EXPECT_EQ(
                            crab_test::read_id<T>(crab_test::forward_like<decltype(res)>(unchecked).unwrap_unchecked()),
                            42
                        );
                    }
                }

                if constexpr (acc::consume || crab_test::can_duplicate<E>)
                {
                    if (!is_ok)
                    {
                        auto fresh = h.err(3);
                        EXPECT_EQ(crab_test::read_id<E>(crab_test::forward_like<decltype(res)>(fresh).expect_err("boom")), 3);
                    }
                }
            }
        );
    }

    for (bool is_ok : { true, false })
    {
        crab_test::for_categories(
            [&] { return is_ok ? h.ok(42) : h.err(3); },
            [&](auto&& res) {
                using acc = crab_test::access<decltype(res)>;

                if constexpr (acc::consume || crab_test::can_duplicate<T>)
                {
                if constexpr (std::is_lvalue_reference_v<T>)
                {
                    int fallback = 9;
                    decltype(auto) taken = std::forward<decltype(res)>(res).unwrap_or(fallback);
                    EXPECT_EQ(crab_test::read_id<T>(taken), is_ok ? 42 : 9);
                    EXPECT_EQ(std::addressof(taken), is_ok ? &h.anchor_ok : &fallback);
                }
                else if constexpr (std::is_pointer_v<T>)
                {
                    int fallback = 9;
                    decltype(auto) taken = std::forward<decltype(res)>(res).unwrap_or(&fallback);
                    EXPECT_EQ(taken, is_ok ? &h.anchor_ok : &fallback);
                }
                else
                {
                    decltype(auto) taken = std::forward<decltype(res)>(res).unwrap_or(T{ 9 });
                    EXPECT_EQ(taken.value(), is_ok ? 42 : 9);
                }

                int calls = 0;
                int fallback = 9;
                auto fresh = is_ok ? h.ok(42) : h.err(3);
                auto from_else = crab_test::forward_like<decltype(res)>(fresh).unwrap_or_else([&]() -> T {
                    ++calls;

                    if constexpr (std::is_pointer_v<T>)
                    {
                        return &fallback;
                    }
                    else if constexpr (std::is_lvalue_reference_v<T>)
                    {
                        return fallback;
                    }
                    else
                    {
                        return T{ 9 };
                    }
                });
                EXPECT_EQ(calls, is_ok ? 0 : 1);
                EXPECT_EQ(crab_test::read_id<T>(from_else), is_ok ? 42 : 9);
                }
            }
        );
    }

    if constexpr (!std::is_lvalue_reference_v<T> && std::is_default_constructible_v<T>)
    {
        for (bool is_ok : { true, false })
        {
            crab_test::for_categories(
                [&] { return is_ok ? h.ok(42) : h.err(3); },
                [&](auto&& res) {
                    using acc = crab_test::access<decltype(res)>;

                    if constexpr (acc::consume || crab_test::can_duplicate<T>)
                    {
                    decltype(auto) taken = std::forward<decltype(res)>(res).unwrap_or_default();

                    if (is_ok)
                    {
                        EXPECT_EQ(crab_test::read_id<T>(taken), 42);
                    }
                    else if constexpr (std::is_pointer_v<T>)
                    {
                        EXPECT_EQ(taken, nullptr);
                    }
                    else
                    {
                        EXPECT_EQ(taken.value(), 0);
                    }
                    }
                }
            );
        }
    }

    for (bool is_ok : { true, false })
    {
        crab_test::for_categories(
            [&] { return is_ok ? h.ok(42) : h.err(3); },
            [&](auto&& res) {
                using acc = crab_test::access<decltype(res)>;

                if constexpr (acc::consume || crab_test::can_duplicate<E>)
                {
                int arg_cat = 0;
                auto mapped = std::forward<decltype(res)>(res).map([&](auto&& value) {
                    arg_cat = std::is_lvalue_reference_v<decltype(value)> ? 1 : 2;

                    return MapTag{ crab_test::read_id<T>(value) };
                });

                if (is_ok)
                {
                    constexpr bool expect_lvalue = std::is_lvalue_reference_v<T> || acc::is_lvalue;
                    EXPECT_EQ(mapped.as_mut().unwrap().m_id, 42);
                    EXPECT_EQ(arg_cat, expect_lvalue ? 1 : 2);
                    EXPECT_TRUE(res.is_ok());
                    EXPECT_EQ(peek_ok(res), 42);
                }
                else
                {
                    EXPECT_TRUE(mapped.is_err());
                    EXPECT_EQ(crab_test::read_id<E>(mapped.as_mut().unwrap_err()), 3);

                    if (acc::consume && !crab_test::is_indirect<E>)
                    {
                        EXPECT_EQ(peek_err(res), -1);
                    }
                    else
                    {
                        EXPECT_EQ(peek_err(res), 3);
                    }
                }

                auto for_or = is_ok ? h.ok(42) : h.err(3);
                EXPECT_EQ(
                    crab_test::forward_like<decltype(res)>(for_or).map_or(0, [](const auto& value) {
                        return crab_test::read_id<T>(value);
                    }),
                    is_ok ? 42 : 0
                );
                }
            }
        );
    }

    if constexpr (!crab_test::is_indirect<T>)
    {
        auto ok = h.ok(5);
        auto consumed = std::move(ok).map([](T value) { return MapTag{ value.value() }; });
        EXPECT_EQ(consumed.as_mut().unwrap().m_id, 5);
        EXPECT_TRUE(ok.is_ok());
        EXPECT_EQ(peek_ok(ok), -1);
    }

    for (bool is_ok : { true, false })
    {
        crab_test::for_categories(
            [&] { return is_ok ? h.ok(42) : h.err(3); },
            [&](auto&& res) {
                using acc = crab_test::access<decltype(res)>;

                if constexpr (acc::consume || crab_test::can_duplicate<T>)
                {
                auto mapped = std::forward<decltype(res)>(res).map_err([&](auto&& value) {
                    return MapTag{ crab_test::read_id<E>(value) };
                });

                if (is_ok)
                {
                    EXPECT_EQ(crab_test::read_id<T>(mapped.as_mut().unwrap()), 42);

                    if (acc::consume && !crab_test::is_indirect<T>)
                    {
                        EXPECT_EQ(peek_ok(res), -1);
                    }
                    else
                    {
                        EXPECT_EQ(peek_ok(res), 42);
                    }
                }
                else
                {
                    EXPECT_EQ(mapped.as_mut().unwrap_err().m_id, 3);
                    EXPECT_TRUE(res.is_err());
                    EXPECT_EQ(peek_err(res), 3);
                }
                }
            }
        );
    }

    for (bool is_ok : { true, false })
    {
        crab_test::for_categories(
            [&] { return is_ok ? h.ok(42) : h.err(3); },
            [&](auto&& res) {
                using acc = crab_test::access<decltype(res)>;

                if constexpr (acc::consume || crab_test::can_duplicate<E>)
                {
                auto next = std::forward<decltype(res)>(res).and_then([](const auto& value) {
                    return Result<MapTag, E>(std::in_place_index<0>, MapTag{ crab_test::read_id<T>(value) });
                });

                if (is_ok)
                {
                    EXPECT_EQ(next.as_mut().unwrap().m_id, 42);
                    EXPECT_EQ(peek_ok(res), 42);
                }
                else
                {
                    EXPECT_TRUE(next.is_err());
                    EXPECT_EQ(crab_test::read_id<E>(next.as_mut().unwrap_err()), 3);
                }

                auto other = Result<MapTag, E>(std::in_place_index<0>, MapTag{ 7 });
                auto fresh = is_ok ? h.ok(42) : h.err(3);
                auto and_result = crab_test::forward_like<decltype(res)>(fresh).and_(std::move(other));

                if (is_ok)
                {
                    EXPECT_EQ(and_result.as_mut().unwrap().m_id, 7);
                    EXPECT_TRUE(fresh.is_ok());
                    EXPECT_EQ(peek_ok(fresh), 42);
                }
                else
                {
                    EXPECT_TRUE(and_result.is_err());
                    EXPECT_EQ(crab_test::read_id<E>(and_result.as_mut().unwrap_err()), 3);
                }
                }
            }
        );
    }

    for (bool is_ok : { true, false })
    {
        crab_test::for_categories(
            [&] { return is_ok ? h.ok(42) : h.err(3); },
            [&](auto&& res) {
                using acc = crab_test::access<decltype(res)>;

                if constexpr (acc::consume || crab_test::can_duplicate<T>)
                {
                auto other = Result<T, MapTag>(std::in_place_index<1>, MapTag{ 7 });
                auto or_result = std::forward<decltype(res)>(res).or_(std::move(other));

                if (is_ok)
                {
                    EXPECT_EQ(crab_test::read_id<T>(or_result.as_mut().unwrap()), 42);

                    if (acc::consume && !crab_test::is_indirect<T>)
                    {
                        EXPECT_EQ(peek_ok(res), -1);
                    }
                }
                else
                {
                    EXPECT_TRUE(or_result.is_err());
                    EXPECT_EQ(or_result.as_mut().unwrap_err().m_id, 7);
                    EXPECT_EQ(peek_err(res), 3);
                }

                int calls = 0;
                auto fresh = is_ok ? h.ok(42) : h.err(3);
                auto else_result = crab_test::forward_like<decltype(res)>(fresh).or_else([&](const auto& err) {
                    ++calls;

                    return Result<T, MapTag>(std::in_place_index<1>, MapTag{ crab_test::read_id<E>(err) });
                });
                EXPECT_EQ(calls, is_ok ? 0 : 1);

                if (is_ok)
                {
                    EXPECT_EQ(crab_test::read_id<T>(else_result.as_mut().unwrap()), 42);
                }
                else
                {
                    EXPECT_EQ(else_result.as_mut().unwrap_err().m_id, 3);
                }
                }
            }
        );
    }

    for (bool is_ok : { true, false })
    {
        crab_test::for_categories(
            [&] { return is_ok ? h.ok(42) : h.err(3); },
            [&](auto&& res) {
                using acc = crab_test::access<decltype(res)>;
                int seen = 0;
                EXPECT_EQ(res.is_ok_and([&](const auto& value) {
                    seen = crab_test::read_id<T>(value);

                    return seen == 42;
                }), is_ok);
                EXPECT_EQ(res.is_err_and([&](const auto& value) {
                    return crab_test::read_id<E>(value) == 3;
                }), !is_ok);
                EXPECT_EQ(res.is_ok(), is_ok);

                if (is_ok)
                {
                    EXPECT_EQ(peek_ok(res), 42);
                }
                else
                {
                    EXPECT_EQ(peek_err(res), 3);
                }

                if (!is_ok)
                {
                    EXPECT_EQ(seen, 0);
                }

                if constexpr (!(acc::is_const && !acc::is_lvalue) || result_copyable)
                {
                seen = 0;

                if constexpr (acc::is_lvalue)
                {
                    auto& out = std::forward<decltype(res)>(res).inspect([&](const auto& value) {
                        seen = crab_test::read_id<T>(value);
                    });
                    EXPECT_EQ(&out, &res);
                    auto& err_out = std::forward<decltype(res)>(res).inspect_err([&](const auto& value) {
                        if (!is_ok)
                        {
                            seen = crab_test::read_id<E>(value);
                        }
                    });
                    EXPECT_EQ(&err_out, &res);
                }
                else
                {
                    auto out = std::forward<decltype(res)>(res).inspect([&](const auto& value) {
                        seen = crab_test::read_id<T>(value);
                    });
                    EXPECT_EQ(out.is_ok(), is_ok);
                    auto fresh = is_ok ? h.ok(42) : h.err(3);
                    auto err_out = crab_test::forward_like<decltype(res)>(fresh).inspect_err([&](const auto& value) {
                        if (!is_ok)
                        {
                            seen = crab_test::read_id<E>(value);
                        }
                    });
                    EXPECT_EQ(err_out.is_err(), !is_ok);
                }

                EXPECT_EQ(seen, is_ok ? 42 : 3);
                }
            }
        );
    }

    for (bool is_ok : { true, false })
    {
        crab_test::for_categories(
            [&] { return is_ok ? h.ok(42) : h.err(3); },
            [&](auto&& res) {
                using acc = crab_test::access<decltype(res)>;
                int got = -2;
                int cat = -1;
                std::forward<decltype(res)>(res).visit([&](auto&& value) -> int {
                    using value_t = std::remove_cvref_t<decltype(value)>;
                    cat = std::is_lvalue_reference_v<decltype(value)> ? 1 : 2;

                    if constexpr (std::is_pointer_v<value_t>)
                    {
                        got = value == nullptr ? -1 : *value;
                    }
                    else if constexpr (std::is_arithmetic_v<value_t>)
                    {
                        got = static_cast<int>(value);
                    }
                    else
                    {
                        got = value.value();
                    }

                    return 0;
                });
                EXPECT_EQ(got, is_ok ? 42 : 3);
                const bool active_ref = is_ok
                    ? std::is_lvalue_reference_v<T>
                    : std::is_lvalue_reference_v<E>;
                EXPECT_EQ(cat, (active_ref || acc::is_lvalue) ? 1 : 2);
                EXPECT_EQ(res.is_ok(), is_ok);

                if (is_ok)
                {
                    EXPECT_EQ(peek_ok(res), 42);
                }
                else
                {
                    EXPECT_EQ(peek_err(res), 3);
                }

                if constexpr (acc::consume || crab_test::can_duplicate<T>)
                {
                    auto fresh = is_ok ? h.ok(42) : h.err(3);
                    auto as_option = crab_test::forward_like<decltype(res)>(fresh).ok();

                    if (is_ok)
                    {
                        EXPECT_TRUE(as_option.is_some());

                        if constexpr (crab_test::is_indirect<T>)
                        {
                            EXPECT_EQ(crab_test::read_id<T>(as_option.unwrap()), 42);
                        }
                        else
                        {
                            EXPECT_EQ(crab_test::read_id<T>(as_option.as_ref().unwrap()), 42);
                        }
                    }
                    else
                    {
                        EXPECT_TRUE(as_option.is_none());
                    }
                }

                if constexpr (acc::consume || crab_test::can_duplicate<E>)
                {
                    auto fresh = is_ok ? h.ok(42) : h.err(3);
                    auto as_option = crab_test::forward_like<decltype(res)>(fresh).err();
                    EXPECT_EQ(as_option.is_some(), !is_ok);
                }
            }
        );
    }

    {
        auto ok = h.ok(42);
        auto err = h.err(3);

        if constexpr (std::is_pointer_v<T>)
        {
            EXPECT_TRUE(ok.contains(&h.anchor_ok));
            EXPECT_FALSE(err.contains(&h.anchor_ok));
        }
        else if constexpr (std::is_lvalue_reference_v<T>)
        {
            EXPECT_TRUE(ok.contains(42));
            EXPECT_FALSE(ok.contains(0));
            EXPECT_FALSE(err.contains(42));
        }
        else
        {
            EXPECT_TRUE(ok.contains(T{ 42 }));
            EXPECT_FALSE(ok.contains(T{ 1 }));
            EXPECT_FALSE(err.contains(T{ 42 }));
        }

        if constexpr (std::is_pointer_v<E>)
        {
            EXPECT_TRUE(err.contains_err(&h.anchor_err));
            EXPECT_FALSE(ok.contains_err(&h.anchor_err));
        }
        else if constexpr (std::is_lvalue_reference_v<E>)
        {
            EXPECT_TRUE(err.contains_err(3));
            EXPECT_FALSE(err.contains_err(0));
            EXPECT_FALSE(ok.contains_err(3));
        }
        else
        {
            EXPECT_TRUE(err.contains_err(E{ 3 }));
            EXPECT_FALSE(err.contains_err(E{ 1 }));
            EXPECT_FALSE(ok.contains_err(E{ 3 }));
        }

        EXPECT_EQ(ok, h.ok(42));
        EXPECT_EQ(err, h.err(3));
        EXPECT_NE(ok, err);
        EXPECT_LT(ok, err);
        auto ok_hash = std::hash<Result<T, E>>{}(ok);
        auto err_hash = std::hash<Result<T, E>>{}(err);
        auto again_hash = std::hash<Result<T, E>>{}(h.ok(42));
        EXPECT_EQ(ok_hash, hash_value(ok));
        EXPECT_NE(ok_hash, err_hash);
        EXPECT_EQ(ok_hash, again_hash);
    }
}

template<typename U, typename E>
void exercise_flatten()
{
    struct host
    {
        int anchor = 0;

        auto inner_ok(int id) -> Result<U, E>
        {
            if constexpr (std::is_pointer_v<U>)
            {
                this->anchor = id;

                return Result<U, E>(std::in_place_index<0>, &this->anchor);
            }
            else if constexpr (std::is_lvalue_reference_v<U>)
            {
                this->anchor = id;

                return Result<U, E>(std::in_place_index<0>, this->anchor);
            }
            else
            {
                return Result<U, E>(std::in_place_index<0>, U{ id });
            }
        }

        auto inner_err(int id) -> Result<U, E>
        {
            if constexpr (std::is_pointer_v<E> || std::is_lvalue_reference_v<E>)
            {
                this->anchor = id;

                if constexpr (std::is_pointer_v<E>)
                {
                    return Result<U, E>(std::in_place_index<1>, &this->anchor);
                }
                else
                {
                    return Result<U, E>(std::in_place_index<1>, this->anchor);
                }
            }
            else
            {
                return Result<U, E>(std::in_place_index<1>, E{ id });
            }
        }
    };

    host h;
    using inner = Result<U, E>;
    using outer = Result<inner, E>;
    outer ok_ok(std::in_place_index<0>, h.inner_ok(42));

    if constexpr (std::is_copy_constructible_v<outer>)
    {
        auto flat = ok_ok.flatten();
        EXPECT_TRUE(flat.is_ok());
        EXPECT_EQ(crab_test::read_id<U>(flat.as_mut().unwrap()), 42);
        EXPECT_TRUE(ok_ok.is_ok());
    }

    auto flat = std::move(ok_ok).flatten();
    EXPECT_TRUE(flat.is_ok());
    EXPECT_EQ(crab_test::read_id<U>(flat.as_mut().unwrap()), 42);

    outer ok_err(std::in_place_index<0>, h.inner_err(3));
    auto flat_err = std::move(ok_err).flatten();
    EXPECT_TRUE(flat_err.is_err());
    EXPECT_EQ(crab_test::read_id<E>(flat_err.as_mut().unwrap_err()), 3);

    outer err_only = [&] {
        if constexpr (std::is_pointer_v<E>)
        {
            h.anchor = 5;

            return outer(std::in_place_index<1>, &h.anchor);
        }
        else if constexpr (std::is_lvalue_reference_v<E>)
        {
            h.anchor = 5;

            return outer(std::in_place_index<1>, h.anchor);
        }
        else
        {
            return outer(std::in_place_index<1>, E{ 5 });
        }
    }();
    auto flat_outer = std::move(err_only).flatten();
    EXPECT_TRUE(flat_outer.is_err());
    EXPECT_EQ(crab_test::read_id<E>(flat_outer.as_mut().unwrap_err()), 5);
}

template<typename Pair>
class ResultMatrix : public ::testing::Test
{
};

template<typename Pair>
class ResultDeath : public ::testing::Test
{
};

#define RESULT_ROW(T) \
    result_pair<T, CopyDefault>, \
    result_pair<T, CopyNoDefault>, \
    result_pair<T, MoveDefault>, \
    result_pair<T, MoveNoDefault>, \
    result_pair<T, int*>, \
    result_pair<T, int&>, \
    result_pair<T, const int&>

template<typename T, typename E>
struct result_pair
{
    using ok_type = T;
    using err_type = E;
};

using ResultPairs = ::testing::Types<
    RESULT_ROW(CopyDefault),
    RESULT_ROW(CopyNoDefault),
    RESULT_ROW(MoveDefault),
    RESULT_ROW(MoveNoDefault),
    RESULT_ROW(int*),
    RESULT_ROW(int&),
    RESULT_ROW(const int&)
>;

TYPED_TEST_SUITE(ResultMatrix, ResultPairs);

TYPED_TEST(ResultMatrix, All)
{
    exercise_result<typename TypeParam::ok_type, typename TypeParam::err_type>();
}

template<typename T, typename E>
void die_result()
{
    EXPECT_DEATH(
        (Result<T, E>(std::in_place_index<1>, death_value<E>()).unwrap()),
        "Panic encountered: Calling Result<T, E>::unwrap\\(\\) on an Err value"
    );
    EXPECT_DEATH(
        (Result<T, E>(std::in_place_index<1>, death_value<E>()).expect("boom")),
        "Panic encountered: boom"
    );
    EXPECT_DEATH(
        (Result<T, E>(std::in_place_index<0>, death_value<T>()).unwrap_err()),
        "Panic encountered: Calling Result<T, E>::unwrap_err\\(\\) on an Ok value"
    );
    EXPECT_DEATH(
        (Result<T, E>(std::in_place_index<0>, death_value<T>()).expect_err("boom")),
        "Panic encountered: boom"
    );
}

TYPED_TEST_SUITE(ResultDeath, ResultPairs);

TYPED_TEST(ResultDeath, Panic)
{
    die_result<typename TypeParam::ok_type, typename TypeParam::err_type>();
}

TEST(ResultFlatten, Kinds)
{
    exercise_flatten<CopyDefault, CopyDefault>();
    exercise_flatten<CopyNoDefault, CopyDefault>();
    exercise_flatten<MoveDefault, CopyDefault>();
    exercise_flatten<MoveNoDefault, CopyDefault>();
    exercise_flatten<int*, CopyDefault>();
    exercise_flatten<int&, CopyDefault>();
    exercise_flatten<const int&, CopyDefault>();
    exercise_flatten<CopyDefault, MoveDefault>();
    exercise_flatten<CopyDefault, MoveNoDefault>();
    exercise_flatten<CopyDefault, int*>();
    exercise_flatten<CopyDefault, int&>();
    exercise_flatten<CopyDefault, const int&>();
}

TEST(ResultLifetime, RejectsDanglingBinds)
{
    static_assert(!std::is_constructible_v<Result<const int&, std::string>, int>);
    static_assert(std::is_constructible_v<Result<const int&, std::string>, int&>);
    static_assert(!std::is_constructible_v<Result<std::string, const int&>, int>);
    static_assert(std::is_constructible_v<Result<std::string, const int&>, int&>);
    static_assert(!std::is_constructible_v<Result<const int&, int>, std::in_place_index_t<0>, int>);
    static_assert(std::is_constructible_v<Result<const int&, int>, std::in_place_index_t<0>, int&>);
    static_assert(!std::is_constructible_v<Result<int, const int&>, std::in_place_index_t<1>, int>);
    static_assert(std::is_constructible_v<Result<int, const int&>, std::in_place_index_t<1>, int&>);
    static_assert(requires(Result<int, int>& res) { res.as_ref(); res.as_mut(); });
    static_assert(!std::is_default_constructible_v<Result<int&, int>>);
    static_assert(!std::is_default_constructible_v<Result<const int&, int>>);
    SUCCEED();
}

TEST(ResultLifetime, ValueOwnsTemporary)
{
    int flag = 0;
    auto ok = Result<crab_test::Tracked, int>(std::in_place_index<0>, crab_test::Tracked(&flag, 4));
    EXPECT_TRUE(ok.is_ok());
    EXPECT_EQ(flag, 1);
    EXPECT_EQ(ok.as_mut().unwrap().value(), 4);
    auto borrowed = ok.as_ref();
    EXPECT_EQ(std::addressof(borrowed.unwrap()), std::addressof(ok.as_mut().unwrap()));
    EXPECT_EQ(flag, 1);
}

TEST(ResultLifetime, ReferenceDoesNotOwn)
{
    int flag = 0;
    crab_test::Tracked obj(&flag, 4);
    {
        Result<const crab_test::Tracked&, int> ok(std::in_place_index<0>, obj);
        EXPECT_EQ(flag, 1);
        EXPECT_EQ(std::addressof(ok.unwrap()), &obj);
    }
    EXPECT_EQ(flag, 1);
}

TEST(ResultLifetime, ReferenceStaysOkAfterReferentDies)
{
    int flag = 0;
    Result<const crab_test::Tracked&, int>* leaked = nullptr;
    {
        crab_test::Tracked obj(&flag, 4);
        leaked = new Result<const crab_test::Tracked&, int>(std::in_place_index<0>, obj);
        EXPECT_EQ(flag, 1);
    }
    EXPECT_EQ(flag, 0);
    EXPECT_TRUE(leaked->is_ok());
    delete leaked;
}

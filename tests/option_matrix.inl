// Option x type-kind x value-category x Some/None. Included from tests/option.cpp.

using crab_test::CopyDefault;
using crab_test::CopyNoDefault;
using crab_test::MoveDefault;
using crab_test::MoveNoDefault;

template<typename T>
auto peek_option(Option<T>& opt) -> int
{
    if constexpr (crab_test::is_indirect<T>)
    {
        return crab_test::read_id<T>(opt.unwrap());
    }
    else
    {
        return crab_test::read_id<T>(opt.as_mut().unwrap());
    }
}

template<typename T>
auto peek_option(const Option<T>& opt) -> int
{
    if constexpr (crab_test::is_indirect<T>)
    {
        return crab_test::read_id<T>(opt.unwrap());
    }
    else
    {
        return crab_test::read_id<T>(opt.as_ref().unwrap());
    }
}

template<typename T>
void exercise_option()
{
    struct host
    {
        int anchor = 0;
        int other = 0;

        auto some(int id) -> Option<T>
        {
            if constexpr (std::is_pointer_v<T>)
            {
                this->anchor = id;

                return Option<T>(&this->anchor);
            }
            else if constexpr (std::is_lvalue_reference_v<T>)
            {
                this->anchor = id;

                return Option<T>(this->anchor);
            }
            else
            {
                return Option<T>(T{ id });
            }
        }

        auto some_other(int id) -> Option<T>
        {
            if constexpr (std::is_pointer_v<T>)
            {
                this->other = id;

                return Option<T>(&this->other);
            }
            else if constexpr (std::is_lvalue_reference_v<T>)
            {
                this->other = id;

                return Option<T>(this->other);
            }
            else
            {
                return Option<T>(T{ id });
            }
        }

        auto none() -> Option<T>
        {
            return Option<T>(None{});
        }
    };

    host h;
    constexpr bool option_copyable = std::is_copy_constructible_v<Option<T>>;
    constexpr bool mutable_ref =
        std::is_lvalue_reference_v<T> && !std::is_const_v<std::remove_reference_t<T>>;

    if constexpr (std::is_same_v<T, CopyDefault>)
    {
        constexpr auto id = [] {
            Option<CopyDefault> opt(CopyDefault{ 4 });

            return opt.unwrap().value();
        }();
        static_assert(id == 4);
    }

    if constexpr (crab_test::is_indirect<T>)
    {
        EXPECT_EQ(sizeof(Option<T>), sizeof(std::size_t));
    }

    if constexpr (std::is_pointer_v<T>)
    {
        T null_ptr = nullptr;
        Option<T> as_null(null_ptr);
        EXPECT_TRUE(as_null.is_none());
    }

    {
        SCOPED_TRACE("ctors");
        Option<T> def;
        EXPECT_TRUE(def.is_none());
        EXPECT_TRUE(h.none().is_none());
        EXPECT_TRUE(h.some(42).is_some());
        EXPECT_EQ(peek_option(h.some(42)), 42);

        if constexpr (option_copyable)
        {
            auto original = h.some(42);
            auto copied = original;
            EXPECT_EQ(peek_option(original), 42);
            EXPECT_EQ(peek_option(copied), 42);
        }

        auto original = h.some(42);
        auto moved = std::move(original);
        EXPECT_EQ(peek_option(moved), 42);

        if constexpr (crab_test::is_indirect<T>)
        {
            EXPECT_TRUE(original.is_some());
            EXPECT_EQ(peek_option(original), 42);
        }
        else if (original.is_some())
        {
            EXPECT_EQ(peek_option(original), -1);
        }

        auto assigned = h.some(1);
        assigned = None{};
        EXPECT_TRUE(assigned.is_none());

        if constexpr (std::is_pointer_v<T>)
        {
            h.other = 3;
            assigned = &h.other;
            EXPECT_EQ(assigned.unwrap(), &h.other);
        }
        else if constexpr (std::is_lvalue_reference_v<T>)
        {
            h.other = 3;
            assigned = h.other;
            EXPECT_EQ(std::addressof(assigned.unwrap()), &h.other);
        }
        else
        {
            assigned = T{ 3 };
            EXPECT_EQ(peek_option(assigned), 3);
        }

        if constexpr (option_copyable)
        {
            auto source = h.some(8);
            assigned = source;
            EXPECT_EQ(peek_option(assigned), 8);
            EXPECT_EQ(peek_option(source), 8);
        }

        auto source = h.some(8);
        assigned = std::move(source);
        EXPECT_EQ(peek_option(assigned), 8);
    }

    if constexpr (mutable_ref)
    {
        auto opt = h.some(1);
        opt.unwrap() = 5;
        EXPECT_EQ(h.anchor, 5);
        auto alias = opt;
        alias.unwrap() = 6;
        EXPECT_EQ(opt.unwrap(), 6);
        EXPECT_EQ(h.anchor, 6);
    }

    if constexpr (std::is_pointer_v<T>)
    {
        auto opt = h.some(1);
        *opt.unwrap() = 5;
        EXPECT_EQ(h.anchor, 5);
        EXPECT_EQ(*opt.unwrap(), 5);
    }

    for (bool some : { true, false })
    {
        SCOPED_TRACE(some ? "some" : "none");
        crab_test::for_categories(
            [&] { return some ? h.some(42) : h.none(); },
            [&](auto&& opt) {
                using acc = crab_test::access<decltype(opt)>;
                EXPECT_EQ(opt.is_some(), some);
                EXPECT_EQ(opt.is_none(), !some);

                if constexpr (acc::consume || crab_test::can_duplicate<T>)
                {
                if (some)
                {
                    decltype(auto) taken = std::forward<decltype(opt)>(opt).unwrap();
                    EXPECT_EQ(crab_test::read_id<T>(taken), 42);

                    if constexpr (std::is_pointer_v<T>)
                    {
                        EXPECT_EQ(taken, &h.anchor);
                    }
                    else if constexpr (std::is_lvalue_reference_v<T>)
                    {
                        EXPECT_EQ(std::addressof(taken), &h.anchor);
                    }

                    if (acc::consume)
                    {
                        EXPECT_TRUE(opt.is_none());
                    }
                    else
                    {
                        EXPECT_EQ(peek_option(opt), 42);
                    }
                }

                // Rebuild: unwrap above consumed a some rvalue.
                auto fresh = some ? h.some(42) : h.none();
                decltype(auto) live = crab_test::forward_like<decltype(opt)>(fresh);

                if (some)
                {
                    EXPECT_EQ(crab_test::read_id<T>(std::forward<decltype(live)>(live).expect("boom")), 42);
                }

                auto unchecked = some ? h.some(42) : h.none();
                if (some)
                {
                    decltype(auto) taken = crab_test::forward_like<decltype(opt)>(unchecked).unwrap_unchecked();
                    EXPECT_EQ(crab_test::read_id<T>(taken), 42);
                }
                }
            }
        );
    }

    for (bool some : { true, false })
    {
        SCOPED_TRACE(some ? "unwrap_or some" : "unwrap_or none");
        crab_test::for_categories(
            [&] { return some ? h.some(42) : h.none(); },
            [&](auto&& opt) {
                using acc = crab_test::access<decltype(opt)>;

                if constexpr (acc::consume || crab_test::can_duplicate<T>)
                {
                if constexpr (std::is_lvalue_reference_v<T>)
                {
                    int fallback = 9;
                    decltype(auto) taken = std::forward<decltype(opt)>(opt).unwrap_or(fallback);
                    EXPECT_EQ(crab_test::read_id<T>(taken), some ? 42 : 9);

                    if (!some)
                    {
                        EXPECT_EQ(std::addressof(taken), &fallback);
                    }
                    else
                    {
                        EXPECT_EQ(std::addressof(taken), &h.anchor);
                    }
                }
                else if constexpr (std::is_pointer_v<T>)
                {
                    int fallback = 9;
                    decltype(auto) taken = std::forward<decltype(opt)>(opt).unwrap_or(&fallback);
                    EXPECT_EQ(taken, some ? &h.anchor : &fallback);
                }
                else
                {
                    decltype(auto) taken = std::forward<decltype(opt)>(opt).unwrap_or(T{ 9 });
                    EXPECT_EQ(taken.value(), some ? 42 : 9);
                }

                if (acc::consume && some)
                {
                    EXPECT_TRUE(opt.is_none());
                }
                else if (some)
                {
                    EXPECT_EQ(peek_option(opt), 42);
                }
                }
            }
        );
    }

    for (bool some : { true, false })
    {
        SCOPED_TRACE(some ? "unwrap_or_else some" : "unwrap_or_else none");
        crab_test::for_categories(
            [&] { return some ? h.some(42) : h.none(); },
            [&](auto&& opt) {
                using acc = crab_test::access<decltype(opt)>;

                if constexpr (acc::consume || crab_test::can_duplicate<T>)
                {
                int calls = 0;
                int fallback = 9;
                decltype(auto) taken = std::forward<decltype(opt)>(opt).unwrap_or_else([&]() -> T {
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
                EXPECT_EQ(calls, some ? 0 : 1);
                EXPECT_EQ(crab_test::read_id<T>(taken), some ? 42 : 9);

                if constexpr (std::is_lvalue_reference_v<T>)
                {
                    EXPECT_EQ(std::addressof(taken), some ? &h.anchor : &fallback);
                }
                }
            }
        );
    }

    if constexpr (!std::is_lvalue_reference_v<T> && std::is_default_constructible_v<T>)
    {
        for (bool some : { true, false })
        {
            crab_test::for_categories(
                [&] { return some ? h.some(42) : h.none(); },
                [&](auto&& opt) {
                    using acc = crab_test::access<decltype(opt)>;

                    if constexpr (acc::consume || crab_test::can_duplicate<T>)
                    {
                    decltype(auto) taken = std::forward<decltype(opt)>(opt).unwrap_or_default();

                    if (some)
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

    for (bool some : { true, false })
    {
        SCOPED_TRACE(some ? "map some" : "map none");
        crab_test::for_categories(
            [&] { return some ? h.some(42) : h.none(); },
            [&](auto&& opt) {
                using acc = crab_test::access<decltype(opt)>;
                int arg_cat = 0;

                if constexpr (std::is_pointer_v<T> && acc::is_lvalue)
                {
                    if (some)
                    {
                        EXPECT_EQ(opt.unwrap(), &h.anchor);
                    }
                }

                auto mapped = std::forward<decltype(opt)>(opt).map([&](auto&& value) {
                    arg_cat = std::is_lvalue_reference_v<decltype(value)> ? 1 : 2;

                    if constexpr (std::is_pointer_v<T>)
                    {
                        int* pointer = value;
                        EXPECT_EQ(pointer, &h.anchor);

                        return pointer == nullptr ? -1 : *pointer;
                    }
                    else
                    {
                        return crab_test::read_id<T>(value);
                    }
                });

                if (some)
                {
                    constexpr bool expect_lvalue =
                        !std::is_pointer_v<T> &&
                        (std::is_lvalue_reference_v<T> || acc::is_lvalue);
                    EXPECT_EQ(mapped.unwrap(), 42);
                    EXPECT_EQ(arg_cat, expect_lvalue ? 1 : 2);
                }
                else
                {
                    EXPECT_TRUE(mapped.is_none());
                }

                if (acc::consume && some)
                {
                    EXPECT_TRUE(opt.is_none());
                }
                else if (some)
                {
                    EXPECT_EQ(peek_option(opt), 42);
                }

                auto for_map_or = some ? h.some(42) : h.none();
                EXPECT_EQ(
                    crab_test::forward_like<decltype(opt)>(for_map_or).map_or(0, [](const auto& value) {
                        return crab_test::read_id<T>(value);
                    }),
                    some ? 42 : 0
                );
                auto for_map_or_else = some ? h.some(42) : h.none();
                EXPECT_EQ(
                    crab_test::forward_like<decltype(opt)>(for_map_or_else).map_or_else(
                        [] { return 7; },
                        [](const auto& value) { return crab_test::read_id<T>(value); }
                    ),
                    some ? 42 : 7
                );
            }
        );
    }

    if constexpr (std::is_default_constructible_v<int>)
    {
        for (bool some : { true, false })
        {
            crab_test::for_categories(
                [&] { return some ? h.some(42) : h.none(); },
                [&](auto&& opt) {
                    EXPECT_EQ(
                        std::forward<decltype(opt)>(opt).map_or_default([](const auto& value) {
                            return crab_test::read_id<T>(value);
                        }),
                        some ? 42 : 0
                    );
                }
            );
        }
    }

    for (bool some : { true, false })
    {
        SCOPED_TRACE(some ? "and_then some" : "and_then none");
        crab_test::for_categories(
            [&] { return some ? h.some(42) : h.none(); },
            [&](auto&& opt) {
                using acc = crab_test::access<decltype(opt)>;
                auto next = std::forward<decltype(opt)>(opt).and_then([](const auto& value) {
                    return Option<int>(crab_test::read_id<T>(value));
                });

                if (some)
                {
                    EXPECT_EQ(next.unwrap(), 42);
                }
                else
                {
                    EXPECT_TRUE(next.is_none());
                }

                if (acc::consume && some)
                {
                    EXPECT_TRUE(opt.is_none());
                }
                else if (some)
                {
                    EXPECT_EQ(peek_option(opt), 42);
                }
            }
        );
    }

    for (bool self_some : { true, false })
    {
        for (bool other_some : { true, false })
        {
            SCOPED_TRACE("and_");
            crab_test::for_categories(
                [&] { return self_some ? h.some(42) : h.none(); },
                [&](auto&& self) {
                    auto other = other_some ? Option<int>(7) : Option<int>(None{});
                    auto result = std::forward<decltype(self)>(self).and_(crab_test::forward_like<decltype(self)>(other));

                    if (self_some && other_some)
                    {
                        EXPECT_EQ(result.unwrap(), 7);
                    }
                    else
                    {
                        EXPECT_TRUE(result.is_none());
                    }

                    using acc = crab_test::access<decltype(self)>;

                    if (acc::consume)
                    {
                        EXPECT_TRUE(self.is_none());
                        EXPECT_TRUE(other.is_none());
                    }
                    else
                    {
                        EXPECT_EQ(self.is_some(), self_some);
                        EXPECT_EQ(other.is_some(), other_some);
                    }
                }
            );
        }
    }

    for (bool self_some : { true, false })
    {
        for (bool other_some : { true, false })
        {
            SCOPED_TRACE("or_/xor_");
            crab_test::for_categories(
                [&] { return self_some ? h.some(1) : h.none(); },
                [&](auto&& self) {
                    using acc = crab_test::access<decltype(self)>;

                    if constexpr (acc::consume || option_copyable)
                    {
                    auto other = other_some ? h.some_other(2) : h.none();
                    auto or_result = std::forward<decltype(self)>(self).or_(crab_test::forward_like<decltype(self)>(other));

                    if (self_some)
                    {
                        EXPECT_EQ(crab_test::read_id<T>(std::move(or_result).unwrap()), 1);
                    }
                    else if (other_some)
                    {
                        EXPECT_EQ(crab_test::read_id<T>(std::move(or_result).unwrap()), 2);
                    }
                    else
                    {
                        EXPECT_TRUE(or_result.is_none());
                    }

                    auto lhs = self_some ? h.some(1) : h.none();
                    auto rhs = other_some ? h.some_other(2) : h.none();
                    auto xor_result = crab_test::forward_like<decltype(self)>(lhs).xor_(crab_test::forward_like<decltype(self)>(rhs));

                    if (self_some == other_some)
                    {
                        EXPECT_TRUE(xor_result.is_none());
                    }
                    else if (self_some)
                    {
                        EXPECT_EQ(crab_test::read_id<T>(std::move(xor_result).unwrap()), 1);
                    }
                    else
                    {
                        EXPECT_EQ(crab_test::read_id<T>(std::move(xor_result).unwrap()), 2);
                    }
                    }
                }
            );
        }
    }

    for (bool some : { true, false })
    {
        crab_test::for_categories(
            [&] { return some ? h.some(42) : h.none(); },
            [&](auto&& opt) {
                using acc = crab_test::access<decltype(opt)>;

                if constexpr (acc::consume || option_copyable)
                {
                int calls = 0;
                auto next = std::forward<decltype(opt)>(opt).or_else([&] {
                    ++calls;

                    return h.some(7);
                });
                EXPECT_EQ(calls, some ? 0 : 1);

                if (some)
                {
                    EXPECT_EQ(crab_test::read_id<T>(std::move(next).unwrap()), 42);
                }
                else
                {
                    EXPECT_EQ(crab_test::read_id<T>(std::move(next).unwrap()), 7);
                }
                }
            }
        );
    }

    for (bool lhs_some : { true, false })
    {
        for (bool rhs_some : { true, false })
        {
            crab_test::for_categories(
                [&] { return lhs_some ? h.some(1) : h.none(); },
                [&](auto&& lhs) {
                    using acc = crab_test::access<decltype(lhs)>;

                    if constexpr (acc::consume || option_copyable)
                    {
                    auto rhs = rhs_some ? h.some_other(2) : h.none();
                    auto zipped = std::forward<decltype(lhs)>(lhs).zip(crab_test::forward_like<decltype(lhs)>(rhs));

                    if (lhs_some && rhs_some)
                    {
                        ASSERT_TRUE(zipped.is_some());
                        auto pair = std::move(zipped).unwrap();
                        EXPECT_EQ(crab_test::read_id<T>(std::get<0>(pair)), 1);
                        EXPECT_EQ(crab_test::read_id<T>(std::get<1>(pair)), 2);

                        if constexpr (std::is_lvalue_reference_v<T>)
                        {
                            EXPECT_EQ(std::addressof(std::get<0>(pair)), &h.anchor);
                            EXPECT_EQ(std::addressof(std::get<1>(pair)), &h.other);
                        }
                    }
                    else
                    {
                        EXPECT_TRUE(zipped.is_none());
                    }

                    if (acc::consume)
                    {
                        EXPECT_TRUE(lhs.is_none());
                        EXPECT_TRUE(rhs.is_none());
                    }
                    else
                    {
                        EXPECT_EQ(lhs.is_some(), lhs_some);
                        EXPECT_EQ(rhs.is_some(), rhs_some);
                    }
                    }
                }
            );
        }
    }

    for (bool some : { true, false })
    {
        crab_test::for_categories(
            [&] { return some ? h.some(42) : h.none(); },
            [&](auto&& opt) {
                using acc = crab_test::access<decltype(opt)>;

                if constexpr (acc::consume || option_copyable)
                {
                auto kept = std::forward<decltype(opt)>(opt).filter([](const auto& value) {
                    return crab_test::read_id<T>(value) > 0;
                });
                auto dropped_src = some ? h.some(42) : h.none();
                auto dropped = crab_test::forward_like<decltype(opt)>(dropped_src).filter([](const auto&) {
                    return false;
                });

                if (some)
                {
                    EXPECT_EQ(crab_test::read_id<T>(std::move(kept).unwrap()), 42);
                    EXPECT_TRUE(dropped.is_none());
                }
                else
                {
                    EXPECT_TRUE(kept.is_none());
                    EXPECT_TRUE(dropped.is_none());
                }

                if (!acc::consume && some)
                {
                    EXPECT_EQ(peek_option(dropped_src), 42);
                }
                else if (acc::consume)
                {
                    EXPECT_TRUE(dropped_src.is_none());
                }
                }
            }
        );
    }

    for (bool some : { true, false })
    {
        crab_test::for_categories(
            [&] { return some ? h.some(42) : h.none(); },
            [&](auto&& opt) {
                using acc = crab_test::access<decltype(opt)>;
                int seen = 0;
                EXPECT_EQ(opt.is_some_and([&](const auto& value) {
                    seen = crab_test::read_id<T>(value);

                    return seen == 42;
                }), some);
                EXPECT_EQ(seen, some ? 42 : 0);
                EXPECT_EQ(opt.is_none_or([](const auto& value) {
                    return crab_test::read_id<T>(value) == 42;
                }), true);
                EXPECT_EQ(opt.is_some(), some);

                if (some)
                {
                    EXPECT_EQ(peek_option(opt), 42);
                }

                if constexpr (!(acc::is_const && !acc::is_lvalue) || option_copyable)
                {
                seen = 0;

                if constexpr (acc::is_lvalue)
                {
                    auto& out = std::forward<decltype(opt)>(opt).inspect([&](const auto& value) {
                        seen = crab_test::read_id<T>(value);
                    });
                    EXPECT_EQ(&out, &opt);
                }
                else
                {
                    auto out = std::forward<decltype(opt)>(opt).inspect([&](const auto& value) {
                        seen = crab_test::read_id<T>(value);
                    });
                    EXPECT_EQ(out.is_some(), some);

                    if (some)
                    {
                        EXPECT_EQ(peek_option(out), 42);
                    }
                }

                EXPECT_EQ(seen, some ? 42 : 0);
                }
            }
        );
    }

    {
        auto some = h.some(42);
        auto none = h.none();

        if constexpr (std::is_pointer_v<T>)
        {
            EXPECT_TRUE(some.contains(&h.anchor));
            EXPECT_FALSE(some.contains(&h.other));
        }
        else if constexpr (std::is_lvalue_reference_v<T>)
        {
            EXPECT_TRUE(some.contains(42));
            EXPECT_TRUE(some.contains(h.anchor));
            h.other = 42;
            EXPECT_TRUE(some.contains(h.other));
            h.other = 1;
            EXPECT_FALSE(some.contains(h.other));
        }
        else
        {
            EXPECT_TRUE(some.contains(T{ 42 }));
            EXPECT_FALSE(some.contains(T{ 1 }));
        }

        if constexpr (std::is_pointer_v<T>)
        {
            EXPECT_FALSE(none.contains(&h.anchor));
        }
        else if constexpr (std::is_lvalue_reference_v<T>)
        {
            EXPECT_FALSE(none.contains(0));
        }
        else
        {
            EXPECT_FALSE(none.contains(T{ 0 }));
        }
    }

    for (bool some : { true, false })
    {
        crab_test::for_categories(
            [&] { return some ? h.some(42) : h.none(); },
            [&](auto&& opt) {
                int cat = 0;
                int got = -2;
                std::forward<decltype(opt)>(opt).visit([&](auto&& value) -> int {
                    using value_t = std::remove_cvref_t<decltype(value)>;

                    if constexpr (std::is_same_v<value_t, None>)
                    {
                        cat = 0;

                        return 0;
                    }
                    else if constexpr (std::is_pointer_v<T>)
                    {
                        int* pointer = value;
                        cat = 2;
                        got = pointer == nullptr ? -1 : *pointer;

                        return 0;
                    }
                    else
                    {
                        cat = std::is_lvalue_reference_v<decltype(value)> ? 1 : 2;
                        got = crab_test::read_id<T>(value);

                        return 0;
                    }
                });

                if (some)
                {
                    constexpr bool expect_lvalue =
                        !std::is_pointer_v<T> &&
                        (std::is_lvalue_reference_v<T> || crab_test::access<decltype(opt)>::is_lvalue);
                    EXPECT_EQ(got, 42);
                    EXPECT_EQ(cat, expect_lvalue ? 1 : 2);
                    EXPECT_TRUE(opt.is_some());
                    EXPECT_EQ(peek_option(opt), 42);
                }
                else
                {
                    EXPECT_EQ(cat, 0);
                    EXPECT_TRUE(opt.is_none());
                }
            }
        );
    }

    for (bool some : { true, false })
    {
        crab_test::for_categories(
            [&] { return some ? h.some(42) : h.none(); },
            [&](auto&& opt) {
                using acc = crab_test::access<decltype(opt)>;

                if constexpr (acc::consume || crab_test::can_duplicate<T>)
                {
                auto ok = std::forward<decltype(opt)>(opt).ok_or(5);

                if (some)
                {
                    EXPECT_TRUE(ok.is_ok());
                    EXPECT_EQ(crab_test::read_id<T>(ok.as_mut().unwrap()), 42);

                    if constexpr (std::is_lvalue_reference_v<T>)
                    {
                        EXPECT_EQ(std::addressof(ok.as_mut().unwrap()), &h.anchor);
                    }
                }
                else
                {
                    EXPECT_TRUE(ok.is_err());
                    EXPECT_EQ(ok.unwrap_err(), 5);
                }

                auto fresh = some ? h.some(42) : h.none();
                auto else_ok = crab_test::forward_like<decltype(opt)>(fresh).ok_or_else([] { return 5; });
                EXPECT_EQ(else_ok.is_ok(), some);
                }
            }
        );
    }

    {
        SCOPED_TRACE("replace");
        auto some = h.some(1);

        if constexpr (std::is_pointer_v<T>)
        {
            h.other = 2;
            auto old = some.replace(&h.other);
            EXPECT_EQ(old.unwrap(), &h.anchor);
            EXPECT_EQ(some.unwrap(), &h.other);
        }
        else if constexpr (std::is_lvalue_reference_v<T>)
        {
            h.other = 2;
            auto old = some.replace(h.other);
            EXPECT_EQ(std::addressof(old.unwrap()), &h.anchor);
            EXPECT_EQ(std::addressof(some.unwrap()), &h.other);
        }
        else
        {
            auto old = some.replace(T{ 2 });
            EXPECT_EQ(peek_option(old), 1);
            EXPECT_EQ(peek_option(some), 2);
        }

        auto empty = h.none();

        if constexpr (std::is_pointer_v<T>)
        {
            auto old = empty.replace(&h.anchor);
            EXPECT_TRUE(old.is_none());
            EXPECT_EQ(empty.unwrap(), &h.anchor);
        }
        else if constexpr (std::is_lvalue_reference_v<T>)
        {
            auto old = empty.replace(h.anchor);
            EXPECT_TRUE(old.is_none());
            EXPECT_EQ(std::addressof(empty.unwrap()), &h.anchor);
        }
        else
        {
            auto old = empty.replace(T{ 4 });
            EXPECT_TRUE(old.is_none());
            EXPECT_EQ(peek_option(empty), 4);
        }
    }

    if constexpr (!crab_test::is_indirect<T>)
    {
        SCOPED_TRACE("insert");
        auto empty = h.none();
        auto& inserted = empty.insert(T{ 3 });
        EXPECT_EQ(inserted.value(), 3);
        EXPECT_EQ(std::addressof(inserted), std::addressof(empty.as_mut().unwrap()));
        auto& replaced = empty.insert(T{ 4 });
        EXPECT_EQ(replaced.value(), 4);
        EXPECT_EQ(peek_option(empty), 4);

        auto present = h.some(1);
        T extra{ 2 };
        auto& kept = present.get_or_insert(std::move(extra));
        EXPECT_EQ(kept.value(), 1);
        EXPECT_EQ(extra.value(), 2);

        auto missing = h.none();
        auto& filled = missing.get_or_insert(T{ 2 });
        EXPECT_EQ(filled.value(), 2);
        EXPECT_EQ(std::addressof(filled), std::addressof(missing.as_mut().unwrap()));

        int calls = 0;
        EXPECT_EQ(present.get_or_insert_with([&] {
            ++calls;

            return T{ 9 };
        }).value(), 1);
        EXPECT_EQ(calls, 0);
        auto produced = h.none();
        EXPECT_EQ(produced.get_or_insert_with([&] {
            ++calls;

            return T{ 9 };
        }).value(), 9);
        EXPECT_EQ(calls, 1);

        if constexpr (std::is_default_constructible_v<T>)
        {
            auto def = h.none();
            EXPECT_EQ(def.get_or_insert_default().value(), 0);
            EXPECT_EQ(def.get_or_insert_default().value(), 0);
        }

        auto owned = h.some(5);
        auto ref = owned.as_ref();
        auto mut = owned.as_mut();
        EXPECT_EQ(std::addressof(ref.unwrap()), std::addressof(mut.unwrap()));
        EXPECT_EQ(ref.unwrap().value(), 5);
        EXPECT_TRUE(owned.as_ref().is_some());
        auto empty_ref = h.none();
        EXPECT_TRUE(empty_ref.as_ref().is_none());
        EXPECT_TRUE(empty_ref.as_mut().is_none());
    }

    {
        SCOPED_TRACE("flatten");
        Option<Option<T>> outer(h.some(42));

        if constexpr (std::is_copy_constructible_v<Option<Option<T>>>)
        {
            auto flat = outer.flatten();
            EXPECT_EQ(peek_option(flat), 42);
            EXPECT_TRUE(outer.is_some());
        }

        auto flat = std::move(outer).flatten();
        EXPECT_EQ(peek_option(flat), 42);
        EXPECT_TRUE(outer.is_none());

        Option<Option<T>> empty(None{});

        if constexpr (std::is_copy_constructible_v<Option<Option<T>>>)
        {
            EXPECT_TRUE(empty.flatten().is_none());
        }

        EXPECT_TRUE(std::move(empty).flatten().is_none());

        Option<Option<T>> inner_none(h.none());

        if constexpr (std::is_copy_constructible_v<Option<Option<T>>>)
        {
            EXPECT_TRUE(inner_none.flatten().is_none());
            EXPECT_TRUE(inner_none.as_mut().unwrap().is_none());
        }

        EXPECT_TRUE(std::move(inner_none).flatten().is_none());
    }

    {
        SCOPED_TRACE("unzip");
        constexpr bool pair_copyable = std::is_copy_constructible_v<std::pair<T, T>>;
        auto make_pair = [&](bool filled) {
            if (!filled)
            {
                return Option<std::pair<T, T>>(None{});
            }

            if constexpr (std::is_pointer_v<T>)
            {
                h.anchor = 1;
                h.other = 2;

                return Option<std::pair<T, T>>(std::pair<T, T>(&h.anchor, &h.other));
            }
            else if constexpr (std::is_lvalue_reference_v<T>)
            {
                h.anchor = 1;
                h.other = 2;

                return Option<std::pair<T, T>>(std::pair<T, T>(h.anchor, h.other));
            }
            else
            {
                return Option<std::pair<T, T>>(std::pair<T, T>(T{ 1 }, T{ 2 }));
            }
        };

        if constexpr (pair_copyable)
        {
            auto empty = make_pair(false);
            auto unzipped_none = empty.unzip();
            EXPECT_TRUE(std::get<0>(unzipped_none).is_none());
            EXPECT_TRUE(std::get<1>(unzipped_none).is_none());
        }

        {
            auto empty = make_pair(false);
            auto unzipped_none = std::move(empty).unzip();
            EXPECT_TRUE(std::get<0>(unzipped_none).is_none());
            EXPECT_TRUE(std::get<1>(unzipped_none).is_none());
        }

        if constexpr (pair_copyable)
        {
            auto opt = make_pair(true);
            auto unzipped = opt.unzip();
            EXPECT_EQ(crab_test::read_id<T>(std::get<0>(unzipped).unwrap()), 1);
            EXPECT_EQ(crab_test::read_id<T>(std::get<1>(unzipped).unwrap()), 2);
            EXPECT_TRUE(opt.is_some());
        }

        auto opt = make_pair(true);
        auto unzipped = std::move(opt).unzip();
        EXPECT_EQ(crab_test::read_id<T>(std::move(std::get<0>(unzipped)).unwrap()), 1);
        EXPECT_EQ(crab_test::read_id<T>(std::move(std::get<1>(unzipped)).unwrap()), 2);
        EXPECT_TRUE(opt.is_none());

        if constexpr (mutable_ref)
        {
            h.anchor = 1;
            h.other = 2;
            Option<std::pair<T, T>> ref_pair(std::pair<T, T>(h.anchor, h.other));
            auto parts = ref_pair.unzip();
            std::get<0>(parts).unwrap() = 9;
            EXPECT_EQ(h.anchor, 9);
        }
    }

    {
        SCOPED_TRACE("compare");
        auto low = h.some(1);
        auto high = h.some_other(2);
        auto none = h.none();
        EXPECT_EQ(none, h.none());
        EXPECT_NE(low, none);
        EXPECT_LT(none, low);
        EXPECT_EQ(std::hash<Option<T>>{}(none), hash_value(none));
        EXPECT_NE(std::hash<Option<T>>{}(none), std::hash<Option<T>>{}(low));

        if constexpr (std::is_pointer_v<T>)
        {
            EXPECT_NE(low, high);

            if (std::less<>{}(&h.anchor, &h.other))
            {
                EXPECT_LT(low, high);
            }
            else
            {
                EXPECT_GT(low, high);
            }
        }
        else
        {
            EXPECT_LT(low, high);
            EXPECT_EQ(low, h.some(1));
            EXPECT_EQ(std::hash<Option<T>>{}(low), std::hash<Option<T>>{}(h.some(1)));
        }
    }
}

template<typename T>
class OptionMatrix : public ::testing::Test
{
};

using OptionKinds = ::testing::Types<
    CopyDefault,
    CopyNoDefault,
    MoveDefault,
    MoveNoDefault,
    int*,
    int&,
    const int&
>;

TYPED_TEST_SUITE(OptionMatrix, OptionKinds);

TYPED_TEST(OptionMatrix, All)
{
    exercise_option<TypeParam>();
}

TEST(OptionDeath, CopyDefault)
{
    EXPECT_DEATH(Option<CopyDefault>(None{}).unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
    EXPECT_DEATH(Option<CopyDefault>(None{}).expect("boom"), "Panic encountered: boom");
    Option<CopyDefault> none(None{});
    EXPECT_DEATH(none.unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
}

TEST(OptionDeath, CopyNoDefault)
{
    EXPECT_DEATH(Option<CopyNoDefault>(None{}).unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
    EXPECT_DEATH(Option<CopyNoDefault>(None{}).expect("boom"), "Panic encountered: boom");
}

TEST(OptionDeath, MoveDefault)
{
    EXPECT_DEATH(Option<MoveDefault>(None{}).unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
    EXPECT_DEATH(Option<MoveDefault>(None{}).expect("boom"), "Panic encountered: boom");
}

TEST(OptionDeath, MoveNoDefault)
{
    EXPECT_DEATH(Option<MoveNoDefault>(None{}).unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
    EXPECT_DEATH(Option<MoveNoDefault>(None{}).expect("boom"), "Panic encountered: boom");
}

TEST(OptionDeath, Pointer)
{
    EXPECT_DEATH(Option<int*>(None{}).unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
    EXPECT_DEATH(Option<int*>(None{}).expect("boom"), "Panic encountered: boom");
}

TEST(OptionDeath, Reference)
{
    EXPECT_DEATH(Option<int&>(None{}).unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
    EXPECT_DEATH(Option<int&>(None{}).expect("boom"), "Panic encountered: boom");
}

TEST(OptionDeath, ConstReference)
{
    EXPECT_DEATH(Option<const int&>(None{}).unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
    EXPECT_DEATH(Option<const int&>(None{}).expect("boom"), "Panic encountered: boom");
}

TEST(OptionLifetime, RejectsDanglingBinds)
{
    static_assert(!std::is_constructible_v<Option<int&>, int>);
    static_assert(!std::is_constructible_v<Option<const int&>, int>);
    static_assert(std::is_constructible_v<Option<int&>, int&>);
    static_assert(std::is_constructible_v<Option<const int&>, int&>);
    static_assert(std::is_constructible_v<Option<const int&>, const int&>);
    static_assert(!std::is_assignable_v<Option<const int&>, int>);
    static_assert(std::is_assignable_v<Option<int&>, int&>);
    static_assert(requires(Option<const int&> opt, const int& value) { opt.unwrap_or(value); });
    static_assert(requires(Option<int&> opt, int& value) { opt.unwrap_or(value); });
    static_assert(requires(Option<int&> opt, int& value) {
        opt.unwrap_or_else([&]() -> int& { return value; });
    });
    static_assert(requires(Option<CopyDefault>& opt) { opt.as_ref(); opt.as_mut(); });
    static_assert(sizeof(Option<int*>) == sizeof(std::size_t));
    static_assert(sizeof(Option<int&>) == sizeof(std::size_t));
    static_assert(sizeof(Option<const int&>) == sizeof(std::size_t));
    static_assert(!std::is_default_constructible_v<CopyNoDefault>);
    static_assert(std::is_default_constructible_v<Option<CopyNoDefault>>);
    static_assert(!std::is_copy_constructible_v<MoveDefault>);
    static_assert(std::is_move_constructible_v<MoveDefault>);
    static_assert(!std::is_copy_constructible_v<Option<MoveDefault>>);
    static_assert(std::is_copy_constructible_v<Option<int&>>);
    SUCCEED();
}

TEST(OptionLifetime, ValueOwnsTemporary)
{
    int flag = 0;
    auto opt = Option<crab_test::Tracked>(crab_test::Tracked(&flag, 4));
    EXPECT_TRUE(opt.is_some());
    EXPECT_EQ(flag, 1);
    EXPECT_EQ(opt.as_mut().unwrap().value(), 4);
    auto borrowed = opt.as_ref();
    EXPECT_EQ(std::addressof(borrowed.unwrap()), std::addressof(opt.as_mut().unwrap()));
    EXPECT_EQ(flag, 1);
}

TEST(OptionLifetime, DestroyingValueOptionRunsDestructor)
{
    int flag = 0;
    {
        Option<crab_test::Tracked> opt(crab_test::Tracked(&flag, 1));
        EXPECT_EQ(flag, 1);
    }
    EXPECT_EQ(flag, 0);
}

TEST(OptionLifetime, ReferenceDoesNotOwn)
{
    int flag = 0;
    crab_test::Tracked obj(&flag, 4);
    {
        Option<const crab_test::Tracked&> opt(obj);
        EXPECT_EQ(flag, 1);
        EXPECT_EQ(std::addressof(opt.unwrap()), &obj);
    }
    EXPECT_EQ(flag, 1);
    EXPECT_EQ(obj.value(), 4);
}

TEST(OptionLifetime, ReferenceStaysSomeAfterReferentDies)
{
    int flag = 0;
    Option<const crab_test::Tracked&>* leaked = nullptr;
    {
        crab_test::Tracked obj(&flag, 4);
        leaked = new Option<const crab_test::Tracked&>(obj);
        EXPECT_EQ(flag, 1);
        EXPECT_TRUE(leaked->is_some());
    }
    EXPECT_EQ(flag, 0);
    EXPECT_TRUE(leaked->is_some());
    delete leaked;
}

TEST(OptionLifetime, ValueCopyOutlivesSource)
{
    int flag = 0;
    Option<crab_test::Tracked>* owned = nullptr;
    {
        crab_test::Tracked obj(&flag, 4);
        owned = new Option<crab_test::Tracked>(obj);
        EXPECT_EQ(flag, 2);
    }
    EXPECT_EQ(flag, 1);
    EXPECT_TRUE(owned->is_some());
    EXPECT_EQ(owned->as_mut().unwrap().value(), 4);
    delete owned;
    EXPECT_EQ(flag, 0);
}

TEST(OptionLifetime, UnwrapOrBindsFallback)
{
    int fallback = 9;
    Option<int&> none(None{});
    auto& got = none.unwrap_or(fallback);
    EXPECT_EQ(&got, &fallback);
    int value = 4;
    Option<int&> some(value);
    auto& kept = some.unwrap_or(fallback);
    EXPECT_EQ(&kept, &value);

    const int const_fallback = 3;
    Option<const int&> const_none(None{});
    auto& const_got = const_none.unwrap_or(const_fallback);
    EXPECT_EQ(&const_got, &const_fallback);
}

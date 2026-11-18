#define CRAB_CPP_NO_SHORT_MACROS

#include <gtest/gtest.h>

#include "crab_cpp/macros.h"

#ifdef match
#error "match macro must not be defined when CRAB_CPP_NO_SHORT_MACROS is set"
#endif

import std;
import crab_cpp;

using namespace crab_cpp;

namespace
{
    template<typename A, typename B>
    auto make_pair_option(bool some, A a, B b) -> Option<std::pair<A, B>>
    {
        if (!some)
        {
            return None{};
        }

        return std::pair<A, B>{ a, b };
    }

    template<typename A, typename B>
    auto make_sum_result(bool ok, A a, B b) -> Result<A, std::string>
    {
        if (!ok)
        {
            return std::string("bad");
        }

        return static_cast<A>(a + static_cast<A>(b));
    }

    constexpr auto constexpr_try_result(bool ok) -> Result<int, int>
    {
        auto step = [](bool succeed) -> Result<int, int>
        {
            if (succeed)
            {
                return Result<int, int>(std::in_place_index<0>, 21);
            }

            return Result<int, int>(std::in_place_index<1>, -5);
        };

        CRAB_TRY(auto v, step(ok));

        return Result<int, int>(std::in_place_index<0>, v * 2);
    }

    constexpr auto constexpr_try_option(bool some) -> Option<int>
    {
        auto step = [](bool present) -> Option<int>
        {
            return present ? Option<int>(10) : Option<int>(None{});
        };

        CRAB_TRY(auto v, step(some));

        return v + 5;
    }

    static_assert(constexpr_try_result(true).unwrap() == 42);
    static_assert(constexpr_try_result(false).unwrap_err() == -5);
    static_assert(constexpr_try_option(true).unwrap() == 15);
    static_assert(constexpr_try_option(false).is_none());
}

TEST(TryMacroTest, NoShortMacrosAndCrabMatch)
{
    // Local variable named `match` must work cleanly when CRAB_CPP_NO_SHORT_MACROS is defined.
    int match = 7;
    Option<int> opt(match);

    const int out = CRAB_MATCH(opt)
    (
        [](int& x) { return x * 2; },
        [](None) { return 0; }
    );
    EXPECT_EQ(out, 14);
}

TEST(TryMacroTest, OptionPropagationAndValueCategory)
{
    auto add_two = [](Option<int> a, const Option<int>& b) -> Option<double>
    {
        CRAB_TRY(auto x, std::move(a));
        CRAB_TRY(auto y, b);

        return static_cast<double>(x + y);
    };

    Option<int> left(4);
    Option<int> right(6);
    auto ok = add_two(left, right);
    ASSERT_TRUE(ok.is_some());
    EXPECT_DOUBLE_EQ(ok.unwrap(), 10.0);
    EXPECT_TRUE(right.is_some());

    EXPECT_TRUE(add_two(None{}, right).is_none());
    EXPECT_TRUE(add_two(left, None{}).is_none());

    // Lvalue borrow leaves source intact; explicit move consumes
    Option<std::string> owned(std::string("hi"));
    auto borrow_fn = [&]() -> Option<std::size_t>
    {
        CRAB_TRY(auto s, owned);

        return s.size();
    };
    EXPECT_EQ(borrow_fn().unwrap(), 2u);
    EXPECT_TRUE(owned.is_some());
    EXPECT_EQ(owned.unwrap(), "hi");

    auto move_fn = [&]() -> Option<std::string>
    {
        CRAB_TRY(auto s, std::move(owned));

        return s + "!";
    };
    EXPECT_EQ(move_fn().unwrap(), "hi!");
    EXPECT_TRUE(owned.is_none());
}

TEST(TryMacroTest, OptionMoveOnlyReferenceAndCommas)
{
    auto unique_chain = [](bool succeed) -> Option<int>
    {
        auto make = [&](bool ok) -> Option<std::unique_ptr<int>>
        {
            if (!ok)
            {
                return None{};
            }

            return std::make_unique<int>(33);
        };

        CRAB_TRY(auto ptr, make(succeed));

        return *ptr + 9;
    };

    EXPECT_EQ(unique_chain(true).unwrap(), 42);
    EXPECT_TRUE(unique_chain(false).is_none());

    int value = 5;
    auto mutate_ref = [](Option<int&> ref) -> Option<int>
    {
        CRAB_TRY(auto& r, ref);
        r += 10;

        return r;
    };
    EXPECT_EQ(mutate_ref(Option<int&>(value)).unwrap(), 15);
    EXPECT_EQ(value, 15);
    EXPECT_TRUE(mutate_ref(Option<int&>(None{})).is_none());

    // Expressions with template argument commas & existing variable assignment
    auto comma_fn = [](bool ok) -> Option<int>
    {
        std::pair<int, int> pair{};
        CRAB_TRY(pair, make_pair_option<int, int>(ok, 3, 4));

        return pair.first + pair.second;
    };
    EXPECT_EQ(comma_fn(true).unwrap(), 7);
    EXPECT_TRUE(comma_fn(false).is_none());
}

TEST(TryMacroTest, OptionEvaluatesOnceAndVoid)
{
    int calls = 0;
    auto next = [&](bool ok) -> Option<int>
    {
        ++calls;

        return ok ? Option<int>(1) : Option<int>(None{});
    };

    auto run = [&](bool first_ok, bool second_ok) -> Option<int>
    {
        if (true)
            CRAB_TRY_VOID(next(first_ok));

        CRAB_TRY(auto v, next(second_ok));

        return v + 10;
    };

    EXPECT_EQ(run(true, true).unwrap(), 11);
    EXPECT_EQ(calls, 2);

    calls = 0;
    EXPECT_TRUE(run(false, true).is_none());
    EXPECT_EQ(calls, 1);
}

TEST(TryMacroTest, ResultPropagationAndConversions)
{
    auto parse_and_add = [](bool ok1, bool ok2) -> Result<double, std::string>
    {
        CRAB_TRY(auto a, make_sum_result<int, int>(ok1, 10, 2));
        CRAB_TRY(auto b, make_sum_result<int, int>(ok2, 3, 5));

        return static_cast<double>(a + b);
    };

    auto ok = parse_and_add(true, true);
    ASSERT_TRUE(ok.is_ok());
    EXPECT_DOUBLE_EQ(ok.unwrap(), 20.0);

    auto err = parse_and_add(false, true);
    ASSERT_TRUE(err.is_err());
    EXPECT_EQ(err.unwrap_err(), "bad");

    // Convertible error type: const char* -> std::string
    auto conv_fn = [](bool fail) -> Result<int, std::string>
    {
        auto inner = [&]() -> Result<int, const char*>
        {
            if (fail)
            {
                return Result<int, const char*>(std::in_place_index<1>, "oops");
            }

            return 7;
        };

        CRAB_TRY(auto v, inner());

        return v * 3;
    };

    EXPECT_EQ(conv_fn(false).unwrap(), 21);
    EXPECT_EQ(conv_fn(true).unwrap_err(), "oops");

    // Same Ok and Err type (T == E)
    auto same_type_fn = [](bool fail) -> Result<int, int>
    {
        auto inner = [&]() -> Result<std::string, int>
        {
            if (fail)
            {
                return 99;
            }

            return std::string("abc");
        };

        CRAB_TRY(auto s, inner());

        return Result<int, int>(std::in_place_index<0>, static_cast<int>(s.size()));
    };

    EXPECT_EQ(same_type_fn(false).unwrap(), 3);
    EXPECT_EQ(same_type_fn(true).unwrap_err(), 99);
}

TEST(TryMacroTest, ResultMoveOnlyReferenceAnyAndVoid)
{
    // Move-only Ok and move-only Err
    auto move_only_fn = [](int mode) -> Result<std::unique_ptr<int>, std::unique_ptr<int>>
    {
        auto step = [&]() -> Result<std::unique_ptr<int>, std::unique_ptr<int>>
        {
            if (mode == 0)
            {
                return Result<std::unique_ptr<int>, std::unique_ptr<int>>(
                    std::in_place_index<0>,
                    std::make_unique<int>(5)
                );
            }

            return Result<std::unique_ptr<int>, std::unique_ptr<int>>(
                std::in_place_index<1>,
                std::make_unique<int>(-1)
            );
        };

        CRAB_TRY(auto p, step());

        return Result<std::unique_ptr<int>, std::unique_ptr<int>>(
            std::in_place_index<0>,
            std::make_unique<int>(*p + 1)
        );
    };

    EXPECT_EQ(*move_only_fn(0).unwrap(), 6);
    EXPECT_EQ(*move_only_fn(1).unwrap_err(), -1);

    // Reference error preservation
    std::string err_slot = "initial";
    auto ref_err_fn = [&](bool fail) -> Result<int, std::string&>
    {
        auto inner = [&]() -> Result<int, std::string&>
        {
            if (fail)
            {
                return Result<int, std::string&>(std::in_place_index<1>, err_slot);
            }

            return Result<int, std::string&>(std::in_place_index<0>, 12);
        };

        CRAB_TRY(auto v, inner());

        return Result<int, std::string&>(std::in_place_index<0>, v);
    };

    EXPECT_EQ(ref_err_fn(false).unwrap(), 12);
    auto ref_err = ref_err_fn(true);
    ASSERT_TRUE(ref_err.is_err());
    ref_err.unwrap_err() += "!";
    EXPECT_EQ(err_slot, "initial!");

    // Result<std::any, int> must propagate Err instead of wrapping carrier into std::any
    auto any_fn = [](bool fail) -> Result<std::any, int>
    {
        auto inner = [&]() -> Result<int, int>
        {
            if (fail)
            {
                return Result<int, int>(std::in_place_index<1>, 77);
            }

            return Result<int, int>(std::in_place_index<0>, 11);
        };

        CRAB_TRY(auto v, inner());

        return std::any(v);
    };

    auto any_ok = any_fn(false);
    ASSERT_TRUE(any_ok.is_ok());
    EXPECT_EQ(std::any_cast<int>(any_ok.unwrap()), 11);

    auto any_err = any_fn(true);
    ASSERT_TRUE(any_err.is_err());
    EXPECT_EQ(any_err.unwrap_err(), 77);

    // CRAB_TRY_VOID on Result<None, std::string>
    auto void_step = [](bool fail) -> Result<None, std::string>
    {
        if (fail)
        {
            return std::string("stopped");
        }

        return None{};
    };

    auto void_fn = [&](bool fail) -> Result<int, std::string>
    {
        CRAB_TRY_VOID(void_step(fail));

        return 100;
    };

    EXPECT_EQ(void_fn(false).unwrap(), 100);
    EXPECT_EQ(void_fn(true).unwrap_err(), "stopped");
}

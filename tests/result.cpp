#include <gtest/gtest.h>

import std;
import crab_cpp;

#include "probe_types.h"

using namespace crab_cpp;

TEST(ResultTest, BasicOperations)
{
    // Test Ok
    auto ok = Result<int, std::string>(42);
    EXPECT_TRUE(ok.is_ok());
    EXPECT_FALSE(ok.is_err());

    // Test Err
    auto err = Result<int, std::string>("error");
    EXPECT_FALSE(err.is_ok());
    EXPECT_TRUE(err.is_err());
}

TEST(ResultTest, Unwrap)
{
    // Test unwrap on Ok
    auto ok = Result<int, std::string>(42);
    EXPECT_EQ(ok.unwrap(), 42);

    // Test unwrap on Err (should panic)
    auto err = Result<int, std::string>("error");
    EXPECT_DEATH(err.unwrap(), "Panic encountered: Calling Result<T, E>::unwrap\\(\\) on an Err value");

    // Test unwrap_err on Err
    EXPECT_EQ(err.unwrap_err(), "error");

    // Test unwrap_err on Ok (should panic)
    EXPECT_DEATH(ok.unwrap_err(), "Panic encountered: Calling Result<T, E>::unwrap_err\\(\\) on an Ok value");
}

TEST(ResultTest, Expect)
{
    // Test expect on Ok
    auto ok = Result<int, std::string>(42);
    EXPECT_EQ(ok.expect("should not panic"), 42);

    // Test expect on Err (should panic)
    auto err = Result<int, std::string>("error");
    EXPECT_DEATH(err.expect("custom panic message"), "Panic encountered: custom panic message");

    // Test expect_err on Err
    EXPECT_EQ(err.expect_err("should not panic"), "error");

    // Test expect_err on Ok (should panic)
    EXPECT_DEATH(ok.expect_err("custom panic message"), "Panic encountered: custom panic message");
}

TEST(ResultTest, Map)
{
    // Test map on Ok
    auto ok = Result<int, std::string>(42);
    auto mapped = ok.map([](int x) { return x * 2; });
    EXPECT_TRUE(mapped.is_ok());
    EXPECT_EQ(mapped.unwrap(), 84);

    // Test map on Err
    auto err = Result<int, std::string>("error");
    auto mapped_err = err.map([](int x) { return x * 2; });
    EXPECT_TRUE(mapped_err.is_err());
    EXPECT_EQ(mapped_err.unwrap_err(), "error");

    // Test map_err on Err
    auto mapped_err2 = err.map_err([](const std::string& e) { return e + " occurred"; });
    EXPECT_TRUE(mapped_err2.is_err());
    EXPECT_EQ(mapped_err2.unwrap_err(), "error occurred");

    // Test map_err on Ok
    auto mapped_ok = ok.map_err([](const std::string& e) { return e + " occurred"; });
    EXPECT_TRUE(mapped_ok.is_ok());
    EXPECT_EQ(mapped_ok.unwrap(), 42);
}

TEST(ResultTest, AndThen)
{
    // Test and_then on Ok
    auto ok = Result<int, std::string>(42);
    auto result = ok.and_then([](int x) { return Result<int, std::string>(x * 2); });
    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.unwrap(), 84);

    // Test and_then on Err
    auto err = Result<int, std::string>("error");
    auto result_err = err.and_then([](int x) { return Result<int, std::string>(x * 2); });
    EXPECT_TRUE(result_err.is_err());
    EXPECT_EQ(result_err.unwrap_err(), "error");
}

TEST(ResultTest, OkAndErr)
{
    // Test ok() on Ok
    auto ok = Result<int, std::string>(42);
    auto option = ok.ok();
    EXPECT_TRUE(option.is_some());
    EXPECT_EQ(option.unwrap(), 42);
    EXPECT_TRUE(ok.is_ok()); // ok() should not invalidate the result

    // Test ok() on Err
    auto err = Result<int, std::string>("error");
    option = err.ok();
    EXPECT_TRUE(option.is_none());

    // Test err() on Err
    auto err_option = err.err();
    EXPECT_TRUE(err_option.is_some());
    EXPECT_EQ(err_option.unwrap(), "error");
    EXPECT_TRUE(err.is_err()); // err() should not invalidate the result

    // Test err() on Ok
    err_option = ok.err();
    EXPECT_TRUE(err_option.is_none());
}

TEST(ResultTest, Inspect)
{
    int value = 0;
    std::string error;

    // Test inspect on Ok
    auto ok = Result<int, std::string>(42);
    ok.inspect([&value](int x) { value = x; });
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(ok.is_ok());

    // Test inspect on Err
    value = 0;
    auto err = Result<int, std::string>("error");
    err.inspect([&value](int x) { value = x; });
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(err.is_err());

    // Test inspect_err on Err
    err.inspect_err([&error](const std::string& e) { error = e; });
    EXPECT_EQ(error, "error");
    EXPECT_TRUE(err.is_err());

    // Test inspect_err on Ok
    error.clear();
    ok.inspect_err([&error](const std::string& e) { error = e; });
    EXPECT_TRUE(error.empty());
    EXPECT_TRUE(ok.is_ok());
}


TEST(ResultTest, AndThenTypeChange)
{
    // Test and_then on Ok changing the value type
    auto ok = Result<int, std::string>(42);
    auto changed = ok.and_then([](int x) { return Result<double, std::string>(x * 1.5); });
    static_assert(std::is_same_v<decltype(changed), Result<double, std::string>>);
    EXPECT_TRUE(changed.is_ok());
    EXPECT_DOUBLE_EQ(changed.unwrap(), 63.0);

    // Test and_then on Err propagates the error
    auto err = Result<int, std::string>("error");
    auto propagated = err.and_then([](int x) { return Result<double, std::string>(x * 1.5); });
    EXPECT_TRUE(propagated.is_err());
    EXPECT_EQ(propagated.unwrap_err(), "error");
}

TEST(ResultTest, MapTypeChange)
{
    // Test map on Ok changing the value type
    auto ok = Result<int, std::string>(42);
    auto mapped = ok.map([](int x) { return x * 1.5; });
    static_assert(std::is_same_v<decltype(mapped), Result<double, std::string>>);
    EXPECT_TRUE(mapped.is_ok());
    EXPECT_DOUBLE_EQ(mapped.unwrap(), 63.0);

    // Test map on Err keeps the error
    auto err = Result<int, std::string>("error");
    auto mapped_err = err.map([](int x) { return x * 1.5; });
    EXPECT_TRUE(mapped_err.is_err());
    EXPECT_EQ(mapped_err.unwrap_err(), "error");
}

TEST(ResultTest, MapErrTypeChange)
{
    // Test map_err on Err changing the error type
    auto err = Result<int, std::string>("error");
    auto mapped = err.map_err([](const std::string& e) { return static_cast<double>(e.size()); });
    static_assert(std::is_same_v<decltype(mapped), Result<int, double>>);
    EXPECT_TRUE(mapped.is_err());
    EXPECT_DOUBLE_EQ(mapped.unwrap_err(), 5.0);

    // Test map_err on Ok keeps the value
    auto ok = Result<int, std::string>(42);
    auto mapped_ok = ok.map_err([](const std::string& e) { return static_cast<double>(e.size()); });
    EXPECT_TRUE(mapped_ok.is_ok());
    EXPECT_EQ(mapped_ok.unwrap(), 42);
}

TEST(ResultTest, ReferenceStorage)
{
    int value = 4;
    auto ok = Result<int, std::string>(value);
    auto ref = ok.as_ref();
    static_assert(std::is_same_v<decltype(ref), Result<const int&, const std::string&>>);
    EXPECT_EQ(ref.unwrap(), 4);

    auto mut = ok.as_mut();
    static_assert(std::is_same_v<decltype(mut), Result<int&, std::string&>>);
    mut.unwrap() = 9;
    EXPECT_EQ(ok.unwrap(), 9);
    EXPECT_EQ(value, 4);

    Result<int&, std::string> ref_ok(value);
    EXPECT_EQ(ref_ok.unwrap(), 4);
    ref_ok.unwrap() = 11;
    EXPECT_EQ(value, 11);

    Result<int&, std::string> ref_err(std::string("missing"));
    EXPECT_TRUE(ref_err.is_err());
    EXPECT_EQ(ref_err.unwrap_err(), "missing");

    auto borrowed = Option<int&>(value).ok_or(std::string("missing"));
    static_assert(std::is_same_v<decltype(borrowed), Result<int&, std::string>>);
    EXPECT_EQ(borrowed.unwrap(), 11);

    auto err = Result<int, std::string>("e");
    auto err_ref = err.as_mut();
    EXPECT_EQ(err_ref.unwrap_err(), "e");
    err_ref.unwrap_err() += "!";
    EXPECT_EQ(err.unwrap_err(), "e!");

    auto mapped = std::move(ref_ok).map([](int& item) { return item + 1; });
    EXPECT_EQ(mapped.unwrap(), 12);
    EXPECT_EQ(value, 11);
}

TEST(ResultTest, MapErrInPlace)
{
    auto err = Result<int, std::string>("error");
    auto mapped = err.map_err([](const std::string& e) { return static_cast<int>(e.size()); });
    static_assert(std::is_same_v<decltype(mapped), Result<int, int>>);
    EXPECT_TRUE(mapped.is_err());
    EXPECT_EQ(mapped.unwrap_err(), 5);
    EXPECT_EQ(err.unwrap_err(), "error");

    auto ok = Result<std::unique_ptr<int>, std::string>(std::make_unique<int>(3));
    auto moved = std::move(ok).map_err([](std::string) { return 0; });
    EXPECT_EQ(*std::move(moved).unwrap(), 3);
    EXPECT_TRUE(ok.is_ok());
    EXPECT_EQ(ok.as_mut().unwrap(), nullptr);
}

TEST(ResultTest, Flatten)
{
    auto ok_ok = Result<Result<int, std::string>, std::string>(Result<int, std::string>(7));
    EXPECT_EQ(ok_ok.flatten().unwrap(), 7);
    EXPECT_EQ(ok_ok.unwrap().unwrap(), 7);

    auto ok_err = Result<Result<int, std::string>, std::string>(Result<int, std::string>(std::string("inner")));
    EXPECT_EQ(ok_err.flatten().unwrap_err(), "inner");
    EXPECT_TRUE(ok_err.is_ok());

    auto err = Result<Result<int, std::string>, std::string>(std::string("outer"));
    EXPECT_EQ(err.flatten().unwrap_err(), "outer");
    EXPECT_EQ(err.unwrap_err(), "outer");

    auto nested = Result<Result<std::string, int>, int>(Result<std::string, int>(std::string("hi")));
    auto flat = std::move(nested).flatten();
    EXPECT_EQ(flat.unwrap(), "hi");
    EXPECT_TRUE(nested.is_ok());
    EXPECT_TRUE(nested.unwrap().unwrap().empty());

    auto outer_err = Result<Result<std::string, std::string>, std::string>(std::string("outer"));
    auto flat_err = std::move(outer_err).flatten();
    EXPECT_EQ(flat_err.unwrap_err(), "outer");
    EXPECT_TRUE(outer_err.is_err());
    EXPECT_TRUE(outer_err.unwrap_err().empty());
}

TEST(ResultTest, UnwrapUnchecked)
{
    auto ok = Result<std::string, int>(std::string("hi"));
    EXPECT_EQ(ok.unwrap_unchecked(), "hi");
    EXPECT_EQ(ok.unwrap(), "hi");

    auto consumed = std::move(ok).unwrap_unchecked();
    EXPECT_EQ(consumed, "hi");
    EXPECT_TRUE(ok.is_ok());
    EXPECT_TRUE(ok.unwrap().empty());

    int value = 4;
    Result<int&, std::string> ref_ok(value);
    EXPECT_EQ(ref_ok.unwrap_unchecked(), 4);
    ref_ok.unwrap_unchecked() = 8;
    EXPECT_EQ(value, 8);

    const auto locked = Result<std::string, int>(std::string("stay"));
    auto from_const_rvalue = static_cast<const Result<std::string, int>&&>(locked).unwrap_unchecked();
    EXPECT_EQ(from_const_rvalue, "stay");
    EXPECT_EQ(locked.unwrap(), "stay");
}

TEST(ResultTest, UnwrapErrUnchecked)
{
    auto err = Result<int, std::string>(std::string("boom"));
    EXPECT_EQ(err.unwrap_err_unchecked(), "boom");
    EXPECT_EQ(err.unwrap_err(), "boom");

    auto consumed = std::move(err).unwrap_err_unchecked();
    EXPECT_EQ(consumed, "boom");
    EXPECT_TRUE(err.is_err());
    EXPECT_TRUE(err.unwrap_err().empty());

    std::string message = "err";
    Result<int, std::string&> ref_err(std::in_place_index<1>, message);
    EXPECT_EQ(ref_err.unwrap_err_unchecked(), "err");
    ref_err.unwrap_err_unchecked() += "!";
    EXPECT_EQ(message, "err!");

    int ok_slot = 7;
    Result<int&, std::string> mixed(std::in_place_index<1>, std::string("mixed"));
    EXPECT_EQ(mixed.unwrap_err_unchecked(), "mixed");
    auto moved_mixed = std::move(mixed).unwrap_err_unchecked();
    EXPECT_EQ(moved_mixed, "mixed");
    EXPECT_TRUE(mixed.unwrap_err().empty());
    (void)ok_slot;

    const auto locked = Result<int, std::string>(std::string("stay"));
    auto from_const_rvalue = static_cast<const Result<int, std::string>&&>(locked).unwrap_err_unchecked();
    EXPECT_EQ(from_const_rvalue, "stay");
    EXPECT_EQ(locked.unwrap_err(), "stay");
}

TEST(ResultTest, MapOrElse)
{
    auto ok = Result<std::string, std::string>(std::in_place_index<0>, std::string("hello"));
    int fallback_calls = 0;
    auto ok_len = ok.map_or_else(
        [&](const std::string& e)
        {
            ++fallback_calls;
            return e.size() + 100;
        },
        [](const std::string& s) { return s.size(); }
    );
    EXPECT_EQ(ok_len, 5);
    EXPECT_EQ(fallback_calls, 0);
    EXPECT_EQ(ok.unwrap(), "hello");

    auto err = Result<std::string, std::string>(std::in_place_index<1>, std::string("fail"));
    int ok_calls = 0;
    auto err_len = err.map_or_else(
        [](const std::string& e) { return e.size() + 100; },
        [&](const std::string& s)
        {
            ++ok_calls;
            return s.size();
        }
    );
    EXPECT_EQ(err_len, 104);
    EXPECT_EQ(ok_calls, 0);
    EXPECT_EQ(err.unwrap_err(), "fail");

    auto moved_ok = std::move(ok).map_or_else(
        [](std::string) { return std::string("fallback"); },
        [](std::string s) { return s + "!"; }
    );
    EXPECT_EQ(moved_ok, "hello!");
    EXPECT_TRUE(ok.unwrap().empty());

    auto moved_err = std::move(err).map_or_else(
        [](std::string e) { return e + "!"; },
        [](std::string) { return std::string("ok"); }
    );
    EXPECT_EQ(moved_err, "fail!");
    EXPECT_TRUE(err.unwrap_err().empty());

    int ok_val = 10;
    int err_val = 20;
    Result<int&, int&> ref_res(std::in_place_index<0>, ok_val);
    EXPECT_EQ(ref_res.map_or_else([](int& e) { return e * 2; }, [](int& v) { return v + 1; }), 11);
    Result<int&, int&> ref_err_res(std::in_place_index<1>, err_val);
    EXPECT_EQ(ref_err_res.map_or_else([](int& e) { return e * 2; }, [](int& v) { return v + 1; }), 40);
}

TEST(ResultTest, MapOrDefault)
{
    auto ok = Result<std::string, int>(std::string("abc"));
    EXPECT_EQ(ok.map_or_default([](const std::string& s) { return s.size(); }), 3);
    EXPECT_EQ(ok.unwrap(), "abc");

    auto err = Result<std::string, int>(404);
    int calls = 0;
    auto fallback = err.map_or_default([&](const std::string& s)
    {
        ++calls;
        return s.size();
    });
    EXPECT_EQ(fallback, 0);
    EXPECT_EQ(calls, 0);

    auto moved = std::move(ok).map_or_default([](std::string s) { return s + "!"; });
    EXPECT_EQ(moved, "abc!");
    EXPECT_TRUE(ok.unwrap().empty());

    int slot = 9;
    Result<int&, int> ref_ok(std::in_place_index<0>, slot);
    EXPECT_EQ(ref_ok.map_or_default([](int& v) { return v * 3; }), 27);
    Result<int&, int> ref_err(std::in_place_index<1>, 1);
    EXPECT_EQ(ref_err.map_or_default([](int& v) { return v * 3; }), 0);

    struct NoDefault
    {
        NoDefault() = delete;
        explicit NoDefault(int) {}
    };
    auto can_map_or_default = []<typename Res>(Res& res)
    {
        return requires { res.map_or_default([](int v) { return NoDefault(v); }); };
    };
    Result<int, int> dummy(std::in_place_index<0>, 1);
    static_assert(!can_map_or_default(dummy));
}

TEST(ResultTest, StdExpectedConversion)
{
    // From const std::expected&
    const std::expected<std::string, int> ok_src = "hello";
    Result<std::string, int> from_ok = ok_src;
    EXPECT_TRUE(from_ok.is_ok());
    EXPECT_EQ(from_ok.unwrap(), "hello");

    const std::expected<std::string, int> err_src = std::unexpected(404);
    Result<std::string, int> from_err = err_src;
    EXPECT_TRUE(from_err.is_err());
    EXPECT_EQ(from_err.unwrap_err(), 404);

    // From std::expected&& (move-only Ok / Err)
    std::expected<std::unique_ptr<int>, std::string> moved_ok_src = std::make_unique<int>(7);
    Result<std::unique_ptr<int>, std::string> moved_ok = std::move(moved_ok_src);
    ASSERT_TRUE(moved_ok.is_ok());
    EXPECT_EQ(*std::move(moved_ok).unwrap(), 7);

    std::expected<int, std::unique_ptr<int>> moved_err_src = std::unexpected(std::make_unique<int>(8));
    Result<int, std::unique_ptr<int>> moved_err = std::move(moved_err_src);
    ASSERT_TRUE(moved_err.is_err());
    EXPECT_EQ(*std::move(moved_err).unwrap_err(), 8);

    // Same Ok/Err types (T == E) via std::expected and std::unexpected
    Result<int, int> same_ok = std::expected<int, int>(std::in_place, 10);
    EXPECT_TRUE(same_ok.is_ok());
    EXPECT_EQ(same_ok.unwrap(), 10);

    Result<int, int> same_err = std::expected<int, int>(std::unexpect, 20);
    EXPECT_TRUE(same_err.is_err());
    EXPECT_EQ(same_err.unwrap_err(), 20);

    Result<int, int> from_unexp = std::unexpected(30);
    EXPECT_TRUE(from_unexp.is_err());
    EXPECT_EQ(from_unexp.unwrap_err(), 30);

    // std::expected<void, G> -> Result<None, E>
    std::expected<void, std::string> void_ok;
    Result<None, std::string> res_void_ok = void_ok;
    EXPECT_TRUE(res_void_ok.is_ok());

    std::expected<void, std::string> void_err = std::unexpected(std::string("bad"));
    Result<None, std::string> res_void_err = std::move(void_err);
    EXPECT_TRUE(res_void_err.is_err());
    EXPECT_EQ(res_void_err.unwrap_err(), "bad");

    // Explicitness check
    struct ExplicitBox
    {
        int v = 0;
        explicit ExplicitBox(int v) : v(v) {}
    };
    static_assert(std::is_constructible_v<Result<ExplicitBox, int>, std::expected<int, int>>);
    static_assert(!std::is_convertible_v<std::expected<int, int>, Result<ExplicitBox, int>>);

    // Nested Result<std::expected<int, int>, int> wraps rather than unwrapping
    Result<std::expected<int, int>, int> nested(std::expected<int, int>(std::unexpect, 5));
    EXPECT_TRUE(nested.is_ok());
    EXPECT_FALSE(nested.unwrap().has_value());
    EXPECT_EQ(nested.unwrap().error(), 5);

    // Reference alternatives reject std::expected conversion and to_std()
    auto can_to_std = []<typename Res>(Res&& r)
    {
        return requires { std::forward<Res>(r).to_std(); };
    };
    int dummy_ref_slot = 0;
    Result<int&, int> ref_ok_res(std::in_place_index<0>, dummy_ref_slot);
    Result<int, int&> ref_err_res(std::in_place_index<1>, dummy_ref_slot);
    static_assert(!std::is_constructible_v<Result<int&, int>, std::expected<int, int>>);
    static_assert(!can_to_std(ref_ok_res));
    static_assert(!can_to_std(ref_err_res));

    // to_std() borrows on lvalue / const rvalue, moves on non-const rvalue
    Result<std::string, int> ok_res = std::string("crab");
    std::expected<std::string, int> exp_lval = ok_res.to_std();
    ASSERT_TRUE(exp_lval.has_value());
    EXPECT_EQ(*exp_lval, "crab");
    EXPECT_EQ(ok_res.unwrap(), "crab");

    const Result<std::string, int> locked_res = std::string("stay");
    std::expected<std::string, int> exp_const_rval = static_cast<const Result<std::string, int>&&>(locked_res).to_std();
    ASSERT_TRUE(exp_const_rval.has_value());
    EXPECT_EQ(*exp_const_rval, "stay");
    EXPECT_EQ(locked_res.unwrap(), "stay");

    std::expected<std::string, int> exp_moved = std::move(ok_res).to_std();
    ASSERT_TRUE(exp_moved.has_value());
    EXPECT_EQ(*exp_moved, "crab");
    EXPECT_TRUE(ok_res.unwrap().empty());

    Result<int, std::string> err_res = std::string("oops");
    std::expected<int, std::string> exp_err_moved = std::move(err_res).to_std();
    ASSERT_FALSE(exp_err_moved.has_value());
    EXPECT_EQ(exp_err_moved.error(), "oops");
    EXPECT_TRUE(err_res.unwrap_err().empty());

    // Move-only to_std() works on rvalue and is disabled on lvalue
    Result<std::unique_ptr<int>, int> unique_res(std::make_unique<int>(55));
    static_assert(!can_to_std(unique_res));
    std::expected<std::unique_ptr<int>, int> unique_exp = std::move(unique_res).to_std();
    ASSERT_TRUE(unique_exp.has_value());
    EXPECT_EQ(**unique_exp, 55);
}

#include "result_matrix.inl"
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

#include "result_matrix.inl"
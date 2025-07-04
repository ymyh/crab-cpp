#include <gtest/gtest.h>

import crab_cpp;

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

TEST(ResultTest, Replace)
{
    {
        auto ok = Result<int, std::string>(42);
        auto opt = ok.replace(84);
        EXPECT_EQ(ok.unwrap(), 84);
        EXPECT_EQ(opt.unwrap(), 42);
    }

    {
        auto err = Result<int, std::string>("error");
        auto opt = err.replace(84);
        EXPECT_EQ(err.unwrap(), 84);
        EXPECT_TRUE(opt.is_none());
    }
}
#include <gtest/gtest.h>

import crab_cpp;

using namespace crab_cpp;

TEST(OptionTest, BasicOperations)
{
    // Test None
    auto none = Option<int>(None{});
    EXPECT_TRUE(none.is_none());
    EXPECT_FALSE(none.is_some());

    // Test Some
    auto some = Option<int>(42);
    EXPECT_FALSE(some.is_none());
    EXPECT_TRUE(some.is_some());
}

TEST(OptionTest, Unwrap)
{
    // Test unwrap on Some
    auto some = Option<int>(42);
    EXPECT_EQ(some.unwrap(), 42);

    // Test unwrap on None (should panic)
    auto none = Option<int>(None{});
    EXPECT_DEATH(none.unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
}

TEST(OptionTest, Map)
{
    // Test map on Some
    auto some = Option<int>(42);
    auto mapped = some.map([](int x) { return x * 2; });
    EXPECT_TRUE(mapped.is_some());
    EXPECT_EQ(mapped.unwrap(), 84);

    // Test map on None
    auto none = Option<int>(None{});
    auto mapped_none = none.map([](int x) { return x * 2; });
    EXPECT_TRUE(mapped_none.is_none());
}

TEST(OptionTest, AndThen)
{
    // Test and_then on Some
    auto some = Option<int>(42);
    auto result = some.and_then([](int x) { return Option<int>(x * 2); });
    EXPECT_TRUE(result.is_some());
    EXPECT_EQ(result.unwrap(), 84);

    // Test and_then on None
    auto none = Option<int>(None{});
    auto result_none = none.and_then([](int x) { return Option<int>(x * 2); });
    EXPECT_TRUE(result_none.is_none());
}

TEST(OptionTest, Take)
{
    // Test take on Some
    auto some = Option<int>(42);
    auto value = some.take();
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(some.is_none());

    // Test take on None (should panic)
    auto none = Option<int>(None{});
    EXPECT_DEATH(none.take(), "Panic encountered: Calling Option<T>::take\\(\\) on a None value");
}

TEST(OptionTest, Replace)
{
    // Test replace on Some
    auto some = Option<int>(42);
    auto old_value = some.replace(100);
    EXPECT_TRUE(old_value.is_some());
    EXPECT_EQ(old_value.unwrap(), 42);
    EXPECT_EQ(some.unwrap(), 100);

    // Test replace on None
    auto none = Option<int>(None{});
    auto old_none = none.replace(100);
    EXPECT_TRUE(old_none.is_none());
    EXPECT_EQ(none.unwrap(), 100);
}

TEST(OptionTest, TakeIf)
{
    // Test take_if on Some with true predicate
    auto some = Option<int>(42);
    auto value = some.take_if([](int x) { return x > 0; });
    EXPECT_TRUE(value.is_some());
    EXPECT_EQ(value.unwrap(), 42);
    EXPECT_TRUE(some.is_none());

    // Test take_if on Some with false predicate
    some = Option<int>(42);
    value = some.take_if([](int x) { return x < 0; });
    EXPECT_TRUE(value.is_none());
    EXPECT_TRUE(some.is_some());
    EXPECT_EQ(some.unwrap(), 42);

    // Test take_if on None
    auto none = Option<int>(None{});
    value = none.take_if([](int x) { return x > 0; });
    EXPECT_TRUE(value.is_none());
    EXPECT_TRUE(none.is_none());
}

TEST(OptionTest, TakeOrDefault)
{
    // Test take_or_default on Some
    auto some = Option<int>(42);
    auto value = some.take_or_default();
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(some.is_none());

    // Test take_or_default on None
    auto none = Option<int>(None{});
    value = none.take_or_default();
    EXPECT_EQ(value, 0); // Default constructed int
    EXPECT_TRUE(none.is_none());
}

TEST(OptionTest, TakeOrElse)
{
    // Test take_or_else on Some
    auto some = Option<int>(42);
    auto value = some.take_or_else([]() { return 100; });
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(some.is_none());

    // Test take_or_else on None
    auto none = Option<int>(None{});
    value = none.take_or_else([]() { return 100; });
    EXPECT_EQ(value, 100);
    EXPECT_TRUE(none.is_none());
}

TEST(OptionTest, Inspect)
{
    int value = 0;

    // Test inspect on Some
    auto some = Option<int>(42);
    some.inspect([&value](int x) { value = x; });
    EXPECT_EQ(value, 42);
    EXPECT_TRUE(some.is_some());

    // Test inspect on None
    value = 0;
    auto none = Option<int>(None{});
    none.inspect([&value](int x) { value = x; });
    EXPECT_EQ(value, 0);
    EXPECT_TRUE(none.is_none());
}

TEST(OptionTest, ArithmeticOperators)
{
    // Test addition
    auto some1 = Option<int>(42);
    auto some2 = Option<int>(58);
    auto none = Option<int>(None{});

    auto sum = some1 + some2;
    EXPECT_TRUE(sum.is_some());
    EXPECT_EQ(sum.unwrap(), 100);

    auto sum_none = some1 + none;
    EXPECT_TRUE(sum_none.is_none());

    // Test multiplication
    auto product = some1 * some2;
    EXPECT_TRUE(product.is_some());
    EXPECT_EQ(product.unwrap(), 42 * 58);

    auto product_none = some1 * none;
    EXPECT_TRUE(product_none.is_none());
}

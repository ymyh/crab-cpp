#include <gtest/gtest.h>

import crab_cpp;

using namespace crab_cpp;

TEST(OptionTest, IsSomeNone)
{
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

    {
        // Test None
        auto none = Option<int*>(None{});
        EXPECT_TRUE(none.is_none());
        EXPECT_FALSE(none.is_some());

        // Test Some
        auto num = 42;
        auto some = Option<int*>(&num);
        EXPECT_FALSE(some.is_none());
        EXPECT_TRUE(some.is_some());
    }

    {
        // Test None
        auto none = Option<int&>(None{});
        EXPECT_TRUE(none.is_none());
        EXPECT_FALSE(none.is_some());

        // Test Some
        int num = 42;
        auto some = Option<int&>(num);
        EXPECT_FALSE(some.is_none());
        EXPECT_TRUE(some.is_some());
    }
}

TEST(OptionTest, Unwrap)
{
    {
        // Test unwrap on Some
        auto some = Option<int>(42);
        EXPECT_EQ(some.unwrap(), 42);

        // Test unwrap on None (should panic)
        auto none = Option<int>(None{});
        EXPECT_DEATH(none.unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
    }

    {
        // Test unwrap on Some
        int num = 42;
        auto some = Option<int*>(&num);
        EXPECT_EQ(some.unwrap(), &num);

        // Test unwrap on None (should panic)
        auto none = Option<int*>(None{});
        EXPECT_DEATH(none.unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
    }

    {
        // Test unwrap on Some
        int num = 42;
        auto some = Option<int&>(num);
        EXPECT_EQ(some.unwrap(), num);

        // Test unwrap on None (should panic)
        auto none = Option<int&>(None{});
        EXPECT_DEATH(none.unwrap(), "Panic encountered: Calling Option<T>::unwrap\\(\\) on a None value");
    }
}

TEST(OptionTest, Map)
{
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

    {
        // Test map on Some with pointer
        int num = 42;
        auto some_ptr = Option<int*>(&num);
        auto mapped_ptr = some_ptr.map([](int* x) { return *x * 2; });
        EXPECT_TRUE(mapped_ptr.is_some());
        EXPECT_EQ(mapped_ptr.unwrap(), 84);

        // Test map on None with pointer
        auto none_ptr = Option<int*>(None{});
        auto mapped_none_ptr = none_ptr.map([](int* x) { return *x * 2; });
        EXPECT_TRUE(mapped_none_ptr.is_none());
    }

    {
        // Test map on Some with reference
        int num = 42;
        auto some_ref = Option<int&>(num);
        auto mapped_ref = some_ref.map([](int x) { return x * 2; });
        EXPECT_TRUE(mapped_ref.is_some());
        EXPECT_EQ(mapped_ref.unwrap(), 84);

        // Test map on None with reference
        auto none_ref = Option<int&>(None{});
        auto mapped_none_ref = none_ref.map([](int x) { return x * 2; });
        EXPECT_TRUE(mapped_none_ref.is_none());
    }
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

    {
        // Test take on Some
        int num = 42;
        auto some = Option<int*>(&num);
        auto value = some.take();
        EXPECT_EQ(value, &num);
        EXPECT_TRUE(some.is_none());

        // Test take on None (should panic)
        auto none = Option<int*>(None{});
        EXPECT_DEATH(none.take(), "Panic encountered: Calling Option<T>::take\\(\\) on a None value");
    }

    {
        // Test take on Some
        int num = 42;
        auto some = Option<int&>(num);
        auto value = some.take();
        EXPECT_EQ(value, num);
        EXPECT_TRUE(some.is_none());

        // Test take on None (should panic)
        auto none = Option<int&>(None{});
    }
}

TEST(OptionTest, Replace)
{
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

    {
        // Test replace on Some with pointer
        int num = 42;
        auto some = Option<int*>(&num);
        int new_num = 100;
        auto old_value = some.replace(&new_num);
        EXPECT_TRUE(old_value.is_some());
        EXPECT_EQ(old_value.unwrap(), &num);
        EXPECT_EQ(some.unwrap(), &new_num);

        // Test replace on None with pointer
        auto none = Option<int*>(None{});
        old_value = none.replace(&new_num);
        EXPECT_TRUE(old_value.is_none());
        EXPECT_EQ(none.unwrap(), &new_num);
    }

    {
        // Test replace on Some with reference
        int num = 42;
        auto some = Option<int&>(num);
        int new_num = 100;
        auto old_value = some.replace(new_num);
        EXPECT_EQ(old_value.unwrap(), 42);
        EXPECT_EQ(some.unwrap(), 100);

        // Test replace on None with reference
        auto none = Option<int&>(None{});
        old_value = none.replace(new_num);
        EXPECT_TRUE(old_value.is_none());
        EXPECT_EQ(none.unwrap(), 100);
    }
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

    {
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

    value = 0;
    {
        // Test inspect on Some
        int num = 42;
        auto some = Option<int*>(&num);
        some.inspect([&value](int* x) { value = *x; });
        EXPECT_EQ(value, 42);
        EXPECT_TRUE(some.is_some());

        // Test inspect on None
        value = 0;
        auto none = Option<int*>(None{});
        none.inspect([&value](int* x) { value = *x; });
        EXPECT_EQ(value, 0);
        EXPECT_TRUE(none.is_none());
    }

    value = 0;
    {
        // Test inspect on Some
        int num = 42;
        auto some = Option<int&>(num);
        some.inspect([&value](int x) { value = x; });
        EXPECT_EQ(value, 42);
        EXPECT_TRUE(some.is_some());

        // Test inspect on None
        value = 0;
        auto none = Option<int&>(None{});
        none.inspect([&value](int x) { value = x; });
        EXPECT_EQ(value, 0);
        EXPECT_TRUE(none.is_none());
    }
}

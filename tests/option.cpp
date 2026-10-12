#include <gtest/gtest.h>

import std;
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
        EXPECT_DEATH(none.take(), "Panic encountered: Calling Option<T>::take\\(\\) on a None value");
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

TEST(OptionTest, And)
{
    {
        // Test and_ on Some
        auto some = Option<int>(42);
        auto other = Option<double>(3.14);
        auto kept = some.and_(other);
        EXPECT_TRUE(kept.is_some());
        EXPECT_DOUBLE_EQ(kept.unwrap(), 3.14);

        // Test and_ on None
        auto none = Option<int>(None{});
        auto result = none.and_(other);
        EXPECT_TRUE(result.is_none());
    }

    {
        // Test and_ on Some with pointer
        int num = 42;
        double pi = 3.14;
        auto some = Option<int*>(&num);
        auto other = Option<double*>(&pi);
        auto kept = some.and_(other);
        EXPECT_TRUE(kept.is_some());
        EXPECT_EQ(kept.unwrap(), &pi);

        // Test and_ on None with pointer
        auto none = Option<int*>(None{});
        auto result = none.and_(other);
        EXPECT_TRUE(result.is_none());
    }

    {
        // Test and_ on Some with reference
        int num = 42;
        double pi = 3.14;
        auto some = Option<int&>(num);
        auto other = Option<double&>(pi);
        auto kept = some.and_(other);
        EXPECT_TRUE(kept.is_some());
        EXPECT_DOUBLE_EQ(kept.unwrap(), 3.14);

        // Test and_ on None with reference
        auto none = Option<int&>(None{});
        auto result = none.and_(other);
        EXPECT_TRUE(result.is_none());
    }
}

TEST(OptionTest, Expect)
{
    {
        // Test expect on Some
        auto some = Option<int>(42);
        EXPECT_EQ(some.expect("should not panic"), 42);

        // Test expect on None (should panic)
        auto none = Option<int>(None{});
        EXPECT_DEATH(none.expect("custom message"), "Panic encountered: custom message");
    }

    {
        // Test expect on Some with pointer
        int num = 42;
        auto some = Option<int*>(&num);
        EXPECT_EQ(some.expect("should not panic"), &num);

        // Test expect on None with pointer (should panic)
        auto none = Option<int*>(None{});
        EXPECT_DEATH(none.expect("custom message"), "Panic encountered: custom message");
    }

    {
        // Test expect on Some with reference
        int num = 42;
        auto some = Option<int&>(num);
        EXPECT_EQ(some.expect("should not panic"), 42);

        // Test expect on None with reference (should panic)
        auto none = Option<int&>(None{});
        EXPECT_DEATH(none.expect("custom message"), "Panic encountered: custom message");
    }
}

TEST(OptionTest, ExpectTake)
{
    {
        // Test expect_take on Some
        auto some = Option<int>(42);
        EXPECT_EQ(some.expect_take("should not panic"), 42);
        EXPECT_TRUE(some.is_none());

        // Test expect_take on None (should panic)
        auto none = Option<int>(None{});
        EXPECT_DEATH(none.expect_take("custom message"), "Panic encountered: custom message");
    }

    {
        // Test expect_take on Some with pointer
        int num = 42;
        auto some = Option<int*>(&num);
        EXPECT_EQ(some.expect_take("should not panic"), &num);
        EXPECT_TRUE(some.is_none());

        // Test expect_take on None with pointer (should panic)
        auto none = Option<int*>(None{});
        EXPECT_DEATH(none.expect_take("custom message"), "Panic encountered: custom message");
    }

    {
        // Test expect_take on Some with reference
        int num = 42;
        auto some = Option<int&>(num);
        EXPECT_EQ(some.expect_take("should not panic"), 42);
        EXPECT_TRUE(some.is_none());

        // Test expect_take on None with reference (should panic)
        auto none = Option<int&>(None{});
        EXPECT_DEATH(none.expect_take("custom message"), "Panic encountered: custom message");
    }
}

TEST(OptionTest, MapOrElse)
{
    // Test map_or_else on Some
    auto some = Option<int>(42);
    auto mapped = some.map_or_else([]() { return 7; }, [](int x) { return x * 2; });
    EXPECT_EQ(mapped, 84);

    // Test map_or_else on None
    auto none = Option<int>(None{});
    auto defaulted = none.map_or_else([]() { return 7; }, [](int x) { return x * 2; });
    EXPECT_EQ(defaulted, 7);
}

TEST(OptionTest, Or)
{
    {
        // Test or_ on Some
        auto some = Option<int>(42);
        auto other = Option<int>(7);
        EXPECT_EQ(some.or_(other).unwrap(), 42);

        // Test or_ on None
        auto none = Option<int>(None{});
        EXPECT_EQ(none.or_(other).unwrap(), 7);
    }

    {
        // Test or_ on Some with pointer
        int a = 1;
        int b = 2;
        auto some = Option<int*>(&a);
        auto other = Option<int*>(&b);
        EXPECT_EQ(some.or_(other).unwrap(), &a);

        // Test or_ on None with pointer
        auto none = Option<int*>(None{});
        EXPECT_EQ(none.or_(other).unwrap(), &b);
    }

    {
        // Test or_ on Some with reference
        int a = 1;
        int b = 2;
        auto some = Option<int&>(a);
        auto other = Option<int&>(b);
        EXPECT_EQ(some.or_(other).unwrap(), 1);

        // Test or_ on None with reference
        auto none = Option<int&>(None{});
        EXPECT_EQ(none.or_(other).unwrap(), 2);
    }
}

TEST(OptionTest, UnwrapUnchecked)
{
    {
        // Unchecked access skips the None check entirely.
        auto some = Option<int>(42);
        EXPECT_EQ(some.unwrap_unchecked(), 42);
    }

    {
        int num = 42;
        auto some = Option<int*>(&num);
        EXPECT_EQ(some.unwrap_unchecked(), &num);
    }

    {
        int num = 42;
        auto some = Option<int&>(num);
        EXPECT_EQ(some.unwrap_unchecked(), 42);
    }
}

TEST(OptionTest, Unzip)
{
    {
        // Some((a, b)) -> (Some(a), Some(b))
        auto some = Option<std::pair<int, std::string>>{ std::pair<int, std::string>{ 42, "hello" } };

        auto unzipped = some.unzip();
        static_assert(std::is_same_v<decltype(unzipped), std::tuple<Option<int>, Option<std::string>>>);

        EXPECT_TRUE(std::get<0>(unzipped).is_some());
        EXPECT_EQ(std::get<0>(unzipped).unwrap(), 42);
        EXPECT_TRUE(std::get<1>(unzipped).is_some());
        EXPECT_EQ(std::get<1>(unzipped).unwrap(), "hello");

        // The source option is emptied.
        EXPECT_TRUE(some.is_none());
    }

    {
        // None -> (None, None)
        auto none = Option<std::tuple<int, double>>{ None{} };

        auto unzipped = none.unzip();
        EXPECT_TRUE(std::get<0>(unzipped).is_none());
        EXPECT_TRUE(std::get<1>(unzipped).is_none());
    }

    {
        // Reference elements are preserved by reference.
        int a = 1;
        double b = 2.5;
        auto some = Option<std::pair<int&, double&>>{ std::pair<int&, double&>{ a, b } };

        auto unzipped = some.unzip();
        EXPECT_EQ(std::get<0>(unzipped).unwrap(), 1);
        EXPECT_DOUBLE_EQ(std::get<1>(unzipped).unwrap(), 2.5);

        // Mutating through an unzipped reference affects the original object.
        std::get<0>(unzipped).unwrap() = 10;
        EXPECT_EQ(a, 10);
    }
}

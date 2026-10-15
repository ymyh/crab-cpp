#include <gtest/gtest.h>

import std;
import crab_cpp;

#include "probe_types.h"

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

        // Lvalue unzip copies the elements and leaves the source in place.
        EXPECT_TRUE(some.is_some());
        EXPECT_EQ(some.unwrap().first, 42);
        EXPECT_EQ(some.unwrap().second, "hello");
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

TEST(OptionTest, UnwrapOr)
{
    {
        auto some = Option<int>(42);
        EXPECT_EQ(some.unwrap_or(7), 42);
        EXPECT_EQ(some.unwrap(), 42);

        auto none = Option<int>(None{});
        EXPECT_EQ(none.unwrap_or(7), 7);
        EXPECT_TRUE(none.is_none());
    }

    {
        auto some = Option<std::string>("hello");
        auto taken = std::move(some).unwrap_or(std::string("default"));
        EXPECT_EQ(taken, "hello");
        EXPECT_TRUE(some.is_none());

        auto none = Option<std::string>(None{});
        EXPECT_EQ(std::move(none).unwrap_or(std::string("default")), "default");
        EXPECT_TRUE(none.is_none());
    }

    {
        auto some = Option<int>(42);
        EXPECT_EQ(some.unwrap_or_else([]() { return 7; }), 42);
        EXPECT_TRUE(some.is_some());

        auto none = Option<int>(None{});
        EXPECT_EQ(none.unwrap_or_else([]() { return 7; }), 7);
    }

    {
        auto some = Option<int>(42);
        EXPECT_EQ(some.unwrap_or_default(), 42);
        EXPECT_TRUE(some.is_some());

        auto none = Option<int>(None{});
        EXPECT_EQ(none.unwrap_or_default(), 0);
        EXPECT_TRUE(none.is_none());
    }

    {
        auto some = Option<std::unique_ptr<int>>(std::make_unique<int>(1));
        auto taken = std::move(some).unwrap_or(std::unique_ptr<int>{});
        EXPECT_TRUE(some.is_none());
        ASSERT_TRUE(taken != nullptr);
        EXPECT_EQ(*taken, 1);

        auto none = Option<std::unique_ptr<int>>(None{});
        auto fallback = std::move(none).unwrap_or_else([]() { return std::make_unique<int>(3); });
        EXPECT_TRUE(none.is_none());
        ASSERT_TRUE(fallback != nullptr);
        EXPECT_EQ(*fallback, 3);
    }
}

TEST(OptionTest, OkOr)
{
    {
        auto some = Option<int>(42);
        auto ok = some.ok_or(std::string("missing"));
        EXPECT_TRUE(ok.is_ok());
        EXPECT_EQ(ok.unwrap(), 42);
        EXPECT_EQ(some.unwrap(), 42);

        auto none = Option<int>(None{});
        auto err = none.ok_or(std::string("missing"));
        EXPECT_TRUE(err.is_err());
        EXPECT_EQ(err.unwrap_err(), "missing");
        EXPECT_TRUE(none.is_none());
    }

    {
        auto some = Option<std::string>("hello");
        auto ok = std::move(some).ok_or(std::string("missing"));
        EXPECT_EQ(ok.unwrap(), "hello");
        EXPECT_TRUE(some.is_none());
    }

    {
        auto some = Option<int>(42);
        auto ok = some.ok_or_else([]() { return std::string("missing"); });
        EXPECT_TRUE(ok.is_ok());
        EXPECT_EQ(ok.unwrap(), 42);

        auto none = Option<int>(None{});
        auto err = none.ok_or_else([]() { return std::string("missing"); });
        EXPECT_TRUE(err.is_err());
        EXPECT_EQ(err.unwrap_err(), "missing");
    }

    {
        // Same Ok and Err type must still pick the right alternative.
        auto some = Option<int>(1);
        auto ok = some.ok_or(2);
        EXPECT_TRUE(ok.is_ok());
        EXPECT_EQ(ok.unwrap(), 1);

        auto none = Option<int>(None{});
        auto err = none.ok_or(2);
        EXPECT_TRUE(err.is_err());
        EXPECT_EQ(err.unwrap_err(), 2);
    }
}

TEST(OptionTest, Filter)
{
    {
        auto some = Option<int>(42);
        auto kept = some.filter([](int x) { return x > 0; });
        EXPECT_EQ(kept.unwrap(), 42);
        EXPECT_EQ(some.unwrap(), 42);

        auto dropped = some.filter([](int x) { return x < 0; });
        EXPECT_TRUE(dropped.is_none());
        EXPECT_EQ(some.unwrap(), 42);

        auto none = Option<int>(None{});
        EXPECT_TRUE(none.filter([](int x) { return x > 0; }).is_none());
        EXPECT_TRUE(none.is_none());
    }

    {
        auto some = Option<std::string>("hello");
        auto kept = std::move(some).filter([](const std::string& s) { return s == "hello"; });
        EXPECT_EQ(kept.unwrap(), "hello");
        EXPECT_TRUE(some.is_none());

        auto other = Option<std::string>("hello");
        auto dropped = std::move(other).filter([](const std::string& s) { return s.empty(); });
        EXPECT_TRUE(dropped.is_none());
        EXPECT_TRUE(other.is_none());
    }
}

TEST(OptionTest, IsSomeAnd)
{
    auto some = Option<int>(42);
    EXPECT_TRUE(some.is_some_and([](int x) { return x == 42; }));
    EXPECT_FALSE(some.is_some_and([](int x) { return x < 0; }));
    EXPECT_EQ(some.unwrap(), 42);

    const auto const_some = Option<int>(42);
    EXPECT_TRUE(const_some.is_some_and([](const int& x) { return x == 42; }));

    auto none = Option<int>(None{});
    EXPECT_FALSE(none.is_some_and([](int) { return true; }));
}

TEST(OptionTest, Contains)
{
    auto some = Option<int>(42);
    EXPECT_TRUE(some.contains(42));
    EXPECT_FALSE(some.contains(7));

    auto none = Option<int>(None{});
    EXPECT_FALSE(none.contains(0));
}

TEST(OptionTest, RvalueCombinatorsResetToNone)
{
    {
        auto some = Option<std::string>("hello");
        auto mapped = some.map([](const std::string& s) { return static_cast<int>(s.size()); });
        EXPECT_EQ(mapped.unwrap(), 5);
        EXPECT_EQ(some.unwrap(), "hello");

        auto consumed = std::move(some).map([](std::string s) { return static_cast<int>(s.size()); });
        EXPECT_EQ(consumed.unwrap(), 5);
        EXPECT_TRUE(some.is_none());

        auto borrowed = Option<std::string>("hello");
        auto consumed_ref = std::move(borrowed).map([](const std::string& s) { return static_cast<int>(s.size()); });
        EXPECT_EQ(consumed_ref.unwrap(), 5);
        EXPECT_TRUE(borrowed.is_none());
    }

    {
        auto some = Option<std::string>("ab");
        auto next = std::move(some).and_then([](std::string s) { return Option<int>(static_cast<int>(s.size())); });
        EXPECT_EQ(next.unwrap(), 2);
        EXPECT_TRUE(some.is_none());

        auto kept = Option<std::string>("ab");
        auto copied = kept.and_then([](const std::string& s) { return Option<int>(static_cast<int>(s.size())); });
        EXPECT_EQ(copied.unwrap(), 2);
        EXPECT_EQ(kept.unwrap(), "ab");
    }

    {
        auto some = Option<std::string>("hi");
        EXPECT_EQ(std::move(some).map_or(0, [](std::string s) { return static_cast<int>(s.size()); }), 2);
        EXPECT_TRUE(some.is_none());

        auto kept = Option<std::string>("hi");
        EXPECT_EQ(kept.map_or(0, [](const std::string& s) { return static_cast<int>(s.size()); }), 2);
        EXPECT_EQ(kept.unwrap(), "hi");
    }

    {
        auto some = Option<std::string>("hi");
        auto n = std::move(some).map_or_else([]() { return 0; }, [](std::string s) { return static_cast<int>(s.size()); });
        EXPECT_EQ(n, 2);
        EXPECT_TRUE(some.is_none());
    }

    {
        auto some = Option<std::string>("hi");
        auto next = std::move(some).or_else([]() { return Option<std::string>("no"); });
        EXPECT_EQ(next.unwrap(), "hi");
        EXPECT_TRUE(some.is_none());

        auto kept = Option<std::string>("hi");
        auto copied = kept.or_else([]() { return Option<std::string>("no"); });
        EXPECT_EQ(copied.unwrap(), "hi");
        EXPECT_EQ(kept.unwrap(), "hi");
    }

    {
        auto some = Option<std::string>("hi");
        auto next = std::move(some).and_(Option<int>(7));
        EXPECT_EQ(next.unwrap(), 7);
        EXPECT_TRUE(some.is_none());

        auto kept = Option<std::string>("hi");
        auto copied = kept.and_(Option<int>(7));
        EXPECT_EQ(copied.unwrap(), 7);
        EXPECT_EQ(kept.unwrap(), "hi");
    }

    {
        auto none = Option<std::string>(None{});
        EXPECT_TRUE(std::move(none).map([](std::string s) { return static_cast<int>(s.size()); }).is_none());
        EXPECT_TRUE(none.is_none());
    }
}

TEST(OptionTest, AndValueCategory)
{
    auto some = Option<std::string>("keep");
    auto other = Option<int>(7);
    auto copied = some.and_(other);
    EXPECT_EQ(copied.unwrap(), 7);
    EXPECT_EQ(some.unwrap(), "keep");
    EXPECT_EQ(other.unwrap(), 7);

    auto consumed = std::move(some).and_(std::move(other));
    EXPECT_EQ(consumed.unwrap(), 7);
    EXPECT_TRUE(some.is_none());
    EXPECT_TRUE(other.is_none());

    auto none = Option<std::string>(None{});
    auto dropped = Option<std::unique_ptr<int>>(std::make_unique<int>(1));
    EXPECT_TRUE(none.and_(std::move(dropped)).is_none());
    EXPECT_TRUE(dropped.is_none());

    // The returned operand is copied before the rvalue operand is reset.
    auto aliased = Option<std::string>("keep");
    auto from_alias = std::move(aliased).and_(aliased);
    EXPECT_EQ(from_alias.unwrap(), "keep");
    EXPECT_TRUE(aliased.is_none());
}

TEST(OptionTest, OrValueCategory)
{
    auto some = Option<std::string>("keep");
    auto other = Option<std::string>("drop");
    auto copied = some.or_(other);
    EXPECT_EQ(copied.unwrap(), "keep");
    EXPECT_EQ(some.unwrap(), "keep");
    EXPECT_EQ(other.unwrap(), "drop");

    auto dropped = Option<std::string>("drop");
    auto kept = some.or_(std::move(dropped));
    EXPECT_EQ(kept.unwrap(), "keep");
    EXPECT_EQ(some.unwrap(), "keep");
    EXPECT_TRUE(dropped.is_none());

    auto none = Option<std::unique_ptr<int>>(None{});
    auto fallback = Option<std::unique_ptr<int>>(std::make_unique<int>(4));
    auto taken = std::move(none).or_(std::move(fallback));
    EXPECT_TRUE(none.is_none());
    EXPECT_TRUE(fallback.is_none());
    EXPECT_EQ(*std::move(taken).unwrap(), 4);

    // Capturing self before discarding the rvalue keeps the value when the two names alias.
    auto aliased = Option<std::string>("keep");
    auto from_alias = aliased.or_(std::move(aliased));
    EXPECT_EQ(from_alias.unwrap(), "keep");
    EXPECT_TRUE(aliased.is_none());
}

TEST(OptionTest, Xor)
{
    {
        auto lhs = Option<int>(1);
        auto rhs = Option<int>(2);
        EXPECT_TRUE(lhs.xor_(rhs).is_none());
        EXPECT_EQ(lhs.unwrap(), 1);
        EXPECT_EQ(rhs.unwrap(), 2);

        auto none = Option<int>(None{});
        EXPECT_EQ(lhs.xor_(none).unwrap(), 1);
        EXPECT_EQ(none.xor_(rhs).unwrap(), 2);
        EXPECT_TRUE(none.xor_(Option<int>(None{})).is_none());

        auto consumed_lhs = Option<std::string>("left");
        auto consumed_rhs = Option<std::string>("right");
        EXPECT_TRUE(std::move(consumed_lhs).xor_(std::move(consumed_rhs)).is_none());
        EXPECT_TRUE(consumed_lhs.is_none());
        EXPECT_TRUE(consumed_rhs.is_none());

        auto only = Option<std::string>("only");
        auto empty = Option<std::string>(None{});
        auto result = std::move(only).xor_(std::move(empty));
        EXPECT_EQ(result.unwrap(), "only");
        EXPECT_TRUE(only.is_none());
        EXPECT_TRUE(empty.is_none());
    }

    {
        int a = 1;
        int b = 2;
        auto lhs = Option<int*>(&a);
        auto rhs = Option<int*>(&b);
        EXPECT_TRUE(lhs.xor_(rhs).is_none());
        EXPECT_EQ(lhs.xor_(Option<int*>(None{})).unwrap(), &a);

        auto consumed = std::move(lhs).xor_(Option<int*>(None{}));
        EXPECT_EQ(consumed.unwrap(), &a);
        EXPECT_TRUE(lhs.is_none());
    }

    {
        int a = 1;
        int b = 2;
        auto lhs = Option<int&>(a);
        auto rhs = Option<int&>(b);
        EXPECT_TRUE(lhs.xor_(rhs).is_none());
        EXPECT_EQ(lhs.xor_(Option<int&>(None{})).unwrap(), 1);

        auto consumed = std::move(lhs).xor_(Option<int&>(None{}));
        EXPECT_EQ(consumed.unwrap(), 1);
        EXPECT_TRUE(lhs.is_none());
    }
}

TEST(OptionTest, Zip)
{
    {
        auto lhs = Option<int>(1);
        auto rhs = Option<std::string>("a");
        auto zipped = lhs.zip(rhs);
        ASSERT_TRUE(zipped.is_some());
        EXPECT_EQ(std::get<0>(zipped.unwrap()), 1);
        EXPECT_EQ(std::get<1>(zipped.unwrap()), "a");
        EXPECT_EQ(lhs.unwrap(), 1);
        EXPECT_EQ(rhs.unwrap(), "a");

        auto consumed = std::move(lhs).zip(std::move(rhs));
        ASSERT_TRUE(consumed.is_some());
        auto pair = std::move(consumed).unwrap();
        EXPECT_EQ(std::get<0>(pair), 1);
        EXPECT_EQ(std::get<1>(pair), "a");
        EXPECT_TRUE(lhs.is_none());
        EXPECT_TRUE(rhs.is_none());

        auto none = Option<int>(None{});
        auto still = Option<int>(3);
        EXPECT_TRUE(none.zip(still).is_none());
        EXPECT_TRUE(still.is_some());
        EXPECT_TRUE(std::move(still).zip(none).is_none());
        EXPECT_TRUE(still.is_none());

        const auto locked = Option<std::string>("hi");
        auto from_const_rvalue = static_cast<const Option<std::string>&&>(locked).zip(Option<int>(1));
        ASSERT_TRUE(from_const_rvalue.is_some());
        EXPECT_EQ(std::get<0>(from_const_rvalue.unwrap()), "hi");
        EXPECT_EQ(locked.unwrap(), "hi");
    }

    {
        auto lhs = Option<std::unique_ptr<int>>(std::make_unique<int>(1));
        auto rhs = Option<std::unique_ptr<int>>(std::make_unique<int>(2));
        auto zipped = std::move(lhs).zip(std::move(rhs));
        ASSERT_TRUE(zipped.is_some());
        auto pair = std::move(zipped).unwrap();
        EXPECT_EQ(*std::get<0>(pair), 1);
        EXPECT_EQ(*std::get<1>(pair), 2);
        EXPECT_TRUE(lhs.is_none());
        EXPECT_TRUE(rhs.is_none());
    }

    {
        int a = 1;
        int b = 2;
        auto lhs = Option<int*>(&a);
        auto rhs = Option<int*>(&b);
        auto zipped = std::move(lhs).zip(rhs);
        ASSERT_TRUE(zipped.is_some());
        EXPECT_EQ(std::get<0>(zipped.unwrap()), &a);
        EXPECT_EQ(std::get<1>(zipped.unwrap()), &b);
        EXPECT_TRUE(lhs.is_none());
        EXPECT_TRUE(rhs.is_some());
    }

    {
        int a = 1;
        int b = 2;
        auto lhs = Option<int&>(a);
        auto rhs = Option<int&>(b);
        auto zipped = lhs.zip(rhs);
        ASSERT_TRUE(zipped.is_some());
        std::get<0>(zipped.as_mut().unwrap()) = 9;
        EXPECT_EQ(a, 9);
        EXPECT_EQ(b, 2);
        EXPECT_TRUE(lhs.is_some());
        EXPECT_TRUE(rhs.is_some());
    }
}

TEST(OptionTest, Insert)
{
    auto empty = Option<std::string>(None{});
    auto& inserted = empty.insert("hello");
    EXPECT_EQ(inserted, "hello");
    inserted = "edited";
    EXPECT_EQ(empty.unwrap(), "edited");

    EXPECT_EQ(empty.insert(std::string("next")), "next");

    auto owned = Option<std::unique_ptr<int>>(None{});
    auto& pointer = owned.insert(std::make_unique<int>(3));
    EXPECT_EQ(*pointer, 3);

    auto present = Option<std::unique_ptr<int>>(std::make_unique<int>(1));
    auto extra = std::make_unique<int>(2);
    auto& kept = present.get_or_insert(std::move(extra));
    EXPECT_EQ(*kept, 1);
    EXPECT_NE(extra, nullptr);

    auto missing = Option<std::unique_ptr<int>>(None{});
    auto& filled = missing.get_or_insert(std::move(extra));
    EXPECT_EQ(*filled, 2);
    EXPECT_EQ(extra, nullptr);

    auto number = Option<int>(None{});
    auto& def = number.get_or_insert_default();
    EXPECT_EQ(def, 0);
    def = 6;
    EXPECT_EQ(number.get_or_insert_default(), 6);

    auto calls = 0;
    auto ready = Option<int>(4);
    EXPECT_EQ(ready.get_or_insert_with([&] { ++calls; return 9; }), 4);
    EXPECT_EQ(calls, 0);

    auto text = Option<std::string>(None{});
    auto& produced = text.get_or_insert_with([] { return "hi"; });
    EXPECT_EQ(produced, "hi");
    EXPECT_EQ(calls, 0);
}

#include "option_matrix.inl"

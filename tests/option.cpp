#include <gtest/gtest.h>

import crab_cpp;

using namespace crab_cpp;

TEST(OptionTest, IsNone)
{
    Option<int> noneOption = None{};
    EXPECT_TRUE(noneOption.is_none());
    EXPECT_FALSE(noneOption.is_some());
}

TEST(OptionTest, IsSome)
{
    Option<int> someOption = 42;
    EXPECT_TRUE(someOption.is_some());
    EXPECT_FALSE(someOption.is_none());
}

TEST(OptionTest, Unwrap)
{
    Option<int> someOption = 42;
    EXPECT_EQ(someOption.unwrap(), 42);
}

TEST(OptionTest, Map)
{
    Option<int> someOption = 42;
    auto mappedOption = someOption.map([](int value) { return value * 2; });
    EXPECT_TRUE(mappedOption.is_some());
    EXPECT_EQ(mappedOption.unwrap(), 84);

    Option<int> noneOption = None{};
    auto mappedNone = noneOption.map([](int value) { return value * 2; });
    EXPECT_TRUE(mappedNone.is_none());
}

TEST(OptionTest, Take)
{
    Option<int> someOption = 42;
    EXPECT_EQ(someOption.take(), 42);
    EXPECT_TRUE(someOption.is_none());
}

TEST(OptionTest, Replace)
{
    Option<int> someOption = 42;
    auto oldValue = someOption.replace(100);
    EXPECT_EQ(oldValue.unwrap(), 42);
    EXPECT_EQ(someOption.unwrap(), 100);
}

TEST(OptionTest, AndThen)
{
    Option<int> someOption = 42;
    auto result = someOption.and_then([](int value) { return Option<int>(value * 2); });
    EXPECT_TRUE(result.is_some());
    EXPECT_EQ(result.unwrap(), 84);

    Option<int> noneOption = None{};
    auto noneResult = noneOption.and_then([](int value) { return Option<int>(value * 2); });
    EXPECT_TRUE(noneResult.is_none());
}


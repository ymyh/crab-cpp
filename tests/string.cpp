#include <gtest/gtest.h>

import crab_cpp;

using namespace crab_cpp;

#include <gtest/gtest.h>

import crab_cpp;

using namespace crab_cpp;

TEST(StringTest, DefaultConstructor)
{
    String s;
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.capacity(), 0);
    EXPECT_TRUE(s.empty());
}

TEST(StringTest, FromRawParts)
{
    // Test empty string
    auto result = String::from_raw_parts(nullptr, 0);
    EXPECT_TRUE(result.is_ok());
    auto s = result.unwrap();
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.capacity(), 0);

    // Test valid UTF-8 string
    result = String::from_raw_parts("hello", 5);
    EXPECT_TRUE(result.is_ok());
    s = result.unwrap();
    EXPECT_EQ(s.size(), 5);
    EXPECT_EQ(s.capacity(), 5);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello");

    // Test invalid UTF-8 string
    const char invalid_utf8[] = {'\xC0', '\x80'}; // Invalid UTF-8 sequence
    result = String::from_raw_parts(invalid_utf8, 2);
    EXPECT_TRUE(result.is_err());
}

TEST(StringTest, From)
{
    // Test empty string
    auto result = String::from("");
    EXPECT_TRUE(result.is_ok());
    auto s = result.unwrap();
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.capacity(), 0);

    // Test valid UTF-8 string
    result = String::from("hello");
    EXPECT_TRUE(result.is_ok());
    s = result.unwrap();
    EXPECT_EQ(s.size(), 5);
    EXPECT_EQ(s.capacity(), 5);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello");

    // Test invalid UTF-8 string
    const char invalid_utf8[] = {'\xC0', '\x80', '\0'}; // Invalid UTF-8 sequence
    result = String::from(invalid_utf8);
    EXPECT_TRUE(result.is_err());
}

TEST(StringTest, CopyConstructor)
{
    String s1 = String::from("hello").unwrap();
    String s2(s1);
    EXPECT_EQ(s1.size(), s2.size());
    EXPECT_EQ(s1.capacity(), s2.capacity());
    EXPECT_EQ(std::string_view(s1.as_raw(), s1.size()), std::string_view(s2.as_raw(), s2.size()));
}

TEST(StringTest, MoveConstructor)
{
    String s1 = String::from("hello").unwrap();
    String s2(std::move(s1));
    EXPECT_EQ(s2.size(), 5);
    EXPECT_EQ(s2.capacity(), 5);
    EXPECT_EQ(std::string_view(s2.as_raw(), s2.size()), "hello");
    EXPECT_EQ(s1.size(), 0);
    EXPECT_EQ(s1.capacity(), 0);
}

TEST(StringTest, Reserve)
{
    String s;
    s.reserve(10);
    EXPECT_GE(s.capacity(), 10);
    EXPECT_EQ(s.size(), 0);

    s = String::from("hello").unwrap();
    s.reserve(20);
    EXPECT_GE(s.capacity(), 20);
    EXPECT_EQ(s.size(), 5);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello");
}

TEST(StringTest, Clear)
{
    String s = String::from("hello").unwrap();
    s.clear();
    EXPECT_EQ(s.size(), 0);
    EXPECT_GE(s.capacity(), 5); // Capacity should remain unchanged
}

TEST(StringTest, Push)
{
    String s;
    s.push(Char('h'));
    s.push(Char('e'));
    s.push(Char('l'));
    s.push(Char('l'));
    s.push(Char('o'));
    EXPECT_EQ(s.size(), 5);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello");
}

TEST(StringTest, PushStr)
{
    using namespace literal;

    String s;
    s.push_str("hello"_s);
    EXPECT_EQ(s.size(), 5);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello");

    s.push_str(" world"_s);
    EXPECT_EQ(s.size(), 11);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello world");
}

TEST(StringTest, SplitOff)
{
    String s = String::from("hello world").unwrap();
    String s2 = s.split_off(6);
    EXPECT_EQ(s.size(), 6);
    EXPECT_EQ(s2.size(), 5);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello ");
    EXPECT_EQ(std::string_view(s2.as_raw(), s2.size()), "world");

    // Test invalid split position
    EXPECT_DEATH(s.split_off(10), ".*");
}

TEST(StringTest, Truncate)
{
    using namespace literal;

    String s = String::from("hello world").unwrap();
    s.truncate(5);
    EXPECT_EQ(s.size(), 5);
    EXPECT_EQ(s, "hello"_s);

    s.truncate(10);
    EXPECT_EQ(s.size(), 5);

    s = "你好，世界"_s;
    EXPECT_DEATH(s.truncate(2), ".*");
}

TEST(StringTest, Pop)
{
    String s = String::from("hello").unwrap();
    auto ch = s.pop();
    EXPECT_TRUE(ch.is_some());
    EXPECT_EQ(ch.unwrap(), Char('o'));
    EXPECT_EQ(s.size(), 4);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hell");

    ch = s.pop();
    EXPECT_TRUE(ch.is_some());
    EXPECT_EQ(ch.unwrap(), Char('l'));
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hel");

    s.clear();
    ch = s.pop();
    EXPECT_TRUE(ch.is_none());
}

TEST(StringTest, MakeAsciiLowercase)
{
    String s = String::from("HELLO").unwrap();
    s.make_ascii_lowercase();
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello");

    // Test non-ASCII characters
    s = String::from("HÉLLO").unwrap();
    s.make_ascii_lowercase();
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hÉllo");
}

TEST(StringTest, MakeAsciiUppercase)
{
    String s = String::from("hello").unwrap();
    s.make_ascii_uppercase();
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "HELLO");

    // Test non-ASCII characters
    s = String::from("héllo").unwrap();
    s.make_ascii_uppercase();
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "HéLLO");
}

TEST(StringTest, AssignmentOperator)
{
    String s1 = String::from("hello").unwrap();
    String s2;
    s2 = s1;
    EXPECT_EQ(s1.size(), s2.size());
    EXPECT_EQ(s1.capacity(), s2.capacity());
    EXPECT_EQ(std::string_view(s1.as_raw(), s1.size()), std::string_view(s2.as_raw(), s2.size()));
}

TEST(StringTest, PlusEqualsOperator)
{
    using namespace literal;

    String s = String::from("hello").unwrap();
    s += " world"_s;
    EXPECT_EQ(s.size(), 11);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello world");

    s += Char('!');
    EXPECT_EQ(s.size(), 12);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello world!");

    String s2 = String::from(" hello").unwrap();
    s += s2;
    EXPECT_EQ(s.size(), 18);
    EXPECT_EQ(std::string_view(s.as_raw(), s.size()), "hello world! hello");
}

TEST(StringTest, PlusOperator)
{
    using namespace literal;

    String s1 = String::from("hello").unwrap();
    String s2 = String::from(" world").unwrap();
    String s3 = s1 + s2;
    EXPECT_EQ(s3.size(), 11);
    EXPECT_EQ(std::string_view(s3.as_raw(), s3.size()), "hello world");

    String s4 = s1 + " world"_s;
    EXPECT_EQ(s4.size(), 11);
    EXPECT_EQ(std::string_view(s4.as_raw(), s4.size()), "hello world");

    String s5 = s1 + Char('!');
    EXPECT_EQ(s5.size(), 6);
    EXPECT_EQ(std::string_view(s5.as_raw(), s5.size()), "hello!");
}

TEST(StringTest, ComparisonOperators)
{
    String s1 = String::from("hello").unwrap();
    String s2 = String::from("world").unwrap();
    String s3 = String::from("hello").unwrap();

    EXPECT_TRUE(s1 == s3);
    EXPECT_FALSE(s1 == s2);
    EXPECT_TRUE(s1 < s2);
    EXPECT_FALSE(s2 < s1);
    EXPECT_TRUE(s1 <= s3);
    EXPECT_TRUE(s1 >= s3);
    EXPECT_TRUE(s1 > String());
    EXPECT_TRUE(String() < s1);
}

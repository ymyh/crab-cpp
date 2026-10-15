#ifdef CRAB_CPP_ENABLE_STRING

#include <gtest/gtest.h>
#include <ranges>

import crab_cpp;
import std;

using namespace crab_cpp;

namespace
{
    /**
     * @brief Allocator that fills every allocation with 0xAB, so that a missing
     *        null terminator (or a size mismatch) is caught deterministically
     *        instead of reading uninitialized heap memory
     */
    struct FillingAllocator
    {
        using value_type = std::byte;

        [[nodiscard]] auto allocate(std::size_t n) -> std::byte*
        {
            auto* data = static_cast<std::byte*>(::operator new(n + 16));
            std::fill(data, data + n + 16, std::byte{0xAB});

            return data;
        }

        auto deallocate(std::byte* data, std::size_t) noexcept -> void
        {
            ::operator delete(data);
        }

        [[nodiscard]] auto operator==(const FillingAllocator&) const noexcept -> bool = default;
    };

    /**
     * @brief Checks that a String is null-terminated and matches its declared size
     */
    template<typename Alloc>
    void expect_terminated(const raw::String<Alloc>& s)
    {
        ASSERT_NE(s.c_str(), nullptr);
        EXPECT_EQ(std::strlen(s.c_str()), s.size());
    }
}

TEST(StringTest, DefaultConstructor)
{
    String s;
    EXPECT_EQ(s.size(), 0);
    EXPECT_EQ(s.capacity(), 0);
    EXPECT_TRUE(s.empty());
}

TEST(StringTest, FromRawParts)
{
    using namespace literal;

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
    EXPECT_GE(s.capacity(), 5);
    EXPECT_EQ(s, "hello"_s);

    // Test invalid UTF-8 string
    const char invalid_utf8[] = {'\xC0', '\x80'}; // Invalid UTF-8 sequence
    result = String::from_raw_parts(invalid_utf8, 2);
    EXPECT_TRUE(result.is_err());
}

TEST(StringTest, From)
{
    using namespace literal;

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
    EXPECT_GE(s.capacity(), 5);
    EXPECT_EQ(s, "hello"_s);

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
    EXPECT_EQ(s1, s2);
}

TEST(StringTest, MoveConstructor)
{
    using namespace literal;

    String s1 = String::from("hello").unwrap();
    String s2(std::move(s1));
    EXPECT_EQ(s2.size(), 5);
    EXPECT_GE(s2.capacity(), 5);
    EXPECT_EQ(s2, "hello"_s);
    EXPECT_EQ(s1.size(), 0);
    EXPECT_EQ(s1.capacity(), 0);
}

TEST(StringTest, Literal)
{
    using namespace literal;

    auto s = "hello"_S;
    EXPECT_EQ(s, "hello"_s);
}

TEST(StringTest, ComparisonOperators)
{
    using namespace literal;

    String s1 = "hello"_S;
    String s2 = "world"_S;
    String s3 = "hello"_S;
    const char* str = "hello";

    EXPECT_TRUE(s1 == s3);
    EXPECT_TRUE(s1 == str);
    EXPECT_FALSE(s1 == s2);
    EXPECT_TRUE(s1 < s2);
    EXPECT_FALSE(s1 > s2);
    EXPECT_FALSE(s2 < s1);
    EXPECT_TRUE(s1 <= s3);
    EXPECT_TRUE(s1 >= s3);
    EXPECT_TRUE(s1 > String());
    EXPECT_TRUE(String() < s1);
}

TEST(StringTest, Reserve)
{
    using namespace literal;

    String s;
    s.reserve(10);
    EXPECT_GE(s.capacity(), 10);
    EXPECT_EQ(s.size(), 0);

    s = "hello"_S;
    s.reserve(20);
    EXPECT_GE(s.capacity(), 20);
    EXPECT_EQ(s.size(), 5);
    EXPECT_EQ(s, "hello"_s);
}

TEST(StringTest, Clear)
{
    String s = String::from("hello").unwrap();
    s.clear();
    EXPECT_EQ(s.size(), 0);
    EXPECT_GE(s.capacity(), 5);
}

TEST(StringTest, Push)
{
    using namespace literal;

    String s;
    s.push(Char('h'));
    s.push(Char('e'));
    s.push(Char('l'));
    s.push(Char('l'));
    s.push(Char('o'));
    EXPECT_EQ(s.size(), 5);
    EXPECT_EQ(s, "hello"_s);
}

TEST(StringTest, PushStr)
{
    using namespace literal;

    String s;
    s.push_str("hello"_s);
    EXPECT_EQ(s.size(), 5);
    EXPECT_EQ(s, "hello"_s);

    s.push_str(" world"_s);
    EXPECT_EQ(s.size(), 11);
    EXPECT_EQ(s, "hello world"_s);
}

TEST(StringTest, SplitOff)
{
    using namespace literal;

    String s = String::from("hello world").unwrap();
    String s2 = s.split_off(6);
    EXPECT_EQ(s.size(), 6);
    EXPECT_EQ(s2.size(), 5);
    EXPECT_EQ(s, "hello "_s);
    EXPECT_EQ(s2, "world"_s);

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
    using namespace literal;

    String s = String::from("hello").unwrap();
    auto ch = s.pop();
    EXPECT_TRUE(ch.is_some());
    EXPECT_EQ(ch.unwrap(), Char('o'));
    EXPECT_EQ(s.size(), 4);
    EXPECT_EQ(s, "hell"_s);

    ch = s.pop();
    EXPECT_TRUE(ch.is_some());
    EXPECT_EQ(ch.unwrap(), Char('l'));
    EXPECT_EQ(s.size(), 3);
    EXPECT_EQ(s, "hel"_s);

    s.clear();
    ch = s.pop();
    EXPECT_TRUE(ch.is_none());
}

TEST(StringTest, MakeAsciiLowercase)
{
    using namespace literal;

    String s = String::from("ABCDEFGHIJKLMNOPQRSTUVWXYZ").unwrap();
    s.make_ascii_lowercase();
    EXPECT_EQ(s, "abcdefghijklmnopqrstuvwxyz"_s);

    // Test non-ASCII characters
    s = String::from("HÉLLO").unwrap();
    s.make_ascii_lowercase();
    EXPECT_EQ(s, "hÉllo"_s);
}

TEST(StringTest, MakeAsciiUppercase)
{
    using namespace literal;

    String s = "abcdefghijklmnopqrstuvwxyz"_S;
    s.make_ascii_uppercase();
    EXPECT_EQ(s, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"_s);

    // Test non-ASCII characters
    s = "héllo"_S;
    s.make_ascii_uppercase();
    EXPECT_EQ(s, "HéLLO"_s);
}

TEST(StringTest, AssignmentOperator)
{
    String s1 = String::from("hello").unwrap();
    String s2;
    s2 = s1;
    EXPECT_EQ(s1.size(), s2.size());
    EXPECT_EQ(s1, s2);
}

TEST(StringTest, PlusEqualsOperator)
{
    using namespace literal;

    String s = "hello"_S;
    s += " world"_s;
    EXPECT_EQ(s.size(), 11);
    EXPECT_EQ(s, "hello world"_s);

    s += Char('!');
    EXPECT_EQ(s.size(), 12);
    EXPECT_EQ(s, "hello world!"_s);

    String s2 = " hello"_S;
    s += s2;
    EXPECT_EQ(s.size(), 18);
    EXPECT_EQ(s, "hello world! hello"_s);
}

TEST(StringTest, PlusOperator)
{
    using namespace literal;

    String s1 = "hello"_S;
    String s2 = " world"_S;
    String s3 = s1 + s2;
    EXPECT_EQ(s3.size(), 11);
    EXPECT_EQ(s3, "hello world"_s);

    String s4 = s1 + " world"_s;
    EXPECT_EQ(s4.size(), 11);
    EXPECT_EQ(s4, "hello world"_s);

    String s5 = s1 + Char('!');
    EXPECT_EQ(s5.size(), 6);
    EXPECT_EQ(s5, "hello!"_s);
}

TEST(StringTest, CStrTermination)
{
    using namespace literal;
    using FString = raw::String<FillingAllocator>;

    // from_raw_parts / from / literal _S (allocate exactly what is needed + terminator)
    expect_terminated(String::from_raw_parts("hello", 5).unwrap());
    expect_terminated(String::from("hello").unwrap());
    expect_terminated("hello"_S);

    // from a str view
    expect_terminated(String("hello"_s));

    // deterministic: the filling allocator would expose a missing terminator
    expect_terminated(FString::from_raw_parts("hello", 5).unwrap());
    expect_terminated(FString::from("world").unwrap());
    expect_terminated(FString("hi"_s));
    {
        auto bytes = "span"_s.as_bytes();
        expect_terminated(FString(bytes));
    }

    // the view constructor path used by std::ranges::to<String>
    String joined = "a,b,c"_s.split(",") | strings::join_with("-") | std::ranges::to<String>();
    EXPECT_EQ(joined, "a-b-c"_s);
    expect_terminated(joined);
}

TEST(StringTest, ClearKeepsTerminatorAndCapacity)
{
    using namespace literal;
    using FString = raw::String<FillingAllocator>;

    FString s = FString::from("hello").unwrap();
    const auto capacity = s.capacity();

    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.size(), 0u);
    EXPECT_EQ(s.capacity(), capacity);
    EXPECT_EQ(std::strlen(s.c_str()), 0u); // would be "hello" without the fix

    // reusable after clear
    s += "bye"_s;
    EXPECT_EQ(s, "bye"_s);
    expect_terminated(s);
}

TEST(StringTest, SplitOffEdgeCases)
{
    using namespace literal;

    // at == 0 keeps everything in the returned string
    {
        String s = "hello"_S;
        String tail = s.split_off(0);
        EXPECT_EQ(s, ""_s);
        EXPECT_EQ(tail, "hello"_s);
        expect_terminated(s);
        expect_terminated(tail);
    }

    // at == size returns an empty string and leaves self unchanged (matches Rust)
    {
        String s = "hello"_S;
        String tail = s.split_off(s.size());
        EXPECT_EQ(s, "hello"_s);
        EXPECT_EQ(tail, ""_s);
        expect_terminated(s);
    }

    // multi-byte boundary
    {
        String s = "你好世界"_S;
        String tail = s.split_off(3); // after the first 3-byte character
        EXPECT_EQ(s, "你"_s);
        EXPECT_EQ(tail, "好世界"_s);
        expect_terminated(s);
        expect_terminated(tail);
    }

    // mid-codepoint and out-of-range panic
    String utf8 = "你好"_S;
    EXPECT_DEATH(utf8.split_off(1), ".*");
    String latin = "hello"_S;
    EXPECT_DEATH(latin.split_off(6), ".*");
}

TEST(StringTest, TruncateEdgeCases)
{
    using namespace literal;

    String s = "hello world"_S;
    s.truncate(0);
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(std::strlen(s.c_str()), 0u);

    // reusable after truncating to zero
    s += "again"_s;
    EXPECT_EQ(s, "again"_s);

    // truncating to the current length is a no-op
    s.truncate(s.size());
    EXPECT_EQ(s, "again"_s);

    // truncating an empty string is fine
    String empty;
    empty.truncate(0);
    EXPECT_TRUE(empty.empty());

    // mid-codepoint panics
    String utf8 = "你好"_S;
    EXPECT_DEATH(utf8.truncate(1), ".*");
}

TEST(StringTest, PopEdgeCases)
{
    using namespace literal;

    // multi-byte last character: "好" is U+597D
    String s = "a你b好"_S;
    const auto last = s.pop();
    EXPECT_TRUE(last.is_some());
    EXPECT_EQ(last.unwrap(), Char(0x597D));
    EXPECT_EQ(s, "a你b"_s);
    expect_terminated(s);

    // pop down to empty, then None
    String letters = "ab"_S;
    EXPECT_EQ(letters.pop().unwrap(), Char('b'));
    EXPECT_EQ(letters.pop().unwrap(), Char('a'));
    EXPECT_TRUE(letters.empty());
    EXPECT_EQ(std::strlen(letters.c_str()), 0u);
    EXPECT_TRUE(letters.pop().is_none());
    EXPECT_TRUE(letters.empty());
}

TEST(StringTest, PushAndAppendEdgeCases)
{
    using namespace literal;

    String s;
    s.push(Char(0x4E2D)); // 中 (3 bytes)
    EXPECT_EQ(s, "中"_s);
    expect_terminated(s);

    // appending nothing is a no-op
    s.push_str(""_s);
    EXPECT_EQ(s, "中"_s);
    s += ""_s;
    EXPECT_EQ(s, "中"_s);

    s += "a"_s;
    EXPECT_EQ(s, "中a"_s);

    String other = "bc"_S;
    s += other;
    EXPECT_EQ(s, "中abc"_s);
    expect_terminated(s);

    // invalid UTF-8 from a C string panics
    EXPECT_DEATH((void)(s += "\xC0\x80"), ".*");
}

TEST(StringTest, ReserveEdgeCases)
{
    using namespace literal;
    using FString = raw::String<FillingAllocator>;

    String s;
    s.reserve(0);
    EXPECT_EQ(s.capacity(), 0u);

    s.reserve(10);
    EXPECT_GE(s.capacity(), 10u);
    EXPECT_EQ(s.size(), 0u);

    s = "hello"_S;
    s.reserve(100);
    EXPECT_GE(s.capacity(), 105u);
    EXPECT_EQ(s, "hello"_s);
    expect_terminated(s);

    // reserving below the current capacity is a no-op
    const auto before = s.capacity();
    s.reserve(1);
    EXPECT_EQ(s.capacity(), before);

    // growth must keep the terminator as well
    FString growing = FString::from("hello").unwrap();
    growing.reserve(64);
    expect_terminated(growing);
    EXPECT_EQ(growing, "hello"_s);
}

TEST(StringTest, SelfAssignment)
{
    using namespace literal;

    String s = "hello"_S;
    String& ref = s;
    s = ref; // self-assignment through a reference must keep the contents

    EXPECT_EQ(s, "hello"_s);
    expect_terminated(s);
}

TEST(StringTest, ConstructionEdgeCases)
{
    using namespace literal;

    // from_raw_parts with a null pointer is treated as empty
    auto null_result = String::from_raw_parts(nullptr, 5);
    EXPECT_TRUE(null_result.is_ok());
    EXPECT_TRUE(null_result.unwrap().empty());

    // from an empty C string
    auto empty = String::from("").unwrap();
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.capacity(), 0u);

    // copy and move preserve content and termination
    String original = "hello"_S;
    String copied(original);
    EXPECT_EQ(copied, "hello"_s);
    expect_terminated(copied);

    // deep copy: mutating the copy must not touch the original
    copied += " world"_s;
    EXPECT_EQ(original, "hello"_s);
    EXPECT_EQ(copied, "hello world"_s);

    String moved(std::move(copied));
    EXPECT_EQ(moved, "hello world"_s);
    expect_terminated(moved);
    EXPECT_TRUE(copied.empty());
}

#endif

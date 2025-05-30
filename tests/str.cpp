#ifdef CRAB_CPP_ENABLE_STRING

#include <gtest/gtest.h>

import crab_cpp;
import std;

using namespace crab_cpp;

TEST(StringTest, StrConstruction)
{
    // Test empty string
    auto empty = str();
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.size(), 0);

    // Test from raw parts
    const char* hello = "Hello";
    auto hello_str = str::from_raw_parts(hello, 5).unwrap();
    EXPECT_FALSE(hello_str.empty());
    EXPECT_EQ(hello_str.size(), 5);
    EXPECT_EQ(hello_str.as_raw(), hello);

    // Test from null-terminated string
    auto world_str = str::from("World").unwrap();
    EXPECT_FALSE(world_str.empty());
    EXPECT_EQ(world_str.size(), 5);
    EXPECT_EQ(std::strcmp(world_str.as_raw(), "World"), 0);

    // Test invalid UTF-8
    const char invalid_utf8[] = {'\xC0', '\x80', 0}; // Invalid UTF-8 sequence
    auto result = str::from_raw_parts(invalid_utf8, 2);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.unwrap_err().pos, 0);
}

TEST(StringTest, StrLiterals)
{
    using namespace literal;

    auto hello = "Hello"_s;
    auto world = "World"_s;
    auto hello_world = "Hello World"_s;

    EXPECT_EQ(hello.size(), 5);
    EXPECT_EQ(world.size(), 5);
    EXPECT_EQ(hello_world.size(), 11);
    EXPECT_EQ(hello_world, str::from("Hello World").unwrap());
}

TEST(StringTest, StrComparison)
{
    using namespace literal;

    auto hello1 = "Hello"_s;
    auto hello2 = "Hello"_s;
    auto world = "World"_s;
    auto hello_world = "Hello World"_s;
    const char* str = "Hello";

    // Test equality
    EXPECT_EQ(hello1, hello2);
    EXPECT_EQ(hello1, str);
    EXPECT_NE(hello1, world);

    // Test ordering
    EXPECT_LT(hello1, world);
    EXPECT_GT(world, hello1);
    EXPECT_LT(hello1, hello_world);

    // Test eq_ignore_ascii_case
    {
        auto upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"_s;
        auto lower = "abcdefghijklmnopqrstuvwxyz"_s;

        EXPECT_TRUE(upper.eq_ignore_ascii_case(lower));
        EXPECT_TRUE(lower.eq_ignore_ascii_case(upper));
    }

    {
        auto a = "RésumÉ"_s;
        auto b = "rÉsumé"_s;

        EXPECT_FALSE(a.eq_ignore_ascii_case(b));
        EXPECT_FALSE(b.eq_ignore_ascii_case(a));
    }
}

TEST(StringTest, IsASCII)
{
    using namespace literal;

    auto resume1 = "resume"_s;
    auto resume2 = "résumé"_s;

    EXPECT_TRUE(resume1.is_ascii());
    EXPECT_FALSE(resume2.is_ascii());
}

TEST(StringTest, StrOperations)
{
    using namespace literal;

    auto hello = "Hello"_s;
    auto world = "World"_s;
    auto hello_world = "Hello World"_s;

    // Test contains
    EXPECT_TRUE(hello_world.contains(hello));
    EXPECT_TRUE(hello_world.contains(world));
    EXPECT_FALSE(hello.contains(world));

    // Test starts_with
    EXPECT_TRUE(hello_world.starts_with(hello));
    EXPECT_FALSE(hello_world.starts_with(world));

    // Test ends_with
    EXPECT_TRUE(hello_world.ends_with(world));
    EXPECT_FALSE(hello_world.ends_with(hello));

    // Test find
    EXPECT_EQ(hello_world.find(hello).unwrap(), 0);
    EXPECT_EQ(hello_world.find(world).unwrap(), 6);
    EXPECT_TRUE(hello_world.find("NotExist"_s).is_none());

    // Test rfind
    EXPECT_EQ(hello_world.rfind(hello).unwrap(), 0);
    EXPECT_EQ(hello_world.rfind(world).unwrap(), 6);
    EXPECT_TRUE(hello_world.rfind("NotExist"_s).is_none());
}

TEST(StringTest, StrSlicing)
{
    using namespace literal;

    auto hello_world = "Hello World"_s;
    auto hello = "Hello"_s;
    auto world = "World"_s;

    // Test slice
    EXPECT_EQ(hello_world.slice(0, 5), hello);
    EXPECT_EQ(hello_world.slice(6), world);

    // Test invalid slice
    EXPECT_DEATH(hello_world.slice(0, 20), ".*");
    EXPECT_DEATH(hello_world.slice(20), ".*");
}

TEST(StringTest, StrTrim)
{
    using namespace literal;

    auto str_with_ws = "  Hello World  "_s;
    auto hello_world = "Hello World"_s;

    // Test trim_ascii
    EXPECT_EQ(str_with_ws.trim_ascii(), hello_world);

    // Test trim_ascii_start
    EXPECT_EQ(str_with_ws.trim_ascii_start(), "Hello World  "_s);

    // Test trim_ascii_end
    EXPECT_EQ(str_with_ws.trim_ascii_end(), "  Hello World"_s);
}

TEST(StringTest, StrStrip)
{
    using namespace literal;

    auto hello_world = "Hello World"_s;
    auto hello = "Hello"_s;
    auto world = "World"_s;

    // Test strip_prefix
    EXPECT_EQ(hello_world.strip_prefix(hello).unwrap(), " World"_s);
    EXPECT_TRUE(hello_world.strip_prefix(world).is_none());

    // Test strip_suffix
    EXPECT_EQ(hello_world.strip_suffix(world).unwrap(), "Hello "_s);
    EXPECT_TRUE(hello_world.strip_suffix(hello).is_none());
}

TEST(StringTest, StrSplit)
{
    using namespace literal;

    auto hello = "Hello"_s;
    auto world = "World"_s;
    auto space = " "_s;

    {
        auto hello_world = "Hello World"_s;

        // Test split
        auto split = hello_world.split(space);
        auto it = split.begin();
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), hello);
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), world);
        ++it;
        EXPECT_EQ(it, split.end());
    }

    {
        auto hello_world = " Hello World "_s;

        // Test split
        auto split = hello_world.split(space);
        auto it = split.begin();
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), "");
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), hello);
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), world);
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), "");
        ++it;
        EXPECT_EQ(it, split.end());
    }

    {
        // Test split_ascii_whitespace
        auto str_with_ws = "  Hello   World  "_s;
        auto split_ws = str_with_ws.split_ascii_whitespace();
        auto it2 = split_ws.begin();
        EXPECT_EQ(str::from_bytes_unchecked(it2->data, it2->len), hello);
        ++it2;
        EXPECT_EQ(str::from_bytes_unchecked(it2->data, it2->len), world);
        ++it2;
        EXPECT_EQ(it2, split_ws.end());
    }
}

TEST(StringTest, StrLines)
{
    using namespace literal;

    auto hello = "Hello"_s;
    auto world = "World"_s;

    // Test \n line endings
    {
        auto multi_line = "Hello\nWorld\n"_s;
        auto lines = multi_line.lines();
        auto it = lines.begin();

        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), hello);
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), world);
        ++it;
        EXPECT_EQ(it, lines.end());
    }

    // Test \r\n line endings
    {
        auto multi_line = "Hello\r\nWorld\r\n"_s;
        auto lines = multi_line.lines();
        auto it = lines.begin();

        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), hello);
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), world);
        ++it;
        EXPECT_EQ(it, lines.end());
    }

    // Test mixed line endings
    {
        auto multi_line = "Hello\nWorld\r\n"_s;
        auto lines = multi_line.lines();
        auto it = lines.begin();

        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), hello);
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), world);
        ++it;
        EXPECT_EQ(it, lines.end());
    }

    // Test empty lines
    {
        auto multi_line = "\n\r\n\n"_s;
        auto empty = ""_s;
        auto lines = multi_line.lines();
        auto it = lines.begin();

        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), empty);
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), empty);
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), empty);
        ++it;
        EXPECT_EQ(it, lines.end());
    }
}

TEST(StringTest, StrChars)
{
    using namespace literal;

    auto s = "y̆"_s;
    auto chars = s.chars();
    auto it = chars.begin();

    EXPECT_EQ(it->code_point(), std::uint32_t('y'));
    ++it;
    EXPECT_EQ(it->code_point(), std::uint32_t(774));
    ++it;
    EXPECT_EQ(it, chars.end());
}

TEST(StringTest, StrParse)
{
    using namespace literal;

    // Test integer parsing
    auto int_str = "123"_s;
    auto int_result = int_str.parse<int32_t>();
    EXPECT_TRUE(int_result.is_ok());
    EXPECT_EQ(int_result.unwrap(), 123);

    // Test negative integer parsing
    auto neg_int_str = "-456"_s;
    auto neg_int_result = neg_int_str.parse<int32_t>();
    EXPECT_TRUE(neg_int_result.is_ok());
    EXPECT_EQ(neg_int_result.unwrap(), -456);

    // Test floating point parsing
    auto float_str = "3.14"_s;
    auto float_result = float_str.parse<double>();
    EXPECT_TRUE(float_result.is_ok());
    EXPECT_DOUBLE_EQ(float_result.unwrap(), 3.14);

    // Test negative floating point parsing
    auto neg_float_str = "-2.718"_s;
    auto neg_float_result = neg_float_str.parse<double>();
    EXPECT_TRUE(neg_float_result.is_ok());
    EXPECT_DOUBLE_EQ(neg_float_result.unwrap(), -2.718);

    // Test invalid integer parsing
    auto invalid_int_str = "abc"_s;
    auto invalid_int_result = invalid_int_str.parse<int32_t>();
    EXPECT_TRUE(invalid_int_result.is_err());
    EXPECT_EQ(invalid_int_result.unwrap_err().ec, std::errc::invalid_argument);

    // Test invalid floating point parsing
    auto invalid_float_str = "xyz"_s;
    auto invalid_float_result = invalid_float_str.parse<double>();
    EXPECT_TRUE(invalid_float_result.is_err());
    EXPECT_EQ(invalid_float_result.unwrap_err().ec, std::errc::invalid_argument);

    // Test empty string parsing
    auto empty_str = ""_s;
    auto empty_result = empty_str.parse<int32_t>();
    EXPECT_TRUE(empty_result.is_err());
    EXPECT_EQ(empty_result.unwrap_err().ec, std::errc::invalid_argument);

    // Test overflow integer parsing
    auto overflow_str = "999999999999999999999999999999"_s;
    auto overflow_result = overflow_str.parse<int32_t>();
    EXPECT_TRUE(overflow_result.is_err());
    EXPECT_EQ(overflow_result.unwrap_err().ec, std::errc::result_out_of_range);

    // Test partial number parsing
    auto partial_str = "123abc"_s;
    auto partial_result = partial_str.parse<int32_t>();
    EXPECT_TRUE(partial_result.is_err());
    EXPECT_EQ(partial_result.unwrap_err().ec, std::errc::invalid_argument);
}

TEST(StringTest, StrAsciiCase)
{
    using namespace literal;

    // Test to_ascii_lowercase
    auto upper_str = "HELLO WORLD"_s;
    auto lower_str = "hello world"_s;
    EXPECT_EQ(upper_str.to_ascii_lowercase(), lower_str);

    // Test to_ascii_uppercase
    EXPECT_EQ(lower_str.to_ascii_uppercase(), upper_str);

    // Test mixed case
    auto mixed_str = "HeLlO WoRlD"_s;
    EXPECT_EQ(mixed_str.to_ascii_lowercase(), lower_str);
    EXPECT_EQ(mixed_str.to_ascii_uppercase(), upper_str);

    // Test non-ASCII characters remain unchanged
    auto non_ascii_str = "Héllö Wörld"_s;
    EXPECT_EQ(non_ascii_str.to_ascii_lowercase(), "héllö wörld"_s);
    EXPECT_EQ(non_ascii_str.to_ascii_uppercase(), "HéLLö WöRLD"_s);

    // Test empty string
    auto empty_str = ""_s;
    EXPECT_EQ(empty_str.to_ascii_lowercase(), empty_str);
    EXPECT_EQ(empty_str.to_ascii_uppercase(), empty_str);
}

TEST(StringTest, StrToStdString)
{
    using namespace literal;

    // Test basic ASCII string
    auto hello_str = "Hello World"_s;
    auto hello_std = hello_str.to_std_string();
    EXPECT_EQ(hello_std, "Hello World");
    EXPECT_EQ(hello_std.size(), hello_str.size());

    // Test empty string
    auto empty_str = ""_s;
    auto empty_std = empty_str.to_std_string();
    EXPECT_TRUE(empty_std.empty());
    EXPECT_EQ(empty_std.size(), 0);
}

TEST(StringTest, StrReplace)
{
    using namespace literal;

    // Test basic replace
    auto hello_world = "Hello World"_s;
    auto world = "World"_s;
    auto universe = "Universe"_s;
    EXPECT_EQ(hello_world.replace(world, universe), "Hello Universe"_s);

    // Test replace with empty pattern
    auto empty_pattern = ""_s;
    auto replacement = "X"_s;
    EXPECT_EQ(hello_world.replace(empty_pattern, replacement), "Hello World"_s);

    // Test replace with empty replacement
    auto empty_replacement = ""_s;
    auto space = " "_s;
    EXPECT_EQ(hello_world.replace(space, empty_replacement), "HelloWorld"_s);

    // Test replace with non-ASCII characters
    auto non_ascii_str = "Héllö Wörld"_s;
    auto non_ascii_pattern = "Wörld"_s;
    auto non_ascii_replacement = "Universe"_s;
    EXPECT_EQ(non_ascii_str.replace(non_ascii_pattern, non_ascii_replacement), "Héllö Universe"_s);

    // Test replace with UTF-8 characters
    auto utf8_str = "你好世界"_s;
    auto utf8_pattern = "世界"_s;
    auto utf8_replacement = "宇宙"_s;
    EXPECT_EQ(utf8_str.replace(utf8_pattern, utf8_replacement), "你好宇宙"_s);

    // Test replace_n with limit
    auto repeated = "aaa"_s;
    auto a = "a"_s;
    auto b = "b"_s;
    EXPECT_EQ(repeated.replace_n(a, b, 2), "bba"_s);
    EXPECT_EQ(repeated.replace_n(a, b, 1), "baa"_s);
    EXPECT_EQ(repeated.replace_n(a, b, 0), repeated);
    EXPECT_EQ(repeated.replace_n(a, b, 3), "bbb"_s);
    EXPECT_EQ(repeated.replace_n(a, b, 4), "bbb"_s); // More than occurrences

    // Test replace_n with empty pattern
    EXPECT_EQ(hello_world.replace_n(empty_pattern, replacement, 2), "Hello World"_s);

    // Test replace_n with empty replacement
    EXPECT_EQ(hello_world.replace_n(space, empty_replacement, 1), "HelloWorld"_s);
}

TEST(StringTest, StrRepeat)
{
    using namespace literal;

    // Test basic repeat
    auto hello = "Hello"_s;
    auto hello_hello = "HelloHello"_s;
    EXPECT_EQ(hello.repeat(2), hello_hello);

    // Test repeat with empty string
    auto empty = ""_s;
    EXPECT_EQ(empty.repeat(5), empty);

    auto alphabet = "abcdefghijklmnopqrstuvwxyz"_s;
    EXPECT_DEATH(alphabet.repeat(0xFFFFFFFFFFFFFFFF), "Repeat times overflow");
}

TEST(StringTest, JoinWith)
{
    using namespace literal;

    auto s = "hello,world"_s;
    EXPECT_EQ(s.split(",") | strings::join_with(".") | std::ranges::to<String>(), "hello.world"_s);
}

TEST(StringTest, StrMatches)
{
    using namespace literal;

    // Test basic pattern matching
    auto text = "Hello World Hello"_s;
    auto pattern = "Hello"_s;
    auto matches = text.matches(pattern);
    auto it = matches.begin();

    EXPECT_EQ(*it, 0);
    ++it;
    EXPECT_EQ(*it, 12);
    ++it;
    EXPECT_EQ(it, matches.end());

    // Test no matches
    auto no_matches = text.matches("xyz"_s);
    EXPECT_EQ(no_matches.begin(), no_matches.end());

    // Test empty pattern
    auto empty_pattern = ""_s;
    auto empty_matches = text.matches(empty_pattern);
    EXPECT_EQ(empty_matches.begin(), empty_matches.end());

    // Test pattern at start and end
    auto start_end = "abcabc"_s;
    auto abc = "abc"_s;
    auto abc_matches = start_end.matches(abc);
    it = abc_matches.begin();

    EXPECT_EQ(*it, 0);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(it, abc_matches.end());

    // Test UTF-8 pattern matching
    auto utf8_text = "你好世界你好"_s;
    auto utf8_pattern = "你好"_s;
    auto utf8_matches = utf8_text.matches(utf8_pattern);
    it = utf8_matches.begin();

    EXPECT_EQ(*it, 0);
    ++it;
    EXPECT_EQ(*it, 12);
    ++it;
    EXPECT_EQ(it, utf8_matches.end());
}

#endif
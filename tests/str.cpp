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

TEST(StringTest, StrComparison)
{
    auto hello1 = str::from("Hello").unwrap();
    auto hello2 = str::from("Hello").unwrap();
    auto world = str::from("World").unwrap();
    auto hello_world = str::from("Hello World").unwrap();

    // Test equality
    EXPECT_EQ(hello1, hello2);
    EXPECT_NE(hello1, world);

    // Test ordering
    EXPECT_LT(hello1, world);
    EXPECT_GT(world, hello1);
    EXPECT_LT(hello1, hello_world);
}

TEST(StringTest, StrOperations)
{
    auto hello = str::from("Hello").unwrap();
    auto world = str::from("World").unwrap();
    auto hello_world = str::from("Hello World").unwrap();

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
    EXPECT_TRUE(hello_world.find(str::from("NotExist").unwrap()).is_none());

    // Test rfind
    EXPECT_EQ(hello_world.rfind(hello).unwrap(), 0);
    EXPECT_EQ(hello_world.rfind(world).unwrap(), 6);
    EXPECT_TRUE(hello_world.rfind(str::from("NotExist").unwrap()).is_none());
}

TEST(StringTest, StrSlicing)
{
    auto hello_world = str::from("Hello World").unwrap();
    auto hello = str::from("Hello").unwrap();
    auto world = str::from("World").unwrap();

    // Test slice
    EXPECT_EQ(hello_world.slice(0, 5), hello);
    EXPECT_EQ(hello_world.slice(6), world);

    // Test invalid slice
    EXPECT_DEATH(hello_world.slice(0, 20), ".*");
    EXPECT_DEATH(hello_world.slice(20), ".*");
}

TEST(StringTest, StrTrim)
{
    auto str_with_ws = str::from("  Hello World  ").unwrap();
    auto hello_world = str::from("Hello World").unwrap();

    // Test trim_ascii
    EXPECT_EQ(str_with_ws.trim_ascii(), hello_world);

    // Test trim_ascii_start
    EXPECT_EQ(str_with_ws.trim_ascii_start(), str::from("Hello World  ").unwrap());

    // Test trim_ascii_end
    EXPECT_EQ(str_with_ws.trim_ascii_end(), str::from("  Hello World").unwrap());
}

TEST(StringTest, StrStrip)
{
    auto hello_world = str::from("Hello World").unwrap();
    auto hello = str::from("Hello").unwrap();
    auto world = str::from("World").unwrap();

    // Test strip_prefix
    EXPECT_EQ(hello_world.strip_prefix(hello).unwrap(), str::from(" World").unwrap());
    EXPECT_TRUE(hello_world.strip_prefix(world).is_none());

    // Test strip_suffix
    EXPECT_EQ(hello_world.strip_suffix(world).unwrap(), str::from("Hello ").unwrap());
    EXPECT_TRUE(hello_world.strip_suffix(hello).is_none());
}

TEST(StringTest, StrSplit)
{
    auto hello = str::from("Hello").unwrap();
    auto world = str::from("World").unwrap();
    auto space = str::from(" ").unwrap();

    {
        auto hello_world = str::from("Hello World").unwrap();

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
        auto hello_world = str::from(" Hello World ").unwrap();

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
        auto str_with_ws = str::from("  Hello   World  ").unwrap();
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
    // Test \n line endings
    {
        auto multi_line = str::from("Hello\nWorld\n").unwrap();
        auto hello = str::from("Hello").unwrap();
        auto world = str::from("World").unwrap();

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
        auto multi_line = str::from("Hello\r\nWorld\r\n").unwrap();
        auto hello = str::from("Hello").unwrap();
        auto world = str::from("World").unwrap();

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
        auto multi_line = str::from("Hello\nWorld\r\n").unwrap();
        auto hello = str::from("Hello").unwrap();
        auto world = str::from("World").unwrap();

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
        auto multi_line = str::from("\n\r\n\n").unwrap();
        auto empty = str::from("").unwrap();

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
    auto s = str::from("y̆").unwrap();
    auto chars = s.chars();
    auto it = chars.begin();

    EXPECT_EQ(it->code_point(), std::uint32_t('y'));
    ++it;
    EXPECT_EQ(it->code_point(), std::uint32_t(774));
    ++it;
    EXPECT_EQ(it, chars.end());
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

TEST(StringTest, StrParse)
{
    // Test integer parsing
    auto int_str = str::from("123").unwrap();
    auto int_result = int_str.parse<int32_t>();
    EXPECT_TRUE(int_result.is_ok());
    EXPECT_EQ(int_result.unwrap(), 123);

    // Test negative integer parsing
    auto neg_int_str = str::from("-456").unwrap();
    auto neg_int_result = neg_int_str.parse<int32_t>();
    EXPECT_TRUE(neg_int_result.is_ok());
    EXPECT_EQ(neg_int_result.unwrap(), -456);

    // Test floating point parsing
    auto float_str = str::from("3.14").unwrap();
    auto float_result = float_str.parse<double>();
    EXPECT_TRUE(float_result.is_ok());
    EXPECT_DOUBLE_EQ(float_result.unwrap(), 3.14);

    // Test negative floating point parsing
    auto neg_float_str = str::from("-2.718").unwrap();
    auto neg_float_result = neg_float_str.parse<double>();
    EXPECT_TRUE(neg_float_result.is_ok());
    EXPECT_DOUBLE_EQ(neg_float_result.unwrap(), -2.718);

    // Test invalid integer parsing
    auto invalid_int_str = str::from("abc").unwrap();
    auto invalid_int_result = invalid_int_str.parse<int32_t>();
    EXPECT_TRUE(invalid_int_result.is_err());
    EXPECT_EQ(invalid_int_result.unwrap_err().ec, std::errc::invalid_argument);

    // Test invalid floating point parsing
    auto invalid_float_str = str::from("xyz").unwrap();
    auto invalid_float_result = invalid_float_str.parse<double>();
    EXPECT_TRUE(invalid_float_result.is_err());
    EXPECT_EQ(invalid_float_result.unwrap_err().ec, std::errc::invalid_argument);

    // Test empty string parsing
    auto empty_str = str::from("").unwrap();
    auto empty_result = empty_str.parse<int32_t>();
    EXPECT_TRUE(empty_result.is_err());
    EXPECT_EQ(empty_result.unwrap_err().ec, std::errc::invalid_argument);

    // Test overflow integer parsing
    auto overflow_str = str::from("999999999999999999999999999999").unwrap();
    auto overflow_result = overflow_str.parse<int32_t>();
    EXPECT_TRUE(overflow_result.is_err());
    EXPECT_EQ(overflow_result.unwrap_err().ec, std::errc::result_out_of_range);

    // Test partial number parsing
    auto partial_str = str::from("123abc").unwrap();
    auto partial_result = partial_str.parse<int32_t>();
    EXPECT_TRUE(partial_result.is_err());
    EXPECT_EQ(partial_result.unwrap_err().ec, std::errc::invalid_argument);
}

TEST(StringTest, StrAsciiCase)
{
    // Test to_ascii_lowercase
    auto upper_str = str::from("HELLO WORLD").unwrap();
    auto lower_str = str::from("hello world").unwrap();
    EXPECT_EQ(upper_str.to_ascii_lowercase(), lower_str);

    // Test to_ascii_uppercase
    EXPECT_EQ(lower_str.to_ascii_uppercase(), upper_str);

    // Test mixed case
    auto mixed_str = str::from("HeLlO WoRlD").unwrap();
    EXPECT_EQ(mixed_str.to_ascii_lowercase(), lower_str);
    EXPECT_EQ(mixed_str.to_ascii_uppercase(), upper_str);

    // Test non-ASCII characters remain unchanged
    auto non_ascii_str = str::from("Héllö Wörld").unwrap();
    EXPECT_EQ(non_ascii_str.to_ascii_lowercase(), str::from("héllö wörld").unwrap());
    EXPECT_EQ(non_ascii_str.to_ascii_uppercase(), str::from("HéLLö WöRLD").unwrap());

    // Test empty string
    auto empty_str = str::from("").unwrap();
    EXPECT_EQ(empty_str.to_ascii_lowercase(), empty_str);
    EXPECT_EQ(empty_str.to_ascii_uppercase(), empty_str);
}

TEST(StringTest, StrToStdString)
{
    // Test basic ASCII string
    auto hello_str = str::from("Hello World").unwrap();
    auto hello_std = hello_str.to_std_string();
    EXPECT_EQ(hello_std, "Hello World");
    EXPECT_EQ(hello_std.size(), hello_str.size());

    // Test empty string
    auto empty_str = str::from("").unwrap();
    auto empty_std = empty_str.to_std_string();
    EXPECT_TRUE(empty_std.empty());
    EXPECT_EQ(empty_std.size(), 0);
}

TEST(StringTest, StrReplace)
{
    // Test basic replace
    auto hello_world = str::from("Hello World").unwrap();
    auto world = str::from("World").unwrap();
    auto universe = str::from("Universe").unwrap();
    EXPECT_EQ(hello_world.replace(world, universe), str::from("Hello Universe").unwrap());

    // Test replace with empty pattern
    auto empty_pattern = str::from("").unwrap();
    auto replacement = str::from("X").unwrap();
    EXPECT_EQ(hello_world.replace(empty_pattern, replacement), str::from("Hello World").unwrap());

    // Test replace with empty replacement
    auto empty_replacement = str::from("").unwrap();
    auto space = str::from(" ").unwrap();
    EXPECT_EQ(hello_world.replace(space, empty_replacement), str::from("HelloWorld").unwrap());

    // Test replace with non-ASCII characters
    auto non_ascii_str = str::from("Héllö Wörld").unwrap();
    auto non_ascii_pattern = str::from("Wörld").unwrap();
    auto non_ascii_replacement = str::from("Universe").unwrap();
    EXPECT_EQ(non_ascii_str.replace(non_ascii_pattern, non_ascii_replacement), str::from("Héllö Universe").unwrap());

    // Test replace with UTF-8 characters
    auto utf8_str = str::from("你好世界").unwrap();
    auto utf8_pattern = str::from("世界").unwrap();
    auto utf8_replacement = str::from("宇宙").unwrap();
    EXPECT_EQ(utf8_str.replace(utf8_pattern, utf8_replacement), str::from("你好宇宙").unwrap());

    // Test replace_n with limit
    auto repeated = str::from("aaa").unwrap();
    auto a = str::from("a").unwrap();
    auto b = str::from("b").unwrap();
    EXPECT_EQ(repeated.replace_n(a, b, 2), str::from("bba").unwrap());
    EXPECT_EQ(repeated.replace_n(a, b, 1), str::from("baa").unwrap());
    EXPECT_EQ(repeated.replace_n(a, b, 0), repeated);
    EXPECT_EQ(repeated.replace_n(a, b, 3), str::from("bbb").unwrap());
    EXPECT_EQ(repeated.replace_n(a, b, 4), str::from("bbb").unwrap()); // More than occurrences

    // Test replace_n with empty pattern
    EXPECT_EQ(hello_world.replace_n(empty_pattern, replacement, 2), str::from("Hello World").unwrap());

    // Test replace_n with empty replacement
    EXPECT_EQ(hello_world.replace_n(space, empty_replacement, 1), str::from("HelloWorld").unwrap());
}

TEST(StringTest, StrRepeat)
{
    // Test basic repeat
    auto hello = str::from("Hello").unwrap();
    auto hello_hello = str::from("HelloHello").unwrap();
    EXPECT_EQ(hello.repeat(2), hello_hello);

    // Test repeat with empty string
    auto empty = str::from("").unwrap();
    EXPECT_EQ(empty.repeat(5), empty);

    auto alphabet = str::from("abcdefghijklmnopqrstuvwxyz").unwrap();
    EXPECT_DEATH(alphabet.repeat(0xFFFFFFFFFFFFFFFF), "Repeat times overflow");
}

TEST(StringTest, JoinWith)
{
    using namespace literal;

    auto s = str::from("hello,world").unwrap();
    EXPECT_EQ(s.split(",") | strings::join_with(".") | std::ranges::to<String>(), "hello.world"_s);
}

TEST(StringTest, StrMatches)
{
    // Test basic pattern matching
    auto text = str::from("Hello World Hello").unwrap();
    auto pattern = str::from("Hello").unwrap();
    auto matches = text.matches(pattern);
    auto it = matches.begin();

    EXPECT_EQ(*it, 0);
    ++it;
    EXPECT_EQ(*it, 12);
    ++it;
    EXPECT_EQ(it, matches.end());

    // Test no matches
    auto no_matches = text.matches(str::from("xyz").unwrap());
    EXPECT_EQ(no_matches.begin(), no_matches.end());

    // Test empty pattern
    auto empty_pattern = str::from("").unwrap();
    auto empty_matches = text.matches(empty_pattern);
    EXPECT_EQ(empty_matches.begin(), empty_matches.end());

    // Test pattern at start and end
    auto start_end = str::from("abcabc").unwrap();
    auto abc = str::from("abc").unwrap();
    auto abc_matches = start_end.matches(abc);
    it = abc_matches.begin();

    EXPECT_EQ(*it, 0);
    ++it;
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(it, abc_matches.end());

    // Test UTF-8 pattern matching
    auto utf8_text = str::from("你好世界你好").unwrap();
    auto utf8_pattern = str::from("你好").unwrap();
    auto utf8_matches = utf8_text.matches(utf8_pattern);
    it = utf8_matches.begin();

    EXPECT_EQ(*it, 0);
    ++it;
    EXPECT_EQ(*it, 12);
    ++it;
    EXPECT_EQ(it, utf8_matches.end());
}
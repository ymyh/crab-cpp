#ifdef CRAB_CPP_ENABLE_STRING

#include <gtest/gtest.h>
#include <ranges>

import crab_cpp;
import std;

using namespace crab_cpp;

namespace
{
    /**
     * @brief Collects the pieces of a split/lines range into a vector of str views
     */
    template<typename Range>
    auto collect_pieces(Range&& range) -> std::vector<str>
    {
        std::vector<str> result;

        for (const auto& piece : range)
        {
            result.emplace_back(piece);
        }

        return result;
    }

    /**
     * @brief Minimal byte allocator used to exercise the allocator template
     *        parameters of `str::to_string<Alloc>` and `str::replace<Alloc>`
     */
    struct TestAllocator
    {
        using value_type = std::byte;

        [[nodiscard]] auto allocate(std::size_t n) -> std::byte*
        {
            return static_cast<std::byte*>(::operator new(n));
        }

        auto deallocate(std::byte* data, std::size_t) noexcept -> void
        {
            ::operator delete(data);
        }

        [[nodiscard]] auto operator==(const TestAllocator&) const noexcept -> bool = default;
    };
}

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

// The literal operators use the length the compiler provides, so embedded NUL
// bytes survive instead of being truncated by strlen.
TEST(StringTest, StrLiteralEmbeddedNul)
{
    using namespace literal;

    const auto with_nul = "a\0b"_s;
    EXPECT_EQ(with_nul.size(), 3u);
    EXPECT_EQ(std::to_integer<char>(with_nul.as_bytes()[0]), 'a');
    EXPECT_EQ(std::to_integer<char>(with_nul.as_bytes()[1]), '\0');
    EXPECT_EQ(std::to_integer<char>(with_nul.as_bytes()[2]), 'b');
    EXPECT_EQ(with_nul, str::from_raw_parts("a\0b", 3).unwrap());

    const auto owned = "a\0b"_S;
    EXPECT_EQ(owned.size(), 3u);
    EXPECT_EQ(owned.as_str(), with_nul);
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
    EXPECT_DEATH(hello_world.slice(0, 20), "Invalid parameter\\(s\\) while calling str::slice");
    EXPECT_DEATH(hello_world.slice(20), "Invalid parameter\\(s\\) while calling str::slice");
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
        auto hello_world = "HelloWorld"_s;

        // An empty pattern splits at every code point boundary (Rust)
        auto split = hello_world.split("");
        auto it = split.begin();
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), "");
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), "H"_s);
        ++it;
        EXPECT_EQ(str::from_bytes_unchecked(it->data, it->len), "e"_s);
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

    // Test non-ASCII content is copied byte-for-byte
    auto cjk_str = "你好,世界"_s;
    auto cjk_std = cjk_str.to_std_string();
    EXPECT_EQ(cjk_std, std::string("你好,世界"));
    EXPECT_EQ(cjk_std.size(), cjk_str.size());
    EXPECT_EQ(cjk_std.size(), 13u); // 4 CJK chars (3 bytes each) + ',' (1 byte)

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

    // Test basic pattern matching: matches yields the matched slices
    auto text = "Hello World Hello"_s;
    auto pattern = "Hello"_s;
    auto matches = text.matches(pattern);
    auto it = matches.begin();

    EXPECT_EQ(*it, "Hello"_s);
    ++it;
    EXPECT_EQ(*it, "Hello"_s);
    ++it;
    EXPECT_EQ(it, matches.end());

    // Test no matches
    auto no_matches = text.matches("xyz"_s);
    EXPECT_EQ(no_matches.begin(), no_matches.end());

    // Empty pattern matches at every code point boundary (Rust); each match is
    // an empty slice.
    auto empty_pattern = ""_s;
    auto empty_matches = text.matches(empty_pattern);
    size_t empty_match_count = 0;
    for (const auto match : empty_matches)
    {
        EXPECT_EQ(match, ""_s);
        empty_match_count += 1;
    }
    EXPECT_EQ(empty_match_count, text.size() + 1);

    // Test pattern at start and end
    auto start_end = "abcabc"_s;
    auto abc = "abc"_s;
    auto abc_matches = start_end.matches(abc);
    it = abc_matches.begin();

    EXPECT_EQ(*it, "abc"_s);
    ++it;
    EXPECT_EQ(*it, "abc"_s);
    ++it;
    EXPECT_EQ(it, abc_matches.end());

    // Test UTF-8 pattern matching
    auto utf8_text = "你好世界你好"_s;
    auto utf8_pattern = "你好"_s;
    auto utf8_matches = utf8_text.matches(utf8_pattern);
    it = utf8_matches.begin();

    EXPECT_EQ(*it, "你好"_s);
    ++it;
    EXPECT_EQ(*it, "你好"_s);
    ++it;
    EXPECT_EQ(it, utf8_matches.end());
}

TEST(StringTest, TrimStartMatches)
{
    using namespace literal;

    // Test basic pattern matching
    auto text = "HelloHelloWorld"_s;
    auto pattern = "Hello"_s;
    EXPECT_EQ(text.trim_start_matches(pattern), "World"_s);

    // Test no matches at start
    auto text2 = "WorldHelloHello"_s;
    EXPECT_EQ(text2.trim_start_matches(pattern), text2);

    // Test empty pattern
    auto text3 = "Hello World"_s;
    auto empty_pattern = ""_s;
    EXPECT_EQ(text3.trim_start_matches(empty_pattern), text3);

    // Test empty string
    auto empty_text = ""_s;
    EXPECT_EQ(empty_text.trim_start_matches(pattern), empty_text);

    // Test UTF-8 pattern matching
    auto utf8_text = "你好你好世界"_s;
    auto utf8_pattern = "你好"_s;
    EXPECT_EQ(utf8_text.trim_start_matches(utf8_pattern), "世界"_s);

    // Test pattern longer than text
    auto short_text = "Hi"_s;
    auto long_pattern = "Hello"_s;
    EXPECT_EQ(short_text.trim_start_matches(long_pattern), short_text);
}

TEST(StringTest, TrimEndMatches)
{
    using namespace literal;

    // Test basic pattern matching
    auto text = "WorldHelloHello"_s;
    auto pattern = "Hello"_s;
    EXPECT_EQ(text.trim_end_matches(pattern), "World"_s);

    // Test no matches at end
    auto text2 = "HelloHelloWorld"_s;
    EXPECT_EQ(text2.trim_end_matches(pattern), text2);

    // Test empty pattern
    auto text3 = "Hello World"_s;
    auto empty_pattern = ""_s;
    EXPECT_EQ(text3.trim_end_matches(empty_pattern), text3);

    // Test empty string
    auto empty_text = ""_s;
    EXPECT_EQ(empty_text.trim_end_matches(pattern), empty_text);

    // Test UTF-8 pattern matching
    auto utf8_text = "世界你好你好"_s;
    auto utf8_pattern = "你好"_s;
    EXPECT_EQ(utf8_text.trim_end_matches(utf8_pattern), "世界"_s);

    // Test pattern longer than text
    auto short_text = "Hi"_s;
    auto long_pattern = "Hello"_s;
    EXPECT_EQ(short_text.trim_end_matches(long_pattern), short_text);
}

TEST(StringTest, StrTrimAsciiFullWhitespace)
{
    using namespace literal;

    // trim_ascii must strip the whole ASCII whitespace set, not just spaces
    auto s = "\t\n\f\r Hello World \t\n"_s;
    EXPECT_EQ(s.trim_ascii(), "Hello World"_s);
    EXPECT_EQ(s.trim_ascii_start(), "Hello World \t\n"_s);
    EXPECT_EQ(s.trim_ascii_end(), "\t\n\f\r Hello World"_s);

    // Regression: an all-whitespace string used to loop past the end
    auto spaces = "   "_s;
    EXPECT_EQ(spaces.trim_ascii(), ""_s);
    EXPECT_EQ(spaces.trim_ascii_start(), ""_s);
    EXPECT_EQ(spaces.trim_ascii_end(), ""_s);

    auto tabs = "\t\t"_s;
    EXPECT_EQ(tabs.trim_ascii(), ""_s);
}

TEST(StringTest, StrCharBoundary)
{
    using namespace literal;

    // "a" (1) + "é" (2) + "中" (3) + "😀" (4) = 10 bytes
    auto s = "aé中😀"_s;
    EXPECT_EQ(s.size(), 10u);

    EXPECT_TRUE(s.is_char_boundary(0));
    EXPECT_TRUE(s.is_char_boundary(1));
    EXPECT_FALSE(s.is_char_boundary(2));
    EXPECT_TRUE(s.is_char_boundary(3));
    EXPECT_FALSE(s.is_char_boundary(4));
    EXPECT_FALSE(s.is_char_boundary(5));
    EXPECT_TRUE(s.is_char_boundary(6));
    EXPECT_FALSE(s.is_char_boundary(7));
    EXPECT_TRUE(s.is_char_boundary(10));
    EXPECT_FALSE(s.is_char_boundary(11));
}

TEST(StringTest, StrGet)
{
    using namespace literal;

    auto s = "Hello World"_s;
    EXPECT_EQ(s.get(0, 5).unwrap(), "Hello"_s);
    EXPECT_EQ(s.get(6).unwrap(), "World"_s);
    EXPECT_EQ(s.get(0, 0).unwrap(), ""_s);
    EXPECT_EQ(s.get(s.size()).unwrap(), ""_s);

    EXPECT_TRUE(s.get(0, 20).is_none());
    EXPECT_TRUE(s.get(6, 5).is_none());

    auto utf8 = "aé中"_s; // boundaries: 0, 1, 3, 6
    EXPECT_TRUE(utf8.get(2).is_none());
    EXPECT_TRUE(utf8.get(0, 2).is_none());
    EXPECT_EQ(utf8.get(1, 3).unwrap(), "é"_s);
    EXPECT_EQ(utf8.get(3).unwrap(), "中"_s);
}

TEST(StringTest, StrSplitAt)
{
    using namespace literal;

    auto s = "Hello World"_s;

    const auto [head, tail] = s.split_at(5);
    EXPECT_EQ(head, "Hello"_s);
    EXPECT_EQ(tail, " World"_s);

    const auto [all, rest] = s.split_at(s.size());
    EXPECT_EQ(all, s);
    EXPECT_EQ(rest, ""_s);

    EXPECT_DEATH(s.split_at(20), "Invalid index while calling str::split_at");

    auto utf8 = "aé中"_s;
    EXPECT_DEATH(utf8.split_at(2), "Invalid index while calling str::split_at");

    const auto checked = utf8.split_at_checked(3).unwrap();
    EXPECT_EQ(checked.first, "aé"_s);
    EXPECT_EQ(checked.second, "中"_s);
    EXPECT_TRUE(utf8.split_at_checked(2).is_none());
}

TEST(StringTest, StrCharIndices)
{
    using namespace literal;

    auto s = "aé中"_s; // starts at byte 0, 1, 3
    auto indices = s.char_indices();
    auto it = indices.begin();

    EXPECT_EQ(it->first, 0u);
    EXPECT_EQ(it->second.code_point(), std::uint32_t('a'));
    ++it;
    EXPECT_EQ(it->first, 1u);
    EXPECT_EQ(it->second.code_point(), std::uint32_t(0xE9));
    ++it;
    EXPECT_EQ(it->first, 3u);
    EXPECT_EQ(it->second.code_point(), std::uint32_t(0x4E2D));
    ++it;
    EXPECT_EQ(it, indices.end());

    size_t count = 0;
    size_t last_index = 0;
    for (const auto [index, ch] : s.char_indices())
    {
        (void)ch;
        last_index = index;
        count += 1;
    }
    EXPECT_EQ(count, 3u);
    EXPECT_EQ(last_index, 3u);

    size_t empty_count = 0;
    for (const auto [index, ch] : ""_s.char_indices())
    {
        (void)index;
        (void)ch;
        empty_count += 1;
    }
    EXPECT_EQ(empty_count, 0u);
}

TEST(StringTest, StrSplitChar)
{
    using namespace literal;

    auto s = "a,b,,c,"_s;
    const auto parts = s.split(',') | std::ranges::to<std::vector<str>>();
    ASSERT_EQ(parts.size(), 5u);
    EXPECT_EQ(parts[0], "a"_s);
    EXPECT_EQ(parts[1], "b"_s);
    EXPECT_EQ(parts[2], ""_s);
    EXPECT_EQ(parts[3], "c"_s);
    EXPECT_EQ(parts[4], ""_s);

    // multi-byte separator via Char
    auto u = "a中b中"_s;
    const auto u_parts = u.split(Char(0x4E2D)) | std::ranges::to<std::vector<str>>();
    ASSERT_EQ(u_parts.size(), 3u);
    EXPECT_EQ(u_parts[0], "a"_s);
    EXPECT_EQ(u_parts[1], "b"_s);
    EXPECT_EQ(u_parts[2], ""_s);

    // no separator: a single piece
    const auto single = "Hello"_s.split(',') | std::ranges::to<std::vector<str>>();
    ASSERT_EQ(single.size(), 1u);
    EXPECT_EQ(single[0], "Hello"_s);

    // empty subject
    const auto empty = ""_s.split(',') | std::ranges::to<std::vector<str>>();
    ASSERT_EQ(empty.size(), 1u);
    EXPECT_EQ(empty[0], ""_s);
}

TEST(StringTest, StrSplitStrTrailingEmpty)
{
    using namespace literal;

    // Rust keeps a trailing empty piece when the string ends with the separator
    auto s = "a,b,"_s;
    const auto parts = s.split(",") | std::ranges::to<std::vector<str>>();
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts[0], "a"_s);
    EXPECT_EQ(parts[1], "b"_s);
    EXPECT_EQ(parts[2], ""_s);
}

TEST(StringTest, StrSplitOnce)
{
    using namespace literal;

    auto s = "key=value=more"_s;

    const auto once = s.split_once('=').unwrap();
    EXPECT_EQ(once.first, "key"_s);
    EXPECT_EQ(once.second, "value=more"_s);

    const auto ronce = s.rsplit_once('=').unwrap();
    EXPECT_EQ(ronce.first, "key=value"_s);
    EXPECT_EQ(ronce.second, "more"_s);

    EXPECT_TRUE(s.split_once('@').is_none());
    EXPECT_TRUE(s.rsplit_once('@').is_none());

    // string, const char* and raw::String patterns
    EXPECT_EQ(s.split_once("="_s).unwrap().first, "key"_s);
    EXPECT_EQ(s.split_once("=").unwrap().second, "value=more"_s);
    EXPECT_EQ(s.split_once(String::from("=").unwrap()).unwrap().first, "key"_s);

    // multi-byte char
    const auto u = "a中b"_s.split_once(Char(0x4E2D)).unwrap();
    EXPECT_EQ(u.first, "a"_s);
    EXPECT_EQ(u.second, "b"_s);

    // empty pattern: Rust yields ("", self)
    const auto empty = "abc"_s.split_once(""_s).unwrap();
    EXPECT_EQ(empty.first, ""_s);
    EXPECT_EQ(empty.second, "abc"_s);
}

TEST(StringTest, StrCharAndLiteralOverloads)
{
    using namespace literal;

    auto s = "Hello World"_s;

    EXPECT_TRUE(s.contains("World"));
    EXPECT_TRUE(s.contains('W'));
    EXPECT_FALSE(s.contains('z'));
    EXPECT_TRUE(s.starts_with("Hello"));
    EXPECT_TRUE(s.starts_with('H'));
    EXPECT_FALSE(s.starts_with('h'));
    EXPECT_TRUE(s.ends_with("World"));
    EXPECT_TRUE(s.ends_with('d'));
    EXPECT_FALSE(s.ends_with('D'));

    // raw::String patterns
    EXPECT_TRUE(s.contains(String::from("World").unwrap()));
    EXPECT_TRUE(s.starts_with(String::from("Hello").unwrap()));
    EXPECT_TRUE(s.ends_with(String::from("World").unwrap()));

    auto multi = "a中b"_s;
    EXPECT_TRUE(multi.contains(Char(0x4E2D)));
    EXPECT_EQ(multi.find(Char(0x4E2D)).unwrap(), 1u);
    EXPECT_EQ(multi.rfind(Char(0x4E2D)).unwrap(), 1u);
    EXPECT_EQ(multi.strip_prefix('a').unwrap(), "中b"_s);
    EXPECT_EQ(multi.strip_suffix('b').unwrap(), "a中"_s);
    EXPECT_EQ("中b"_s.strip_prefix(Char(0x4E2D)).unwrap(), "b"_s);
    EXPECT_EQ(multi.strip_suffix(String::from("中b").unwrap()).unwrap(), "a"_s);
    EXPECT_TRUE(multi.strip_suffix(Char(0x4E2D)).is_none());
}

TEST(StringTest, StrToString)
{
    using namespace literal;

    auto s = "Hello"_s;
    auto owned = s.to_string();

    static_assert(std::same_as<decltype(owned), String>);
    EXPECT_EQ(owned, s);
    EXPECT_EQ(owned, "Hello"_s);
}

TEST(StringTest, CharWhitespaceAndEncoding)
{
    EXPECT_TRUE(Char(0x20).is_whitespace());
    EXPECT_TRUE(Char(0x09).is_whitespace());
    EXPECT_TRUE(Char(0x85).is_whitespace());   // NEL
    EXPECT_TRUE(Char(0xA0).is_whitespace());   // NBSP
    EXPECT_TRUE(Char(0x3000).is_whitespace()); // ideographic space
    EXPECT_FALSE(Char(0x41).is_whitespace());
    EXPECT_FALSE(Char(0x4E2D).is_whitespace());

    EXPECT_EQ(Char('a').len_utf8(), 1u);
    EXPECT_EQ(Char(0xE9).len_utf8(), 2u);
    EXPECT_EQ(Char(0x4E2D).len_utf8(), 3u);
    EXPECT_EQ(Char(0x1F600).len_utf8(), 4u);

    std::byte buffer[4] = {};
    EXPECT_EQ(Char(0x4E2D).encode_utf8(buffer), 3u);
    EXPECT_EQ(buffer[0], std::byte{0xE4});
    EXPECT_EQ(buffer[1], std::byte{0xB8});
    EXPECT_EQ(buffer[2], std::byte{0xAD});

    EXPECT_EQ(Char('a').to_ascii_uppercase().code_point(), std::uint32_t('A'));
    EXPECT_EQ(Char('Z').to_ascii_lowercase().code_point(), std::uint32_t('z'));
    EXPECT_EQ(Char(0x4E2D).to_ascii_uppercase().code_point(), std::uint32_t(0x4E2D));
}

TEST(StringTest, StrCharsRangeFor)
{
    using namespace literal;

    // Regression: the last character used to be dropped by a range-for loop
    auto s = "aé中"_s;
    std::vector<std::uint32_t> codepoints;
    for (const auto ch : s.chars())
    {
        codepoints.push_back(ch.code_point());
    }

    ASSERT_EQ(codepoints.size(), 3u);
    EXPECT_EQ(codepoints[0], std::uint32_t('a'));
    EXPECT_EQ(codepoints[1], std::uint32_t(0xE9));
    EXPECT_EQ(codepoints[2], std::uint32_t(0x4E2D));

    size_t empty_count = 0;
    for (const auto ch : ""_s.chars())
    {
        (void)ch;
        empty_count += 1;
    }
    EXPECT_EQ(empty_count, 0u);
}

TEST(StringTest, StrGraphemesRangeFor)
{
    using namespace literal;

    // "y" + combining breve is a single grapheme cluster
    size_t count = 0;
    for (const auto grapheme : "y̆"_s.graphemes())
    {
        (void)grapheme;
        count += 1;
    }
    EXPECT_EQ(count, 1u);

    // Two base+combining clusters and a plain char: 3 clusters
    count = 0;
    for (const auto grapheme : "y̆y̆y"_s.graphemes())
    {
        (void)grapheme;
        count += 1;
    }
    EXPECT_EQ(count, 3u);
}

TEST(StringTest, StrContainsEdgeCases)
{
    using namespace literal;

    auto s = "banana"_s;

    EXPECT_TRUE(s.contains(""));
    EXPECT_TRUE(s.contains("banana"));
    EXPECT_TRUE(s.contains("nan"));
    EXPECT_TRUE(s.contains("a"));
    EXPECT_FALSE(s.contains("bananas"));
    EXPECT_FALSE(s.contains("xyz"));
    EXPECT_FALSE(s.contains("X"));

    EXPECT_FALSE(""_s.contains("a"));
    EXPECT_TRUE(""_s.contains(""));

    EXPECT_TRUE(s.contains('a'));
    EXPECT_FALSE(s.contains('z'));

    auto utf8 = "héllo→世界"_s;
    EXPECT_TRUE(utf8.contains("世界"));
    EXPECT_TRUE(utf8.contains(Char(0x2192))); // →
    EXPECT_FALSE(utf8.contains(Char(0x2190))); // ←
    EXPECT_FALSE(utf8.contains("世界里"));

    EXPECT_TRUE(s.contains(String::from("nan").unwrap()));
    EXPECT_FALSE(s.contains(String::from("zzz").unwrap()));
}

TEST(StringTest, StrFindEdgeCases)
{
    using namespace literal;

    auto s = "ababab"_s;

    // first and last occurrence
    EXPECT_EQ(s.find("ab").unwrap(), 0u);
    EXPECT_EQ(s.find("ba").unwrap(), 1u);
    EXPECT_EQ(s.rfind("ab").unwrap(), 4u);
    EXPECT_EQ(s.rfind("ba").unwrap(), 3u);

    EXPECT_EQ(s.find('a').unwrap(), 0u);
    EXPECT_EQ(s.rfind('a').unwrap(), 4u);
    EXPECT_EQ(s.find('b').unwrap(), 1u);
    EXPECT_EQ(s.rfind('b').unwrap(), 5u);

    // not found
    EXPECT_TRUE(s.find("c").is_none());
    EXPECT_TRUE(s.rfind("c").is_none());
    EXPECT_TRUE(s.find('c').is_none());
    EXPECT_TRUE(s.rfind('c').is_none());

    // pattern longer than the subject
    EXPECT_TRUE(s.find("abababab").is_none());
    EXPECT_TRUE(s.rfind("abababab").is_none());

    // const char* and raw::String overloads
    EXPECT_EQ(s.find("ab").unwrap(), 0u);
    EXPECT_EQ(s.rfind("ab").unwrap(), 4u);
    EXPECT_EQ(s.find(String::from("ba").unwrap()).unwrap(), 1u);
    EXPECT_EQ(s.rfind(String::from("ba").unwrap()).unwrap(), 3u);

    // empty pattern matches at index 0 / at the length (Rust)
    EXPECT_EQ(s.find(""_s).unwrap(), 0u);
    EXPECT_EQ(s.rfind(""_s).unwrap(), s.size());

    // empty subject
    EXPECT_TRUE(""_s.find("a").is_none());
    EXPECT_EQ(""_s.find(""_s).unwrap(), 0u);
    EXPECT_EQ(""_s.rfind(""_s).unwrap(), 0u);

    // byte index points at the start of a multi-byte match: "x" + "→"(3) + "y" + "→"(3) + "z"
    auto utf8 = "x→y→z"_s;
    EXPECT_EQ(utf8.find(Char(0x2192)).unwrap(), 1u);
    EXPECT_EQ(utf8.rfind(Char(0x2192)).unwrap(), 5u);
    EXPECT_EQ(utf8.find("→y").unwrap(), 1u);

    // invalid UTF-8 in a const char* pattern panics
    EXPECT_DEATH((void)s.find("\xC0\x80"), "Invalid UTF-8 sequence while calling str::find#pattern");
    EXPECT_DEATH((void)s.contains("\xC0\x80"), "Invalid UTF-8 sequence while calling str::contains#pattern");
}

TEST(StringTest, StrStartsWithEndsWithEdgeCases)
{
    using namespace literal;

    auto s = "hello"_s;

    EXPECT_TRUE(s.starts_with(""));
    EXPECT_TRUE(s.ends_with(""));
    EXPECT_TRUE(s.starts_with("hello"));
    EXPECT_TRUE(s.ends_with("hello"));
    EXPECT_TRUE(s.starts_with("he"));
    EXPECT_TRUE(s.ends_with("lo"));
    EXPECT_FALSE(s.starts_with("hello!"));
    EXPECT_FALSE(s.ends_with("!hello"));
    EXPECT_FALSE(s.starts_with("Hello")); // case sensitive
    EXPECT_FALSE(s.ends_with("LO"));

    EXPECT_TRUE(""_s.starts_with(""));
    EXPECT_TRUE(""_s.ends_with(""));
    EXPECT_FALSE(""_s.starts_with("a"));
    EXPECT_FALSE(""_s.ends_with("a"));

    EXPECT_TRUE(s.starts_with('h'));
    EXPECT_FALSE(s.starts_with('H'));
    EXPECT_TRUE(s.ends_with('o'));
    EXPECT_FALSE(s.ends_with('O'));

    auto utf8 = "世界abc"_s;
    EXPECT_TRUE(utf8.starts_with(Char(0x4E16))); // 世
    EXPECT_TRUE(utf8.ends_with("abc"));
    EXPECT_FALSE(utf8.ends_with(Char(0x4E16)));
    EXPECT_TRUE(utf8.starts_with("世"));
}

TEST(StringTest, StrStripEdgeCases)
{
    using namespace literal;

    auto s = "aaabaaa"_s;

    // only strips once
    EXPECT_EQ(s.strip_prefix("a").unwrap(), "aabaaa"_s);
    EXPECT_EQ(s.strip_suffix("a").unwrap(), "aaabaa"_s);
    EXPECT_EQ(s.strip_prefix("aa").unwrap(), "abaaa"_s);
    EXPECT_EQ(s.strip_prefix("aaa").unwrap(), "baaa"_s);
    EXPECT_TRUE(s.strip_prefix("aaaa").is_none());

    // empty pattern returns the whole string
    EXPECT_EQ(s.strip_prefix("").unwrap(), s);
    EXPECT_EQ(s.strip_suffix("").unwrap(), s);

    // not found
    EXPECT_TRUE(s.strip_prefix("b").is_none());
    EXPECT_TRUE(s.strip_suffix("b").is_none());

    // pattern longer than the subject
    EXPECT_TRUE(s.strip_prefix("aaabaaaa").is_none());
    EXPECT_TRUE(s.strip_suffix("aaabaaaa").is_none());

    // empty subject
    EXPECT_EQ(""_s.strip_prefix("").unwrap(), ""_s);
    EXPECT_TRUE(""_s.strip_prefix("a").is_none());
    EXPECT_EQ(""_s.strip_suffix("").unwrap(), ""_s);
    EXPECT_TRUE(""_s.strip_suffix("a").is_none());

    // char overloads
    EXPECT_EQ(s.strip_prefix('a').unwrap(), "aabaaa"_s);
    EXPECT_EQ(s.strip_suffix('a').unwrap(), "aaabaa"_s);
    EXPECT_TRUE(s.strip_prefix('b').is_none());
    EXPECT_TRUE(s.strip_suffix('b').is_none());

    // multi-byte
    auto utf8 = "中中中"_s;
    EXPECT_EQ(utf8.strip_prefix(Char(0x4E2D)).unwrap(), "中中"_s);
    EXPECT_EQ(utf8.strip_suffix("中").unwrap(), "中中"_s);
    EXPECT_EQ(utf8.strip_prefix("中中").unwrap(), "中"_s);

    // raw::String overload
    EXPECT_EQ(s.strip_prefix(String::from("aaa").unwrap()).unwrap(), "baaa"_s);
    EXPECT_EQ(s.strip_suffix(String::from("aaa").unwrap()).unwrap(), "aaab"_s);

    // stripping the whole string yields an empty slice
    EXPECT_EQ("abc"_s.strip_prefix("abc").unwrap(), ""_s);
    EXPECT_EQ("abc"_s.strip_suffix("abc").unwrap(), ""_s);
}

TEST(StringTest, StrSplitEdgeCases)
{
    using namespace literal;

    // no separator: a single piece
    {
        const auto pieces = collect_pieces("abc"_s.split(','));
        ASSERT_EQ(pieces.size(), 1u);
        EXPECT_EQ(pieces[0], "abc"_s);
    }

    // leading/trailing/consecutive separators are all kept
    {
        auto parts = collect_pieces("a,,b,"_s.split(','));
        ASSERT_EQ(parts.size(), 4u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], ""_s);
        EXPECT_EQ(parts[2], "b"_s);
        EXPECT_EQ(parts[3], ""_s);
    }

    // only separators
    {
        auto parts = collect_pieces(",,,"_s.split(','));
        ASSERT_EQ(parts.size(), 4u);
        for (const auto& part : parts)
        {
            EXPECT_EQ(part, ""_s);
        }
    }

    // empty subject
    {
        auto parts = collect_pieces(""_s.split(','));
        ASSERT_EQ(parts.size(), 1u);
        EXPECT_EQ(parts[0], ""_s);
    }

    // multi-char separator
    {
        auto parts = collect_pieces("a<>b<>c"_s.split("<>"_s));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], "b"_s);
        EXPECT_EQ(parts[2], "c"_s);
    }

    // adjacent multi-char separators
    {
        auto parts = collect_pieces("a<><>b"_s.split("<>"_s));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], ""_s);
        EXPECT_EQ(parts[2], "b"_s);
    }

    // separator not present / longer than the subject
    {
        const auto pieces = collect_pieces("ab"_s.split("abc"_s));
        ASSERT_EQ(pieces.size(), 1u);
        EXPECT_EQ(pieces[0], "ab"_s);
    }

    // non-overlapping: "aaa" split by "aa" is ["", "a"]
    {
        auto parts = collect_pieces("aaa"_s.split("aa"_s));
        ASSERT_EQ(parts.size(), 2u);
        EXPECT_EQ(parts[0], ""_s);
        EXPECT_EQ(parts[1], "a"_s);
    }

    // multi-byte separator via Char and via str must agree
    {
        auto via_char = collect_pieces("a中b中"_s.split(Char(0x4E2D)));
        auto via_str = collect_pieces("a中b中"_s.split("中"_s));
        ASSERT_EQ(via_char.size(), 3u);
        ASSERT_EQ(via_str.size(), 3u);
        for (size_t i = 0; i < 3; i += 1)
        {
            EXPECT_EQ(via_char[i], via_str[i]);
        }
    }

    // const char* and raw::String patterns
    {
        const auto by_cstr = collect_pieces("a,b"_s.split(","));
        ASSERT_EQ(by_cstr.size(), 2u);
        EXPECT_EQ(by_cstr[0], "a"_s);
        EXPECT_EQ(by_cstr[1], "b"_s);

        const auto by_string = collect_pieces("a,b"_s.split(String::from(",").unwrap()));
        ASSERT_EQ(by_string.size(), 2u);
        EXPECT_EQ(by_string[0], "a"_s);
        EXPECT_EQ(by_string[1], "b"_s);
    }

    // empty pattern splits at every code point boundary (Rust)
    {
        auto parts = collect_pieces("abc"_s.split(""_s));
        ASSERT_EQ(parts.size(), 5u);
        EXPECT_EQ(parts[0], ""_s);
        EXPECT_EQ(parts[1], "a"_s);
        EXPECT_EQ(parts[2], "b"_s);
        EXPECT_EQ(parts[3], "c"_s);
        EXPECT_EQ(parts[4], ""_s);

        auto empty = collect_pieces(""_s.split(""_s));
        ASSERT_EQ(empty.size(), 2u);
        EXPECT_EQ(empty[0], ""_s);
        EXPECT_EQ(empty[1], ""_s);

        // boundaries respect multi-byte characters
        auto utf8 = collect_pieces("中a"_s.split(""_s));
        ASSERT_EQ(utf8.size(), 4u);
        EXPECT_EQ(utf8[0], ""_s);
        EXPECT_EQ(utf8[1], "中"_s);
        EXPECT_EQ(utf8[2], "a"_s);
        EXPECT_EQ(utf8[3], ""_s);
    }
}

TEST(StringTest, StrSplitOnceOnlySplitsOnce)
{
    using namespace literal;

    auto s = "a=b=c=d"_s;

    // split_once stops at the first separator and keeps the remainder intact
    const auto first = s.split_once('=').unwrap();
    EXPECT_EQ(first.first, "a"_s);
    EXPECT_EQ(first.second, "b=c=d"_s);

    // rsplit_once stops at the last separator
    const auto last = s.rsplit_once('=').unwrap();
    EXPECT_EQ(last.first, "a=b=c"_s);
    EXPECT_EQ(last.second, "d"_s);

    // same through the str / const char* / raw::String overloads
    EXPECT_EQ(s.split_once("="_s).unwrap().second, "b=c=d"_s);
    EXPECT_EQ(s.split_once("=").unwrap().second, "b=c=d"_s);
    EXPECT_EQ(s.split_once(String::from("=").unwrap()).unwrap().second, "b=c=d"_s);
    EXPECT_EQ(s.rsplit_once("="_s).unwrap().first, "a=b=c"_s);

    // multi-char separator occurring several times
    auto m = "a::b::c"_s;
    EXPECT_EQ(m.split_once("::"_s).unwrap().first, "a"_s);
    EXPECT_EQ(m.split_once("::"_s).unwrap().second, "b::c"_s);
    EXPECT_EQ(m.rsplit_once("::"_s).unwrap().first, "a::b"_s);
    EXPECT_EQ(m.rsplit_once("::"_s).unwrap().second, "c"_s);

    // separator at the very start / end
    EXPECT_EQ("=x"_s.split_once('=').unwrap().first, ""_s);
    EXPECT_EQ("=x"_s.split_once('=').unwrap().second, "x"_s);
    EXPECT_EQ("x="_s.split_once('=').unwrap().first, "x"_s);
    EXPECT_EQ("x="_s.split_once('=').unwrap().second, ""_s);
    EXPECT_EQ("=x"_s.rsplit_once('=').unwrap().first, ""_s);
    EXPECT_EQ("x="_s.rsplit_once('=').unwrap().second, ""_s);

    // no occurrence
    EXPECT_TRUE("abc"_s.split_once('=').is_none());
    EXPECT_TRUE("abc"_s.rsplit_once('=').is_none());
    EXPECT_TRUE("abc"_s.split_once("==").is_none());

    // empty subject
    EXPECT_TRUE(""_s.split_once('=').is_none());
    EXPECT_TRUE(""_s.rsplit_once('=').is_none());
    EXPECT_EQ(""_s.split_once(""_s).unwrap().first, ""_s);
    EXPECT_EQ(""_s.split_once(""_s).unwrap().second, ""_s);

    // multi-byte separator
    auto utf8 = "a中b中c"_s;
    const auto whole = utf8.split_once(Char(0x4E2D)).unwrap();
    EXPECT_EQ(whole.first, "a"_s);
    EXPECT_EQ(whole.second, "b中c"_s);
    const auto rwhole = utf8.rsplit_once(Char(0x4E2D)).unwrap();
    EXPECT_EQ(rwhole.first, "a中b"_s);
    EXPECT_EQ(rwhole.second, "c"_s);
}

TEST(StringTest, StrReplaceEdgeCases)
{
    using namespace literal;

    // non-overlapping replacements
    EXPECT_EQ("aaa"_s.replace("aa", "b"), "ba"_s);
    EXPECT_EQ("aaaa"_s.replace("aa", "b"), "bb"_s);
    EXPECT_EQ("abc"_s.replace("a", "aa"), "aabc"_s);
    EXPECT_EQ("abc"_s.replace("d", "X"), "abc"_s);
    EXPECT_EQ("abc"_s.replace("abc", ""), ""_s);
    EXPECT_EQ("abc"_s.replace("abc", "abc"), "abc"_s);
    EXPECT_EQ(""_s.replace("a", "b"), ""_s);

    // the replacement text is not re-scanned
    EXPECT_EQ("aaa"_s.replace("a", "aa"), "aaaaaa"_s);

    // empty pattern leaves the string unchanged (current behaviour)
    EXPECT_EQ("abc"_s.replace("", "X"), "abc"_s);

    // multi-byte
    EXPECT_EQ("你好世界你好"_s.replace("你好", "hi"), "hi世界hi"_s);
    EXPECT_EQ("中中中"_s.replace("中中", "X"), "X中"_s);

    // replace_n limits the number of replacements
    EXPECT_EQ("aaaa"_s.replace_n("a"_s, "b"_s, 0), "aaaa"_s);
    EXPECT_EQ("aaaa"_s.replace_n("a"_s, "b"_s, 1), "baaa"_s);
    EXPECT_EQ("aaaa"_s.replace_n("a"_s, "b"_s, 3), "bbba"_s);
    EXPECT_EQ("aaaa"_s.replace_n("a"_s, "b"_s, 10), "bbbb"_s);
    EXPECT_EQ("aaaa"_s.replace_n("aa"_s, "b"_s, 1), "baa"_s);
    EXPECT_EQ("aaaa"_s.replace_n("aa"_s, "b"_s, 2), "bb"_s);

    // replace_n with no match or an empty pattern
    EXPECT_EQ("abc"_s.replace_n("z"_s, "y"_s, 2), "abc"_s);
    EXPECT_EQ("abc"_s.replace_n(""_s, "y"_s, 2), "abc"_s);

    // const char* overload
    EXPECT_EQ("Hello World"_s.replace("World", "Crab"), "Hello Crab"_s);
}

TEST(StringTest, StrTrimEdgeCases)
{
    using namespace literal;

    // trim_ascii on empty / unchanged / all-whitespace
    EXPECT_EQ(""_s.trim_ascii(), ""_s);
    EXPECT_EQ("abc"_s.trim_ascii(), "abc"_s);
    EXPECT_EQ("   "_s.trim_ascii(), ""_s);
    EXPECT_EQ(" \t\r\n\f abc \t\r\n\f "_s.trim_ascii(), "abc"_s);

    // inner whitespace is preserved
    EXPECT_EQ("  a b  "_s.trim_ascii(), "a b"_s);
    EXPECT_EQ("  a b  "_s.trim_ascii_start(), "a b  "_s);
    EXPECT_EQ("  a b  "_s.trim_ascii_end(), "  a b"_s);

    // non-ASCII whitespace is not ASCII and is kept
    EXPECT_EQ("\u00A0abc\u00A0"_s.trim_ascii(), "\u00A0abc\u00A0"_s);

    // trim_start_matches / trim_end_matches remove every repetition
    EXPECT_EQ("aaa"_s.trim_start_matches("a"), ""_s);
    EXPECT_EQ("aaabaaa"_s.trim_start_matches("a"), "baaa"_s);
    EXPECT_EQ("aaabaaa"_s.trim_end_matches("a"), "aaab"_s);
    EXPECT_EQ("abc"_s.trim_start_matches("x"), "abc"_s);
    EXPECT_EQ("abc"_s.trim_end_matches("x"), "abc"_s);

    // empty pattern leaves the string unchanged
    EXPECT_EQ("  abc  "_s.trim_start_matches(""), "  abc  "_s);
    EXPECT_EQ("  abc  "_s.trim_end_matches(""), "  abc  "_s);

    // empty subject
    EXPECT_EQ(""_s.trim_start_matches("a"), ""_s);
    EXPECT_EQ(""_s.trim_end_matches("a"), ""_s);

    // const char* overload (removes each whole repetition, not one byte at a time)
    EXPECT_EQ("aaabaaa"_s.trim_start_matches("aa"), "abaaa"_s);
    EXPECT_EQ("aaabaaa"_s.trim_end_matches("aa"), "aaaba"_s);

    // multi-byte
    EXPECT_EQ("中中abc中中"_s.trim_start_matches("中"), "abc中中"_s);
    EXPECT_EQ("中中abc中中"_s.trim_end_matches("中"), "中中abc"_s);

    // removing every repetition yields an empty slice
    EXPECT_EQ("aaa"_s.trim_end_matches("a"), ""_s);
    EXPECT_EQ("中中中"_s.trim_start_matches("中"), ""_s);
}

TEST(StringTest, StrLinesEdgeCases)
{
    using namespace literal;

    // an empty string has no lines
    EXPECT_EQ(collect_pieces(""_s.lines()).size(), 0u);

    // no trailing newline
    {
        auto lines = collect_pieces("abc"_s.lines());
        ASSERT_EQ(lines.size(), 1u);
        EXPECT_EQ(lines[0], "abc"_s);
    }

    // a trailing newline does not add an empty line
    {
        auto lines = collect_pieces("a\nb\n"_s.lines());
        ASSERT_EQ(lines.size(), 2u);
        EXPECT_EQ(lines[0], "a"_s);
        EXPECT_EQ(lines[1], "b"_s);
    }

    // a single line with a trailing newline must not keep the newline
    {
        auto lines = collect_pieces("a\n"_s.lines());
        ASSERT_EQ(lines.size(), 1u);
        EXPECT_EQ(lines[0], "a"_s);
    }
    {
        auto lines = collect_pieces("a\r\n"_s.lines());
        ASSERT_EQ(lines.size(), 1u);
        EXPECT_EQ(lines[0], "a"_s);
    }

    // a lone newline is a single empty line
    {
        auto lines = collect_pieces("\n"_s.lines());
        ASSERT_EQ(lines.size(), 1u);
        EXPECT_EQ(lines[0], ""_s);
    }

    // consecutive newlines
    {
        auto lines = collect_pieces("a\n\nb"_s.lines());
        ASSERT_EQ(lines.size(), 3u);
        EXPECT_EQ(lines[0], "a"_s);
        EXPECT_EQ(lines[1], ""_s);
        EXPECT_EQ(lines[2], "b"_s);
    }

    // a lone carriage return is not a separator and is kept
    {
        auto lines = collect_pieces("a\rb"_s.lines());
        ASSERT_EQ(lines.size(), 1u);
        EXPECT_EQ(lines[0], "a\rb"_s);
    }
}

TEST(StringTest, StrSplitAsciiWhitespaceEdgeCases)
{
    using namespace literal;

    EXPECT_EQ(collect_pieces(""_s.split_ascii_whitespace()).size(), 0u);
    EXPECT_EQ(collect_pieces("   "_s.split_ascii_whitespace()).size(), 0u);
    EXPECT_EQ(collect_pieces("\t\n\f\r "_s.split_ascii_whitespace()).size(), 0u);

    {
        auto parts = collect_pieces("  a\tb\nc\rd  "_s.split_ascii_whitespace());
        ASSERT_EQ(parts.size(), 4u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], "b"_s);
        EXPECT_EQ(parts[2], "c"_s);
        EXPECT_EQ(parts[3], "d"_s);
    }

    // non-ASCII whitespace is not a separator here
    {
        auto parts = collect_pieces("a\u00A0b"_s.split_ascii_whitespace());
        ASSERT_EQ(parts.size(), 1u);
        EXPECT_EQ(parts[0], "a\u00A0b"_s);
    }
}

TEST(StringTest, StrMatchesEdgeCases)
{
    using namespace literal;

    // every occurrence
    {
        auto s = "ababab"_s;
        std::vector<str> found;
        for (const auto match : s.matches("ab"))
        {
            found.push_back(match);
        }
        EXPECT_EQ(found, (std::vector<str>{"ab"_s, "ab"_s, "ab"_s}));
    }

    // non-overlapping: "aaa" matched by "aa" is a single match
    {
        auto s = "aaa"_s;
        std::vector<str> found;
        for (const auto match : s.matches("aa"))
        {
            found.push_back(match);
        }
        EXPECT_EQ(found, (std::vector<str>{"aa"_s}));
    }

    // no match / longer pattern / empty subject
    {
        std::vector<str> found;
        for (const auto match : "abc"_s.matches("xyz"))
        {
            found.push_back(match);
        }
        for (const auto match : "abc"_s.matches("abcd"))
        {
            found.push_back(match);
        }
        for (const auto match : ""_s.matches("a"))
        {
            found.push_back(match);
        }
        EXPECT_TRUE(found.empty());
    }

    // empty pattern matches at every code point boundary (Rust); each match is
    // an empty slice, one per boundary plus the end
    {
        std::vector<str> found;
        for (const auto match : "abc"_s.matches(""_s))
        {
            found.push_back(match);
        }
        EXPECT_EQ(found, (std::vector<str>{""_s, ""_s, ""_s, ""_s}));
    }

    // const char* overload
    {
        auto s = "a.b.c"_s;
        std::vector<str> found;
        for (const auto match : s.matches("."))
        {
            found.push_back(match);
        }
        EXPECT_EQ(found, (std::vector<str>{"."_s, "."_s}));
    }

    // multi-byte pattern: "中"(3) + "a"(1) + "中"(3) + "b"(1) + "中"(3)
    {
        auto s = "中a中b中"_s;
        std::vector<str> found;
        for (const auto match : s.matches("中"_s))
        {
            found.push_back(match);
        }
        EXPECT_EQ(found, (std::vector<str>{"中"_s, "中"_s, "中"_s}));
    }
}

TEST(StringTest, StrEqIgnoreAsciiCaseEdgeCases)
{
    using namespace literal;

    EXPECT_TRUE(""_s.eq_ignore_ascii_case(""_s));
    EXPECT_FALSE(""_s.eq_ignore_ascii_case("a"_s));
    EXPECT_FALSE("a"_s.eq_ignore_ascii_case(""_s));
    EXPECT_TRUE("abc"_s.eq_ignore_ascii_case("ABC"_s));
    EXPECT_TRUE("ABC"_s.eq_ignore_ascii_case("abc"_s));
    EXPECT_TRUE("AbC dEf"_s.eq_ignore_ascii_case("aBc DeF"_s));
    EXPECT_FALSE("abc"_s.eq_ignore_ascii_case("abcd"_s));
    EXPECT_FALSE("abc"_s.eq_ignore_ascii_case("abd"_s));
    EXPECT_FALSE("123"_s.eq_ignore_ascii_case("12"_s));
    EXPECT_TRUE("123"_s.eq_ignore_ascii_case("123"_s));

    // non-ASCII characters are compared byte-wise, not folded
    EXPECT_FALSE("é"_s.eq_ignore_ascii_case("É"_s));
    EXPECT_TRUE("é"_s.eq_ignore_ascii_case("é"_s));
}

TEST(StringTest, StrSliceEdgeCases)
{
    using namespace literal;

    auto s = "abc"_s;
    EXPECT_EQ(s.slice(0, 0), ""_s);
    EXPECT_EQ(s.slice(0, 3), s);
    EXPECT_EQ(s.slice(3), ""_s);
    EXPECT_EQ(s.slice(3, 3), ""_s);
    EXPECT_EQ(s.slice(1, 1), ""_s);

    EXPECT_DEATH(s.slice(0, 4), "Invalid parameter\\(s\\) while calling str::slice");
    EXPECT_DEATH(s.slice(4), "Invalid parameter\\(s\\) while calling str::slice");
    EXPECT_DEATH(s.slice(2, 1), "Invalid parameter\\(s\\) while calling str::slice"); // to < from (the fixed underflow)
    EXPECT_DEATH(s.slice(5, 2), "Invalid parameter\\(s\\) while calling str::slice");

    // multi-byte boundary: "a" + "é"(2), boundaries are 0, 1, 3
    auto utf8 = "aé"_s;
    EXPECT_EQ(utf8.slice(1, 3), "é"_s);
    EXPECT_DEATH(utf8.slice(2, 3), "Invalid UTF-8 sequence while calling str::slice");
}

TEST(StringTest, StrGetEdgeCases)
{
    using namespace literal;

    auto s = ""_s;
    EXPECT_EQ(s.get(0).unwrap(), ""_s);
    EXPECT_TRUE(s.get(1).is_none());
    EXPECT_EQ(s.get(0, 0).unwrap(), ""_s);
    EXPECT_TRUE(s.get(0, 1).is_none());
    EXPECT_TRUE(s.get(1, 0).is_none());
}

TEST(StringTest, StrRepeatEdgeCases)
{
    using namespace literal;

    EXPECT_EQ("abc"_s.repeat(0), ""_s);
    EXPECT_EQ("abc"_s.repeat(1), "abc"_s);
    EXPECT_EQ("abc"_s.repeat(3), "abcabcabc"_s);
    EXPECT_EQ(""_s.repeat(100), ""_s);
    EXPECT_EQ("中"_s.repeat(2), "中中"_s);
}

TEST(StringTest, StrToStringIsIndependent)
{
    using namespace literal;

    auto s = "hello"_s;
    auto owned = s.to_string();
    owned.push(Char('!'));

    EXPECT_EQ(owned, "hello!"_s);
    EXPECT_EQ(s, "hello"_s); // the borrowed view is unaffected
}

TEST(StringTest, StrByteAccess)
{
    using namespace literal;

    auto s = "a中"_s; // 'a' + U+4E2D (3 bytes)
    EXPECT_EQ(s.size(), 4u);

    auto bytes = s.as_bytes();
    ASSERT_EQ(bytes.size(), 4u);
    EXPECT_EQ(std::to_integer<unsigned>(bytes[0]), 0x61u); // 'a'
    EXPECT_EQ(std::to_integer<unsigned>(bytes[1]), 0xE4u);
    EXPECT_EQ(std::to_integer<unsigned>(bytes[2]), 0xB8u);
    EXPECT_EQ(std::to_integer<unsigned>(bytes[3]), 0xADu);
    EXPECT_EQ(s.data(), std::bit_cast<const std::byte*>(s.as_raw()));

    size_t count = 0;
    for (const auto byte_value : s)
    {
        (void)byte_value;
        count += 1;
    }
    EXPECT_EQ(count, 4u);
}

TEST(StringTest, StrComparisonEdgeCases)
{
    using namespace literal;

    EXPECT_EQ(""_s, ""_s);
    EXPECT_NE(""_s, "a"_s);
    EXPECT_LT(""_s, "a"_s);
    EXPECT_GT("a"_s, ""_s);
    EXPECT_LT("abc"_s, "abcd"_s);
    EXPECT_GT("abcd"_s, "abc"_s);
    EXPECT_LT("ABC"_s, "abc"_s);
    EXPECT_GT("b"_s, "a"_s);

    EXPECT_TRUE("abc"_s == std::string("abc"));
    EXPECT_TRUE(std::string("abc") == "abc"_s);
    EXPECT_TRUE("abc"_s == "abc");
    EXPECT_TRUE("abc" == "abc"_s);
    EXPECT_FALSE("abc"_s == "abd");
    EXPECT_TRUE("abc"_s == String::from("abc").unwrap());
}

TEST(StringTest, StrParseEdgeCases)
{
    using namespace literal;

    EXPECT_TRUE(""_s.parse<int>().is_err());
    EXPECT_TRUE(" "_s.parse<int>().is_err());
    EXPECT_TRUE("7 "_s.parse<int>().is_err());
    EXPECT_EQ("-0"_s.parse<int>().unwrap(), 0);
    EXPECT_EQ("007"_s.parse<int>().unwrap(), 7);
    EXPECT_EQ("2147483647"_s.parse<int>().unwrap(), 2147483647);
    EXPECT_TRUE("2147483648"_s.parse<int>().is_err());
}

TEST(StringTest, StrEmptyStringBehaviour)
{
    using namespace literal;

    auto empty = ""_s;
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.size(), 0u);
    EXPECT_EQ(empty, ""_s);
    EXPECT_TRUE(empty.is_ascii());
    EXPECT_TRUE(empty.is_char_boundary(0));
    EXPECT_FALSE(empty.is_char_boundary(1)); // out of range
    EXPECT_EQ(empty.to_string(), ""_s);
    EXPECT_TRUE(empty.to_std_string().empty());

    size_t char_count = 0;
    for (const auto ch : empty.chars())
    {
        (void)ch;
        char_count += 1;
    }
    EXPECT_EQ(char_count, 0u);

    size_t grapheme_count = 0;
    for (const auto grapheme : empty.graphemes())
    {
        (void)grapheme;
        grapheme_count += 1;
    }
    EXPECT_EQ(grapheme_count, 0u);

    size_t line_count = 0;
    for (const auto line : empty.lines())
    {
        (void)line;
        line_count += 1;
    }
    EXPECT_EQ(line_count, 0u);

    size_t byte_count = 0;
    for (const auto byte_value : empty)
    {
        (void)byte_value;
        byte_count += 1;
    }
    EXPECT_EQ(byte_count, 0u);
}

TEST(StringTest, StrMatchIndices)
{
    using namespace literal;

    auto s = "ababab"_s;
    std::vector<std::pair<size_t, str>> matches;
    for (const auto& [index, matched] : s.match_indices("ab"))
    {
        matches.emplace_back(index, matched);
    }
    ASSERT_EQ(matches.size(), 3u);
    EXPECT_EQ(matches[0].first, 0u);
    EXPECT_EQ(matches[0].second, "ab"_s);
    EXPECT_EQ(matches[1].first, 2u);
    EXPECT_EQ(matches[2].first, 4u);

    // rmatch_indices yields the same matches in reverse order
    std::vector<size_t> reverse_indices;
    for (const auto& [index, matched] : s.rmatch_indices("ab"))
    {
        (void)matched;
        reverse_indices.push_back(index);
    }
    ASSERT_EQ(reverse_indices.size(), 3u);
    EXPECT_EQ(reverse_indices[0], 4u);
    EXPECT_EQ(reverse_indices[1], 2u);
    EXPECT_EQ(reverse_indices[2], 0u);

    // multi-byte char pattern
    auto u = "中a中"_s;
    std::vector<size_t> u_indices;
    std::vector<str> u_matches;
    for (const auto& [index, matched] : u.match_indices(Char(0x4E2D)))
    {
        u_indices.push_back(index);
        u_matches.push_back(matched);
    }
    ASSERT_EQ(u_indices.size(), 2u);
    EXPECT_EQ(u_indices[0], 0u);
    EXPECT_EQ(u_indices[1], 4u);
    EXPECT_EQ(u_matches[0], "中"_s);

    // empty pattern matches at every code point boundary
    std::vector<size_t> empty_indices;
    for (const auto& [index, matched] : "abc"_s.match_indices(""_s))
    {
        EXPECT_EQ(matched, ""_s);
        empty_indices.push_back(index);
    }
    ASSERT_EQ(empty_indices.size(), 4u);
    EXPECT_EQ(empty_indices[0], 0u);
    EXPECT_EQ(empty_indices[3], 3u);

    // no match yields an empty range
    size_t no_match_count = 0;
    for (const auto& [index, matched] : "abc"_s.match_indices("z"_s))
    {
        (void)index;
        (void)matched;
        no_match_count += 1;
    }
    EXPECT_EQ(no_match_count, 0u);

    // const char* and raw::String overloads
    size_t count = 0;
    for (const auto& [index, matched] : "a.b"_s.match_indices("."))
    {
        (void)index;
        (void)matched;
        count += 1;
    }
    EXPECT_EQ(count, 1u);
}

TEST(StringTest, StrSplitn)
{
    using namespace literal;

    auto s = "a,b,c"_s;

    {
        auto parts = collect_pieces(s.splitn(2, ','));
        ASSERT_EQ(parts.size(), 2u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], "b,c"_s);
    }
    {
        auto parts = collect_pieces(s.splitn(1, ','));
        ASSERT_EQ(parts.size(), 1u);
        EXPECT_EQ(parts[0], "a,b,c"_s);
    }
    EXPECT_EQ(collect_pieces(s.splitn(0, ',')).size(), 0u);
    {
        const auto parts = collect_pieces(s.splitn(5, ','));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], "b"_s);
        EXPECT_EQ(parts[2], "c"_s);
    }

    // trailing empty is kept
    {
        auto parts = collect_pieces("a,b,"_s.splitn(5, ','));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], "b"_s);
        EXPECT_EQ(parts[2], ""_s);
    }

    // reverse variants
    {
        auto parts = collect_pieces(s.rsplitn(2, ','));
        ASSERT_EQ(parts.size(), 2u);
        EXPECT_EQ(parts[0], "c"_s);
        EXPECT_EQ(parts[1], "a,b"_s);
    }
    {
        auto parts = collect_pieces(s.rsplitn(1, ','));
        ASSERT_EQ(parts.size(), 1u);
        EXPECT_EQ(parts[0], "a,b,c"_s);
    }

    // empty subject
    {
        auto parts = collect_pieces(""_s.splitn(3, ','));
        ASSERT_EQ(parts.size(), 1u);
        EXPECT_EQ(parts[0], ""_s);
    }

    // char pattern
    {
        auto parts = collect_pieces("a中b中c"_s.splitn(2, Char(0x4E2D)));
        ASSERT_EQ(parts.size(), 2u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], "b中c"_s);
    }

    // empty pattern
    {
        auto parts = collect_pieces("abc"_s.splitn(3, ""_s));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], ""_s);
        EXPECT_EQ(parts[1], "a"_s);
        EXPECT_EQ(parts[2], "bc"_s);
    }
}

TEST(StringTest, StrRsplit)
{
    using namespace literal;

    {
        auto parts = collect_pieces("a,b,"_s.rsplit(','));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], ""_s);
        EXPECT_EQ(parts[1], "b"_s);
        EXPECT_EQ(parts[2], "a"_s);
    }
    {
        auto parts = collect_pieces("abc"_s.rsplit(""_s));
        ASSERT_EQ(parts.size(), 5u);
        EXPECT_EQ(parts[0], ""_s);
        EXPECT_EQ(parts[1], "c"_s);
        EXPECT_EQ(parts[2], "b"_s);
        EXPECT_EQ(parts[3], "a"_s);
        EXPECT_EQ(parts[4], ""_s);
    }
    {
        auto parts = collect_pieces("Hello"_s.rsplit(','));
        ASSERT_EQ(parts.size(), 1u);
        EXPECT_EQ(parts[0], "Hello"_s);
    }
    {
        const auto by_cstr = collect_pieces("a,b"_s.rsplit(","));
        ASSERT_EQ(by_cstr.size(), 2u);
        EXPECT_EQ(by_cstr[0], "b"_s);
        EXPECT_EQ(by_cstr[1], "a"_s);

        const auto by_string = collect_pieces("a,b"_s.rsplit(String::from(",").unwrap()));
        ASSERT_EQ(by_string.size(), 2u);
        EXPECT_EQ(by_string[0], "b"_s);
        EXPECT_EQ(by_string[1], "a"_s);
    }
    {
        auto parts = collect_pieces("a中b中"_s.rsplit(Char(0x4E2D)));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], ""_s);
        EXPECT_EQ(parts[1], "b"_s);
        EXPECT_EQ(parts[2], "a"_s);
    }
}

TEST(StringTest, StrSplitTerminator)
{
    using namespace literal;

    // split_terminator drops a trailing empty part
    {
        auto parts = collect_pieces("a,b,"_s.split_terminator(','));
        ASSERT_EQ(parts.size(), 2u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], "b"_s);
    }
    {
        auto parts = collect_pieces(",a,b"_s.split_terminator(','));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], ""_s);
        EXPECT_EQ(parts[1], "a"_s);
        EXPECT_EQ(parts[2], "b"_s);
    }

    // rsplit_terminator behaves like rsplit but drops the first part when it is empty
    // (i.e. it is the reverse of split_terminator)
    {
        auto parts = collect_pieces(",a,b"_s.rsplit_terminator(','));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], "b"_s);
        EXPECT_EQ(parts[1], "a"_s);
        EXPECT_EQ(parts[2], ""_s);
    }
    {
        auto parts = collect_pieces("a,b,"_s.rsplit_terminator(','));
        ASSERT_EQ(parts.size(), 2u);
        EXPECT_EQ(parts[0], "b"_s);
        EXPECT_EQ(parts[1], "a"_s);
    }

    // empty pattern
    {
        auto parts = collect_pieces("abc"_s.split_terminator(""_s));
        ASSERT_EQ(parts.size(), 4u);
        EXPECT_EQ(parts[0], ""_s);
        EXPECT_EQ(parts[3], "c"_s);
    }
    {
        auto parts = collect_pieces("abc"_s.rsplit_terminator(""_s));
        ASSERT_EQ(parts.size(), 4u);
        EXPECT_EQ(parts[0], "c"_s);
        EXPECT_EQ(parts[3], ""_s);
    }

    EXPECT_EQ(collect_pieces(""_s.split_terminator(',')).size(), 0u);

    // const char* / raw::String / char overloads
    {
        const auto by_cstr = collect_pieces("a,b,"_s.split_terminator(","));
        ASSERT_EQ(by_cstr.size(), 2u);
        EXPECT_EQ(by_cstr[0], "a"_s);
        EXPECT_EQ(by_cstr[1], "b"_s);

        const auto by_string = collect_pieces("a,b,"_s.split_terminator(String::from(",").unwrap()));
        ASSERT_EQ(by_string.size(), 2u);
        EXPECT_EQ(by_string[0], "a"_s);
        EXPECT_EQ(by_string[1], "b"_s);

        const auto by_char = collect_pieces("a,b,"_s.split_terminator(','));
        ASSERT_EQ(by_char.size(), 2u);
        EXPECT_EQ(by_char[0], "a"_s);
        EXPECT_EQ(by_char[1], "b"_s);

        const auto reverse = collect_pieces("a,b,"_s.rsplit_terminator(","));
        ASSERT_EQ(reverse.size(), 2u);
        EXPECT_EQ(reverse[0], "b"_s);
        EXPECT_EQ(reverse[1], "a"_s);
    }
}

TEST(StringTest, StrSplitInclusive)
{
    using namespace literal;

    {
        auto parts = collect_pieces("a,b,"_s.split_inclusive(','));
        ASSERT_EQ(parts.size(), 2u);
        EXPECT_EQ(parts[0], "a,"_s);
        EXPECT_EQ(parts[1], "b,"_s);
    }
    {
        auto parts = collect_pieces("a,b,c"_s.split_inclusive(','));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], "a,"_s);
        EXPECT_EQ(parts[1], "b,"_s);
        EXPECT_EQ(parts[2], "c"_s);
    }
    {
        auto parts = collect_pieces(",a,b"_s.split_inclusive(','));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], ","_s);
        EXPECT_EQ(parts[1], "a,"_s);
        EXPECT_EQ(parts[2], "b"_s);
    }

    // rsplit_inclusive is split_inclusive reversed
    {
        auto parts = collect_pieces("a,b,c"_s.rsplit_inclusive(','));
        ASSERT_EQ(parts.size(), 3u);
        EXPECT_EQ(parts[0], "c"_s);
        EXPECT_EQ(parts[1], "b,"_s);
        EXPECT_EQ(parts[2], "a,"_s);
    }

    // empty pattern keeps every boundary as the head of a part
    {
        auto parts = collect_pieces("abc"_s.split_inclusive(""_s));
        ASSERT_EQ(parts.size(), 4u);
        EXPECT_EQ(parts[0], ""_s);
        EXPECT_EQ(parts[1], "a"_s);
        EXPECT_EQ(parts[2], "b"_s);
        EXPECT_EQ(parts[3], "c"_s);
    }

    EXPECT_EQ(collect_pieces(""_s.split_inclusive(',')).size(), 0u);

    // char pattern
    {
        auto parts = collect_pieces("a中b中"_s.split_inclusive(Char(0x4E2D)));
        ASSERT_EQ(parts.size(), 2u);
        EXPECT_EQ(parts[0], "a中"_s);
        EXPECT_EQ(parts[1], "b中"_s);
    }

    // const char* / raw::String overloads
    {
        const auto by_cstr = collect_pieces("a,b"_s.split_inclusive(","));
        ASSERT_EQ(by_cstr.size(), 2u);
        EXPECT_EQ(by_cstr[0], "a,"_s);
        EXPECT_EQ(by_cstr[1], "b"_s);

        const auto by_string = collect_pieces("a,b"_s.split_inclusive(String::from(",").unwrap()));
        ASSERT_EQ(by_string.size(), 2u);
        EXPECT_EQ(by_string[0], "a,"_s);
        EXPECT_EQ(by_string[1], "b"_s);
    }
}

TEST(StringTest, StrSplitWhitespace)
{
    using namespace literal;

    EXPECT_EQ(collect_pieces(""_s.split_whitespace()).size(), 0u);
    EXPECT_EQ(collect_pieces(" \t\n"_s.split_whitespace()).size(), 0u);

    {
        auto parts = collect_pieces("  a\u00A0b\tc\n d "_s.split_whitespace());
        ASSERT_EQ(parts.size(), 4u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], "b"_s);
        EXPECT_EQ(parts[2], "c"_s);
        EXPECT_EQ(parts[3], "d"_s);
    }

    // U+3000 ideographic space is Unicode whitespace
    {
        auto parts = collect_pieces("a\u3000b"_s.split_whitespace());
        ASSERT_EQ(parts.size(), 2u);
        EXPECT_EQ(parts[0], "a"_s);
        EXPECT_EQ(parts[1], "b"_s);
    }

    // unlike split_ascii_whitespace, NBSP is a separator here
    {
        const auto by_unicode = collect_pieces("a\u00A0b"_s.split_whitespace());
        ASSERT_EQ(by_unicode.size(), 2u);
        EXPECT_EQ(by_unicode[0], "a"_s);
        EXPECT_EQ(by_unicode[1], "b"_s);

        const auto by_ascii = collect_pieces("a\u00A0b"_s.split_ascii_whitespace());
        ASSERT_EQ(by_ascii.size(), 1u);
        EXPECT_EQ(by_ascii[0], "a\u00A0b"_s);
    }
}

TEST(StringTest, StrTrimUnicode)
{
    using namespace literal;

    // NBSP, U+2000 and U+3000 are stripped by the Unicode trim
    EXPECT_EQ("\u00A0\u2000abc\u3000\u0085"_s.trim(), "abc"_s);
    EXPECT_EQ("\u00A0abc "_s.trim_start(), "abc "_s);
    EXPECT_EQ(" abc\u3000"_s.trim_end(), " abc"_s);

    // trim_ascii leaves non-ASCII whitespace alone
    EXPECT_EQ("\u00A0abc\u00A0"_s.trim_ascii(), "\u00A0abc\u00A0"_s);

    // empty / unchanged / inner whitespace preserved
    EXPECT_EQ(""_s.trim(), ""_s);
    EXPECT_EQ("abc"_s.trim(), "abc"_s);
    EXPECT_EQ(" a b "_s.trim(), "a b"_s);
}

TEST(StringTest, StrTrimMatches)
{
    using namespace literal;

    auto s = "xxabcxx"_s;
    EXPECT_EQ(s.trim_matches('x'), "abc"_s);
    EXPECT_EQ(s.trim_start_matches('x'), "abcxx"_s);
    EXPECT_EQ(s.trim_end_matches('x'), "xxabc"_s);
    EXPECT_EQ(s.trim_matches(Char('x')), "abc"_s);

    // str pattern
    EXPECT_EQ("xyabcxy"_s.trim_matches("xy"_s), "abc"_s);

    // predicate
    EXPECT_EQ("abab"_s.trim_matches([](Char ch) { return ch == Char('a') || ch == Char('b'); }), ""_s);
    EXPECT_EQ("  x  "_s.trim_matches([](Char ch) { return ch.is_whitespace(); }), "x"_s);

    // multi-byte char
    EXPECT_EQ("中中abc中中"_s.trim_matches(Char(0x4E2D)), "abc"_s);

    // nothing to trim
    EXPECT_EQ("abc"_s.trim_matches('z'), "abc"_s);

    // const char* / raw::String overloads
    EXPECT_EQ("xxabcxx"_s.trim_matches("x"), "abc"_s);
    EXPECT_EQ("xxabcxx"_s.trim_matches(String::from("xx").unwrap()), "abc"_s);
}

TEST(StringTest, StrUnicodeCase)
{
    using namespace literal;

    EXPECT_EQ("HELLO"_s.to_lowercase(), "hello"_s);
    EXPECT_EQ("hello"_s.to_uppercase(), "HELLO"_s);
    EXPECT_EQ("Éé中"_s.to_lowercase(), "éé中"_s);
    EXPECT_EQ("éé中"_s.to_uppercase(), "ÉÉ中"_s);

    // simple 1:1 mapping: U+1E9E <-> U+00DF. Rust's full mapping would give "SS" for
    // `"ß".to_uppercase()`, while utf8proc yields U+1E9E.
    EXPECT_EQ("ẞ"_s.to_lowercase(), "ß"_s);
    EXPECT_EQ("ß"_s.to_uppercase(), "ẞ"_s);

    EXPECT_EQ(""_s.to_lowercase(), ""_s);
    EXPECT_EQ(""_s.to_uppercase(), ""_s);

    static_assert(std::same_as<decltype("abc"_s.to_lowercase()), String>);
}

TEST(StringTest, StrEscapes)
{
    using namespace literal;

    // escape_default escapes tab, both quotes, backslash and non-printable code points
    EXPECT_EQ("a\tb\"c'\\d"_s.escape_default(), "a\\tb\\\"c\\'\\\\d"_s);
    EXPECT_EQ("\u007F\u0080\u4E2D"_s.escape_default(), "\\u{7f}\\u{80}\\u{4e2d}"_s);
    EXPECT_EQ(" "_s.escape_default(), " "_s);

    // escape_debug keeps printable non-ASCII and writes NUL as \0
    EXPECT_EQ("a\tb\"c'\\d\u007F\u0080中"_s.escape_debug(), "a\\tb\\\"c\\'\\\\d\\u{7f}\\u{80}中"_s);
    const char nul[] = { '\0' };
    auto with_nul = str::from_raw_parts(nul, 1).unwrap();
    EXPECT_EQ(with_nul.escape_debug(), "\\0"_s);

    // escape_unicode escapes everything
    EXPECT_EQ("a\tb\u4E2D😀"_s.escape_unicode(), "\\u{61}\\u{9}\\u{62}\\u{4e2d}\\u{1f600}"_s);
    EXPECT_EQ(with_nul.escape_unicode(), "\\u{0}"_s);

    EXPECT_EQ(""_s.escape_default(), ""_s);
    EXPECT_EQ(""_s.escape_debug(), ""_s);
    EXPECT_EQ(""_s.escape_unicode(), ""_s);
}

// P0 #1: Char(std::uint32_t) is the only validating constructor and must panic
// on values that are not Unicode scalar values.
TEST(StringTest, CharInvalidScalarValuePanics)
{
    EXPECT_DEATH((void)Char(0x110000), "Invalid Unicode scalar value"); // > U+10FFFF
    EXPECT_DEATH((void)Char(0xD800), "Invalid Unicode scalar value");   // first surrogate
    EXPECT_DEATH((void)Char(0xDFFF), "Invalid Unicode scalar value");   // last surrogate

    // Neighbouring values must still be accepted.
    EXPECT_EQ(Char(0xD7FF).code_point(), 0xD7FFu);
    EXPECT_EQ(Char(0xE000).code_point(), 0xE000u);
    EXPECT_EQ(Char(0x10FFFF).code_point(), 0x10FFFFu);
}

// P0 #2: the _s literal validates UTF-8 and terminates on invalid input via
// Result::expect -> panic -> std::terminate.
TEST(StringTest, StrLiteralInvalidUtf8Panics)
{
    using namespace literal;

    EXPECT_DEATH((void)"\xC0\x80"_s, "Invalid UTF-8 sequence");       // overlong NUL
    EXPECT_DEATH((void)"\xED\xA0\x80"_s, "Invalid UTF-8 sequence");   // encoded surrogate
}

// P0 #4: StringHasher is exported for heterogeneous unordered lookup.
TEST(StringTest, StringHasherTransparent)
{
    using namespace literal;

    static_assert(std::same_as<StringHasher::is_transparent, void>);

    const StringHasher hasher;
    const std::string std_string = "hello";
    const std::string_view view = std_string;

    const auto expected = std::hash<std::string_view>{}(std::string_view("hello"));

    EXPECT_EQ(hasher("hello"), expected);
    EXPECT_EQ(hasher(std_string), expected);
    EXPECT_EQ(hasher(view), expected);
    EXPECT_EQ(hasher("hello"_s), expected);
    EXPECT_EQ(hasher(String("hello"_s)), expected);
}

TEST(StringTest, StringHasherHeterogeneousLookup)
{
    using namespace literal;

    std::unordered_map<str, int, StringHasher, std::equal_to<>> map;
    map.emplace("hello"_s, 1);

    // Every supported key type finds the str entry without constructing a str.
    EXPECT_EQ(map.count("hello"), 1u);
    EXPECT_EQ(map.count(std::string("hello")), 1u);
    EXPECT_EQ(map.count(std::string_view("hello")), 1u);
    EXPECT_EQ(map.count(String("hello"_s)), 1u);
    EXPECT_EQ(map.count("missing"), 0u);
}

TEST(StringTest, StrStringViewComparison)
{
    using namespace literal;

    const str s = "hello"_s;

    EXPECT_TRUE(s == std::string_view("hello"));
    EXPECT_TRUE(std::string_view("hello") == s);
    EXPECT_FALSE(s == std::string_view("hellO"));
    EXPECT_FALSE(s == std::string_view("hell"));
    EXPECT_FALSE(s == std::string_view("helloo"));
    EXPECT_FALSE(std::string_view("hell") == s);

    // Empty and default-constructed str compare equal to an empty view.
    EXPECT_TRUE(""_s == std::string_view{});
    EXPECT_TRUE(std::string_view{} == ""_s);

    const str empty;
    EXPECT_TRUE(empty == std::string_view{});
    EXPECT_TRUE(std::string_view{} == empty);
}

// P0 #5: stream insertion for str / Char / String.
TEST(StringTest, OperatorStreamInsertion)
{
    using namespace literal;

    {
        std::ostringstream os;
        os << "hi "_s;
        EXPECT_EQ(os.str(), "hi ");
    }

    {
        std::ostringstream os;
        os << String("hello"_s);
        EXPECT_EQ(os.str(), "hello");
    }

    {
        std::ostringstream os;
        os << Char(0x4E2D); // 'u' followed by the lowercase hex code point
        EXPECT_EQ(os.str(), "u4e2d");
    }

    {
        std::ostringstream os;
        os << "["_s << Char('A') << "]"_s;
        EXPECT_EQ(os.str(), "[u41]");
    }

    // Regression: Char's operator<< must not leak std::hex onto the stream it
    // writes to (it used to save/restore flags on std::cout instead).
    {
        std::ostringstream os;
        const auto before = os.flags();
        os << Char('A');
        EXPECT_EQ(os.flags(), before);
    }
}

// P0 #6: null / degenerate construction must be well defined.
TEST(StringTest, StrNullPointerHandling)
{
    using namespace literal;

    EXPECT_TRUE(str::from(nullptr).unwrap().empty());

    const auto empty = str::from_raw_parts(nullptr, 0);
    ASSERT_TRUE(empty.is_ok());
    EXPECT_TRUE(empty.unwrap().empty());

    const auto err = str::from_raw_parts(nullptr, 3);
    ASSERT_TRUE(err.is_err());
    EXPECT_EQ(err.unwrap_err().pos, 0u);

    // A default-constructed str is null-backed but behaves as empty.
    const str def;
    EXPECT_TRUE(def.empty());
    EXPECT_EQ(def.size(), 0u);
    EXPECT_TRUE(def.as_bytes().empty());
    EXPECT_TRUE(def.trim_ascii().empty());
    EXPECT_EQ(def, ""_s);
    EXPECT_TRUE(def.parse<int>().is_err());
    EXPECT_TRUE(def.to_string().empty());
}

// P1 #7: the ASCII classification family. These seven predicates had no tests.
TEST(StringTest, CharAsciiClassification)
{
    // is_ascii_alphabetic
    EXPECT_TRUE(Char('a').is_ascii_alphabetic());
    EXPECT_TRUE(Char('z').is_ascii_alphabetic());
    EXPECT_TRUE(Char('A').is_ascii_alphabetic());
    EXPECT_TRUE(Char('Z').is_ascii_alphabetic());
    EXPECT_FALSE(Char('0').is_ascii_alphabetic());
    EXPECT_FALSE(Char(0x40).is_ascii_alphabetic()); // '@', just before 'A'
    EXPECT_FALSE(Char(0x5B).is_ascii_alphabetic()); // '[', just after 'Z'
    EXPECT_FALSE(Char(0x60).is_ascii_alphabetic()); // '`', just before 'a'
    EXPECT_FALSE(Char(0x7B).is_ascii_alphabetic()); // '{', just after 'z'
    EXPECT_FALSE(Char(0x00C9).is_ascii_alphabetic()); // É

    // is_ascii_alphanumeric
    EXPECT_TRUE(Char('0').is_ascii_alphanumeric());
    EXPECT_TRUE(Char('9').is_ascii_alphanumeric());
    EXPECT_TRUE(Char('a').is_ascii_alphanumeric());
    EXPECT_TRUE(Char('Z').is_ascii_alphanumeric());
    EXPECT_FALSE(Char('_').is_ascii_alphanumeric());
    EXPECT_FALSE(Char(' ').is_ascii_alphanumeric());
    EXPECT_FALSE(Char('@').is_ascii_alphanumeric());

    // is_ascii_lowercase / is_ascii_uppercase
    EXPECT_TRUE(Char('a').is_ascii_lowercase());
    EXPECT_TRUE(Char('z').is_ascii_lowercase());
    EXPECT_FALSE(Char('A').is_ascii_lowercase());
    EXPECT_FALSE(Char(0x80).is_ascii_lowercase());
    EXPECT_TRUE(Char('A').is_ascii_uppercase());
    EXPECT_TRUE(Char('Z').is_ascii_uppercase());
    EXPECT_FALSE(Char('a').is_ascii_uppercase());
    EXPECT_FALSE(Char(0x80).is_ascii_uppercase());

    // is_ascii_control
    EXPECT_TRUE(Char(0x00).is_ascii_control());
    EXPECT_TRUE(Char(0x1F).is_ascii_control());
    EXPECT_TRUE(Char(0x7F).is_ascii_control());
    EXPECT_FALSE(Char(0x20).is_ascii_control());
    EXPECT_FALSE(Char(0x80).is_ascii_control());

    // is_ascii_digit
    EXPECT_TRUE(Char('0').is_ascii_digit());
    EXPECT_TRUE(Char('9').is_ascii_digit());
    EXPECT_FALSE(Char(0x2F).is_ascii_digit()); // '/', just before '0'
    EXPECT_FALSE(Char(0x3A).is_ascii_digit()); // ':', just after '9'

    // is_ascii_hexdigit
    EXPECT_TRUE(Char('0').is_ascii_hexdigit());
    EXPECT_TRUE(Char('9').is_ascii_hexdigit());
    EXPECT_TRUE(Char('a').is_ascii_hexdigit());
    EXPECT_TRUE(Char('f').is_ascii_hexdigit());
    EXPECT_TRUE(Char('A').is_ascii_hexdigit());
    EXPECT_TRUE(Char('F').is_ascii_hexdigit());
    EXPECT_FALSE(Char('g').is_ascii_hexdigit());
    EXPECT_FALSE(Char('G').is_ascii_hexdigit());
    EXPECT_FALSE(Char(0x60).is_ascii_hexdigit()); // '`'
}

// P1 #8: matches(raw::String) plus the Char/char overloads that were missing.
TEST(StringTest, MatchesOverloads)
{
    using namespace literal;

    const str text = "ababab"_s;

    // matches(const raw::String&)
    std::vector<str> from_owned;
    for (const auto match : text.matches(String("ab"_s)))
    {
        from_owned.push_back(match);
    }
    EXPECT_EQ(from_owned, (std::vector<str>{"ab"_s, "ab"_s, "ab"_s}));

    // matches(Char)
    std::vector<str> from_char;
    for (const auto match : "a.b.a"_s.matches(Char('.')))
    {
        from_char.push_back(match);
    }
    EXPECT_EQ(from_char, (std::vector<str>{"."_s, "."_s}));

    // matches(char) agrees, and a multi-byte Char works too
    std::vector<str> from_plain_char;
    for (const auto match : "a.b.a"_s.matches('.'))
    {
        from_plain_char.push_back(match);
    }
    EXPECT_EQ(from_plain_char, from_char);

    std::vector<str> from_cjk;
    for (const auto match : "中a中"_s.matches(Char(0x4E2D)))
    {
        from_cjk.push_back(match);
    }
    EXPECT_EQ(from_cjk, (std::vector<str>{"中"_s, "中"_s}));
}

// rmatches yields the same disjoint matches as matches, found from the right.
// Since every match of a fixed pattern is the same text, the two differ only in
// iteration order, which is not observable from the slices alone (reverse
// ordering is covered by the rmatch_indices tests).
TEST(StringTest, RMatches)
{
    using namespace literal;

    const str text = "ababab"_s;

    std::vector<str> forward;
    for (const auto match : text.matches("ab"))
    {
        forward.push_back(match);
    }

    // str / const char* / raw::String overloads all agree with matches
    std::vector<str> by_str;
    for (const auto match : text.rmatches("ab"_s))
    {
        by_str.push_back(match);
    }
    EXPECT_EQ(by_str, forward);

    std::vector<str> by_cstr;
    for (const auto match : text.rmatches("ab"))
    {
        by_cstr.push_back(match);
    }
    EXPECT_EQ(by_cstr, forward);

    std::vector<str> by_owned;
    for (const auto match : text.rmatches(String("ab"_s)))
    {
        by_owned.push_back(match);
    }
    EXPECT_EQ(by_owned, forward);

    // Char / char overloads
    std::vector<str> by_char;
    for (const auto match : "a.b.a"_s.rmatches(Char('.')))
    {
        by_char.push_back(match);
    }
    EXPECT_EQ(by_char, (std::vector<str>{"."_s, "."_s}));

    std::vector<str> by_plain_char;
    for (const auto match : "a.b.a"_s.rmatches('.'))
    {
        by_plain_char.push_back(match);
    }
    EXPECT_EQ(by_plain_char, by_char);

    // empty pattern yields one empty match per boundary, from the right
    std::vector<str> empty_matches;
    for (const auto match : "abc"_s.rmatches(""_s))
    {
        empty_matches.push_back(match);
    }
    EXPECT_EQ(empty_matches, (std::vector<str>{""_s, ""_s, ""_s, ""_s}));

    // no matches yields an empty range
    const auto none = "abc"_s;
    EXPECT_EQ(none.rmatches("xyz").begin(), none.rmatches("xyz").end());
}

// P1 #8: every rmatch_indices overload was untested (only the const char* one
// was exercised, via "ab" at the StrMatchIndices test).
TEST(StringTest, RMatchIndicesOverloads)
{
    using namespace literal;

    const str s = "ababab"_s;

    std::vector<size_t> by_str;
    for (const auto& [index, matched] : s.rmatch_indices("ab"_s))
    {
        EXPECT_EQ(matched, "ab"_s);
        by_str.push_back(index);
    }
    EXPECT_EQ(by_str, (std::vector<size_t>{4u, 2u, 0u}));

    std::vector<size_t> by_owned;
    for (const auto& [index, matched] : s.rmatch_indices(String("ab"_s)))
    {
        EXPECT_EQ(matched, "ab"_s);
        by_owned.push_back(index);
    }
    EXPECT_EQ(by_owned, by_str);

    std::vector<size_t> by_char;
    for (const auto& [index, matched] : "中a中"_s.rmatch_indices(Char(0x4E2D)))
    {
        EXPECT_EQ(matched, "中"_s);
        by_char.push_back(index);
    }
    EXPECT_EQ(by_char, (std::vector<size_t>{4u, 0u}));

    std::vector<size_t> by_plain_char;
    for (const auto& [index, matched] : "a.b"_s.rmatch_indices('.'))
    {
        (void)matched;
        by_plain_char.push_back(index);
    }
    EXPECT_EQ(by_plain_char, (std::vector<size_t>{1u}));
}

// P1 #9: U+000B VERTICAL TAB has the Unicode White_Space property, so (like
// Rust's '\u{b}'.is_whitespace()) it counts as Unicode whitespace, even though
// it is not ASCII whitespace.
TEST(StringTest, VerticalTabIsWhitespace)
{
    using namespace literal;

    EXPECT_TRUE(Char(0x0B).is_whitespace());
    EXPECT_FALSE(Char(0x0B).is_ascii_whitespace()); // the narrower ASCII notion
    EXPECT_TRUE(Char(0x09).is_whitespace()); // HT (ASCII)
    EXPECT_TRUE(Char(0x0C).is_whitespace()); // FF (ASCII)

    EXPECT_EQ("\va\v"_s.trim(), "a"_s);
    EXPECT_EQ("\va\v"_s.trim_start(), "a\v"_s);
    EXPECT_EQ("\va\v"_s.trim_end(), "\va"_s);

    const auto pieces = collect_pieces("\va\v"_s.split_whitespace());
    ASSERT_EQ(pieces.size(), 1u);
    EXPECT_EQ(pieces[0], "a"_s);

    // trim_ascii stays ASCII-only, so it leaves VT in place (matches Rust's
    // `trim_ascii`).
    EXPECT_EQ("\va\v"_s.trim_ascii(), "\va\v"_s);
}

// P1 #10: a trailing lone \r is not a line ending and must be kept (matches
// Rust). Only the mid-string case ("a\rb") was tested before.
TEST(StringTest, LinesTrailingCarriageReturn)
{
    using namespace literal;

    {
        const auto lines = collect_pieces("a\r"_s.lines());
        ASSERT_EQ(lines.size(), 1u);
        EXPECT_EQ(lines[0], "a\r"_s);
    }
    {
        const auto lines = collect_pieces("\r"_s.lines());
        ASSERT_EQ(lines.size(), 1u);
        EXPECT_EQ(lines[0], "\r"_s);
    }
    {
        // "\r\n" is one CRLF ending -> a single empty line, not zero lines.
        const auto lines = collect_pieces("\r\n"_s.lines());
        ASSERT_EQ(lines.size(), 1u);
        EXPECT_EQ(lines[0], ""_s);
    }
    {
        // A "\r\n" ending followed by a trailing lone "\r".
        const auto lines = collect_pieces("a\r\n\r"_s.lines());
        ASSERT_EQ(lines.size(), 2u);
        EXPECT_EQ(lines[0], "a"_s);
        EXPECT_EQ(lines[1], "\r"_s);
    }
}

// P1 #11: the backward empty-pattern path. split_inclusive("") is tested, but
// rsplit_inclusive("") / rsplitn(n, "") were not. rsplit_inclusive is a
// crate extension (Rust's str has no such method); the expected values below
// were checked against Rust for split_inclusive and rsplitn.
TEST(StringTest, RsplitInclusiveAndRsplitnEmptyPattern)
{
    using namespace literal;

    EXPECT_EQ(collect_pieces("abc"_s.split_inclusive(""_s)),
              (std::vector<str>{""_s, "a"_s, "b"_s, "c"_s}));

    EXPECT_EQ(collect_pieces("abc"_s.rsplit_inclusive(""_s)),
              (std::vector<str>{"c"_s, "b"_s, "a"_s, ""_s}));

    EXPECT_EQ(collect_pieces("abc"_s.rsplitn(2, ""_s)),
              (std::vector<str>{""_s, "abc"_s}));
    EXPECT_EQ(collect_pieces("abc"_s.rsplitn(1, ""_s)),
              (std::vector<str>{"abc"_s}));
    EXPECT_EQ(collect_pieces("abc"_s.rsplitn(4, ""_s)),
              (std::vector<str>{""_s, "c"_s, "b"_s, "a"_s}));
    EXPECT_EQ(collect_pieces("abc"_s.rsplitn(9, ""_s)),
              (std::vector<str>{""_s, "c"_s, "b"_s, "a"_s, ""_s}));

    EXPECT_EQ(collect_pieces(""_s.rsplit_inclusive(""_s)),
              (std::vector<str>{""_s}));
}

// P1 #12: FromUtf8Error::pos at a non-zero offset (only position 0 was tested).
TEST(StringTest, FromUtf8ErrorPosition)
{
    using namespace literal;

    // "ab" then an overlong NUL encoding -> position 2
    const auto overlong = str::from_raw_parts("ab\xC0\x80", 4);
    ASSERT_TRUE(overlong.is_err());
    EXPECT_EQ(overlong.unwrap_err().pos, 2u);

    // 'a' then an encoded surrogate -> position 1
    const auto surrogate = str::from_raw_parts("a\xED\xA0\x80", 4);
    ASSERT_TRUE(surrogate.is_err());
    EXPECT_EQ(surrogate.unwrap_err().pos, 1u);

    // a valid 3-byte character before the bad byte -> position 3
    const auto after_cjk = str::from_raw_parts("中\xC0\x80", 5);
    ASSERT_TRUE(after_cjk.is_err());
    EXPECT_EQ(after_cjk.unwrap_err().pos, 3u);

    // all valid -> Ok
    EXPECT_TRUE(str::from_raw_parts("\xE4\xB8\xAD", 3).is_ok());
}

// P1 #13: join_with overloads (only the const char* one was covered), plus the
// empty-range and single-element cases.
TEST(StringTest, JoinWithOverloads)
{
    using namespace literal;

    const std::vector<str> parts = {"a"_s, "b"_s, "c"_s};

    EXPECT_EQ(parts | strings::join_with('-') | std::ranges::to<String>(), "a-b-c"_s);
    EXPECT_EQ(parts | strings::join_with("-"_s) | std::ranges::to<String>(), "a-b-c"_s);
    EXPECT_EQ(parts | strings::join_with(String("-"_s)) | std::ranges::to<String>(), "a-b-c"_s);
    EXPECT_EQ(parts | strings::join_with(".") | std::ranges::to<String>(), "a.b.c"_s);

    const std::vector<str> single = {"x"_s};
    EXPECT_EQ(single | strings::join_with('-') | std::ranges::to<String>(), "x"_s);

    const std::vector<str> none;
    EXPECT_EQ(none | strings::join_with('-') | std::ranges::to<String>(), ""_s);
}

// P1 #14: grapheme cluster content, not just counts.
TEST(StringTest, GraphemeClusterContent)
{
    using namespace literal;

    // Emoji ZWJ sequence: a single 25-byte cluster (4 code points + 3 ZWJ).
    {
        const auto clusters = collect_pieces("👨‍👩‍👧‍👦"_s.graphemes());
        ASSERT_EQ(clusters.size(), 1u);
        EXPECT_EQ(clusters[0], "👨‍👩‍👧‍👦"_s);
        EXPECT_EQ(clusters[0].size(), 25u);
    }

    // Flag: two regional indicators form a single 8-byte cluster.
    {
        const auto clusters = collect_pieces("🇺🇸"_s.graphemes());
        ASSERT_EQ(clusters.size(), 1u);
        EXPECT_EQ(clusters[0], "🇺🇸"_s);
        EXPECT_EQ(clusters[0].size(), 8u);
    }

    // CRLF is a single cluster.
    {
        const auto clusters = collect_pieces("\r\n"_s.graphemes());
        ASSERT_EQ(clusters.size(), 1u);
        EXPECT_EQ(clusters[0], "\r\n"_s);
    }

    // Base + combining mark is one cluster, asserted by content.
    {
        const auto clusters = collect_pieces("y\u0306"_s.graphemes());
        ASSERT_EQ(clusters.size(), 1u);
        EXPECT_EQ(clusters[0], "y\u0306"_s);
    }
}

// P1 #15: escape_default for \r and \n (only \t and other escapes were covered).
TEST(StringTest, EscapeDefaultCarriageReturnAndNewline)
{
    using namespace literal;

    EXPECT_EQ("\r"_s.escape_default(), "\\r"_s);
    EXPECT_EQ("\n"_s.escape_default(), "\\n"_s);
    EXPECT_EQ("a\r\nb"_s.escape_default(), "a\\r\\nb"_s);

    // escape_debug uses the same short escapes.
    EXPECT_EQ("\r"_s.escape_debug(), "\\r"_s);
    EXPECT_EQ("\n"_s.escape_debug(), "\\n"_s);
}

// P2: parse<bool> goes through an explicit specialization (std::from_chars has
// no bool overload). Only the exact strings "true"/"false" are accepted,
// matching Rust's `str::parse::<bool>()`.
TEST(StringTest, StrParseBool)
{
    using namespace literal;

    EXPECT_TRUE("true"_s.parse<bool>().is_ok());
    EXPECT_EQ("true"_s.parse<bool>().unwrap(), true);
    EXPECT_TRUE("false"_s.parse<bool>().is_ok());
    EXPECT_EQ("false"_s.parse<bool>().unwrap(), false);

    // Case sensitive, no surrounding whitespace, no numeric spellings
    EXPECT_TRUE("TRUE"_s.parse<bool>().is_err());
    EXPECT_TRUE("False"_s.parse<bool>().is_err());
    EXPECT_TRUE("1"_s.parse<bool>().is_err());
    EXPECT_TRUE("0"_s.parse<bool>().is_err());
    EXPECT_TRUE("yes"_s.parse<bool>().is_err());
    EXPECT_TRUE(" true"_s.parse<bool>().is_err());
    EXPECT_TRUE("true "_s.parse<bool>().is_err());
    EXPECT_TRUE("truex"_s.parse<bool>().is_err());

    // Empty input and junk both report std::errc::invalid_argument
    EXPECT_TRUE(""_s.parse<bool>().is_err());
    EXPECT_EQ(""_s.parse<bool>().unwrap_err().ec, std::errc::invalid_argument);
    EXPECT_EQ("yes"_s.parse<bool>().unwrap_err().ec, std::errc::invalid_argument);
}

// P2: `parse<unsigned>`, including the negative-input errc behaviour.
TEST(StringTest, StrParseUnsigned)
{
    using namespace literal;

    EXPECT_EQ("0"_s.parse<unsigned>().unwrap(), 0u);
    EXPECT_EQ("42"_s.parse<unsigned>().unwrap(), 42u);

    const auto negative = "-1"_s.parse<unsigned>();
    ASSERT_TRUE(negative.is_err());
    EXPECT_EQ(negative.unwrap_err().ec, std::errc::invalid_argument);

    const auto overflow = "99999999999999999999999999999"_s.parse<unsigned>();
    ASSERT_TRUE(overflow.is_err());
    EXPECT_EQ(overflow.unwrap_err().ec, std::errc::result_out_of_range);

    EXPECT_TRUE("12abc"_s.parse<unsigned>().is_err());
    EXPECT_TRUE(""_s.parse<unsigned>().is_err());
    EXPECT_TRUE(" "_s.parse<unsigned>().is_err());
}

// P2: a direct round-trip through `from_bytes_unchecked`; it had only ever been
// used as an iterator-to-str helper inside the chars/split tests.
TEST(StringTest, StrFromBytesUncheckedRoundTrip)
{
    using namespace literal;

    const auto original = "你好, world"_s;

    const auto rebuilt = str::from_bytes_unchecked(original.data(), original.size());
    EXPECT_EQ(rebuilt, original);
    EXPECT_EQ(rebuilt.size(), original.size());

    // A sub-slice round-trips to exactly the slice, not to its parent.
    const auto part = original.slice(0, 7); // "你好,"
    EXPECT_EQ(part, "你好,"_s);
    EXPECT_EQ(str::from_bytes_unchecked(part.data(), part.size()), "你好,"_s);

    // Empty, null-backed input round-trips to an empty str.
    EXPECT_TRUE(str::from_bytes_unchecked(nullptr, 0).empty());
}

// P2: the allocator template parameters of the allocating transforms are public
// API (they were exercised for `raw::String` but never for `str`).
TEST(StringTest, StrAllocatorTransforms)
{
    using namespace literal;

    auto owned = "Hello"_s.to_string<TestAllocator>();
    static_assert(std::same_as<decltype(owned), raw::String<TestAllocator>>);
    EXPECT_EQ(owned.as_str(), "Hello"_s);

    auto replaced = "a中b中c"_s.replace<TestAllocator>("中"_s, "X"_s);
    static_assert(std::same_as<decltype(replaced), raw::String<TestAllocator>>);
    EXPECT_EQ(replaced.as_str(), "aXbXc"_s);
}

#endif
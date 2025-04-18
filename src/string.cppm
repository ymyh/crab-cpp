module;

#include "include/utf8proc.h"
#define offsetof(t, d) __builtin_offsetof(t, d)

export module crab_cpp:string;

import :panic;
import :option;
import :result;
import std;

/**
 * @brief A compressed pair implementation that optimizes storage for empty types
 * @tparam T1 First type
 * @tparam T2 Second type
 * @tparam IsEmpty Whether T1 is empty and not final
 */
template<typename T1, typename T2, bool = std::is_empty_v<T1> && !std::is_final_v<T1>>
struct compressed_pair : private T1
{
    T2 second;

    constexpr compressed_pair() noexcept : T1(), second() {}
    constexpr compressed_pair(const T1& x, const T2& y) noexcept : T1(x), second(y) {}

    [[nodiscard]] constexpr auto first() noexcept -> T1& { return *this; }
    [[nodiscard]] constexpr auto first() const noexcept -> const T1& { return *this; }

    constexpr auto offsetof_second() const noexcept -> std::size_t { return 0; }
};

template<typename T1, typename T2>
struct compressed_pair<T1, T2, false>
{
    T1 first;
    T2 second;

    constexpr compressed_pair(T1& x, const T2& y) noexcept : first(x), second(y) {}

    [[nodiscard]] constexpr auto get_first() noexcept -> T1& { return first; }
    [[nodiscard]] constexpr auto get_first() const noexcept -> const T1& { return first; }

    [[nodiscard]] constexpr auto offsetof_second() const noexcept -> std::size_t { return offsetof(compressed_pair, T2); }
};

struct plain_str
{
    const std::byte* data = nullptr;
    size_t len = 0;

    explicit constexpr plain_str() noexcept = default;

    constexpr plain_str(const plain_str&) = default;

    explicit constexpr plain_str(const std::byte* data, size_t len) noexcept : data(data), len(len) {}

public:
    auto operator=(const plain_str& other) noexcept -> plain_str& = default;

public:
    struct Iter
    {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::byte;
        using pointer = const std::byte*;
        using reference = const std::byte&;

    public:
        const std::byte* ptr = nullptr;

    public:
        explicit Iter() = default;

        explicit Iter(const std::byte* ptr) : ptr(ptr) {}

    public:
        constexpr auto operator++() noexcept -> Iter&
        {
            this->ptr += 1;
            return *this;
        }

        constexpr auto operator++(int) noexcept -> Iter
        {
            auto temp = *this;
            this->ptr += 1;

            return temp;
        }

        auto operator*() const -> const std::byte&
        {
            return *const_cast<std::byte*>(this->ptr);
        }

        auto operator->() const -> const std::byte*
        {
            return const_cast<std::byte*>(this->ptr);
        }

        constexpr auto operator<=>(const Iter&) const noexcept -> std::strong_ordering = default;
    };

    constexpr auto begin() const noexcept -> Iter
    {
        return Iter(this->data);
    }

    constexpr auto end() const noexcept -> Iter
    {
        return Iter(this->data + this->len);
    }

    [[nodiscard]] constexpr auto operator==(const plain_str& other) const noexcept -> bool = default;

    using iterator = Iter;
    using const_iterator = Iter;
};

export namespace crab_cpp
{

struct FromUtf8Error
{
    /**
     * @brief The position where the invalid UTF-8 sequence was found
     */
    std::size_t pos;

    /**
     * @brief Constructs a FromUtf8Error with the given position
     */
    constexpr explicit FromUtf8Error(std::size_t pos) noexcept : pos(pos) {}
};

/**
 * @brief Validates if the given string is valid UTF-8
 * @param str The string to validate
 * @param len The length of the string in bytes
 * @return true if the string is valid UTF-8
 */
[[nodiscard]] constexpr auto is_valid_utf8(const std::byte* str, std::size_t len) noexcept -> bool
{
    if (str == nullptr || len == 0)
    {
        return true;
    }

    utf8proc_int32_t codepoint;
    std::size_t pos = 0;

    while (pos < len)
    {
        const auto advance = utf8proc_iterate(
            reinterpret_cast<const utf8proc_uint8_t*>(str + pos),
            len - pos,
            &codepoint
        );

        if (advance < 0)
        {
            return false;
        }
        pos += advance;
    }
    return true;
}

[[nodiscard]] constexpr auto is_valid_utf8(const char* str, std::size_t len) noexcept -> bool
{
    return is_valid_utf8(std::bit_cast<const std::byte*>(str), len);
}

/**
 * @brief A class representing a Unicode scalar value
 */
struct Char
{
private:
    std::uint32_t m_value;

public:
    /**
     * @brief Default constructor
     */
    constexpr explicit Char() noexcept : m_value(0) {}

    /**
     * @brief Constructs a Char from a Unicode scalar value
     * @param value The Unicode scalar value
     * @panics If the value is not a valid Unicode scalar value
     */
    constexpr explicit Char(std::uint32_t value)
    {
        if (!is_valid_scalar_value(value))
        {
            panic("Invalid Unicode scalar value");
        }
        this->m_value = value;
    }

private:
    /**
     * @brief Checks if a value is a valid Unicode scalar value
     * @param value The value to check
     * @return true if the value is a valid Unicode scalar value
     */
    [[nodiscard]] static constexpr auto is_valid_scalar_value(std::uint32_t value) noexcept -> bool
    {
        // Unicode scalar values are in the range [0, 0x10FFFF] excluding surrogate pairs
        return value <= 0x10FFFF && (value < 0xD800 || value > 0xDFFF);
    }

public:
    /**
     * @brief Returns the Unicode scalar value of this char
     * @return The Unicode scalar value
     */
    [[nodiscard]] constexpr auto code_point() const noexcept -> std::uint32_t
    {
        return this->m_value;
    }

    /**
     * @brief Returns true if this char is an ASCII character
     * @return true if this char is an ASCII character
     */
    [[nodiscard]] constexpr auto is_ascii() const noexcept -> bool
    {
        return this->m_value <= 0x7F;
    }

    /**
     * @brief Returns true if this char is an ASCII alphabetic character
     * @return true if this char is an ASCII alphabetic character
     */
    [[nodiscard]] constexpr auto is_ascii_alphabetic() const noexcept -> bool
    {
        return (this->m_value >= static_cast<std::uint32_t>('a') && this->m_value <= static_cast<std::uint32_t>('z')) ||
               (this->m_value >= static_cast<std::uint32_t>('A') && this->m_value <= static_cast<std::uint32_t>('Z'));
    }

    /**
     * @brief Returns true if this char is an ASCII alphanumeric character
     * @return true if this char is an ASCII alphanumeric character
     */
    [[nodiscard]] constexpr auto is_ascii_alphanumeric() const noexcept -> bool
    {
        return (this->m_value >= static_cast<std::uint32_t>('0') && this->m_value <= static_cast<std::uint32_t>('9')) ||
               (this->m_value >= static_cast<std::uint32_t>('a') && this->m_value <= static_cast<std::uint32_t>('z')) ||
               (this->m_value >= static_cast<std::uint32_t>('A') && this->m_value <= static_cast<std::uint32_t>('Z'));
    }

    /**
     * @brief Returns true if this char is an ASCII lowercase character
     * @return true if this char is an ASCII lowercase character
     */
    [[nodiscard]] constexpr auto is_ascii_lowercase() const noexcept -> bool
    {
        return this->m_value >= static_cast<std::uint32_t>('a') && this->m_value <= static_cast<std::uint32_t>('z');
    }

    /**
     * @brief Returns true if this char is an ASCII uppercase character
     * @return true if this char is an ASCII uppercase character
     */
    [[nodiscard]] constexpr auto is_ascii_uppercase() const noexcept -> bool
    {
        return this->m_value >= static_cast<std::uint32_t>('A') && this->m_value <= static_cast<std::uint32_t>('Z');
    }

    /**
     * @brief Returns true if this char is an ASCII control character
     * @details Checks if the value is an ASCII control character: U+0000 NUL ..= U+001F UNIT SEPARATOR, or U+007F DELETE.
     *          Note that most ASCII whitespace characters are control characters, but SPACE is not.
     * @return true if this char is an ASCII control character
     */
    [[nodiscard]] constexpr auto is_ascii_control() const noexcept -> bool
    {
        return (this->m_value <= 0x1F) || (this->m_value == 0x7F);
    }

    /**
     * @brief Returns true if this char is an ASCII whitespace character
     * @details Checks if the value is an ASCII whitespace character:
     *          - U+0020 SPACE
     *          - U+0009 HORIZONTAL TAB
     *          - U+000A LINE FEED
     *          - U+000C FORM FEED
     *          - U+000D CARRIAGE RETURN
     * @return true if this char is an ASCII whitespace character
     */
    [[nodiscard]] constexpr auto is_ascii_whitespace() const noexcept -> bool
    {
        return (this->m_value == 0x20) || // SPACE
               (this->m_value == 0x09) || // HORIZONTAL TAB
               (this->m_value == 0x0A) || // LINE FEED
               (this->m_value == 0x0C) || // FORM FEED
               (this->m_value == 0x0D);   // CARRIAGE RETURN
    }
};

namespace raw
{

template<typename Alloc>
struct String;

}

struct str
{
    using pointer = const std::byte*;

private:
    pointer m_data;
    std::size_t m_len;

    /**
     * @brief Private constructor for internal use
     */
    constexpr str(pointer data, std::size_t len) noexcept : m_data(data), m_len(len) {}

public:
    /**
     * @brief Default constructor
     */
    constexpr str() noexcept : m_data(nullptr), m_len(0) {}

    /**
     * @brief Constructs a str from a plain_str
     * @param s The plain_str to construct from
     */
    constexpr str(const plain_str& s) noexcept : m_data(s.data), m_len(s.len) {}

    constexpr str(char ch) noexcept {}

public:
    constexpr auto begin() const noexcept -> const std::byte*
    {
        return reinterpret_cast<const std::byte*>(this->m_data);
    }

    constexpr auto end() const noexcept -> const std::byte*
    {
        return reinterpret_cast<const std::byte*>(this->m_data + this->m_len);
    }

    /**
     * @brief Creates a string view from a pointer and length
     * @param data Pointer to string data
     * @param len Length of the string
     * @return A Result containing either a str or a FromUtf8Error
     */
    [[nodiscard]] static constexpr auto from_raw_parts(const char* data, std::size_t len) noexcept -> Result<str, FromUtf8Error>
    {
        if (data == nullptr && len > 0)
        {
            return FromUtf8Error(0);
        }

        if (!is_valid_utf8(data, len))
        {
            // Find the position of the invalid UTF-8 sequence
            utf8proc_int32_t codepoint;
            std::size_t pos = 0;

            while (pos < len)
            {
                const auto result = utf8proc_iterate(
                    reinterpret_cast<const utf8proc_uint8_t*>(data + pos),
                    len - pos,
                    &codepoint
                );

                if (result < 0)
                {
                    return FromUtf8Error(pos);
                }
                pos += result;
            }
        }

        return str(std::bit_cast<pointer>(data), len);
    }

    /**
     * @brief Constructs a str from a pointer and length
     * @param data Pointer to string data
     * @param len Length of the string
     * @return A str containing the string data
     */
    [[nodiscard]] static constexpr auto from_bytes_unchecked(pointer data, std::size_t len) noexcept -> str
    {
        return str(data, len);
    }

    /**
     * @brief Creates a string view from a null-terminated string
     * @param data Pointer to null-terminated string
     * @return A Result containing either a str or a FromUtf8Error
     */
    [[nodiscard]] static constexpr auto from(const char* data) noexcept -> Result<str, FromUtf8Error>
    {
        return str::from_raw_parts(data, data != nullptr ? std::strlen(data) : 0);
    }

public:
    /**
     * @brief Returns a byte slice of this str's contents
     * @return A span containing the string's bytes
     */
    [[nodiscard]] constexpr auto as_bytes() const noexcept -> std::span<const std::byte>
    {
        return std::span<const std::byte>(this->m_data, this->m_len);
    }

    /**
     * @brief Returns C-style string of this str's contents
     * @return A span containing the string's bytes
     */
    [[nodiscard]] constexpr auto as_raw() const noexcept -> const char*
    {
        return std::bit_cast<const char*>(this->m_data);
    }

    /**
     * @brief Checks if this string contains the given pattern
     * @param pattern The pattern to search for
     * @return true if this string contains the pattern
     */
    [[nodiscard]] constexpr auto contains(const str& pattern) const noexcept -> bool
    {
        if (pattern.m_len > this->m_len)
        {
            return false;
        }

        if (pattern.m_len == 0)
        {
            return true;
        }

        // Create spans for the current string and pattern
        std::span<const std::byte> haystack(this->m_data, this->m_len);
        std::span<const std::byte> needle(pattern.m_data, pattern.m_len);

        // Use std::ranges::search to find the pattern
        auto result = std::ranges::search(haystack, needle);
        return !result.empty();
    }

    /**
     * @brief Returns a pointer to the string data
     */
    [[nodiscard]] constexpr auto data() const noexcept -> pointer
    {
        return this->m_data;
    }

    /**
     * @brief Returns if the string is empty
     */
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return this->m_len == 0;
    }

    /**
     * @brief Creates a new String by repeating a string n times.
     * @param n The number of times to repeat the string
     * @panics This function will panic if the capacity would overflow.
     */
    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto repeat(size_t n) const -> raw::String<Alloc>
    {
        if (n > std::numeric_limits<std::size_t>::max() / this->m_len)
        {
            panic("Repeat times overflow");
        }

        auto string = raw::String<Alloc>();
        // string.clear();
        string.reserve(this->m_len * n);

        for (size_t i = 0; i < n; i += 1)
        {
            string.push_str(*this);
        }

        return string;
    }

    /**
     * @brief Returns the length of the string in bytes
     */
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
    {
        return this->m_len;
    }

    /**
     * @brief Returns a sub str of the given range
     * @param from the sub str beginning index
     * @param to the sub str ending index (inclusive)
     * @panics
        - If `from` or `to` exceeded str's length
        - If `to` is greater than `from`
     */
    [[nodiscard]] constexpr auto slice(std::size_t from, std::size_t to) const noexcept -> str
    {
        if (from > this->m_len || to > this->m_len || std::min(from, to) > this->m_len)
        {
            panic("Invalid parameter(s) while calling str::slice, from: {}, to: {}, size: {}", from, to, this->m_len);
        }

        auto s = str::from_bytes_unchecked(this->m_data + from, to - from);
        if (!is_valid_utf8(s.m_data, s.m_len))
        {
            panic("Invalid UTF-8 sequence while calling str::slice");
        }

        return s;
    }

    /**
     * @brief Returns a sub str of the given range
     * @see str::slice(size_t, size_t) overload for details
     */
    [[nodiscard]] constexpr auto slice(std::size_t from) const noexcept -> str
    {
        return this->slice(from, this->m_len);
    }

    /**
     * @brief Checks if this string starts with the given pattern
     * @param pattern The pattern to check for
     * @return true if this string starts with the pattern
     */
    [[nodiscard]] constexpr auto starts_with(const str& pattern) const noexcept -> bool
    {
        if (pattern.m_len > this->m_len)
        {
            return false;
        }

        if (pattern.m_len == 0)
        {
            return true;
        }

        return std::equal(this->m_data, this->m_data + pattern.m_len, pattern.m_data);
    }

    /**
     * @brief Checks if this string ends with the given pattern
     * @param pattern The pattern to check for
     * @return true if this string ends with the pattern
     */
    [[nodiscard]] constexpr auto ends_with(const str& pattern) const noexcept -> bool
    {
        if (pattern.m_len > this->m_len)
        {
            return false;
        }

        if (pattern.m_len == 0)
        {
            return true;
        }

        return std::equal(
            this->m_data + (this->m_len - pattern.m_len),
            this->m_data + this->m_len,
            pattern.m_data
        );
    }

    /**
     * @brief Converts the String to a std::string
     * @return A std::string containing the String's contents
     */
    [[nodiscard]] constexpr auto to_std_string() const -> std::string
    {
        return std::string(std::bit_cast<const char*>(this->m_data), this->m_len);
    }

    /**
     * @brief Returns the byte index of the first character that matches the pattern
     * @param pattern The pattern to search for
     * @return Option containing the byte index of the first match, or None if not found
     */
    [[nodiscard]] constexpr auto find(const str& pattern) const noexcept -> Option<std::size_t>
    {
        if (pattern.m_len > this->m_len || pattern.m_len == 0)
        {
            return None{};
        }

        // Create spans for the current string and pattern
        std::span<const std::byte> haystack(this->m_data, this->m_len);
        std::span<const std::byte> needle(pattern.m_data, pattern.m_len);

        // Use std::ranges::search to find the pattern
        auto result = std::ranges::search(haystack, needle);

        if (result.empty())
        {
            return None{};
        }

        // Calculate the byte offset from the start of the string
        return size_t(std::distance(haystack.begin(), result.begin()));
    }

    /**
     * @brief Returns the byte index of the first character that matches the pattern
     * @param pattern The pattern to search for
     * @return Option containing the byte index of the first match, or None if not found
     */
    [[nodiscard]] constexpr auto find(const char* pattern) const noexcept -> Option<std::size_t>
    {
        return this->find(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::find#pattern"));
    }

    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto replace(const str& pattern, const str& replacement) const -> raw::String<Alloc>
    {
        return this->replace_n<Alloc>(pattern, replacement, std::numeric_limits<std::size_t>::max());
    }

    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto replace(const char* pattern, const char* replacement) const -> raw::String<Alloc>
    {
        return this->replace<Alloc>(str::from(pattern).expect("Invalid UTF-8 sequence while calling String::replace#pattern"),
            str::from(replacement).expect("Invalid UTF-8 sequence while calling String::replace#replacement"));
    }

    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto replace_n(const str& pattern, const str& replacement, std::size_t n) const -> raw::String<Alloc>
    {
        auto string = raw::String<Alloc>();

        if (pattern.empty())
        {
            string.push_str(*this);
            return string;
        }

        // Calculate the maximum possible size needed for the result
        // This is a worst-case estimate where every character is replaced
        std::size_t max_size = this->m_len + (replacement.size() - pattern.size()) * std::min(n, this->m_len / pattern.size());
        string.reserve(max_size);

        std::size_t pos = 0;
        std::size_t count = 0;

        while (pos < this->m_len && count < n)
        {
            const Option<std::size_t> found = this->find(pattern);
            if (found.is_none())
            {
                break;
            }

            const auto found_idx = found.unwrap();
            // Copy the part before the pattern
            if (found_idx > pos)
            {
                string.push_str(str::from_bytes_unchecked(this->m_data + pos, found_idx - pos));
            }

            // Copy the replacement
            string.push_str(replacement);

            // Move to the next position after the pattern
            pos = found_idx + pattern.size();
            count++;
        }

        // Copy the remaining part of the string
        if (pos < this->m_len)
        {
            string.push_str(str::from_bytes_unchecked(this->m_data + pos, this->m_len - pos));
        }

        return string;
    }

    [[nodiscard]] constexpr auto rfind(const str& pattern) const noexcept -> Option<std::size_t>
    {
        if (pattern.m_len > this->m_len || pattern.m_len == 0)
        {
            return None{};
        }

        std::span<const std::byte> haystack(this->m_data, this->m_len);
        std::span<const std::byte> needle(pattern.m_data, pattern.m_len);

        auto result = std::ranges::find_end(haystack, needle);

        if (result.empty())
        {
            return None{};
        }

        return size_t(std::distance(haystack.begin(), result.begin()));
    }

    [[nodiscard]] constexpr auto rfind(const char* pattern) const noexcept -> Option<std::size_t>
    {
        return this->rfind(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::rfind#pattern"));
    }

    /**
     * @brief Converts ASCII uppercase characters to lowercase
     * @param string A String stores original str's data with ASCII uppercase characters converted to lowercase
     */
    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto to_ascii_lowercase() const -> raw::String<Alloc>
    {
        auto string = raw::String<Alloc>();
        string.push_str(*this);
        string.make_ascii_lowercase();

        return string;
    }

    /**
     * @brief Converts ASCII lowercase characters to uppercase
     * @param string A String stores original str's data with ASCII lowercase characters converted to uppercase
     */
    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto to_ascii_uppercase() const -> raw::String<Alloc>
    {
        auto string = raw::String<Alloc>();
        string.push_str(*this);
        string.make_ascii_uppercase();

        return string;
    }

    /**
     * @brief Returns a str with leading and trailing ASCII whitespace removed.
     * ASCII whitespace refers to U+0020 SPACE, U+0009 HORIZONTAL TAB, U+000A LINE FEED, U+000C FORM FEED, or U+000D CARRIAGE RETURN.
     */
    [[nodiscard]] auto trim_ascii() const -> str
    {
        return this->trim_ascii_start().trim_ascii_end();
    }

    /**
     * @brief Returns a str with leading ASCII whitespace removed.
     * ASCII whitespace refers to U+0020 SPACE, U+0009 HORIZONTAL TAB, U+000A LINE FEED, U+000C FORM FEED, or U+000D CARRIAGE RETURN.
     */
    [[nodiscard]] auto trim_ascii_start() const -> str
    {
        if (this->m_len == 0 || this->m_data == nullptr)
        {
            return *this;
        }

        const std::byte* start = this->m_data;
        const std::byte* end = this->m_data + this->m_len;
        utf8proc_int32_t codepoint = 0;

        while (start < end)
        {
            const auto advance = utf8proc_iterate(reinterpret_cast<const utf8proc_uint8_t*>(start),
                                                        end - start,
                                                        &codepoint);
            if (advance <= 0)
            {
                break;
            }

            if (!std::bit_cast<Char>(codepoint).is_ascii_whitespace())
            {
                break;
            }

            start += advance;
        }

        return str(start, this->m_len - (start - this->m_data));
    }

    /**
     * @brief Returns a str with leading ASCII whitespace removed.
     * ASCII whitespace refers to U+0020 SPACE, U+0009 HORIZONTAL TAB, U+000A LINE FEED, U+000C FORM FEED, or U+000D CARRIAGE RETURN.
     */
    [[nodiscard]] auto trim_ascii_end() const -> str
    {
        const std::byte* start = this->m_data;
        const std::byte* end = this->m_data + this->m_len;
        const std::byte* last_non_ws = start;
        utf8proc_int32_t codepoint = 0;

        for (const std::byte* p = start; p < end; )
        {
            const auto advance = utf8proc_iterate(reinterpret_cast<const utf8proc_uint8_t*>(p),
                                                        end - p,
                                                        &codepoint);
            if (advance <= 0)
            {
                break;
            }

            p += advance;

            if (std::bit_cast<Char>(codepoint).is_ascii_whitespace())
            {
                continue;
            }

            last_non_ws = p;
        }

        return str(start, last_non_ws - start);
    }

// iterators
private:
    struct Lines
    {
        struct LinesIter
        {
            using iterator_category = std::forward_iterator_tag;
            using value_type = plain_str;
            using difference_type = std::ptrdiff_t;
            using pointer = plain_str*;
            using reference = plain_str&;

            const str* s = nullptr;
            plain_str span;
            uint32_t skip = 0;

            constexpr explicit LinesIter() noexcept {}

            constexpr explicit LinesIter(const str* s) noexcept : s(s)
            {
                if (s != nullptr)
                {
                    // Initialize the first line
                    auto bytes = s->as_bytes();
                    std::size_t start = 0;
                    std::size_t pos = 0;

                    // Find the first line ending
                    while (pos < bytes.size())
                    {
                        if (bytes[pos] == std::byte{'\n'})
                        {
                            span = plain_str(bytes.data() + start, pos - start);
                            pos += 1; // Skip \n
                            skip = 1;
                            break;
                        }
                        else if (bytes[pos] == std::byte{'\r'} && pos + 1 < bytes.size() && bytes[pos + 1] == std::byte{'\n'})
                        {
                            span = plain_str(bytes.data() + start, pos - start);
                            pos += 2; // Skip \r\n
                            skip = 2;
                            break;
                        }
                        pos++;
                    }

                    // If no line ending found, take the whole string
                    if (pos >= bytes.size())
                    {
                        span = plain_str(bytes.data(), bytes.size());
                        pos = bytes.size();
                    }
                }
            }

            [[nodiscard]] constexpr auto operator*() const noexcept -> const plain_str&
            {
                return span;
            }

            [[nodiscard]] constexpr auto operator->() const noexcept -> const plain_str*
            {
                return &span;
            }

            constexpr auto operator++() noexcept -> LinesIter&
            {
                if (this->s == nullptr)
                {
                    return *this;
                }

                auto bytes = this->s->as_bytes();
                std::size_t start = span.data - bytes.data() + this->span.len + this->skip;

                // If we're at or past the end, mark as finished
                if (start >= bytes.size())
                {
                    this->s = nullptr;
                    span = plain_str(nullptr, 0);
                    return *this;
                }

                // Skip the line ending we found in the previous iteration
                if (bytes[start] == std::byte{'\n'})
                {
                    this->skip = 1;
                }
                else if (bytes[start] == std::byte{'\r'} && start + 1 < bytes.size() && bytes[start + 1] == std::byte{'\n'})
                {
                    this->skip = 2;
                }
                start += this->skip;

                // If we're at the end after skipping line ending, mark as finished
                if (start >= bytes.size())
                {
                    this->s = nullptr;
                    span = plain_str(nullptr, 0);
                    return *this;
                }

                // Find the next line ending
                std::size_t pos = start;
                while (pos < bytes.size())
                {
                    if (bytes[pos] == std::byte{'\n'})
                    {
                        span = plain_str(bytes.data() + start, pos - start);
                        break;
                    }
                    else if (bytes[pos] == std::byte{'\r'} && pos + 1 < bytes.size() && bytes[pos + 1] == std::byte{'\n'})
                    {
                        span = plain_str(bytes.data() + start, pos - start);
                        break;
                    }
                    pos++;
                }

                // If no line ending found, take the rest of the string
                if (pos >= bytes.size())
                {
                    span = plain_str(bytes.data() + start, bytes.size() - start);
                }

                return *this;
            }

            constexpr auto operator++(int) noexcept -> LinesIter
            {
                LinesIter temp = *this;
                ++(*this);
                return temp;
            }

            [[nodiscard]] constexpr auto operator==(const LinesIter& other) const noexcept -> bool
            {
                return this->s == other.s;
            }
        };

    public:
        const str& s;

    public:
        constexpr explicit Lines(const str& s) : s(s) {}

    public:
        [[nodiscard]] constexpr auto begin() noexcept -> LinesIter
        {
            return LinesIter(&this->s);
        }

        [[nodiscard]] constexpr auto end() noexcept -> LinesIter
        {
            return LinesIter(nullptr);
        }
    };

    struct Split
    {
        struct SplitIter
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = plain_str;
            using pointer = const plain_str*;
            using reference = const plain_str&;

            const str* s = nullptr;
            plain_str pattern;
            plain_str span;

            constexpr SplitIter() noexcept {}

            constexpr SplitIter(const str* s, const plain_str& pattern) noexcept : s(s), pattern(pattern)
            {
                if (s != nullptr)
                {
                    // If pattern is empty, return the entire string
                    if (pattern.len == 0)
                    {
                        span = plain_str(s->m_data, s->m_len);
                        return;
                    }

                    auto ptr = std::search(this->s->m_data, this->s->m_data + this->s->m_len, this->pattern.data, this->pattern.data + this->pattern.len);
                    if (ptr != this->s->m_data + this->s->m_len)
                    {
                        const auto pos = ptr - this->s->m_data;
                        this->span = plain_str(s->m_data, std::size_t(pos));
                    }
                    else
                    {
                        span = plain_str(s->m_data, s->m_len);
                    }
                }
            }

            // [[nodiscard]] auto operator->() noexcept -> plain_str*
            // {
            //     return &this->span;
            // }

            // [[nodiscard]] auto operator*() noexcept -> plain_str&
            // {
            //     return this->span;
            // }

            [[nodiscard]] auto operator->() const noexcept -> const plain_str*
            {
                return const_cast<const plain_str*>(&this->span);
            }

            [[nodiscard]] auto operator*() const noexcept -> const plain_str&
            {
                return const_cast<const plain_str&>(this->span);
            }

            constexpr auto operator++() noexcept -> SplitIter&
            {
                if (this->s == nullptr)
                {
                    return *this;
                }

                std::size_t start = span.data - this->s->m_data + span.len;

                // Skip the delimiter
                start += pattern.len;

                // If reached the end after skipping delimiter, end iteration
                if (start > this->s->m_len)
                {
                    this->s = nullptr;
                    this->span.len = 0;
                    return *this;
                }

                // Find the next split point
                const auto ptr = std::search(this->s->m_data + start, this->s->m_data + this->s->m_len, this->pattern.data, this->pattern.data + this->pattern.len);
                const auto pos = ptr - this->s->m_data;
                this->span = plain_str(this->s->m_data + start, pos - start);

                return *this;
            }

            constexpr auto operator++(int) noexcept -> SplitIter
            {
                SplitIter temp = *this;
                ++(*this);
                return temp;
            }

            [[nodiscard]] constexpr auto operator==(const SplitIter& other) const noexcept -> bool
            {
                return (this->s == other.s) && (this->pattern == other.pattern);
            }
        };

    public:
        using iterator = SplitIter;
        using const_iterator = SplitIter;

        const str* s;
        const plain_str pattern;

    public:
        constexpr explicit Split() = default;

        constexpr Split(const str& s, const plain_str& pattern) : s(&s), pattern(pattern) {}

    public:
        auto operator=(const Split& other) noexcept -> Split&
        {
            std::memcpy(this, &other, sizeof(Split));
            return *this;
        }

    public:
        [[nodiscard]] constexpr auto begin() noexcept -> SplitIter
        {
            return SplitIter(this->s, this->pattern);
        }

        [[nodiscard]] constexpr auto end() noexcept -> SplitIter
        {
            return SplitIter(nullptr, this->pattern);
        }
    };

    struct SplitASCIIWhiteSpace
    {
    public:
        const str& s;

        struct SplitASCIIWhiteSpaceIter
        {
            const str* s;
            plain_str span;

            explicit SplitASCIIWhiteSpaceIter(const str* s) noexcept : s(s), span(s->m_data, s->m_len)
            {
                if (s != nullptr)
                {
                    utf8proc_int32_t codepoint = 0;
                    size_t pos = 0;
                    const auto size = this->s->size();

                    while (pos < size)
                    {
                        const auto advance = utf8proc_iterate(
                            reinterpret_cast<const utf8proc_uint8_t*>(this->s->m_data + pos),
                            size - pos,
                            &codepoint
                        );

                        if (!std::bit_cast<Char>(codepoint).is_ascii_whitespace())
                        {
                            break;
                        }

                        pos += advance;
                    }

                    this->span = plain_str(this->s->m_data, pos);
                }
            }

            [[nodiscard]] auto operator++() noexcept -> SplitASCIIWhiteSpaceIter&
            {
                utf8proc_int32_t codepoint = 0;
                size_t pos = this->span.data - this->s->m_data + this->span.len;
                const auto size = this->s->size();

                while (pos < size)
                {
                    const auto advance = utf8proc_iterate(
                        reinterpret_cast<const utf8proc_uint8_t*>(this->s->m_data + pos),
                        size - pos,
                        &codepoint
                    );

                    if (!std::bit_cast<Char>(codepoint).is_ascii_whitespace())
                    {
                        break;
                    }

                    pos += advance;
                }

                this->span = plain_str(this->s->m_data + pos, size - pos);

                return *this;
            }

            [[nodiscard]] constexpr auto operator*() const noexcept -> const plain_str&
            {
                return this->span;
            }

            [[nodiscard]] constexpr auto operator->() const noexcept -> const plain_str*
            {
                return &this->span;
            }
        };

    public:
        constexpr SplitASCIIWhiteSpace(const str& s) : s(s) {}

    public:
        [[nodiscard]] constexpr auto begin() noexcept -> SplitASCIIWhiteSpaceIter
        {
            return SplitASCIIWhiteSpaceIter(&this->s);
        }

        [[nodiscard]] constexpr auto end() noexcept -> SplitASCIIWhiteSpaceIter
        {
            return SplitASCIIWhiteSpaceIter(nullptr);
        }
    };

// operators
public:
    [[nodiscard]] constexpr auto operator<=>(const str& other) const noexcept -> std::strong_ordering
    {
        const std::size_t min_len = std::min(this->m_len, other.m_len);
        if (min_len == 0)
        {
            return this->m_len <=> other.m_len;
        }

        const int result = std::equal(this->m_data, this->m_data + min_len, other.m_data);
        if (result != 0)
        {
            return result <=> 0;
        }

        return this->m_len <=> other.m_len;
    }

    [[nodiscard]] constexpr auto operator==(const str& other) const noexcept -> bool
    {
        if (this->m_len != other.m_len) {
            return false;
        }
        return std::equal(this->m_data, this->m_data + this->m_len, other.m_data);
    }

    /**
     * @brief Returns an iterator over the lines of this string
     * @return A Lines iterator that yields each line in the string
     */
    [[nodiscard]] constexpr auto lines() const noexcept -> Lines
    {
        return Lines(*this);
    }

    /**
     * @brief Returns an iterator that splits the string by the given pattern
     * @param pattern The pattern to split on
     * @return A Split iterator that yields each part of the split string
     */
    [[nodiscard]] constexpr auto split(const str& pattern) const noexcept -> Split
    {
        return Split(*this, plain_str(pattern.m_data, pattern.m_len));
    }

    /**
     * @brief Returns an iterator that splits the string by the given pattern
     * @param pattern The pattern to split on
     * @return A Split iterator that yields each part of the split string
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto split(const raw::String<Alloc>& pattern) const noexcept -> Split
    {
        return Split(*this, plain_str(pattern.m_data, pattern.m_len));
    }

    /**
     * @brief Returns an iterator that splits the string by the given pattern
     * @param pattern The pattern to split on
     * @return A Split iterator that yields each part of the split string
     * @panics If pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] constexpr auto split(const char* pattern) const noexcept -> Split
    {
        const auto s = str::from(pattern).ok().expect_take("Invalid UTF-8 sequence while calling str::split#pattern");
        return Split(*this, plain_str(s.m_data, s.m_len));
    }
};

namespace strings
{
    constexpr auto join_with(char ch) -> decltype(auto)
    {
        return std::views::join_with(static_cast<std::byte>(ch));
    }

    constexpr auto join_with(const char* str) -> decltype(auto)
    {
        const auto s = str::from(str).expect("Invalid UTF-8 sequence while calling strings::join_with#str");
        return std::views::join_with(s.as_bytes());
    }

    constexpr auto join_with(const str& str) -> decltype(auto)
    {
        return std::views::join_with(str.as_bytes());
    }

    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto join_with(const raw::String<Alloc>& str) -> decltype(auto)
    {
        return std::views::join_with(str.as_bytes());
    }
}

namespace raw
{

/**
 * @brief A UTF-8–encoded, null-terminated, growable string with custom allocator support
 * @tparam Alloc The allocator type to use for memory management
 */
template<typename Alloc = std::allocator<std::byte>>
struct String
{
    using pointer = typename std::allocator_traits<Alloc>::pointer;
    friend struct crab_cpp::str;

private:
    pointer m_data = nullptr;
    std::size_t m_len = 0;
    compressed_pair<Alloc, size_t> m_alloc_and_capacity;

    /**
     * @brief Private constructor for internal use
     */
    String(const Alloc& alloc, pointer data, std::size_t len, std::size_t capacity) noexcept
        : m_data(data), m_len(len), m_alloc_and_capacity(alloc, capacity) {}

    /**
     * @brief Allocates memory for the string
     * @param capacity The capacity to allocate in bytes
     */
    auto allocate(std::size_t capacity) -> void
    {
        if (capacity == 0)
        {
            this->m_data = nullptr;
            this->m_alloc_and_capacity.second = 0;
            return;
        }

        // Add 1 to capacity for null terminator
        this->m_data = std::allocator_traits<Alloc>::allocate(
            this->m_alloc_and_capacity.first(),
            capacity + 1
        );
        this->m_alloc_and_capacity.second = capacity;
        // Add null terminator
        this->m_data[0] = std::byte{0};
    }

// constructors
public:
    constexpr explicit String(const Alloc& alloc = Alloc()) noexcept : m_data(nullptr), m_len(0), m_alloc_and_capacity(alloc, 0) {}

    constexpr String(const str& str, const Alloc& alloc = Alloc()) : String(alloc)
    {
        (*this) += str;
    }

    constexpr String(const plain_str& str, const Alloc& alloc = Alloc()) : String(alloc)
    {
        (*this) += crab_cpp::str(str);
    }

    template<typename View>
        requires std::ranges::view<std::remove_reference_t<View>> &&
            std::same_as<std::byte, std::remove_cv_t<typename std::iterator_traits<std::ranges::iterator_t<View>>::value_type>>
    constexpr String(View&& view, const Alloc& alloc = Alloc()) : String(alloc)
    {
        this->reserve(64);
        auto begin = view.cbegin();

        size_t i = 0;
        for (; begin != view.cend(); i += 1)
        {
            this->m_data[i] = *begin;
            ++begin;

            if (i == this->m_alloc_and_capacity.second - 1)
            {
                this->m_len = i + 1;
                this->reserve(this->m_alloc_and_capacity.second / 2);
            }
        }
        this->m_len = i;
        this->m_data[i + 1] = std::byte{0};

        if (!is_valid_utf8(this->m_data, this->m_len))
        {
            panic("Invalid UTF-8 sequence encountered while calling String constructor");
        }
    }

    /**
     * @brief Creates a string from a UTF-8 string
     * @param str The input string
     * @param len The length of the input string
     * @param alloc The allocator to use
     * @return A Result containing either a String or a FromUtf8Error
     */
    [[nodiscard]] static auto from_raw_parts(const char* str, std::size_t len, const Alloc& alloc = Alloc()) -> Result<String, FromUtf8Error>
    {
        auto alloc_copy = alloc;

        if (str == nullptr || len == 0)
        {
            return String(alloc_copy, nullptr, 0, 0);
        }

        auto result = str::from_raw_parts(str, len);
        if (result.is_err())
        {
            return result.unwrap_err();
        }

        pointer data = std::allocator_traits<Alloc>::allocate(alloc_copy, len);
        std::copy(str, str + len, std::bit_cast<char*>(data));

        return String(alloc_copy, data, len, len);
    }

    /**
     * @brief Creates a string from a null-terminated UTF-8 string
     * @param str The input string
     * @param alloc The allocator to use
     * @return A Result containing either a String or a FromUtf8Error
     */
    [[nodiscard]] static auto from(const char* str, const Alloc& alloc = Alloc()) -> Result<String, FromUtf8Error>
    {
        return String::from_raw_parts(str, std::strlen(str), alloc);
    }

    String(const String& other)
        : m_data(nullptr)
        , m_len(0), m_alloc_and_capacity(std::allocator_traits<Alloc>::select_on_container_copy_construction(other.m_alloc_and_capacity.first()), 0)
    {
        (*this) += other;
    }

    String(String&& other) noexcept
        : m_data(other.m_data), m_len(other.m_len), m_alloc_and_capacity(std::move(other.m_alloc_and_capacity))
    {
        other.m_data = nullptr;
        other.m_len = 0;
        other.m_alloc_and_capacity.second = 0;
    }

    ~String()
    {
        if (this->m_data != nullptr)
        {
            std::allocator_traits<Alloc>::deallocate(
                this->m_alloc_and_capacity.first(),
                this->m_data,
                this->m_alloc_and_capacity.second + 1
            );
        }
    }

// functions
public:
    /**
     * @brief Returns a byte slice of this String's contents
     * @return A span containing the string's bytes
     */
    [[nodiscard]] constexpr auto as_bytes() const noexcept -> std::span<const std::byte>
    {
        return std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(this->m_data),
        this->m_len
        );
    }

    /**
     * @brief Returns a byte slice of this String's contents
     * @return A span containing the string's bytes
     */
    [[nodiscard]] constexpr auto as_raw() const noexcept -> const char*
    {
        return reinterpret_cast<const char*>(this->m_data);
    }

    /**
     * @brief Returns a byte slice of this String's contents
     * @return A span containing the string's bytes
     */
    [[nodiscard]] constexpr auto as_str() const noexcept -> const str&
    {
        return *(reinterpret_cast<const str*>(this));
    }

    /**
     * @brief Returns the current capacity of the buffer in bytes
     * @return The capacity in bytes
     */
    [[nodiscard]] constexpr auto capacity() const noexcept -> std::size_t
    {
        return this->m_alloc_and_capacity.second;
    }

    /**
     * @brief Clears the string content but keeps the allocated memory
     */
    constexpr auto clear() noexcept -> void
    {
        this->m_len = 0;
    }

    /**
     * @brief Returns a pointer to the string data
     */
    [[nodiscard]] constexpr auto data() const noexcept -> const std::byte*
    {
        return this->m_data;
    }

    /**
     * @brief Returns a pointer to the string data
     */
    [[nodiscard]] constexpr auto data() noexcept -> std::byte*
    {
        return this->m_data;
    }

    /**
     * @brief Converts this string to its ASCII lower case equivalent in-place.
     * ASCII letters 'A' to 'Z' are mapped to 'a' to 'z', but non-ASCII letters are unchanged.
     */
    auto make_ascii_lowercase() noexcept -> void
    {
        utf8proc_int32_t codepoint = 0;
        std::size_t pos = 0;

        while (pos < this->m_len)
        {
            const auto advance = utf8proc_iterate(
                reinterpret_cast<const utf8proc_uint8_t*>(this->m_data + pos),
                this->m_len - pos,
                &codepoint
            );

            if (codepoint >= 65 && codepoint <= 90)
            {
                this->m_data[pos] = static_cast<std::byte>(codepoint + 32);
            }

            pos += advance;
        }
    }

    /**
     * @brief Converts this string to its ASCII lower case equivalent in-place.
     * ASCII letters 'a' to 'z' are mapped to 'A' to 'Z', but non-ASCII letters are unchanged.
     */
    auto make_ascii_uppercase() noexcept -> void
    {
        utf8proc_int32_t codepoint = 0;
        std::size_t pos = 0;

        while (pos < this->m_len)
        {
            const auto advance = utf8proc_iterate(
                reinterpret_cast<const utf8proc_uint8_t*>(this->m_data + pos),
                this->m_len - pos,
                &codepoint
            );

            if (codepoint >= 97 && codepoint <= 122)
            {
                this->m_data[pos] = static_cast<std::byte>(codepoint - 32);
            }

            pos += advance;
        }
    }

    /**
     * @brief Returns the current length of the string in bytes
     * @return The length in bytes
     */
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
    {
        return this->m_len;
    }

    /**
     * @brief Ensures that the string has enough capacity to store additional bytes
     * @param additional The number of additional bytes to reserve
     */
    auto reserve(std::size_t additional) -> void
    {
        const std::size_t new_capacity = this->m_len + additional;
        if (new_capacity <= this->m_alloc_and_capacity.second)
        {
            return;
        }

        // Allocate new memory (add 1 for null terminator)
        pointer new_data = std::allocator_traits<Alloc>::allocate(
            this->m_alloc_and_capacity.first(),
            new_capacity + 1
        );

        // Copy existing data
        if (this->m_len > 0)
        {
            std::copy(this->m_data,
                    this->m_data + this->m_len,
                    new_data);
        }
        // Add null terminator
        new_data[this->m_len] = std::byte{0};

        // Deallocate old memory
        if (this->m_data != nullptr)
        {
            std::allocator_traits<Alloc>::deallocate(
                this->m_alloc_and_capacity.first(),
                this->m_data,
                this->m_alloc_and_capacity.second + 1
            );
        }

        // Update members
        this->m_data = new_data;
        this->m_alloc_and_capacity.second = new_capacity;
    }

    /**
     * @brief Appends the given Char to the end of this String.
     * @param ch The Char to append
     */
    auto push(Char ch) -> void
    {
        (*this) += ch;
    }

    /**
     * @brief Appends a given str onto the end of this String.
     * @param str The str to append
     */
    auto push_str(const str& str) -> void
    {
        (*this) += str;
    }

    /**
     * @brief Appends a given C-string onto the end of this String.
     * @param str The C-string to append
     */
    auto push_str(const char* str) -> void
    {
        this->push_str(str::from(str).expect("Invalid UTF-8 sequence while calling String::push_str"));
    }

    /**
     * @brief Splits the string into two at the given byte index
     * @param at The byte index at which to split
     * @return A newly allocated String containing bytes [at, len)
     * @panics If at is not on a UTF-8 code point boundary, or if it is beyond the last code point
     */
    auto split_off(std::size_t at) -> String
    {
        if (at >= this->m_len)
        {
            panic("Split index out of bounds");
        }

        // Verify that 'at' is on a UTF-8 code point boundary
        if (!is_valid_utf8(this->m_data, at) ||
            !is_valid_utf8(this->m_data + at, this->m_len - at))
        {
            panic("Split index not on UTF-8 code point boundary");
        }

        // Create new string with the second part
        String result(this->m_alloc_and_capacity.first(), nullptr, 0, 0);
        result.reserve(this->m_len - at);

        // Copy the bytes [at, len) to the new string
        std::copy(this->m_data + at,
                 this->m_data + this->m_len,
                 result.m_data);
        result.m_len = this->m_len - at;
        // Add null terminator to the new string
        result.m_data[result.m_len] = std::byte{0};

        // Update this string's length and add null terminator
        this->m_len = at;
        this->m_data[this->m_len] = std::byte{0};

        return result;
    }

    /**
     * @brief Converts the String to a std::string
     * @return A std::string containing the String's contents
     */
    [[nodiscard]] auto to_std_string() const -> std::string
    {
        return std::string(std::bit_cast<const char*>(this->m_data), this->m_len);
    }

    /**
     * @brief Shortens this String to the specified length.
     * @details If new_len is greater than or equal to the string's current length, this has no effect.
     *          Note that this method has no effect on the allocated capacity of the string
     * @param new_len The new length of the string
     * @panics Panics if new_len does not lie on a char boundary.
     */
    auto truncate(std::size_t new_len) -> void
    {
        if (new_len >= this->m_len)
        {
            return;
        }

        if (!is_valid_utf8(this->m_data, new_len))
        {
            panic("Invalid UTF-8 sequence while calling String::truncate");
        }

        this->m_len = new_len;
        this->m_data[this->m_len] = std::byte{0};
    }

    /**
     * @brief Removes the last character from the string buffer and returns it
     * @return Option containing the removed character, or None if the string is empty
     */
    [[nodiscard]] auto pop() noexcept -> Option<Char>
    {
        if (this->m_len == 0)
        {
            return None{};
        }

        // Find the start of the last UTF-8 sequence by scanning backwards
        std::size_t start = this->m_len - 1;
        while (start > 0 && (this->m_data[start] & std::byte{0xC0}) == std::byte{0x80})
        {
            start -= 1;
        }

        // Decode the UTF-8 sequence
        utf8proc_int32_t codepoint = 0;
        utf8proc_iterate(
            reinterpret_cast<const utf8proc_uint8_t*>(this->m_data + start),
            this->m_len - start,
            &codepoint
        );

        // Update string length
        this->m_len = start;
        // Add null terminator at new end
        this->m_data[this->m_len] = std::byte{0};

        return Char(static_cast<std::uint32_t>(codepoint));
    }

//operators
public:
    [[nodiscard]] auto operator->() noexcept -> str*
    {
        return std::bit_cast<str*>(this);
    }

    [[nodiscard]] auto operator->() const noexcept -> const str*
    {
        return std::bit_cast<const str*>(this);
    }

    auto operator=(const String& other) -> String&
    {
        this->clear();
        (*this) += other;

        return *this;
    }

    /**
     * @brief Appends the given Char to the end of this String.
     * @param ch The Char to append
     * @return A reference to this String
     */
    auto operator+=(const Char ch) -> String&
    {
        // Convert Unicode scalar value to UTF-8
        utf8proc_uint8_t utf8[4];
        const auto len = utf8proc_encode_char(
            static_cast<utf8proc_int32_t>(ch.code_point()),
            utf8
        );

        if (len < 0)
        {
            panic("Failed to encode Unicode scalar value to UTF-8");
        }

        const std::size_t new_len = this->m_len + static_cast<std::size_t>(len);
        if (new_len > this->m_alloc_and_capacity.second)
        {
            this->reserve(new_len - this->m_alloc_and_capacity.second);
        }

        std::copy(utf8, utf8 + len, this->m_data + this->m_len);
        this->m_len = new_len;
        // Add null terminator
        this->m_data[this->m_len] = std::byte{0};

        return *this;
    }

    /**
     * @brief Appends a str to this String using the += operator
     * @param str The str to append
     * @return A reference to this String
     */
    auto operator+=(const String& str) -> String&
    {
        return (*this) += str.as_str();
    }

    /**
     * @brief Appends a str to this String using the += operator
     * @param str The str to append
     * @return A reference to this String
     */
    auto operator+=(const str& str) -> String&
    {
        if (str.empty())
        {
            return *this;
        }

        const std::size_t new_len = this->m_len + str.size();
        if (new_len > this->m_alloc_and_capacity.second)
        {
            this->reserve(new_len - this->m_alloc_and_capacity.second);
        }

        std::copy(str.data(), str.data() + str.size(), this->m_data + this->m_len);
        this->m_len = new_len;
        // Add null terminator
        this->m_data[this->m_len] = std::byte{0};

        return *this;
    }

    /**
     * @brief Appends a C-string to this String using the += operator
     * @param str The C-string to append
     * @return A reference to this String
     */
    auto operator+=(const char* str) -> String&
    {
        return (*this) += str::from(str).expect("Invalid UTF-8 sequence while calling String::operator+=");
    }

    auto operator+(const String& str) -> String
    {
        return (*this) + str.as_str();
    }

    auto operator+(const str& str) -> String
    {
        auto string = String();
        string.reserve(this->m_len + str.size());
        string += *this;
        string += str;

        return string;
    }

    auto operator+(const char* str) -> String
    {
        return (*this) + str::from(str).expect("Invalid UTF-8 sequence while calling String::operator+=");
    }

    [[nodiscard]] constexpr auto operator<=>(const String& other) const noexcept -> std::strong_ordering
    {
        const std::size_t min_len = std::min(this->m_len, other.m_len);
        if (min_len == 0)
        {
            return this->m_len <=> other.m_len;
        }

        const int result = std::equal(this->m_data, this->m_data + min_len, other.m_data);
        if (result != 0)
        {
            return result <=> 0;
        }

        return this->m_len <=> other.m_len;
    }

    [[nodiscard]] constexpr auto operator==(const String& other) const noexcept -> bool
    {
        if (this->m_len != other.m_len)
        {
            return false;
        }

        return std::equal(this->m_data, this->m_data + this->m_len, other.m_data);
    }
};

}

using String = raw::String<std::allocator<std::byte>>;

template<typename Alloc>
[[nodiscard]] constexpr auto operator==(const raw::String<Alloc>& lhs, const str& rhs) noexcept -> bool
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    return std::equal(lhs.data(), lhs.data() + lhs.size(), rhs.data());
}

template<typename Alloc>
[[nodiscard]] constexpr auto operator==(const str& lhs, const raw::String<Alloc>& rhs) noexcept -> bool
{
    return rhs == lhs;
}

template<typename Alloc>
[[nodiscard]] constexpr auto operator==(const raw::String<Alloc>& lhs, const char* rhs) noexcept -> bool
{
    if (lhs.size() != std::strlen(rhs))
    {
        return false;
    }
    return std::equal(lhs.data(), lhs.data() + lhs.size(), reinterpret_cast<const std::byte*>(rhs));
}

template<typename Alloc>
[[nodiscard]] constexpr auto operator==(const char* lhs, const raw::String<Alloc>& rhs) noexcept -> bool
{
    return rhs == lhs;
}

[[nodiscard]] constexpr auto operator==(const str& lhs, const char* rhs) noexcept -> bool
{
    if (lhs.size() != std::strlen(rhs))
    {
        return false;
    }
    return std::equal(lhs.data(), lhs.data() + lhs.size(), reinterpret_cast<const std::byte*>(rhs));
}

[[nodiscard]] constexpr auto operator==(const char* lhs, const str& rhs) noexcept -> bool
{
    return rhs == lhs;
}

namespace literal
{
    [[nodiscard]] constexpr auto operator""_s(const char* str, std::size_t len) -> crab_cpp::str
    {
        return str::from_raw_parts(str, len).expect("Invalid UTF-8 sequence while calling operator\"\"_s");
    }

    [[nodiscard]] constexpr auto operator""_S(const char* str, std::size_t len) -> crab_cpp::String
    {
        return crab_cpp::String::from(str).ok().expect_take("Invalid UTF-8 sequence while calling operator\"\"_S");
    }
}

}

export template<>
struct std::formatter<crab_cpp::str> : std::formatter<const char*>
{
    auto format(const crab_cpp::str& str, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", std::string_view(str.as_raw(), str.size()));
    }
};

export auto operator<<(std::ostream& os, const crab_cpp::str& str) -> std::ostream&
{
    os << std::string_view(str.as_raw(), str.size());
    return os;
}

export template<typename Alloc>
struct std::formatter<crab_cpp::raw::String<Alloc>> : std::formatter<const char*>
{
    auto format(const crab_cpp::raw::String<Alloc>& str, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", std::string_view(reinterpret_cast<const char*>(str.data()), str.size()));
    }
};

export template<typename Alloc>
auto operator<<(std::ostream& os, const crab_cpp::raw::String<Alloc>& str) -> std::ostream&
{
    os << std::string_view(reinterpret_cast<const char*>(str.data()), str.size());
    return os;
}

export template<>
struct std::hash<crab_cpp::Char>
{
    auto operator()(const crab_cpp::Char& ch) const noexcept -> size_t
    {
		return std::hash<std::uint32_t>{}(ch.code_point());
	}
};

export template<>
struct std::hash<crab_cpp::str>
{
    auto operator()(const crab_cpp::str& str) const noexcept -> size_t
    {
		return std::hash<std::string_view>{}(std::string_view(str.as_raw(), str.size()));
	}
};

export template<typename Alloc>
struct std::hash<crab_cpp::raw::String<Alloc>>
{
    auto operator()(const crab_cpp::raw::String<Alloc>& str) const noexcept -> size_t
    {
		return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(str.data()), str.size()));
	}
};

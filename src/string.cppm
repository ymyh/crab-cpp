module;

#include "utf8proc.h"

export module crab_cpp:string;

import :panic;
import :option;
import :result;
import std;

/**
 * @brief A compressed pair implementation that optimizes storage for empty types
 * @tparam T1 First type
 * @tparam T2 Second type
 */
template<typename T1, typename T2, bool = std::is_empty_v<T1> && !std::is_final_v<T1>>
struct compressed_pair : private T1
{
    T2 second;

    constexpr compressed_pair() noexcept : T1(), second() {}
    constexpr compressed_pair(const T1& x, const T2& y) noexcept : T1(x), second(y) {}

    [[nodiscard]] constexpr auto first() noexcept -> T1& { return *this; }
    [[nodiscard]] constexpr auto first() const noexcept -> const T1& { return *this; }
};

template<typename T1, typename T2>
struct compressed_pair<T1, T2, false>
{
    T1 _first;
    T2 second;

    constexpr compressed_pair(T1& x, const T2& y) noexcept : _first(x), second(y) {}

    [[nodiscard]] constexpr auto first() noexcept -> T1& { return this->_first; }
    [[nodiscard]] constexpr auto first() const noexcept -> const T1& { return this->_first; }
};

struct plain_str
{
    const std::byte* data = nullptr;
    size_t len = 0;

public:
    explicit constexpr plain_str() noexcept = default;

    constexpr plain_str(const plain_str&) = default;

    explicit constexpr plain_str(const std::byte* data, size_t len) noexcept : data(data), len(len) {}

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
        constexpr explicit Iter() noexcept = default;

        constexpr explicit Iter(const std::byte* ptr) noexcept : ptr(ptr) {}

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

        auto operator*() const noexcept -> const std::byte&
        {
            return *this->ptr;
        }

        auto operator->() const noexcept -> const std::byte*
        {
            return this->ptr;
        }

        constexpr auto operator<=>(const Iter&) const noexcept -> std::strong_ordering = default;
    };

    [[nodiscard]] constexpr auto begin() const noexcept -> Iter
    {
        return Iter(this->data);
    }

    [[nodiscard]] constexpr auto end() const noexcept -> Iter
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
    size_t pos;

    /**
     * @brief Constructs a FromUtf8Error with the given position
     */
    constexpr explicit FromUtf8Error(size_t pos) noexcept : pos(pos) {}
};

/**
 * @brief Validates if the given string is valid UTF-8
 * @param str The string to validate
 * @param len The length of the string in bytes
 * @return true if the string is valid UTF-8
 */
[[nodiscard]] constexpr auto is_valid_utf8(const std::byte* str, size_t len) noexcept -> bool
{
    if (str == nullptr || len == 0)
    {
        return true;
    }

    utf8proc_int32_t codepoint;
    size_t pos = 0;

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

[[nodiscard]] constexpr auto is_valid_utf8(const char* str, size_t len) noexcept -> bool
{
    return is_valid_utf8(std::bit_cast<const std::byte*>(str), len);
}

/**
 * @brief A class representing a Unicode scalar value
 */
struct Char
{
private:
    std::uint32_t m_value = 0;

public:
    /**
     * @brief Default constructor
     */
    constexpr explicit Char() noexcept = default;

    /**
     * @brief Constructs a Char from a Unicode scalar value
     * @param value The Unicode scalar value
     * @note Panics If the value is not a valid Unicode scalar value
     */
    constexpr explicit Char(std::uint32_t value) : m_value(value)
    {
        if (!is_valid_scalar_value(value))
        {
            panic("Invalid Unicode scalar value");
        }
    }

private:
    /**
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
     * @return The Unicode scalar value
     */
    [[nodiscard]] constexpr auto code_point() const noexcept -> std::uint32_t
    {
        return this->m_value;
    }

    /**
     * @return true if this char is an ASCII character
     */
    [[nodiscard]] constexpr auto is_ascii() const noexcept -> bool
    {
        return this->m_value <= 0x7F;
    }

    /**
     * @return true if this char is an ASCII alphabetic character
     */
    [[nodiscard]] constexpr auto is_ascii_alphabetic() const noexcept -> bool
    {
        return (this->m_value >= static_cast<std::uint32_t>('a') && this->m_value <= static_cast<std::uint32_t>('z')) ||
               (this->m_value >= static_cast<std::uint32_t>('A') && this->m_value <= static_cast<std::uint32_t>('Z'));
    }

    /**
     * @return true if this char is an ASCII alphanumeric character
     */
    [[nodiscard]] constexpr auto is_ascii_alphanumeric() const noexcept -> bool
    {
        return (this->m_value >= static_cast<std::uint32_t>('0') && this->m_value <= static_cast<std::uint32_t>('9')) ||
               (this->m_value >= static_cast<std::uint32_t>('a') && this->m_value <= static_cast<std::uint32_t>('z')) ||
               (this->m_value >= static_cast<std::uint32_t>('A') && this->m_value <= static_cast<std::uint32_t>('Z'));
    }

    /**
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
     * @details Checks if the value is an ASCII control character: U+0000 NUL ..= U+001F UNIT SEPARATOR, or U+007F DELETE.
     *          Note that most ASCII whitespace characters are control characters, but SPACE is not.
     * @return true if this char is an ASCII control character
     */
    [[nodiscard]] constexpr auto is_ascii_control() const noexcept -> bool
    {
        return (this->m_value <= 0x1F) || (this->m_value == 0x7F);
    }

    /**
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

    /**
     * @brief Checks if the value is an ASCII decimal digit: U+0030 '0' ..= U+0039 '9'.
     */
    [[nodiscard]] constexpr auto is_ascii_digit() const noexcept -> bool
    {
        return this->m_value >= static_cast<std::uint32_t>('0') && this->m_value <= static_cast<std::uint32_t>('9');
    }

    /**
     * @brief Checks if the value is an ASCII hexadecimal digit: U+0030 '0' ..= U+0039 '9', U+0041 'A' ..= U+0046 'F', or U+0061 'a' ..= U+0066 'f'.
     */
    [[nodiscard]] constexpr auto is_ascii_hexdigit() const noexcept -> bool
    {
        return this->is_ascii_digit() ||
               (this->m_value >= static_cast<std::uint32_t>('A') && this->m_value <= static_cast<std::uint32_t>('F')) ||
               (this->m_value >= static_cast<std::uint32_t>('a') && this->m_value <= static_cast<std::uint32_t>('f'));
    }

    /**
     * @details Checks if the value is whitespace as defined by the Unicode White_Space property:
     *          U+0009..U+000D (TAB, LF, VT, FF, CR), U+0020 SPACE, U+0085 NEXT LINE, and the
     *          Zs/Zl/Zp categories.
     * @return true if this char is whitespace
     */
    [[nodiscard]] constexpr auto is_whitespace() const noexcept -> bool
    {
        // U+000B VERTICAL TAB is White_Space but is deliberately excluded from
        // the narrower ASCII notion (`is_ascii_whitespace`), so it is called out
        // explicitly to match Rust's `char::is_whitespace`.
        if (this->m_value == 0x0B || this->is_ascii_whitespace() || this->m_value == 0x85)
        {
            return true;
        }

        const auto category = utf8proc_category(static_cast<utf8proc_int32_t>(this->m_value));

        return category == UTF8PROC_CATEGORY_ZS || category == UTF8PROC_CATEGORY_ZL || category == UTF8PROC_CATEGORY_ZP;
    }

    /**
     * @brief Returns the number of bytes of this char when encoded as UTF-8
     */
    [[nodiscard]] constexpr auto len_utf8() const noexcept -> size_t
    {
        if (this->m_value <= 0x7F)
        {
            return 1;
        }

        if (this->m_value <= 0x7FF)
        {
            return 2;
        }

        if (this->m_value <= 0xFFFF)
        {
            return 3;
        }

        return 4;
    }

    /**
     * @brief Encodes this char as UTF-8 into the given buffer
     * @param out A buffer of at least 4 bytes
     * @return The number of bytes written
     */
    [[nodiscard]] constexpr auto encode_utf8(std::span<std::byte, 4> out) const noexcept -> size_t
    {
        utf8proc_uint8_t encoded[4];
        const auto len = utf8proc_encode_char(static_cast<utf8proc_int32_t>(this->m_value), encoded);

        for (size_t i = 0; i < static_cast<size_t>(len); i += 1)
        {
            out[i] = static_cast<std::byte>(encoded[i]);
        }

        return static_cast<size_t>(len);
    }

    /**
     * @brief Returns the ASCII lowercase equivalent of this char
     * @note Non-ASCII characters are returned unchanged
     */
    [[nodiscard]] constexpr auto to_ascii_lowercase() const noexcept -> Char
    {
        if (this->is_ascii_uppercase())
        {
            return Char::from_u32_unchecked(this->m_value + 0x20);
        }

        return Char::from_u32_unchecked(this->m_value);
    }

    /**
     * @brief Returns the ASCII uppercase equivalent of this char
     * @note Non-ASCII characters are returned unchanged
     */
    [[nodiscard]] constexpr auto to_ascii_uppercase() const noexcept -> Char
    {
        if (this->is_ascii_lowercase())
        {
            return Char::from_u32_unchecked(this->m_value - 0x20);
        }

        return Char::from_u32_unchecked(this->m_value);
    }

    /**
     * @brief Constructs a Char from a Unicode scalar value without validating it
     * @param value The Unicode scalar value
     * @warning This function is unsafe and may cause undefined behavior if value is not a valid scalar value
     */
    [[nodiscard]] static constexpr auto from_u32_unchecked(std::uint32_t value) noexcept -> Char
    {
        Char ch;
        ch.m_value = value;

        return ch;
    }

// operators
public:
    constexpr auto operator==(const Char& other) const noexcept -> bool = default;
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
    size_t m_len;

    /**
     * @brief Private constructor for internal use
     */
    constexpr str(pointer data, size_t len) noexcept : m_data(data), m_len(len) {}

    /**
     * @brief Encodes a Char into the given buffer and returns a str borrowing it
     * @param ch The character to encode
     * @param buffer A buffer of at least 4 bytes, which must outlive the returned str
     * @return A str borrowing the buffer
     */
    [[nodiscard]] static constexpr auto char_pattern(Char ch, std::byte (&buffer)[4]) noexcept -> str
    {
        const auto len = ch.encode_utf8(buffer);

        return str(buffer, len);
    }

    /**
     * @brief Checks whether a byte is ASCII whitespace
     */
    [[nodiscard]] static constexpr auto is_ascii_whitespace_byte(std::byte byte) noexcept -> bool
    {
        const auto value = static_cast<std::uint32_t>(static_cast<std::uint8_t>(byte));

        return Char::from_u32_unchecked(value).is_ascii_whitespace();
    }

    /**
     * @brief Returns the length in bytes of the UTF-8 sequence introduced by `byte`
     * @param byte The leading byte of a UTF-8 sequence
     * @return 1, 2, 3 or 4
     */
    [[nodiscard]] static constexpr auto utf8_sequence_len(std::byte byte) noexcept -> size_t
    {
        const auto value = static_cast<std::uint8_t>(byte);

        if ((value & 0x80) == 0x00)
        {
            return 1;
        }

        if ((value & 0xE0) == 0xC0)
        {
            return 2;
        }

        if ((value & 0xF0) == 0xE0)
        {
            return 3;
        }

        return 4;
    }

    /**
     * @brief Returns the index of the code point boundary strictly before `index`
     * @param index A code point boundary of this string, greater than zero
     */
    [[nodiscard]] constexpr auto previous_boundary(size_t index) const noexcept -> size_t
    {
        size_t i = index - 1;
        while (i > 0 && (static_cast<std::uint8_t>(this->m_data[i]) & 0xC0) == 0x80)
        {
            i -= 1;
        }

        return i;
    }

    /**
     * @brief Decodes the character starting at the given boundary
     * @param index A code point boundary of this string
     * @return The decoded character and its encoded length in bytes
     */
    [[nodiscard]] constexpr auto decode_at(size_t index) const noexcept -> std::pair<Char, size_t>
    {
        utf8proc_int32_t codepoint = 0;
        const auto advance = utf8proc_iterate(
            reinterpret_cast<const utf8proc_uint8_t*>(this->m_data + index),
            this->m_len - index,
            &codepoint
        );

        return std::pair<Char, size_t>{
            Char::from_u32_unchecked(static_cast<std::uint32_t>(codepoint)),
            static_cast<size_t>(advance)
        };
    }

    /**
     * @brief Checks whether a character is printable for the purpose of `escape_debug`
     * @details Printable ASCII and non-ASCII characters whose category is not a control,
     *          format, surrogate, private-use, unassigned or separator category are kept.
     */
    [[nodiscard]] static auto is_printable(Char ch) noexcept -> bool
    {
        const auto value = ch.code_point();

        if (value >= 0x20 && value <= 0x7E)
        {
            return true;
        }

        if (value < 0x80)
        {
            return false;
        }

        switch (utf8proc_category(static_cast<utf8proc_int32_t>(value)))
        {
        case UTF8PROC_CATEGORY_CC:
        case UTF8PROC_CATEGORY_CF:
        case UTF8PROC_CATEGORY_CS:
        case UTF8PROC_CATEGORY_CO:
        case UTF8PROC_CATEGORY_CN:
        case UTF8PROC_CATEGORY_ZS:
        case UTF8PROC_CATEGORY_ZL:
        case UTF8PROC_CATEGORY_ZP:
            return false;
        default:
            return true;
        }
    }

    /**
     * @brief Appends a lowercase `\u{...}` escape for a scalar value to a String
     */
    template<typename Alloc>
    static auto push_unicode_escape(raw::String<Alloc>& out, std::uint32_t value) -> void
    {
        constexpr char digits[] = "0123456789abcdef";

        out.push(Char('\\'));
        out.push(Char('u'));
        out.push(Char('{'));

        char buffer[8] = {};
        size_t count = 0;

        do
        {
            buffer[count] = digits[value & 0xF];
            value >>= 4;
            count += 1;
        } while (value != 0);

        for (size_t i = count; i > 0; i -= 1)
        {
            out.push(Char(buffer[i - 1]));
        }

        out.push(Char('}'));
    }

    /**
     * @brief Appends the `escape_default` form of `ch` to a String
     */
    template<typename Alloc>
    static auto append_escape_default(raw::String<Alloc>& out, Char ch) -> void
    {
        const auto value = ch.code_point();

        if (value == '\t')
        {
            out.push(Char('\\'));
            out.push(Char('t'));
        }
        else if (value == '\r')
        {
            out.push(Char('\\'));
            out.push(Char('r'));
        }
        else if (value == '\n')
        {
            out.push(Char('\\'));
            out.push(Char('n'));
        }
        else if (value == '\\')
        {
            out.push(Char('\\'));
            out.push(Char('\\'));
        }
        else if (value == '\'')
        {
            out.push(Char('\\'));
            out.push(Char('\''));
        }
        else if (value == '"')
        {
            out.push(Char('\\'));
            out.push(Char('"'));
        }
        else if (value >= 0x20 && value <= 0x7E)
        {
            out.push(ch);
        }
        else
        {
            str::push_unicode_escape(out, value);
        }
    }

    /**
     * @brief Appends the `escape_debug` form of `ch` to a String
     * @note Grapheme-extended handling is intentionally not implemented yet (see design P3).
     */
    template<typename Alloc>
    static auto append_escape_debug(raw::String<Alloc>& out, Char ch) -> void
    {
        const auto value = ch.code_point();

        if (value == 0x00)
        {
            out.push(Char('\\'));
            out.push(Char('0'));
        }
        else if (value == '\t')
        {
            out.push(Char('\\'));
            out.push(Char('t'));
        }
        else if (value == '\r')
        {
            out.push(Char('\\'));
            out.push(Char('r'));
        }
        else if (value == '\n')
        {
            out.push(Char('\\'));
            out.push(Char('n'));
        }
        else if (value == '\\')
        {
            out.push(Char('\\'));
            out.push(Char('\\'));
        }
        else if (value == '\'')
        {
            out.push(Char('\\'));
            out.push(Char('\''));
        }
        else if (value == '"')
        {
            out.push(Char('\\'));
            out.push(Char('"'));
        }
        else if (str::is_printable(ch))
        {
            out.push(ch);
        }
        else
        {
            str::push_unicode_escape(out, value);
        }
    }

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

public:
    [[nodiscard]] constexpr auto begin() const noexcept -> const std::byte*
    {
        return std::bit_cast<const std::byte*>(this->m_data);
    }

    [[nodiscard]] constexpr auto end() const noexcept -> const std::byte*
    {
        return std::bit_cast<const std::byte*>(this->m_data + this->m_len);
    }

    /**
     * @brief Creates a string view from a pointer and length
     * @param data Pointer to string data
     * @param len Length of the string
     * @return A Result containing either a str or a FromUtf8Error
     */
    [[nodiscard]] static constexpr auto from_raw_parts(const char* data, size_t len) noexcept -> Result<str, FromUtf8Error>
    {
        if (data == nullptr && len > 0)
        {
            return FromUtf8Error(0);
        }

        if (!is_valid_utf8(data, len))
        {
            // Find the position of the invalid UTF-8 sequence
            utf8proc_int32_t codepoint;
            size_t pos = 0;

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
    [[nodiscard]] static constexpr auto from_bytes_unchecked(pointer data, size_t len) noexcept -> str
    {
        return str(data, len);
    }

    /**
     * @brief Creates a string view from a null-terminated, UTF-8 encoded C style string
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
        return std::span(this->m_data, this->m_len);
    }

    /**
     * @brief Returns a pointer to this str's contents as a `const char*`
     * @return A pointer to the string's bytes
     * @note The returned pointer is **not** guaranteed to be NUL-terminated.
     *       For a sub-slice it points into the middle of the backing buffer,
     *       so reading it as a C string would run past the end of the slice.
     *       Use `raw::String::c_str()` when a NUL-terminated C string is
     *       required, or `to_std_string()` to obtain an owned copy.
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
     * @brief Checks if this string contains the given pattern
     * @param pattern The pattern to search for
     * @return true if this string contains the pattern
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] constexpr auto contains(const char* pattern) const noexcept -> bool
    {
        return this->contains(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::contains#pattern"));
    }

    /**
     * @brief Checks if this string contains the given pattern
     * @param pattern The pattern to search for
     * @return true if this string contains the pattern
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto contains(const raw::String<Alloc>& pattern) const noexcept -> bool
    {
        return this->contains(pattern.as_str());
    }

    /**
     * @brief Checks if this string contains the given character
     * @param ch The character to search for
     * @return true if this string contains the character
     */
    [[nodiscard]] constexpr auto contains(Char ch) const noexcept -> bool
    {
        std::byte buffer[4];

        return this->contains(str::char_pattern(ch, buffer));
    }

    /**
     * @brief Checks if this string contains the given character
     * @param ch The character to search for, interpreted as its numeric value in `[0, 255]`
     * @return true if this string contains the character
     */
    [[nodiscard]] constexpr auto contains(char ch) const noexcept -> bool
    {
        return this->contains(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
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
     * @brief Checks that two strings are an ASCII case-insensitive match.
     * Same as to_ascii_lowercase(a) == to_ascii_lowercase(b), but without allocating and copying temporaries.
     */
    [[nodiscard]] auto eq_ignore_ascii_case(const str& s) const noexcept -> bool
    {
        constexpr std::uint8_t ASCII_ignore_case[] {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
            26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
            50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 97, 98, 99, 100, 101, 102, 103,
            104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
            122, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109,
            110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
        };

        if (this->size() != s.size())
        {
            return false;
        }

        for (size_t i = 0; i < this->size(); i += 1)
        {
            if (this->m_data[i] != s.m_data[i])
            {
                if (static_cast<std::uint8_t>(this->m_data[i]) < 128 && static_cast<std::uint8_t>(s.m_data[i]) < 128)
                {
                    if (ASCII_ignore_case[static_cast<std::uint8_t>(this->m_data[i])] != ASCII_ignore_case[static_cast<std::uint8_t>(s.m_data[i])])
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * @brief Creates a new String by repeating a string n times.
     * @param n The number of times to repeat the string
     * @note Panics if the capacity overflows.
     */
    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto repeat(size_t n) const -> raw::String<Alloc>
    {
        if (n == 0 || this->m_len == 0)
        {
            return raw::String<Alloc>();
        }

        if (n > std::numeric_limits<size_t>::max() / this->m_len)
        {
            panic("Repeat times overflow");
        }

        raw::String<Alloc> string = raw::String<Alloc>();
        string.reserve(this->m_len * n);
        std::byte* ptr = string.data();

        for (size_t i = 0; i < n; i += 1)
        {
            std::memcpy(ptr + i * this->m_len, this->m_data, this->m_len);
        }
        string.m_len = n * this->m_len;
        string.m_data[string.m_len] = std::byte{0};

        return string;
    }

    /**
     * @brief Returns the length of the string in bytes
     */
    [[nodiscard]] constexpr auto size() const noexcept -> size_t
    {
        return this->m_len;
    }

    /**
     * @brief Returns a sub str of the given range
     * @param from the sub str beginning index (inclusive)
     * @param to the sub str ending index (exclusive)
     * @note Panics
        - If `from` or `to` exceeds str's length
        - If `from` is greater than `to`
        - If the resulting range is not on a UTF-8 code point boundary
     */
    [[nodiscard]] constexpr auto slice(size_t from, size_t to) const noexcept -> str
    {
        if (from > to || to > this->m_len)
        {
            panic("Invalid parameter(s) while calling str::slice, from: {}, to: {}, size: {}", from, to, this->m_len);
        }

        const auto s = str::from_bytes_unchecked(this->m_data + from, to - from);
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
    [[nodiscard]] constexpr auto slice(size_t from) const noexcept -> str
    {
        return this->slice(from, this->m_len);
    }

    /**
     * @brief Returns true if the given byte index is a UTF-8 code point boundary
     * @param index The byte index to check
     * @return true if `index` is in `[0, size]` and lies on a code point boundary
     */
    [[nodiscard]] constexpr auto is_char_boundary(size_t index) const noexcept -> bool
    {
        if (index == 0 || index == this->m_len)
        {
            return true;
        }

        if (index > this->m_len)
        {
            return false;
        }

        return (this->m_data[index] & std::byte{0xC0}) != std::byte{0x80};
    }

    /**
     * @brief Returns a sub str of the given range, without panicking
     * @param from the sub str beginning index
     * @param to the sub str ending index (exclusive)
     * @return Some containing the sub str, or None if the range is out of bounds or does not lie
     *         on code point boundaries
     */
    [[nodiscard]] constexpr auto get(size_t from, size_t to) const noexcept -> Option<str>
    {
        if (from > to || to > this->m_len)
        {
            return None{};
        }

        if (!this->is_char_boundary(from) || !this->is_char_boundary(to))
        {
            return None{};
        }

        return str(this->m_data + from, to - from);
    }

    /**
     * @brief Returns a sub str from the given index to the end, without panicking
     * @param from the sub str beginning index
     * @return Some containing the sub str, or None if the index is out of bounds or does not lie
     *         on a code point boundary
     */
    [[nodiscard]] constexpr auto get(size_t from) const noexcept -> Option<str>
    {
        return this->get(from, this->m_len);
    }

    /**
     * @brief Divides the string into two at the given byte index
     * @param mid The byte index at which to split
     * @return A pair of the sub str before and the sub str starting at `mid`
     * @note Panics if mid is out of bounds or not on a UTF-8 code point boundary
     */
    [[nodiscard]] constexpr auto split_at(size_t mid) const -> std::pair<str, str>
    {
        if (!this->is_char_boundary(mid))
        {
            panic("Invalid index while calling str::split_at, mid: {}, size: {}", mid, this->m_len);
        }

        return std::pair<str, str>{
            str(this->m_data, mid),
            str(this->m_data + mid, this->m_len - mid)
        };
    }

    /**
     * @brief Divides the string into two at the given byte index, without panicking
     * @param mid The byte index at which to split
     * @return Some containing the pair of sub strs, or None if mid is out of bounds or not on a
     *         UTF-8 code point boundary
     */
    [[nodiscard]] constexpr auto split_at_checked(size_t mid) const noexcept -> Option<std::pair<str, str>>
    {
        if (!this->is_char_boundary(mid))
        {
            return None{};
        }

        return std::pair<str, str>{
            str(this->m_data, mid),
            str(this->m_data + mid, this->m_len - mid)
        };
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
     * @brief Checks if this string starts with the given pattern
     * @param pattern The pattern to check for
     * @return true if this string starts with the pattern
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] constexpr auto starts_with(const char* pattern) const noexcept -> bool
    {
        return this->starts_with(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::starts_with#pattern"));
    }

    /**
     * @brief Checks if this string starts with the given pattern
     * @param pattern The pattern to check for
     * @return true if this string starts with the pattern
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto starts_with(const raw::String<Alloc>& pattern) const noexcept -> bool
    {
        return this->starts_with(pattern.as_str());
    }

    /**
     * @brief Checks if this string starts with the given character
     * @param ch The character to check for
     * @return true if this string starts with the character
     */
    [[nodiscard]] constexpr auto starts_with(Char ch) const noexcept -> bool
    {
        std::byte buffer[4];

        return this->starts_with(str::char_pattern(ch, buffer));
    }

    /**
     * @brief Checks if this string starts with the given character
     * @param ch The character to check for, interpreted as its numeric value in `[0, 255]`
     * @return true if this string starts with the character
     */
    [[nodiscard]] constexpr auto starts_with(char ch) const noexcept -> bool
    {
        return this->starts_with(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
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
     * @brief Checks if this string ends with the given pattern
     * @param pattern The pattern to check for
     * @return true if this string ends with the pattern
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] constexpr auto ends_with(const char* pattern) const noexcept -> bool
    {
        return this->ends_with(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::ends_with#pattern"));
    }

    /**
     * @brief Checks if this string ends with the given pattern
     * @param pattern The pattern to check for
     * @return true if this string ends with the pattern
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto ends_with(const raw::String<Alloc>& pattern) const noexcept -> bool
    {
        return this->ends_with(pattern.as_str());
    }

    /**
     * @brief Checks if this string ends with the given character
     * @param ch The character to check for
     * @return true if this string ends with the character
     */
    [[nodiscard]] constexpr auto ends_with(Char ch) const noexcept -> bool
    {
        std::byte buffer[4];

        return this->ends_with(str::char_pattern(ch, buffer));
    }

    /**
     * @brief Checks if this string ends with the given character
     * @param ch The character to check for, interpreted as its numeric value in `[0, 255]`
     * @return true if this string ends with the character
     */
    [[nodiscard]] constexpr auto ends_with(char ch) const noexcept -> bool
    {
        return this->ends_with(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
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
     * @brief Creates an owned String from this str
     * @tparam Alloc The allocator to use for the returned String
     * @return A new String containing this str's contents
     * @note Named `to_string` (not Rust's `to_owned`): in C++ the idiomatic
     *       spelling for "produce the owning type" is `to_string`.
     */
    template<typename Alloc = std::allocator<std::byte>>
    [[nodiscard]] constexpr auto to_string() const -> raw::String<Alloc>
    {
        return raw::String<Alloc>(*this);
    }

    /**
     * @brief Returns the byte index of the first character that matches the pattern
     * @param pattern The pattern to search for
     * @return Option containing the byte index of the first match, or None if not found
     */
    [[nodiscard]] constexpr auto find(const str& pattern) const noexcept -> Option<size_t>
    {
        if (pattern.m_len == 0)
        {
            return size_t(0);
        }

        if (pattern.m_len > this->m_len)
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
    [[nodiscard]] constexpr auto find(const char* pattern) const noexcept -> Option<size_t>
    {
        return this->find(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::find#pattern"));
    }

    /**
     * @brief Returns the byte index of the first character that matches the pattern
     * @param pattern The pattern to search for
     * @return Option containing the byte index of the first match, or None if not found
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto find(const raw::String<Alloc>& pattern) const noexcept -> Option<size_t>
    {
        return this->find(pattern.as_str());
    }

    /**
     * @brief Returns the byte index of the first occurrence of the given character
     * @param ch The character to search for
     * @return Option containing the byte index of the first match, or None if not found
     */
    [[nodiscard]] constexpr auto find(Char ch) const noexcept -> Option<size_t>
    {
        std::byte buffer[4];

        return this->find(str::char_pattern(ch, buffer));
    }

    /**
     * @brief Returns the byte index of the first occurrence of the given character
     * @param ch The character to search for, interpreted as its numeric value in `[0, 255]`
     * @return Option containing the byte index of the first match, or None if not found
     */
    [[nodiscard]] constexpr auto find(char ch) const noexcept -> Option<size_t>
    {
        return this->find(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Checks if all characters in this string are within the ASCII range.
     */
    [[nodiscard]] constexpr auto is_ascii() const noexcept -> bool
    {
        for (size_t i = 0; i < this->m_len; i++)
        {
            if (static_cast<std::uint8_t>(this->m_data[i]) > 127)
            {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Parses this string into a numeric type
     * @tparam T The numeric type to parse into
     * @return A Result containing either the parsed value or a std::from_chars_result
     * @note This function only works with numeric types (integral and floating-point)
     */
    template<typename T>
        requires (std::integral<T> || std::floating_point<T>)
    [[nodiscard]] constexpr auto parse() const noexcept -> Result<T, std::from_chars_result>
    {
        if (this->m_len == 0)
        {
            return std::from_chars_result{reinterpret_cast<const char*>(this->m_data), std::errc::invalid_argument};
        }

        T value;
        auto result = std::from_chars(
            reinterpret_cast<const char*>(this->m_data),
            reinterpret_cast<const char*>(this->m_data + this->m_len),
            value
        );

        if (result.ec == std::errc{})
        {
            if (result.ptr == reinterpret_cast<const char*>(this->m_data + this->m_len))
            {
                return value;
            }
            else
            {
                result.ec = std::errc::invalid_argument;
                return result;
            }
        }

        return result;
    }

    /**
     * @brief Replaces all matches of a pattern with another str.
     * replace creates a new String, and copies the data from this str into it.
     * While doing so, it attempts to find matches of a pattern. If it finds any, it replaces them with the replacement str.
     * @param pattern The pattern to search for
     * @param replacement
     * @returns a new String
     */
    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto replace(const str& pattern, const str& replacement) const -> raw::String<Alloc>
    {
        return this->replace_n<Alloc>(pattern, replacement, std::numeric_limits<size_t>::max());
    }

    /**
     * @brief Replaces all matches of a pattern with another str.
     * replace creates a new String, and copies the data from this str into it.
     * While doing so, it attempts to find matches of a pattern. If it finds any, it replaces them with the replacement str.
     * @param pattern The pattern to search for
     * @param replacement
     * @returns a new String
     */
    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto replace(const char* pattern, const char* replacement) const -> raw::String<Alloc>
    {
        return this->replace<Alloc>(str::from(pattern).expect("Invalid UTF-8 sequence while calling String::replace#pattern"),
            str::from(replacement).expect("Invalid UTF-8 sequence while calling String::replace#replacement"));
    }

    /**
     * @brief Replaces first N matches of a pattern with another string.
     * replacen creates a new String, and copies the data from this string slice into it.
     * While doing so, it attempts to find matches of a pattern. If it finds any, it replaces them with the replacement string slice at most `n` times.
     * @param pattern
     * @param replacement
     * @param n
     * @returns a new String
     */
    template<typename Alloc = std::allocator<std::byte>>
    constexpr auto replace_n(const str& pattern, const str& replacement, size_t n) const -> raw::String<Alloc>
    {
        auto string = raw::String<Alloc>();

        if (pattern.empty())
        {
            string.push_str(*this);
            return string;
        }

        // Calculate the maximum possible size needed for the result
        // This is a worst-case estimate where every character is replaced
        const auto size_diff = replacement.size() > pattern.size()
            ? replacement.size() - pattern.size()
            : 0;
        size_t max_size = this->m_len + size_diff * std::min(n, this->m_len / pattern.size());
        string.reserve(max_size);

        size_t pos = 0;
        size_t count = 0;

        while (pos < this->m_len && count < n)
        {
            const Option<size_t> found = this->slice(pos).find(pattern);
            if (found.is_none())
            {
                break;
            }

            const auto found_idx = found.unwrap() + pos;
            // Copy the part before the pattern
            string.push_str_unchecked(str::from_bytes_unchecked(this->m_data + pos, found_idx - pos));
            // Copy the replacement
            string.push_str_unchecked(replacement);

            // Move to the next position after the pattern
            pos = found_idx + pattern.size();
            count++;
        }

        // Copy the remaining part of the string
        if (pos < this->m_len)
        {
            string.push_str_unchecked(str::from_bytes_unchecked(this->m_data + pos, this->m_len - pos));
        }

        return string;
    }

    /**
     * @brief Returns the byte index for the first character of the last match of the pattern in this str slice.
     * @param pattern The pattern to search for
     * @returns `None` if the pattern doesn’t match.
     */
    [[nodiscard]] constexpr auto rfind(const str& pattern) const noexcept -> Option<size_t>
    {
        if (pattern.m_len == 0)
        {
            return this->m_len;
        }

        if (pattern.m_len > this->m_len)
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

    /**
     * @brief Returns the byte index for the first character of the last match of the pattern in this str slice.
     * @param pattern The pattern to search for
     * @returns `None` if the pattern doesn’t match.
     */
    [[nodiscard]] constexpr auto rfind(const char* pattern) const noexcept -> Option<size_t>
    {
        return this->rfind(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::rfind#pattern"));
    }

    /**
     * @brief Returns the byte index for the first character of the last match of the pattern in this str slice.
     * @param pattern The pattern to search for
     * @returns `None` if the pattern doesn’t match.
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto rfind(const raw::String<Alloc>& pattern) const noexcept -> Option<size_t>
    {
        return this->rfind(pattern.as_str());
    }

    /**
     * @brief Returns the byte index of the last occurrence of the given character
     * @param ch The character to search for
     * @return Option containing the byte index of the last match, or None if not found
     */
    [[nodiscard]] constexpr auto rfind(Char ch) const noexcept -> Option<size_t>
    {
        std::byte buffer[4];

        return this->rfind(str::char_pattern(ch, buffer));
    }

    /**
     * @brief Returns the byte index of the last occurrence of the given character
     * @param ch The character to search for, interpreted as its numeric value in `[0, 255]`
     * @return Option containing the byte index of the last match, or None if not found
     */
    [[nodiscard]] constexpr auto rfind(char ch) const noexcept -> Option<size_t>
    {
        return this->rfind(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief If the string starts with the pattern prefix, returns the substring after the prefix, wrapped in Some.
     * This method removes the prefix exactly once.
     * @returns A string slice with the prefix removed, if the string does not start with prefix, returns None.
    */
    [[nodiscard]] auto strip_prefix(const str& pattern) const noexcept -> Option<str>
    {
        if (this->starts_with(pattern))
        {
            return this->slice(pattern.size());
        }

        return None{};
    }

    /**
     * @brief If the string starts with the pattern suffix, returns the substring before the suffix, wrapped in Some.
     * This method removes the suffix exactly once.
     * @returns A str slice with the prefix removed, if the string does not ends with suffix, returns None.
    */
    [[nodiscard]] auto strip_suffix(const str& pattern) const noexcept -> Option<str>
    {
        if (this->ends_with(pattern))
        {
            return this->slice(0, this->m_len - pattern.size());
        }

        return None{};
    }

    /**
     * @brief If the string starts with the pattern prefix, returns the substring after the prefix, wrapped in Some.
     * This method removes the prefix exactly once.
     * @returns A string slice with the prefix removed, if the string does not start with prefix, returns None.
    */
    [[nodiscard]] auto strip_prefix(const char* pattern) const noexcept -> Option<str>
    {
        return this->strip_prefix(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::strip_prefix#pattern"));
    }

    /**
     * @brief If the string starts with the pattern prefix, returns the substring after the prefix, wrapped in Some.
     * This method removes the prefix exactly once.
     * @returns A string slice with the prefix removed, if the string does not start with prefix, returns None.
     */
    template<typename Alloc>
    [[nodiscard]] auto strip_prefix(const raw::String<Alloc>& pattern) const noexcept -> Option<str>
    {
        return this->strip_prefix(pattern.as_str());
    }

    /**
     * @brief If the string starts with the given character, returns the substring after it, wrapped in Some.
     * @param ch The character to strip
     * @returns A str slice with the character removed, or None if the string does not start with it.
     */
    [[nodiscard]] auto strip_prefix(Char ch) const noexcept -> Option<str>
    {
        std::byte buffer[4];

        return this->strip_prefix(str::char_pattern(ch, buffer));
    }

    /**
     * @brief If the string starts with the given character, returns the substring after it, wrapped in Some.
     * @param ch The character to strip, interpreted as its numeric value in `[0, 255]`
     * @returns A str slice with the character removed, or None if the string does not start with it.
     */
    [[nodiscard]] auto strip_prefix(char ch) const noexcept -> Option<str>
    {
        return this->strip_prefix(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief If the string starts with the pattern suffix, returns the substring before the suffix, wrapped in Some.
     * This method removes the suffix exactly once.
     * @returns A str slice with the prefix removed, if the string does not ends with suffix, returns None.
    */
    [[nodiscard]] auto strip_suffix(const char* pattern) const noexcept -> Option<str>
    {
        return this->strip_suffix(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::strip_suffix#pattern"));
    }

    /**
     * @brief If the string ends with the pattern suffix, returns the substring before the suffix, wrapped in Some.
     * This method removes the suffix exactly once.
     * @returns A str slice with the suffix removed, if the string does not ends with suffix, returns None.
     */
    template<typename Alloc>
    [[nodiscard]] auto strip_suffix(const raw::String<Alloc>& pattern) const noexcept -> Option<str>
    {
        return this->strip_suffix(pattern.as_str());
    }

    /**
     * @brief If the string ends with the given character, returns the substring before it, wrapped in Some.
     * @param ch The character to strip
     * @returns A str slice with the character removed, or None if the string does not end with it.
     */
    [[nodiscard]] auto strip_suffix(Char ch) const noexcept -> Option<str>
    {
        std::byte buffer[4];

        return this->strip_suffix(str::char_pattern(ch, buffer));
    }

    /**
     * @brief If the string ends with the given character, returns the substring before it, wrapped in Some.
     * @param ch The character to strip, interpreted as its numeric value in `[0, 255]`
     * @returns A str slice with the character removed, or None if the string does not end with it.
     */
    [[nodiscard]] auto strip_suffix(char ch) const noexcept -> Option<str>
    {
        return this->strip_suffix(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Converts ASCII uppercase characters to lowercase
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
     * @brief Returns a new String with all characters converted to lowercase
     * @tparam Alloc The allocator to use for the returned String
     * @return A new String with the Unicode simple lowercase mapping applied
     * @note utf8proc only exposes the simple 1:1 mapping, so full mappings (e.g. the
     *       Turkish dotted I) are not performed.
     */
    template<typename Alloc = std::allocator<std::byte>>
    [[nodiscard]] auto to_lowercase() const -> raw::String<Alloc>
    {
        auto string = raw::String<Alloc>();
        string.reserve(this->m_len);

        for (const auto ch : this->chars())
        {
            const auto mapped = utf8proc_tolower(static_cast<utf8proc_int32_t>(ch.code_point()));
            string.push(Char::from_u32_unchecked(static_cast<std::uint32_t>(mapped)));
        }

        return string;
    }

    /**
     * @brief Returns a new String with all characters converted to uppercase
     * @tparam Alloc The allocator to use for the returned String
     * @return A new String with the Unicode simple uppercase mapping applied
     * @note utf8proc only exposes the simple 1:1 mapping. Rust performs full mappings, so
     *       `"ß".to_uppercase()` is "SS" there but U+1E9E here.
     */
    template<typename Alloc = std::allocator<std::byte>>
    [[nodiscard]] auto to_uppercase() const -> raw::String<Alloc>
    {
        auto string = raw::String<Alloc>();
        string.reserve(this->m_len);

        for (const auto ch : this->chars())
        {
            const auto mapped = utf8proc_toupper(static_cast<utf8proc_int32_t>(ch.code_point()));
            string.push(Char::from_u32_unchecked(static_cast<std::uint32_t>(mapped)));
        }

        return string;
    }

    /**
     * @brief Returns a new String containing the escaped form of this string
     * @tparam Alloc The allocator to use for the returned String
     * @details Follows Rust's `char::escape_default`: tab, CR, LF, backslash and both quote
     *          characters are escaped, printable ASCII is kept, everything else becomes a
     *          `\u{...}` escape.
     */
    template<typename Alloc = std::allocator<std::byte>>
    [[nodiscard]] auto escape_default() const -> raw::String<Alloc>
    {
        auto string = raw::String<Alloc>();
        string.reserve(this->m_len);

        for (const auto ch : this->chars())
        {
            str::append_escape_default(string, ch);
        }

        return string;
    }

    /**
     * @brief Returns a new String containing the debug-escaped form of this string
     * @tparam Alloc The allocator to use for the returned String
     * @details Like `escape_default`, but NUL becomes `\0` and printable non-ASCII
     *          characters are kept as-is.
     */
    template<typename Alloc = std::allocator<std::byte>>
    [[nodiscard]] auto escape_debug() const -> raw::String<Alloc>
    {
        auto string = raw::String<Alloc>();
        string.reserve(this->m_len);

        for (const auto ch : this->chars())
        {
            str::append_escape_debug(string, ch);
        }

        return string;
    }

    /**
     * @brief Returns a new String containing the Unicode-escaped form of this string
     * @tparam Alloc The allocator to use for the returned String
     * @details Every character is written as a `\u{...}` escape, regardless of whether it
     *          is printable.
     */
    template<typename Alloc = std::allocator<std::byte>>
    [[nodiscard]] auto escape_unicode() const -> raw::String<Alloc>
    {
        auto string = raw::String<Alloc>();
        string.reserve(this->m_len);

        for (const auto ch : this->chars())
        {
            str::push_unicode_escape(string, ch.code_point());
        }

        return string;
    }

    /**
     * @brief Returns a str with leading and trailing ASCII whitespace removed.
     * ASCII whitespace refers to U+0020 SPACE, U+0009 HORIZONTAL TAB, U+000A LINE FEED, U+000C FORM FEED, or U+000D CARRIAGE RETURN.
     */
    [[nodiscard]] auto trim_ascii() const noexcept -> str
    {
        return this->trim_ascii_start().trim_ascii_end();
    }

    /**
     * @brief Returns a str with leading ASCII whitespace removed.
     * ASCII whitespace refers to U+0020 SPACE, U+0009 HORIZONTAL TAB, U+000A LINE FEED, U+000C FORM FEED, or U+000D CARRIAGE RETURN.
     */
    [[nodiscard]] auto trim_ascii_start() const noexcept -> str
    {
        if (this->m_len == 0 || this->m_data == nullptr)
        {
            return *this;
        }

        size_t i = 0;
        for (; i < this->m_len; i += 1)
        {
            if (!is_ascii_whitespace_byte(this->m_data[i]))
            {
                break;
            }
        }

        return str(this->m_data + i, this->m_len - i);
    }

    /**
     * @brief Returns a str with leading ASCII whitespace removed.
     * ASCII whitespace refers to U+0020 SPACE, U+0009 HORIZONTAL TAB, U+000A LINE FEED, U+000C FORM FEED, or U+000D CARRIAGE RETURN.
     */
    [[nodiscard]] auto trim_ascii_end() const noexcept -> str
    {
        if (this->m_len == 0 || this->m_data == nullptr)
        {
            return *this;
        }

        size_t i = this->m_len;
        while (i > 0 && is_ascii_whitespace_byte(this->m_data[i - 1]))
        {
            i -= 1;
        }

        return str(this->m_data, i);
    }

    /**
     * @brief Returns a str with leading and trailing Unicode whitespace removed
     * @details Unicode whitespace is the `White_Space` property, matching Rust. Unlike
     *          `trim_ascii`, this also strips e.g. U+00A0 and U+3000.
     */
    [[nodiscard]] auto trim() const noexcept -> str
    {
        return this->trim_start().trim_end();
    }

    /**
     * @brief Returns a str with leading Unicode whitespace removed
     * @see trim() for the definition of whitespace
     */
    [[nodiscard]] auto trim_start() const noexcept -> str
    {
        size_t i = 0;
        while (i < this->m_len)
        {
            const auto [ch, width] = this->decode_at(i);
            if (!ch.is_whitespace())
            {
                break;
            }

            i += width;
        }

        return str(this->m_data + i, this->m_len - i);
    }

    /**
     * @brief Returns a str with trailing Unicode whitespace removed
     * @see trim() for the definition of whitespace
     */
    [[nodiscard]] auto trim_end() const noexcept -> str
    {
        size_t end = 0;
        size_t i = 0;
        while (i < this->m_len)
        {
            const auto [ch, width] = this->decode_at(i);
            if (!ch.is_whitespace())
            {
                end = i + width;
            }

            i += width;
        }

        return str(this->m_data, end);
    }

    /**
     * @brief Returns a string slice with all prefixes that match a pattern repeatedly removed.
     * @param pattern The pattern to search for
    */
    [[nodiscard]] auto trim_start_matches(const str& pattern) const noexcept -> str
    {
        if (this->m_len == 0 || this->m_data == nullptr || pattern.m_len == 0)
        {
            return *this;
        }

        const std::byte* curr_data = this->m_data;
        size_t curr_len = this->m_len;

        while (curr_len >= pattern.m_len)
        {
            bool matches = std::equal(curr_data, curr_data + pattern.m_len, pattern.m_data);

            if (!matches)
            {
                break;
            }

            curr_data += pattern.m_len;
            curr_len -= pattern.m_len;
        }

        return str(curr_data, curr_len);
    }

    /**
     * @brief Returns a string slice with all suffixes that match a pattern repeatedly removed.
     * @param pattern The pattern to search for
    */
    [[nodiscard]] auto trim_end_matches(const str& pattern) const noexcept -> str
    {
        if (this->m_len == 0 || this->m_data == nullptr || pattern.m_len == 0)
        {
            return *this;
        }

        const std::byte* curr_data = this->m_data;
        size_t curr_len = this->m_len;

        while (curr_len >= pattern.m_len)
        {
            bool matches = std::equal(curr_data + curr_len - pattern.m_len,
                                    curr_data + curr_len,
                                    pattern.m_data);

            if (!matches)
            {
                break;
            }

            curr_len -= pattern.m_len;
        }

        return str(curr_data, curr_len);
    }

    /**
     * @brief Returns a string slice with all prefixes that match a pattern repeatedly removed.
     * @param pattern The pattern to search for
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto trim_start_matches(const char* pattern) const noexcept -> str
    {
        return this->trim_start_matches(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::trim_start_matches#pattern"));
    }

    /**
     * @brief Returns a string slice with all suffixes that match a pattern repeatedly removed.
     * @param pattern The pattern to search for
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto trim_end_matches(const char* pattern) const noexcept -> str
    {
        return this->trim_end_matches(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::trim_end_matches#pattern"));
    }

    /**
     * @brief Returns a string slice with all prefixes that match the pattern repeatedly removed.
     * @param pattern The pattern to search for
     */
    template<typename Alloc>
    [[nodiscard]] auto trim_start_matches(const raw::String<Alloc>& pattern) const noexcept -> str
    {
        return this->trim_start_matches(pattern.as_str());
    }

    /**
     * @brief Returns a string slice with all suffixes that match the pattern repeatedly removed.
     * @param pattern The pattern to search for
     */
    template<typename Alloc>
    [[nodiscard]] auto trim_end_matches(const raw::String<Alloc>& pattern) const noexcept -> str
    {
        return this->trim_end_matches(pattern.as_str());
    }

    /**
     * @brief Returns a string slice with all leading copies of the given character removed.
     * @param ch The character to remove
     */
    [[nodiscard]] auto trim_start_matches(Char ch) const noexcept -> str
    {
        size_t i = 0;
        while (i < this->m_len)
        {
            const auto [current, width] = this->decode_at(i);
            if (current != ch)
            {
                break;
            }

            i += width;
        }

        return str(this->m_data + i, this->m_len - i);
    }

    /**
     * @brief Returns a string slice with all leading copies of the given character removed.
     * @param ch The character to remove, interpreted as its numeric value in `[0, 255]`
     */
    [[nodiscard]] auto trim_start_matches(char ch) const noexcept -> str
    {
        return this->trim_start_matches(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns a string slice with all trailing copies of the given character removed.
     * @param ch The character to remove
     */
    [[nodiscard]] auto trim_end_matches(Char ch) const noexcept -> str
    {
        size_t end = this->m_len;
        while (end > 0)
        {
            const auto start = this->previous_boundary(end);
            const auto [current, width] = this->decode_at(start);
            (void)width;

            if (current != ch)
            {
                break;
            }

            end = start;
        }

        return str(this->m_data, end);
    }

    /**
     * @brief Returns a string slice with all trailing copies of the given character removed.
     * @param ch The character to remove, interpreted as its numeric value in `[0, 255]`
     */
    [[nodiscard]] auto trim_end_matches(char ch) const noexcept -> str
    {
        return this->trim_end_matches(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns a string slice with all leading and trailing matches of the pattern removed.
     * @param pattern The pattern to remove
     */
    [[nodiscard]] auto trim_matches(const str& pattern) const noexcept -> str
    {
        return this->trim_start_matches(pattern).trim_end_matches(pattern);
    }

    /**
     * @brief Returns a string slice with all leading and trailing matches of the pattern removed.
     * @param pattern The pattern to remove
     */
    template<typename Alloc>
    [[nodiscard]] auto trim_matches(const raw::String<Alloc>& pattern) const noexcept -> str
    {
        return this->trim_matches(pattern.as_str());
    }

    /**
     * @brief Returns a string slice with all leading and trailing matches of the pattern removed.
     * @param pattern The pattern to remove
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto trim_matches(const char* pattern) const noexcept -> str
    {
        return this->trim_matches(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::trim_matches#pattern"));
    }

    /**
     * @brief Returns a string slice with all leading and trailing copies of the given character removed.
     * @param ch The character to remove
     */
    [[nodiscard]] auto trim_matches(Char ch) const noexcept -> str
    {
        return this->trim_start_matches(ch).trim_end_matches(ch);
    }

    /**
     * @brief Returns a string slice with all leading and trailing copies of the given character removed.
     * @param ch The character to remove, interpreted as its numeric value in `[0, 255]`
     */
    [[nodiscard]] auto trim_matches(char ch) const noexcept -> str
    {
        return this->trim_matches(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns a string slice with all leading and trailing characters matching `pred` removed.
     * @tparam Pred A predicate callable with a `Char`
     * @param pred The predicate deciding which characters to remove
     */
    template<typename Pred>
        requires std::predicate<Pred, Char>
    [[nodiscard]] auto trim_matches(Pred pred) const noexcept -> str
    {
        size_t start = 0;
        while (start < this->m_len)
        {
            const auto [ch, width] = this->decode_at(start);
            if (!pred(ch))
            {
                break;
            }

            start += width;
        }

        size_t end = this->m_len;
        while (end > start)
        {
            const auto previous = this->previous_boundary(end);
            const auto [ch, width] = this->decode_at(previous);
            (void)width;

            if (!pred(ch))
            {
                break;
            }

            end = previous;
        }

        return str(this->m_data + start, end - start);
    }

// iterators
private:
    /**
     * @brief Owns a byte pattern: either borrowed bytes or the UTF-8 encoding of a Char
     * @details Byte patterns that came from a `str`/`const char*` keep pointing at the
     *          caller's storage (borrow semantics). Character patterns are encoded into
     *          `buffer` and stay valid as long as this holder lives, which lets iterators
     *          own their pattern safely.
     */
    struct pattern_holder
    {
        std::byte buffer[4] = {};
        const std::byte* external = nullptr;
        size_t length = 0;

        [[nodiscard]] static constexpr auto from_str(const str& s) noexcept -> pattern_holder
        {
            pattern_holder result;
            result.external = s.m_data;
            result.length = s.m_len;

            return result;
        }

        [[nodiscard]] static constexpr auto from_char(Char ch) noexcept -> pattern_holder
        {
            pattern_holder result;
            result.length = ch.encode_utf8(result.buffer);

            return result;
        }

        [[nodiscard]] constexpr auto data() const noexcept -> const std::byte*
        {
            return this->external != nullptr ? this->external : this->buffer;
        }

        [[nodiscard]] constexpr auto size() const noexcept -> size_t
        {
            return this->length;
        }

        [[nodiscard]] constexpr auto empty() const noexcept -> bool
        {
            return this->length == 0;
        }
    };

    /**
     * @brief Searches a UTF-8 subject for a byte pattern, forwards and backwards
     * @details Mirrors Rust's `Searcher`. The empty pattern matches at every code point
     *          boundary of the subject (including both ends).
     */
    struct pattern_searcher
    {
        const std::byte* subject = nullptr;
        size_t subject_len = 0;
        pattern_holder pattern;
        size_t front = 0;
        size_t back = 0;
        bool front_done = false;
        bool back_done = false;

        constexpr pattern_searcher() noexcept = default;

        constexpr pattern_searcher(const str& s, const pattern_holder& pattern) noexcept
            : subject(s.m_data), subject_len(s.m_len), pattern(pattern), front(0), back(s.m_len) {}

        /**
         * @brief Returns the code point boundary strictly before `index`
         */
        [[nodiscard]] constexpr auto previous_boundary(size_t index) const noexcept -> size_t
        {
            size_t i = index - 1;
            while (i > 0 && (static_cast<std::uint8_t>(this->subject[i]) & 0xC0) == 0x80)
            {
                i -= 1;
            }

            return i;
        }

        /** @brief Returns the next match as a half-open byte range, or None */
        [[nodiscard]] constexpr auto next_match() noexcept -> Option<std::pair<size_t, size_t>>
        {
            if (this->front_done)
            {
                return None{};
            }

            if (this->pattern.empty())
            {
                if (this->front > this->subject_len)
                {
                    this->front_done = true;
                    return None{};
                }

                const auto at = this->front;
                if (at == this->subject_len)
                {
                    this->front = at + 1;
                }
                else
                {
                    this->front = at + str::utf8_sequence_len(this->subject[at]);
                }

                return std::pair<size_t, size_t>{at, at};
            }

            const auto pattern_len = this->pattern.size();
            if (this->front + pattern_len > this->subject_len)
            {
                this->front_done = true;
                return None{};
            }

            const auto found = std::search(
                this->subject + this->front,
                this->subject + this->subject_len,
                this->pattern.data(),
                this->pattern.data() + pattern_len
            );

            if (found == this->subject + this->subject_len)
            {
                this->front_done = true;
                return None{};
            }

            const auto index = static_cast<size_t>(found - this->subject);
            this->front = index + pattern_len;

            return std::pair<size_t, size_t>{index, index + pattern_len};
        }

        /** @brief Returns the previous match (rightmost first), or None */
        [[nodiscard]] constexpr auto next_match_back() noexcept -> Option<std::pair<size_t, size_t>>
        {
            if (this->back_done)
            {
                return None{};
            }

            if (this->pattern.empty())
            {
                const auto at = this->back;
                if (at == 0)
                {
                    this->back_done = true;
                }
                else
                {
                    this->back = this->previous_boundary(at);
                }

                return std::pair<size_t, size_t>{at, at};
            }

            const auto pattern_len = this->pattern.size();
            if (this->back == 0 || pattern_len > this->back)
            {
                this->back_done = true;
                return None{};
            }

            const auto found = std::find_end(
                this->subject,
                this->subject + this->back,
                this->pattern.data(),
                this->pattern.data() + pattern_len
            );

            if (found == this->subject + this->back)
            {
                this->back_done = true;
                return None{};
            }

            const auto index = static_cast<size_t>(found - this->subject);
            this->back = index;

            return std::pair<size_t, size_t>{index, index + pattern_len};
        }
    };

    /**
     * @brief The shared state behind every split iterator
     * @details Mirrors Rust's `SplitInternal`: `start`/`end` bound the not-yet-emitted
     *          part of the subject while the searcher keeps its own cursor.
     */
    struct split_state
    {
        pattern_searcher search;
        size_t start = 0;
        size_t end = 0;
        bool allow_trailing_empty = true;
        bool finished = false;

        constexpr split_state() noexcept = default;

        constexpr split_state(const str& s, const pattern_holder& pattern, bool allow_trailing_empty) noexcept
            : search(s, pattern), start(0), end(s.m_len), allow_trailing_empty(allow_trailing_empty) {}

        [[nodiscard]] constexpr auto get_end() noexcept -> Option<plain_str>
        {
            if (!this->finished)
            {
                this->finished = true;
                if (this->allow_trailing_empty || this->end - this->start > 0)
                {
                    return plain_str(this->search.subject + this->start, this->end - this->start);
                }
            }

            return None{};
        }

        [[nodiscard]] constexpr auto next() noexcept -> Option<plain_str>
        {
            if (this->finished)
            {
                return None{};
            }

            const auto match = this->search.next_match();
            if (match.is_none())
            {
                return this->get_end();
            }

            const auto index = match.unwrap().first;
            const auto piece = plain_str(this->search.subject + this->start, index - this->start);
            this->start = match.unwrap().second;

            return piece;
        }

        [[nodiscard]] constexpr auto next_inclusive() noexcept -> Option<plain_str>
        {
            if (this->finished)
            {
                return None{};
            }

            const auto match = this->search.next_match();
            if (match.is_none())
            {
                return this->get_end();
            }

            const auto index = match.unwrap().second;
            const auto piece = plain_str(this->search.subject + this->start, index - this->start);
            this->start = index;

            return piece;
        }

        [[nodiscard]] constexpr auto next_back_inner() noexcept -> Option<plain_str>
        {
            const auto match = this->search.next_match_back();
            if (match.is_none())
            {
                this->finished = true;
                return plain_str(this->search.subject + this->start, this->end - this->start);
            }

            const auto start_index = match.unwrap().first;
            const auto end_index = match.unwrap().second;
            const auto piece = plain_str(this->search.subject + end_index, this->end - end_index);
            this->end = start_index;

            return piece;
        }

        [[nodiscard]] constexpr auto next_back_inclusive_inner() noexcept -> Option<plain_str>
        {
            const auto match = this->search.next_match_back();
            if (match.is_none())
            {
                this->finished = true;
                return plain_str(this->search.subject + this->start, this->end - this->start);
            }

            const auto end_index = match.unwrap().second;
            const auto piece = plain_str(this->search.subject + end_index, this->end - end_index);
            this->end = end_index;

            return piece;
        }

        [[nodiscard]] constexpr auto next_back() noexcept -> Option<plain_str>
        {
            if (this->finished)
            {
                return None{};
            }

            if (!this->allow_trailing_empty)
            {
                this->allow_trailing_empty = true;
                const auto candidate = this->next_back_inner();
                if (candidate.is_some() && candidate.unwrap().len != 0)
                {
                    return candidate;
                }
                if (this->finished)
                {
                    return None{};
                }
            }

            return this->next_back_inner();
        }

        [[nodiscard]] constexpr auto next_back_inclusive() noexcept -> Option<plain_str>
        {
            if (this->finished)
            {
                return None{};
            }

            if (!this->allow_trailing_empty)
            {
                this->allow_trailing_empty = true;
                const auto candidate = this->next_back_inclusive_inner();
                if (candidate.is_some() && candidate.unwrap().len != 0)
                {
                    return candidate;
                }
                if (this->finished)
                {
                    return None{};
                }
            }

            return this->next_back_inclusive_inner();
        }
    };

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

                    // An empty string has no lines (matches Rust)
                    if (bytes.empty())
                    {
                        this->s = nullptr;
                        return;
                    }

                    size_t pos = 0;
                    bool found = false;

                    // Find the first line ending
                    while (pos < bytes.size())
                    {
                        size_t start = 0;
                        if (bytes[pos] == std::byte{'\n'})
                        {
                            span = plain_str(bytes.data() + start, pos - start);
                            pos += 1; // Skip \n
                            skip = 1;
                            found = true;

                            break;
                        }

                        if (bytes[pos] == std::byte{'\r'} && pos + 1 < bytes.size() && bytes[pos + 1] == std::byte{'\n'})
                        {
                            span = plain_str(bytes.data() + start, pos - start);
                            pos += 2; // Skip \r\n
                            skip = 2;
                            found = true;

                            break;
                        }
                        pos++;
                    }

                    // If no line ending was found, the whole string is a single line
                    if (!found)
                    {
                        span = plain_str(bytes.data(), bytes.size());
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
                size_t start = span.data - bytes.data() + this->span.len + this->skip;

                // If we're at or past the end, mark as finished
                if (start >= bytes.size())
                {
                    this->s = nullptr;
                    span = plain_str(nullptr, 0);
                    return *this;
                }

                // Find the next line ending
                size_t pos = start;
                while (pos < bytes.size())
                {
                    if (bytes[pos] == std::byte{'\n'})
                    {
                        span = plain_str(bytes.data() + start, pos - start);
                        this->skip = 1;
                        break;
                    }
                    else if (bytes[pos] == std::byte{'\r'} && pos + 1 < bytes.size() && bytes[pos + 1] == std::byte{'\n'})
                    {
                        span = plain_str(bytes.data() + start, pos - start);
                        this->skip = 2;
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
        constexpr explicit Lines(const str& s) noexcept : s(s) {}

    public:
        [[nodiscard]] constexpr auto begin() const noexcept -> LinesIter
        {
            return LinesIter(&this->s);
        }

        [[nodiscard]] constexpr auto end() const noexcept -> LinesIter
        {
            return LinesIter(nullptr);
        }
    };

    /**
     * @brief Iterates over the matched slices, forwards or backwards
     */
    struct Matches
    {
        struct MatchesIter
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = str;
            using pointer = const value_type*;
            using reference = value_type;

            const str* s = nullptr;
            pattern_searcher search;
            bool reverse = false;
            plain_str current_match;

            constexpr explicit MatchesIter() noexcept = default;

            constexpr MatchesIter(const str* s, const pattern_holder& pattern, bool reverse) noexcept
                : s(s), reverse(reverse)
            {
                if (s != nullptr)
                {
                    this->search = pattern_searcher(*s, pattern);
                    this->advance();
                }
            }

            constexpr auto advance() noexcept -> void
            {
                if (this->s == nullptr)
                {
                    return;
                }

                const auto match = this->reverse ? this->search.next_match_back() : this->search.next_match();
                if (match.is_none())
                {
                    this->s = nullptr;
                    return;
                }

                const auto found = match.unwrap();
                this->current_match = plain_str(this->search.subject + found.first, found.second - found.first);
            }

            [[nodiscard]] constexpr auto operator*() const noexcept -> value_type
            {
                return str::from_bytes_unchecked(this->current_match.data, this->current_match.len);
            }

            constexpr auto operator++() noexcept -> MatchesIter&
            {
                this->advance();

                return *this;
            }

            constexpr auto operator++(int) noexcept -> MatchesIter
            {
                MatchesIter temp = *this;
                ++(*this);

                return temp;
            }

            [[nodiscard]] constexpr auto operator==(const MatchesIter& other) const noexcept -> bool
            {
                return this->s == other.s;
            }
        };

        const str* s = nullptr;
        pattern_holder pattern;
        bool reverse = false;

        constexpr Matches(const str& s, const pattern_holder& pattern, bool reverse) noexcept
            : s(&s), pattern(pattern), reverse(reverse) {}

        [[nodiscard]] constexpr auto begin() const noexcept -> MatchesIter
        {
            return MatchesIter(this->s, this->pattern, this->reverse);
        }

        [[nodiscard]] constexpr auto end() const noexcept -> MatchesIter
        {
            return MatchesIter();
        }

        using iterator = MatchesIter;
        using const_iterator = MatchesIter;
    };

    /**
     * @brief Iterates over `(byte index, matched slice)` pairs, forwards or backwards
     */
    struct MatchIndices
    {
        struct MatchIndicesIter
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = std::pair<size_t, str>;
            using pointer = const value_type*;
            using reference = value_type;

            const str* s = nullptr;
            pattern_searcher search;
            bool reverse = false;
            size_t current_index = 0;
            plain_str current_match;

            constexpr explicit MatchIndicesIter() noexcept = default;

            constexpr MatchIndicesIter(const str* s, const pattern_holder& pattern, bool reverse) noexcept
                : s(s), reverse(reverse)
            {
                if (s != nullptr)
                {
                    this->search = pattern_searcher(*s, pattern);
                    this->advance();
                }
            }

            constexpr auto advance() noexcept -> void
            {
                if (this->s == nullptr)
                {
                    return;
                }

                const auto match = this->reverse ? this->search.next_match_back() : this->search.next_match();
                if (match.is_none())
                {
                    this->s = nullptr;
                    return;
                }

                this->current_index = match.unwrap().first;
                this->current_match = plain_str(
                    this->search.subject + this->current_index,
                    match.unwrap().second - this->current_index
                );
            }

            [[nodiscard]] constexpr auto operator*() const noexcept -> value_type
            {
                return value_type{
                    this->current_index,
                    str::from_bytes_unchecked(this->current_match.data, this->current_match.len)
                };
            }

            constexpr auto operator++() noexcept -> MatchIndicesIter&
            {
                this->advance();

                return *this;
            }

            constexpr auto operator++(int) noexcept -> MatchIndicesIter
            {
                MatchIndicesIter temp = *this;
                ++(*this);

                return temp;
            }

            [[nodiscard]] constexpr auto operator==(const MatchIndicesIter& other) const noexcept -> bool
            {
                return this->s == other.s;
            }
        };

        const str* s = nullptr;
        pattern_holder pattern;
        bool reverse = false;

        constexpr MatchIndices(const str& s, const pattern_holder& pattern, bool reverse) noexcept
            : s(&s), pattern(pattern), reverse(reverse) {}

        [[nodiscard]] constexpr auto begin() const noexcept -> MatchIndicesIter
        {
            return MatchIndicesIter(this->s, this->pattern, this->reverse);
        }

        [[nodiscard]] constexpr auto end() const noexcept -> MatchIndicesIter
        {
            return MatchIndicesIter();
        }

        using iterator = MatchIndicesIter;
        using const_iterator = MatchIndicesIter;
    };

    /**
     * @brief A lazy split of a `str` that covers the whole split family
     * @details The same range type backs `split`, `rsplit`, `splitn`, `rsplitn`,
     *          `split_terminator`, `rsplit_terminator`, `split_inclusive` and
     *          `rsplit_inclusive`; the constructor flags select the variant.
     */
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
            split_state state;
            size_t remaining = 0;
            bool limited = false;
            bool reverse = false;
            bool inclusive = false;
            plain_str span;

            constexpr explicit SplitIter() noexcept = default;

            constexpr SplitIter(const str* s, const pattern_holder& pattern, bool allow_trailing_empty,
                                bool limited, size_t limit, bool reverse, bool inclusive) noexcept
                : s(s), remaining(limit), limited(limited), reverse(reverse), inclusive(inclusive)
            {
                if (s == nullptr)
                {
                    return;
                }

                this->state = split_state(*s, pattern, allow_trailing_empty);
                this->advance();
            }

            constexpr auto set_piece(const Option<plain_str>& piece) noexcept -> void
            {
                if (piece.is_none())
                {
                    this->s = nullptr;
                    return;
                }

                this->span = piece.unwrap();
            }

            constexpr auto advance() noexcept -> void
            {
                if (this->s == nullptr)
                {
                    return;
                }

                if (this->limited)
                {
                    if (this->remaining == 0)
                    {
                        this->s = nullptr;
                        return;
                    }

                    if (this->remaining == 1)
                    {
                        this->remaining = 0;
                        this->set_piece(this->state.get_end());
                        return;
                    }

                    this->remaining -= 1;
                }

                if (this->reverse)
                {
                    this->set_piece(this->inclusive ? this->state.next_back_inclusive() : this->state.next_back());
                }
                else
                {
                    this->set_piece(this->inclusive ? this->state.next_inclusive() : this->state.next());
                }
            }

            [[nodiscard]] constexpr auto operator*() const noexcept -> const plain_str&
            {
                return this->span;
            }

            [[nodiscard]] constexpr auto operator->() const noexcept -> const plain_str*
            {
                return &this->span;
            }

            constexpr auto operator++() noexcept -> SplitIter&
            {
                this->advance();

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
                return this->s == other.s;
            }
        };

        using iterator = SplitIter;
        using const_iterator = SplitIter;

        const str* s = nullptr;
        pattern_holder pattern;
        bool allow_trailing_empty = true;
        bool limited = false;
        size_t limit = 0;
        bool reverse = false;
        bool inclusive = false;

        constexpr Split() noexcept = default;

        constexpr Split(const str& s, const pattern_holder& pattern, bool allow_trailing_empty,
                        bool limited, size_t limit, bool reverse, bool inclusive) noexcept
            : s(&s), pattern(pattern), allow_trailing_empty(allow_trailing_empty),
              limited(limited), limit(limit), reverse(reverse), inclusive(inclusive) {}

        [[nodiscard]] constexpr auto begin() const noexcept -> SplitIter
        {
            return SplitIter(
                this->s,
                this->pattern,
                this->allow_trailing_empty,
                this->limited,
                this->limit,
                this->reverse,
                this->inclusive
            );
        }

        [[nodiscard]] constexpr auto end() const noexcept -> SplitIter
        {
            return SplitIter();
        }
    };

    /**
     * @brief Builds a `Split` range for this string
     * @param pattern The pattern to split on
     * @param allow_trailing_empty Whether a trailing empty piece is kept / a leading empty piece is emitted
     * @param limited Whether the split is limited to `limit` parts
     * @param limit The maximum number of parts for `splitn`/`rsplitn`
     * @param reverse Whether parts are produced from the right
     * @param inclusive Whether each separator stays on the part it terminates
     */
    [[nodiscard]] constexpr auto make_split(const pattern_holder& pattern, bool allow_trailing_empty,
        bool limited, size_t limit, bool reverse, bool inclusive) const noexcept -> Split
    {
        return Split(*this, pattern, allow_trailing_empty, limited, limit, reverse, inclusive);
    }

    /**
     * @brief Splits a `str` on runs of Unicode whitespace, yielding no empty pieces
     */
    struct SplitWhitespace
    {
        struct SplitWhitespaceIter
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = plain_str;
            using pointer = const plain_str*;
            using reference = const plain_str&;

            const str* s = nullptr;
            size_t pos = 0;
            plain_str span;

            constexpr explicit SplitWhitespaceIter() noexcept = default;

            explicit SplitWhitespaceIter(const str* s) noexcept : s(s)
            {
                if (s != nullptr)
                {
                    this->advance();
                }
            }

            auto advance() noexcept -> void
            {
                if (this->s == nullptr)
                {
                    return;
                }

                utf8proc_int32_t codepoint = 0;
                const auto size = this->s->size();

                // Skip any run of whitespace
                while (this->pos < size)
                {
                    const auto step = utf8proc_iterate(
                        reinterpret_cast<const utf8proc_uint8_t*>(this->s->m_data + this->pos),
                        size - this->pos,
                        &codepoint
                    );

                    if (!Char::from_u32_unchecked(static_cast<std::uint32_t>(codepoint)).is_whitespace())
                    {
                        break;
                    }

                    this->pos += static_cast<size_t>(step);
                }

                if (this->pos >= size)
                {
                    this->s = nullptr;
                    return;
                }

                // Take the following non-whitespace run
                const auto start = this->pos;
                while (this->pos < size)
                {
                    const auto step = utf8proc_iterate(
                        reinterpret_cast<const utf8proc_uint8_t*>(this->s->m_data + this->pos),
                        size - this->pos,
                        &codepoint
                    );

                    if (Char::from_u32_unchecked(static_cast<std::uint32_t>(codepoint)).is_whitespace())
                    {
                        break;
                    }

                    this->pos += static_cast<size_t>(step);
                }

                this->span = plain_str(this->s->m_data + start, this->pos - start);
            }

            auto operator++() noexcept -> SplitWhitespaceIter&
            {
                this->advance();

                return *this;
            }

            auto operator++(int) noexcept -> SplitWhitespaceIter
            {
                SplitWhitespaceIter temp = *this;
                ++(*this);

                return temp;
            }

            [[nodiscard]] constexpr auto operator==(const SplitWhitespaceIter& other) const noexcept -> bool
            {
                return this->s == other.s;
            }

            [[nodiscard]] constexpr auto operator*() const noexcept -> reference
            {
                return this->span;
            }

            [[nodiscard]] constexpr auto operator->() const noexcept -> pointer
            {
                return &this->span;
            }
        };

        const str* s = nullptr;

        constexpr explicit SplitWhitespace(const str& s) noexcept : s(&s) {}

        [[nodiscard]] auto begin() const noexcept -> SplitWhitespaceIter
        {
            return SplitWhitespaceIter(this->s);
        }

        [[nodiscard]] auto end() const noexcept -> SplitWhitespaceIter
        {
            return SplitWhitespaceIter();
        }

        using iterator = SplitWhitespaceIter;
        using const_iterator = SplitWhitespaceIter;
    };

    struct SplitASCIIWhiteSpace
    {
    public:
        const str* s = nullptr;

        struct SplitASCIIWhiteSpaceIter
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = plain_str;
            using pointer = const plain_str*;
            using reference = const plain_str&;

        public:
            const str* s = nullptr;
            plain_str span;
            size_t pos = 0;

            constexpr explicit SplitASCIIWhiteSpaceIter() noexcept = default;

            explicit SplitASCIIWhiteSpaceIter(const str* s) noexcept : s(s)
            {
                if (s != nullptr)
                {
                    utf8proc_int32_t codepoint = 0;
                    const auto size = this->s->size();

                    // Skip leading whitespace
                    while (this->pos < size)
                    {
                        const auto advance = utf8proc_iterate(
                            reinterpret_cast<const utf8proc_uint8_t*>(this->s->m_data + this->pos),
                            size - this->pos,
                            &codepoint
                        );

                        if (!std::bit_cast<Char>(codepoint).is_ascii_whitespace())
                        {
                            break;
                        }

                        this->pos += advance;
                    }

                    // If we've reached the end or string is all whitespace, set to end iterator
                    if (this->pos >= size)
                    {
                        this->s = nullptr;
                        return;
                    }

                    // Find the end of the current non-whitespace sequence
                    const auto start_pos = this->pos;
                    while (this->pos < size)
                    {
                        const auto advance = utf8proc_iterate(
                            reinterpret_cast<const utf8proc_uint8_t*>(this->s->m_data + this->pos),
                            size - this->pos,
                            &codepoint
                        );

                        if (std::bit_cast<Char>(codepoint).is_ascii_whitespace())
                        {
                            break;
                        }

                        this->pos += advance;
                    }

                    this->span = plain_str(this->s->m_data + start_pos, this->pos - start_pos);
                }
            }

            auto operator++() noexcept -> SplitASCIIWhiteSpaceIter&
            {
                if (this->s == nullptr)
                {
                    return *this;
                }

                utf8proc_int32_t codepoint = 0;
                const auto size = this->s->size();

                // Skip whitespace
                while (this->pos < size)
                {
                    const auto advance = utf8proc_iterate(
                        reinterpret_cast<const utf8proc_uint8_t*>(this->s->m_data + this->pos),
                        size - this->pos,
                        &codepoint
                    );

                    if (!std::bit_cast<Char>(codepoint).is_ascii_whitespace())
                    {
                        break;
                    }

                    this->pos += advance;
                }

                // If we've reached the end, set to end iterator
                if (this->pos >= size)
                {
                    this->s = nullptr;
                    return *this;
                }

                // Find the end of the current non-whitespace sequence
                const auto start_pos = this->pos;
                while (this->pos < size)
                {
                    const auto advance = utf8proc_iterate(
                        reinterpret_cast<const utf8proc_uint8_t*>(this->s->m_data + this->pos),
                        size - this->pos,
                        &codepoint
                    );

                    if (std::bit_cast<Char>(codepoint).is_ascii_whitespace())
                    {
                        break;
                    }

                    this->pos += advance;
                }

                this->span = plain_str(this->s->m_data + start_pos, this->pos - start_pos);

                return *this;
            }

            [[nodiscard]] auto operator++(int) noexcept -> SplitASCIIWhiteSpaceIter
            {
                SplitASCIIWhiteSpaceIter temp = *this;
                ++(*this);
                return temp;
            }

            [[nodiscard]] constexpr auto operator==(const SplitASCIIWhiteSpaceIter& other) const noexcept -> bool
            {
                return this->s == other.s;
            }

            [[nodiscard]] constexpr auto operator*() const noexcept -> reference
            {
                return this->span;
            }

            [[nodiscard]] constexpr auto operator->() const noexcept -> pointer
            {
                return &this->span;
            }
        };

    public:
        constexpr explicit SplitASCIIWhiteSpace(const str& s) : s(&s) {}

    public:
        [[nodiscard]] auto begin() const noexcept -> SplitASCIIWhiteSpaceIter
        {
            return SplitASCIIWhiteSpaceIter(this->s);
        }

        [[nodiscard]] auto end() const noexcept -> SplitASCIIWhiteSpaceIter
        {
            return SplitASCIIWhiteSpaceIter(nullptr);
        }

        using iterator = SplitASCIIWhiteSpaceIter;
        using const_iterator = SplitASCIIWhiteSpaceIter;
    };

    struct Chars
    {
        const str* s = nullptr;

        struct CharsIter
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Char;
            using pointer = const Char*;
            using reference = const Char&;

            const str* s = nullptr;
            size_t pos = 0;
            Char ch;

            constexpr explicit CharsIter() noexcept = default;

            explicit CharsIter(const str* s) noexcept : s(s)
            {
                this->advance();
            }

            constexpr auto advance() noexcept -> void
            {
                if (this->s == nullptr)
                {
                    return;
                }

                if (this->pos >= this->s->size())
                {
                    this->s = nullptr;
                    return;
                }

                this->pos += utf8proc_iterate(
                    reinterpret_cast<const utf8proc_uint8_t*>(this->s->data() + this->pos),
                    this->s->size() - this->pos,
                    reinterpret_cast<utf8proc_int32_t*>(&this->ch)
                );
            }

            constexpr auto operator++() noexcept -> CharsIter&
            {
                this->advance();

                return *this;
            }

            constexpr auto operator++(int) noexcept -> CharsIter
            {
                CharsIter temp = *this;
                ++(*this);

                return temp;
            }

            constexpr auto operator->() const noexcept -> pointer
            {
                return std::bit_cast<const Char*>(&this->ch);
            }

            constexpr auto operator*() const noexcept -> reference
            {
                return this->ch;
            }

            [[nodiscard]] constexpr auto operator==(const CharsIter& other) const noexcept -> bool
            {
                return this->s == other.s;
            }
        };

        constexpr explicit Chars(const str& s) noexcept : s(&s) {}

        auto begin() const noexcept -> CharsIter
        {
            return CharsIter(this->s);
        }

        auto end() const noexcept -> CharsIter
        {
            return CharsIter();
        }

        using iterator = CharsIter;
        using const_iterator = CharsIter;
    };

    struct CharIndices
    {
        const str* s = nullptr;

        struct CharIndicesIter
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = std::pair<size_t, Char>;
            using pointer = const value_type*;
            using reference = const value_type&;

            const str* s = nullptr;
            size_t pos = 0;
            value_type current{};

            constexpr explicit CharIndicesIter() noexcept = default;

            explicit CharIndicesIter(const str* s) noexcept : s(s)
            {
                this->advance();
            }

            constexpr auto advance() noexcept -> void
            {
                if (this->s == nullptr)
                {
                    return;
                }

                if (this->pos >= this->s->size())
                {
                    this->s = nullptr;
                    return;
                }

                utf8proc_int32_t codepoint = 0;
                const auto index = this->pos;
                const auto step = utf8proc_iterate(
                    reinterpret_cast<const utf8proc_uint8_t*>(this->s->data() + this->pos),
                    this->s->size() - this->pos,
                    &codepoint
                );

                this->pos += static_cast<size_t>(step);
                this->current = value_type(index, Char::from_u32_unchecked(static_cast<std::uint32_t>(codepoint)));
            }

            constexpr auto operator++() noexcept -> CharIndicesIter&
            {
                this->advance();

                return *this;
            }

            constexpr auto operator++(int) noexcept -> CharIndicesIter
            {
                CharIndicesIter temp = *this;
                ++(*this);

                return temp;
            }

            [[nodiscard]] constexpr auto operator==(const CharIndicesIter& other) const noexcept -> bool
            {
                return this->s == other.s;
            }

            [[nodiscard]] constexpr auto operator*() const noexcept -> reference
            {
                return this->current;
            }

            [[nodiscard]] constexpr auto operator->() const noexcept -> pointer
            {
                return &this->current;
            }
        };

        constexpr explicit CharIndices(const str& s) noexcept : s(&s) {}

        [[nodiscard]] constexpr auto begin() const noexcept -> CharIndicesIter
        {
            return CharIndicesIter(this->s);
        }

        [[nodiscard]] constexpr auto end() const noexcept -> CharIndicesIter
        {
            return CharIndicesIter();
        }

        using iterator = CharIndicesIter;
        using const_iterator = CharIndicesIter;
    };

    struct Graphemes
    {
        const str* s = nullptr;

        struct GraphemesIter
        {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = plain_str;
            using pointer = const plain_str*;
            using reference = const plain_str&;

            const str* s = nullptr;
            size_t pos = 0;
            utf8proc_int32_t state = 0;
            plain_str current;

            constexpr explicit GraphemesIter() noexcept = default;

            constexpr explicit GraphemesIter(const str* s) noexcept : s(s)
            {
                if (s != nullptr)
                {
                    this->advance();
                }
            }

            constexpr auto advance() noexcept -> void
            {
                if (this->s == nullptr)
                {
                    return;
                }

                if (this->pos >= this->s->size())
                {
                    this->s = nullptr;
                    return;
                }

                utf8proc_int32_t codepoint1 = 0;
                utf8proc_int32_t codepoint2 = 0;
                size_t start_pos = this->pos;

                // Get the first codepoint
                auto advance = utf8proc_iterate(
                    reinterpret_cast<const utf8proc_uint8_t*>(this->s->data() + this->pos),
                    this->s->size() - this->pos,
                    &codepoint1
                );
                this->pos += advance;

                // Keep advancing until we find a grapheme break
                while (this->pos < this->s->size())
                {
                    advance = utf8proc_iterate(
                        reinterpret_cast<const utf8proc_uint8_t*>(this->s->data() + this->pos),
                        this->s->size() - this->pos,
                        &codepoint2
                    );

                    if (utf8proc_grapheme_break_stateful(codepoint1, codepoint2, &this->state))
                    {
                        break;
                    }

                    codepoint1 = codepoint2;
                    this->pos += advance;
                }

                this->current = plain_str(this->s->data() + start_pos, this->pos - start_pos);
            }

            constexpr auto operator++() noexcept -> GraphemesIter&
            {
                this->advance();
                return *this;
            }

            constexpr auto operator++(int) noexcept -> GraphemesIter
            {
                GraphemesIter temp = *this;
                ++(*this);
                return temp;
            }

            [[nodiscard]] constexpr auto operator==(const GraphemesIter& other) const noexcept -> bool
            {
                return this->s == other.s;
            }

            [[nodiscard]] constexpr auto operator*() const noexcept -> reference
            {
                return this->current;
            }

            [[nodiscard]] constexpr auto operator->() const noexcept -> pointer
            {
                return &this->current;
            }
        };

        constexpr explicit Graphemes(const str& s) noexcept : s(&s) {}

        [[nodiscard]] constexpr auto begin() const noexcept -> GraphemesIter
        {
            return GraphemesIter(this->s);
        }

        [[nodiscard]] constexpr auto end() const noexcept -> GraphemesIter
        {
            return GraphemesIter();
        }

        using iterator = GraphemesIter;
        using const_iterator = GraphemesIter;
    };

public:
    [[nodiscard]] constexpr auto chars() const noexcept -> Chars
    {
        return Chars(*this);
    }

    /**
     * @brief Returns an iterator over the byte index and character of this string
     * @return A CharIndices iterator that yields each (byte index, Char) pair
     */
    [[nodiscard]] constexpr auto char_indices() const noexcept -> CharIndices
    {
        return CharIndices(*this);
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
     * @brief Returns an iterator over the disjoint matches of the pattern
     * @param pattern The pattern to match against
     * @return A Matches iterator that yields each matched slice
     */
    [[nodiscard]] constexpr auto matches(const str& pattern) const noexcept -> Matches
    {
        return Matches(*this, pattern_holder::from_str(pattern), false);
    }

    /**
     * @brief Returns an iterator over the disjoint matches of the pattern
     * @param pattern The pattern to match against
     * @return A Matches iterator that yields each matched slice
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto matches(const raw::String<Alloc>& pattern) const noexcept -> Matches
    {
        return Matches(*this, pattern_holder::from_str(pattern.as_str()), false);
    }

    /**
     * @brief Returns an iterator over the disjoint matches of the pattern
     * @param pattern The pattern to match against
     * @return A Matches iterator that yields each matched slice
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto matches(const char* pattern) const noexcept -> Matches
    {
        return Matches(*this, pattern_holder::from_str(
            str::from(pattern).expect("Invalid UTF-8 sequence while calling str::matches#pattern")), false);
    }

    /**
     * @brief Returns an iterator over the disjoint matches of the given character
     * @param ch The character to match
     * @return A Matches iterator that yields each matched slice
     */
    [[nodiscard]] constexpr auto matches(Char ch) const noexcept -> Matches
    {
        return Matches(*this, pattern_holder::from_char(ch), false);
    }

    /**
     * @brief Returns an iterator over the disjoint matches of the given character
     * @param ch The character to match, interpreted as its numeric value in `[0, 255]`
     * @return A Matches iterator that yields each matched slice
     */
    [[nodiscard]] constexpr auto matches(char ch) const noexcept -> Matches
    {
        return this->matches(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator over the disjoint matches of the pattern, in reverse order
     * @param pattern The pattern to match against
     * @return A Matches iterator that yields each matched slice, from the end
     */
    [[nodiscard]] constexpr auto rmatches(const str& pattern) const noexcept -> Matches
    {
        return Matches(*this, pattern_holder::from_str(pattern), true);
    }

    /**
     * @brief Returns an iterator over the disjoint matches of the pattern, in reverse order
     * @param pattern The pattern to match against
     * @return A Matches iterator that yields each matched slice, from the end
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto rmatches(const raw::String<Alloc>& pattern) const noexcept -> Matches
    {
        return Matches(*this, pattern_holder::from_str(pattern.as_str()), true);
    }

    /**
     * @brief Returns an iterator over the disjoint matches of the pattern, in reverse order
     * @param pattern The pattern to match against
     * @return A Matches iterator that yields each matched slice, from the end
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto rmatches(const char* pattern) const noexcept -> Matches
    {
        return Matches(*this, pattern_holder::from_str(
            str::from(pattern).expect("Invalid UTF-8 sequence while calling str::rmatches#pattern")), true);
    }

    /**
     * @brief Returns an iterator over the disjoint matches of the given character, in reverse order
     * @param ch The character to match
     * @return A Matches iterator that yields each matched slice, from the end
     */
    [[nodiscard]] constexpr auto rmatches(Char ch) const noexcept -> Matches
    {
        return Matches(*this, pattern_holder::from_char(ch), true);
    }

    /**
     * @brief Returns an iterator over the disjoint matches of the given character, in reverse order
     * @param ch The character to match, interpreted as its numeric value in `[0, 255]`
     * @return A Matches iterator that yields each matched slice, from the end
     */
    [[nodiscard]] constexpr auto rmatches(char ch) const noexcept -> Matches
    {
        return this->rmatches(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator over the `(byte index, match)` pairs of the pattern
     * @param pattern The pattern to match against
     * @return A MatchIndices iterator yielding each `(index, matched str)` pair
     */
    [[nodiscard]] constexpr auto match_indices(const str& pattern) const noexcept -> MatchIndices
    {
        return MatchIndices(*this, pattern_holder::from_str(pattern), false);
    }

    /**
     * @brief Returns an iterator over the `(byte index, match)` pairs of the pattern
     * @param pattern The pattern to match against
     * @return A MatchIndices iterator yielding each `(index, matched str)` pair
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto match_indices(const raw::String<Alloc>& pattern) const noexcept -> MatchIndices
    {
        return MatchIndices(*this, pattern_holder::from_str(pattern.as_str()), false);
    }

    /**
     * @brief Returns an iterator over the `(byte index, match)` pairs of the pattern
     * @param pattern The pattern to match against
     * @return A MatchIndices iterator yielding each `(index, matched str)` pair
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto match_indices(const char* pattern) const noexcept -> MatchIndices
    {
        return MatchIndices(*this, pattern_holder::from_str(
            str::from(pattern).expect("Invalid UTF-8 sequence while calling str::match_indices#pattern")), false);
    }

    /**
     * @brief Returns an iterator over the `(byte index, match)` pairs of the given character
     * @param ch The character to match
     * @return A MatchIndices iterator yielding each `(index, matched str)` pair
     */
    [[nodiscard]] constexpr auto match_indices(Char ch) const noexcept -> MatchIndices
    {
        return MatchIndices(*this, pattern_holder::from_char(ch), false);
    }

    /**
     * @brief Returns an iterator over the `(byte index, match)` pairs of the given character
     * @param ch The character to match, interpreted as its numeric value in `[0, 255]`
     * @return A MatchIndices iterator yielding each `(index, matched str)` pair
     */
    [[nodiscard]] constexpr auto match_indices(char ch) const noexcept -> MatchIndices
    {
        return this->match_indices(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns a reverse iterator over the `(byte index, match)` pairs of the pattern
     * @param pattern The pattern to match against
     * @return A MatchIndices iterator yielding each `(index, matched str)` pair in reverse order
     */
    [[nodiscard]] constexpr auto rmatch_indices(const str& pattern) const noexcept -> MatchIndices
    {
        return MatchIndices(*this, pattern_holder::from_str(pattern), true);
    }

    /**
     * @brief Returns a reverse iterator over the `(byte index, match)` pairs of the pattern
     * @param pattern The pattern to match against
     * @return A MatchIndices iterator yielding each `(index, matched str)` pair in reverse order
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto rmatch_indices(const raw::String<Alloc>& pattern) const noexcept -> MatchIndices
    {
        return MatchIndices(*this, pattern_holder::from_str(pattern.as_str()), true);
    }

    /**
     * @brief Returns a reverse iterator over the `(byte index, match)` pairs of the pattern
     * @param pattern The pattern to match against
     * @return A MatchIndices iterator yielding each `(index, matched str)` pair in reverse order
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto rmatch_indices(const char* pattern) const noexcept -> MatchIndices
    {
        return MatchIndices(*this, pattern_holder::from_str(
            str::from(pattern).expect("Invalid UTF-8 sequence while calling str::rmatch_indices#pattern")), true);
    }

    /**
     * @brief Returns a reverse iterator over the `(byte index, match)` pairs of the given character
     * @param ch The character to match
     * @return A MatchIndices iterator yielding each `(index, matched str)` pair in reverse order
     */
    [[nodiscard]] constexpr auto rmatch_indices(Char ch) const noexcept -> MatchIndices
    {
        return MatchIndices(*this, pattern_holder::from_char(ch), true);
    }

    /**
     * @brief Returns a reverse iterator over the `(byte index, match)` pairs of the given character
     * @param ch The character to match, interpreted as its numeric value in `[0, 255]`
     * @return A MatchIndices iterator yielding each `(index, matched str)` pair in reverse order
     */
    [[nodiscard]] constexpr auto rmatch_indices(char ch) const noexcept -> MatchIndices
    {
        return this->rmatch_indices(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator that splits the string by the given pattern
     * @param pattern The pattern to split on
     * @return A Split iterator that yields each part of the split string
     */
    [[nodiscard]] constexpr auto split(const str& pattern) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_str(pattern), true, false, 0, false, false);
    }

    /**
     * @brief Returns an iterator that splits the string by the given pattern
     * @param pattern The pattern to split on
     * @return A Split iterator that yields each part of the split string
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto split(const raw::String<Alloc>& pattern) const noexcept -> Split
    {
        return this->split(pattern.as_str());
    }

    /**
     * @brief Returns an iterator that splits the string by the given pattern
     * @param pattern The pattern to split on
     * @return A Split iterator that yields each part of the split string
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto split(const char* pattern) const noexcept -> Split
    {
        return this->split(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::split#pattern"));
    }

    /**
     * @brief Returns an iterator that splits the string by the given character
     * @param ch The character to split on
     * @return A Split iterator that yields each part of the split string
     */
    [[nodiscard]] constexpr auto split(Char ch) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_char(ch), true, false, 0, false, false);
    }

    /**
     * @brief Returns an iterator that splits the string by the given character
     * @param ch The character to split on, interpreted as its numeric value in `[0, 255]`
     * @return A Split iterator that yields each part of the split string
     */
    [[nodiscard]] constexpr auto split(char ch) const noexcept -> Split
    {
        return this->split(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator that splits the string by the pattern, from the right
     * @param pattern The pattern to split on
     * @return A Split iterator that yields each part in reverse order
     */
    [[nodiscard]] constexpr auto rsplit(const str& pattern) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_str(pattern), true, false, 0, true, false);
    }

    /**
     * @brief Returns an iterator that splits the string by the pattern, from the right
     * @param pattern The pattern to split on
     * @return A Split iterator that yields each part in reverse order
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto rsplit(const raw::String<Alloc>& pattern) const noexcept -> Split
    {
        return this->rsplit(pattern.as_str());
    }

    /**
     * @brief Returns an iterator that splits the string by the pattern, from the right
     * @param pattern The pattern to split on
     * @return A Split iterator that yields each part in reverse order
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto rsplit(const char* pattern) const noexcept -> Split
    {
        return this->rsplit(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::rsplit#pattern"));
    }

    /**
     * @brief Returns an iterator that splits the string by the character, from the right
     * @param ch The character to split on
     * @return A Split iterator that yields each part in reverse order
     */
    [[nodiscard]] constexpr auto rsplit(Char ch) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_char(ch), true, false, 0, true, false);
    }

    /**
     * @brief Returns an iterator that splits the string by the character, from the right
     * @param ch The character to split on, interpreted as its numeric value in `[0, 255]`
     * @return A Split iterator that yields each part in reverse order
     */
    [[nodiscard]] constexpr auto rsplit(char ch) const noexcept -> Split
    {
        return this->rsplit(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator that splits the string by the pattern into at most `n` parts
     * @param n The maximum number of parts; the last part holds the remainder
     * @param pattern The pattern to split on
     * @return A Split iterator that yields at most `n` parts
     */
    [[nodiscard]] constexpr auto splitn(size_t n, const str& pattern) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_str(pattern), true, true, n, false, false);
    }

    /**
     * @brief Returns an iterator that splits the string by the pattern into at most `n` parts
     * @param n The maximum number of parts; the last part holds the remainder
     * @param pattern The pattern to split on
     * @return A Split iterator that yields at most `n` parts
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto splitn(size_t n, const raw::String<Alloc>& pattern) const noexcept -> Split
    {
        return this->splitn(n, pattern.as_str());
    }

    /**
     * @brief Returns an iterator that splits the string by the pattern into at most `n` parts
     * @param n The maximum number of parts; the last part holds the remainder
     * @param pattern The pattern to split on
     * @return A Split iterator that yields at most `n` parts
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto splitn(size_t n, const char* pattern) const noexcept -> Split
    {
        return this->splitn(n, str::from(pattern).expect("Invalid UTF-8 sequence while calling str::splitn#pattern"));
    }

    /**
     * @brief Returns an iterator that splits the string by the character into at most `n` parts
     * @param n The maximum number of parts; the last part holds the remainder
     * @param ch The character to split on
     * @return A Split iterator that yields at most `n` parts
     */
    [[nodiscard]] constexpr auto splitn(size_t n, Char ch) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_char(ch), true, true, n, false, false);
    }

    /**
     * @brief Returns an iterator that splits the string by the character into at most `n` parts
     * @param n The maximum number of parts; the last part holds the remainder
     * @param ch The character to split on, interpreted as its numeric value in `[0, 255]`
     * @return A Split iterator that yields at most `n` parts
     */
    [[nodiscard]] constexpr auto splitn(size_t n, char ch) const noexcept -> Split
    {
        return this->splitn(n, Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator that splits the string by the pattern into at most `n` parts, from the right
     * @param n The maximum number of parts; the last part holds the remainder
     * @param pattern The pattern to split on
     * @return A Split iterator that yields at most `n` parts in reverse order
     */
    [[nodiscard]] constexpr auto rsplitn(size_t n, const str& pattern) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_str(pattern), true, true, n, true, false);
    }

    /**
     * @brief Returns an iterator that splits the string by the pattern into at most `n` parts, from the right
     * @param n The maximum number of parts; the last part holds the remainder
     * @param pattern The pattern to split on
     * @return A Split iterator that yields at most `n` parts in reverse order
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto rsplitn(size_t n, const raw::String<Alloc>& pattern) const noexcept -> Split
    {
        return this->rsplitn(n, pattern.as_str());
    }

    /**
     * @brief Returns an iterator that splits the string by the pattern into at most `n` parts, from the right
     * @param n The maximum number of parts; the last part holds the remainder
     * @param pattern The pattern to split on
     * @return A Split iterator that yields at most `n` parts in reverse order
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto rsplitn(size_t n, const char* pattern) const noexcept -> Split
    {
        return this->rsplitn(n, str::from(pattern).expect("Invalid UTF-8 sequence while calling str::rsplitn#pattern"));
    }

    /**
     * @brief Returns an iterator that splits the string by the character into at most `n` parts, from the right
     * @param n The maximum number of parts; the last part holds the remainder
     * @param ch The character to split on
     * @return A Split iterator that yields at most `n` parts in reverse order
     */
    [[nodiscard]] constexpr auto rsplitn(size_t n, Char ch) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_char(ch), true, true, n, true, false);
    }

    /**
     * @brief Returns an iterator that splits the string by the character into at most `n` parts, from the right
     * @param n The maximum number of parts; the last part holds the remainder
     * @param ch The character to split on, interpreted as its numeric value in `[0, 255]`
     * @return A Split iterator that yields at most `n` parts in reverse order
     */
    [[nodiscard]] constexpr auto rsplitn(size_t n, char ch) const noexcept -> Split
    {
        return this->rsplitn(n, Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator that splits the string like `split`, dropping a trailing empty part
     * @param pattern The pattern to split on
     * @return A Split iterator without a trailing empty piece
     */
    [[nodiscard]] constexpr auto split_terminator(const str& pattern) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_str(pattern), false, false, 0, false, false);
    }

    /**
     * @brief Returns an iterator that splits the string like `split`, dropping a trailing empty part
     * @param pattern The pattern to split on
     * @return A Split iterator without a trailing empty piece
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto split_terminator(const raw::String<Alloc>& pattern) const noexcept -> Split
    {
        return this->split_terminator(pattern.as_str());
    }

    /**
     * @brief Returns an iterator that splits the string like `split`, dropping a trailing empty part
     * @param pattern The pattern to split on
     * @return A Split iterator without a trailing empty piece
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto split_terminator(const char* pattern) const noexcept -> Split
    {
        return this->split_terminator(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::split_terminator#pattern"));
    }

    /**
     * @brief Returns an iterator that splits the string like `split`, dropping a trailing empty part
     * @param ch The character to split on
     * @return A Split iterator without a trailing empty piece
     */
    [[nodiscard]] constexpr auto split_terminator(Char ch) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_char(ch), false, false, 0, false, false);
    }

    /**
     * @brief Returns an iterator that splits the string like `split`, dropping a trailing empty part
     * @param ch The character to split on, interpreted as its numeric value in `[0, 255]`
     * @return A Split iterator without a trailing empty piece
     */
    [[nodiscard]] constexpr auto split_terminator(char ch) const noexcept -> Split
    {
        return this->split_terminator(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator that splits the string like `rsplit`, dropping a leading empty part
     * @param pattern The pattern to split on
     * @return A Split iterator without a leading empty piece, in reverse order
     */
    [[nodiscard]] constexpr auto rsplit_terminator(const str& pattern) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_str(pattern), false, false, 0, true, false);
    }

    /**
     * @brief Returns an iterator that splits the string like `rsplit`, dropping a leading empty part
     * @param pattern The pattern to split on
     * @return A Split iterator without a leading empty piece, in reverse order
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto rsplit_terminator(const raw::String<Alloc>& pattern) const noexcept -> Split
    {
        return this->rsplit_terminator(pattern.as_str());
    }

    /**
     * @brief Returns an iterator that splits the string like `rsplit`, dropping a leading empty part
     * @param pattern The pattern to split on
     * @return A Split iterator without a leading empty piece, in reverse order
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto rsplit_terminator(const char* pattern) const noexcept -> Split
    {
        return this->rsplit_terminator(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::rsplit_terminator#pattern"));
    }

    /**
     * @brief Returns an iterator that splits the string like `rsplit`, dropping a leading empty part
     * @param ch The character to split on
     * @return A Split iterator without a leading empty piece, in reverse order
     */
    [[nodiscard]] constexpr auto rsplit_terminator(Char ch) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_char(ch), false, false, 0, true, false);
    }

    /**
     * @brief Returns an iterator that splits the string like `rsplit`, dropping a leading empty part
     * @param ch The character to split on, interpreted as its numeric value in `[0, 255]`
     * @return A Split iterator without a leading empty piece, in reverse order
     */
    [[nodiscard]] constexpr auto rsplit_terminator(char ch) const noexcept -> Split
    {
        return this->rsplit_terminator(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator that splits the string, keeping each separator on the part it terminates
     * @param pattern The pattern to split on
     * @return A Split iterator whose parts include their trailing separator
     */
    [[nodiscard]] constexpr auto split_inclusive(const str& pattern) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_str(pattern), false, false, 0, false, true);
    }

    /**
     * @brief Returns an iterator that splits the string, keeping each separator on the part it terminates
     * @param pattern The pattern to split on
     * @return A Split iterator whose parts include their trailing separator
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto split_inclusive(const raw::String<Alloc>& pattern) const noexcept -> Split
    {
        return this->split_inclusive(pattern.as_str());
    }

    /**
     * @brief Returns an iterator that splits the string, keeping each separator on the part it terminates
     * @param pattern The pattern to split on
     * @return A Split iterator whose parts include their trailing separator
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto split_inclusive(const char* pattern) const noexcept -> Split
    {
        return this->split_inclusive(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::split_inclusive#pattern"));
    }

    /**
     * @brief Returns an iterator that splits the string, keeping each separator on the part it terminates
     * @param ch The character to split on
     * @return A Split iterator whose parts include their trailing separator
     */
    [[nodiscard]] constexpr auto split_inclusive(Char ch) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_char(ch), false, false, 0, false, true);
    }

    /**
     * @brief Returns an iterator that splits the string, keeping each separator on the part it terminates
     * @param ch The character to split on, interpreted as its numeric value in `[0, 255]`
     * @return A Split iterator whose parts include their trailing separator
     */
    [[nodiscard]] constexpr auto split_inclusive(char ch) const noexcept -> Split
    {
        return this->split_inclusive(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator that splits the string in reverse, keeping each separator on the part it terminates
     * @param pattern The pattern to split on
     * @return A Split iterator whose parts include their trailing separator, in reverse order
     */
    [[nodiscard]] constexpr auto rsplit_inclusive(const str& pattern) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_str(pattern), false, false, 0, true, true);
    }

    /**
     * @brief Returns an iterator that splits the string in reverse, keeping each separator on the part it terminates
     * @param pattern The pattern to split on
     * @return A Split iterator whose parts include their trailing separator, in reverse order
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto rsplit_inclusive(const raw::String<Alloc>& pattern) const noexcept -> Split
    {
        return this->rsplit_inclusive(pattern.as_str());
    }

    /**
     * @brief Returns an iterator that splits the string in reverse, keeping each separator on the part it terminates
     * @param pattern The pattern to split on
     * @return A Split iterator whose parts include their trailing separator, in reverse order
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] auto rsplit_inclusive(const char* pattern) const noexcept -> Split
    {
        return this->rsplit_inclusive(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::rsplit_inclusive#pattern"));
    }

    /**
     * @brief Returns an iterator that splits the string in reverse, keeping each separator on the part it terminates
     * @param ch The character to split on
     * @return A Split iterator whose parts include their trailing separator, in reverse order
     */
    [[nodiscard]] constexpr auto rsplit_inclusive(Char ch) const noexcept -> Split
    {
        return this->make_split(pattern_holder::from_char(ch), false, false, 0, true, true);
    }

    /**
     * @brief Returns an iterator that splits the string in reverse, keeping each separator on the part it terminates
     * @param ch The character to split on, interpreted as its numeric value in `[0, 255]`
     * @return A Split iterator whose parts include their trailing separator, in reverse order
     */
    [[nodiscard]] constexpr auto rsplit_inclusive(char ch) const noexcept -> Split
    {
        return this->rsplit_inclusive(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Splits the string at the first occurrence of the pattern
     * @param pattern The pattern to search for
     * @return Some containing the parts before and after the pattern, or None if the pattern is
     *         not found
     */
    [[nodiscard]] constexpr auto split_once(const str& pattern) const noexcept -> Option<std::pair<str, str>>
    {
        if (pattern.m_len == 0)
        {
            return std::pair<str, str>{ str(this->m_data, 0), *this };
        }

        const auto found = this->find(pattern);
        if (found.is_none())
        {
            return None{};
        }

        const auto index = found.unwrap();

        return std::pair<str, str>{
            str(this->m_data, index),
            str(this->m_data + index + pattern.m_len, this->m_len - index - pattern.m_len)
        };
    }

    /**
     * @brief Splits the string at the first occurrence of the pattern
     * @param pattern The pattern to search for
     * @return Some containing the parts before and after the pattern, or None if the pattern is
     *         not found
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto split_once(const raw::String<Alloc>& pattern) const noexcept -> Option<std::pair<str, str>>
    {
        return this->split_once(pattern.as_str());
    }

    /**
     * @brief Splits the string at the first occurrence of the pattern
     * @param pattern The pattern to search for
     * @return Some containing the parts before and after the pattern, or None if the pattern is
     *         not found
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] constexpr auto split_once(const char* pattern) const noexcept -> Option<std::pair<str, str>>
    {
        return this->split_once(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::split_once#pattern"));
    }

    /**
     * @brief Splits the string at the first occurrence of the given character
     * @param ch The character to search for
     * @return Some containing the parts before and after the character, or None if it is not found
     */
    [[nodiscard]] constexpr auto split_once(Char ch) const noexcept -> Option<std::pair<str, str>>
    {
        std::byte buffer[4];

        return this->split_once(str::char_pattern(ch, buffer));
    }

    /**
     * @brief Splits the string at the first occurrence of the given character
     * @param ch The character to search for, interpreted as its numeric value in `[0, 255]`
     * @return Some containing the parts before and after the character, or None if it is not found
     */
    [[nodiscard]] constexpr auto split_once(char ch) const noexcept -> Option<std::pair<str, str>>
    {
        return this->split_once(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Splits the string at the last occurrence of the pattern
     * @param pattern The pattern to search for
     * @return Some containing the parts before and after the pattern, or None if the pattern is
     *         not found
     */
    [[nodiscard]] constexpr auto rsplit_once(const str& pattern) const noexcept -> Option<std::pair<str, str>>
    {
        if (pattern.m_len == 0)
        {
            return std::pair<str, str>{ *this, str(this->m_data + this->m_len, 0) };
        }

        const auto found = this->rfind(pattern);
        if (found.is_none())
        {
            return None{};
        }

        const auto index = found.unwrap();

        return std::pair<str, str>{
            str(this->m_data, index),
            str(this->m_data + index + pattern.m_len, this->m_len - index - pattern.m_len)
        };
    }

    /**
     * @brief Splits the string at the last occurrence of the pattern
     * @param pattern The pattern to search for
     * @return Some containing the parts before and after the pattern, or None if the pattern is
     *         not found
     */
    template<typename Alloc>
    [[nodiscard]] constexpr auto rsplit_once(const raw::String<Alloc>& pattern) const noexcept -> Option<std::pair<str, str>>
    {
        return this->rsplit_once(pattern.as_str());
    }

    /**
     * @brief Splits the string at the last occurrence of the pattern
     * @param pattern The pattern to search for
     * @return Some containing the parts before and after the pattern, or None if the pattern is
     *         not found
     * @note Panics if the pattern is not a valid UTF-8 sequence
     */
    [[nodiscard]] constexpr auto rsplit_once(const char* pattern) const noexcept -> Option<std::pair<str, str>>
    {
        return this->rsplit_once(str::from(pattern).expect("Invalid UTF-8 sequence while calling str::rsplit_once#pattern"));
    }

    /**
     * @brief Splits the string at the last occurrence of the given character
     * @param ch The character to search for
     * @return Some containing the parts before and after the character, or None if it is not found
     */
    [[nodiscard]] constexpr auto rsplit_once(Char ch) const noexcept -> Option<std::pair<str, str>>
    {
        std::byte buffer[4];

        return this->rsplit_once(str::char_pattern(ch, buffer));
    }

    /**
     * @brief Splits the string at the last occurrence of the given character
     * @param ch The character to search for, interpreted as its numeric value in `[0, 255]`
     * @return Some containing the parts before and after the character, or None if it is not found
     */
    [[nodiscard]] constexpr auto rsplit_once(char ch) const noexcept -> Option<std::pair<str, str>>
    {
        return this->rsplit_once(Char(static_cast<std::uint32_t>(static_cast<std::uint8_t>(ch))));
    }

    /**
     * @brief Returns an iterator that splits the string by ASCII whitespaces
     * @return A SplitASCIIWhiteSpace iterator that yields each part of the split string
     */
    [[nodiscard]] constexpr auto split_ascii_whitespace() const noexcept -> SplitASCIIWhiteSpace
    {
       return SplitASCIIWhiteSpace(*this);
    }

    /**
     * @brief Returns an iterator that splits the string by runs of Unicode whitespace
     * @return A SplitWhitespace iterator that yields each non-empty part of the split string
     */
    [[nodiscard]] auto split_whitespace() const noexcept -> SplitWhitespace
    {
        return SplitWhitespace(*this);
    }

    /**
     * @brief Returns an iterator that yields each grapheme in the string
     * @return A Graphemes iterator that yields each grapheme in the string
     */
    [[nodiscard]] constexpr auto graphemes() const noexcept -> Graphemes
    {
        return Graphemes(*this);
    }

// operators
public:
    [[nodiscard]] constexpr auto operator<=>(const str& other) const noexcept -> std::strong_ordering
    {
        // If both strings are empty, they are equal
        if (this->m_len == 0 && other.m_len == 0)
        {
            return std::strong_ordering::equal;
        }

        // If this string is empty, it's less than any non-empty string
        if (this->m_len == 0)
        {
            return std::strong_ordering::less;
        }

        // If other string is empty, this string is greater
        if (other.m_len == 0)
        {
            return std::strong_ordering::greater;
        }

        // Compare byte by byte until we find a difference
        const size_t min_len = std::min(this->m_len, other.m_len);
        for (size_t i = 0; i < min_len; ++i)
        {
            if (this->m_data[i] != other.m_data[i])
            {
                return this->m_data[i] <=> other.m_data[i];
            }
        }

        // If we get here, one string is a prefix of the other
        // The shorter string is considered less
        return this->m_len <=> other.m_len;
    }

    [[nodiscard]] constexpr auto operator==(const str& other) const noexcept -> bool
    {
        if (this->m_len != other.m_len)
        {
            return false;
        }
        return std::equal(this->m_data, this->m_data + this->m_len, other.m_data);
    }
};

namespace strings
{
    constexpr auto join_with(char ch) -> decltype(auto)
    {
        return std::views::join_with(static_cast<std::byte>(ch));
    }

    auto join_with(const char* str) -> decltype(auto)
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
 * @brief A UTF-8 encoded, null-terminated, growable string with custom allocator support
 * @tparam Alloc The allocator type to use for memory management
 */
template<typename Alloc = std::allocator<std::byte>>
struct String
{
    using pointer = typename std::allocator_traits<Alloc>::pointer;
    friend struct crab_cpp::str;

private:
    pointer m_data = nullptr;
    size_t m_len = 0;
    compressed_pair<Alloc, size_t> m_alloc_and_capacity;

    /**
     * @brief Private constructor for internal use
     */
    String(const Alloc& alloc, pointer data, size_t len, size_t capacity) noexcept
        : m_data(data), m_len(len), m_alloc_and_capacity(alloc, capacity) {}

    /**
     * @brief Allocates memory for the string
     * @param capacity The capacity to allocate in bytes
     */
    auto allocate(size_t capacity) -> void
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
        this->m_data[i] = std::byte{0};

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
    [[nodiscard]] static auto from_raw_parts(const char* str, size_t len, const Alloc& alloc = Alloc()) -> Result<String, FromUtf8Error>
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

        pointer data = std::allocator_traits<Alloc>::allocate(alloc_copy, len + 1);
        std::copy(str, str + len, std::bit_cast<char*>(data));
        data[len] = std::byte{0};

        return String(alloc_copy, data, len, len);
    }

    /**
     * @brief Creates a string from a null-terminated, UTF-8 encoded C style string
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
        return std::span(
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
     * @brief Returns a C-style string of this String's contents
     * @return A C-style string of this String's contents
     */
    [[nodiscard]] constexpr auto c_str() const noexcept -> const char*
    {
        return std::bit_cast<const char*>(this->m_data);
    }

    /**
     * @brief Returns the current capacity of the buffer in bytes
     * @return The capacity in bytes
     */
    [[nodiscard]] constexpr auto capacity() const noexcept -> size_t
    {
        return this->m_alloc_and_capacity.second;
    }

    /**
     * @brief Clears the string content but keeps the allocated memory
     */
    constexpr auto clear() noexcept -> void
    {
        this->m_len = 0;
        if (this->m_data != nullptr)
        {
            this->m_data[0] = std::byte{0};
        }
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

    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return this->m_len == 0;
    }

    /**
     * @brief Converts this string to its ASCII lower case equivalent in-place.
     * ASCII letters 'A' to 'Z' are mapped to 'a' to 'z', but non-ASCII letters are unchanged.
     */
    auto make_ascii_lowercase() noexcept -> void
    {
        for (size_t i = 0; i < this->m_len; i += 1)
        {
            const auto ch = static_cast<std::uint8_t>(this->m_data[i]);
            if (ch >= 65 && ch <= 90)
            {
                this->m_data[i] = static_cast<std::byte>(ch | 0x20);
            }
        }
    }

    /**
     * @brief Converts this string to its ASCII lower case equivalent in-place.
     * ASCII letters 'a' to 'z' are mapped to 'A' to 'Z', but non-ASCII letters are unchanged.
     */
    auto make_ascii_uppercase() noexcept -> void
    {
        for (size_t i = 0; i < this->m_len; i += 1)
        {
            const auto ch = static_cast<std::uint8_t>(this->m_data[i]);
            if (ch >= 97 && ch <= 122)
            {
                this->m_data[i] = static_cast<std::byte>(ch & 0xDF);
            }
        }
    }

    /**
     * @brief Returns the current length of the string in bytes
     * @return The length in bytes
     */
    [[nodiscard]] constexpr auto size() const noexcept -> size_t
    {
        return this->m_len;
    }

    /**
     * @brief Reserves capacity for at least additional bytes more than the current length.
     * The allocator may reserve more space to speculatively avoid frequent allocations.
     * After calling reserve, capacity will be greater than or equal to this->size() + additional.
     * Does nothing if capacity is already sufficient.
     * @param additional The number of additional bytes to reserve
     * @note Panics if the new capacity overflows size_t
     */
    auto reserve(size_t additional) -> void
    {
        if (additional > 0 && additional < 16)
        {
            additional = 16;
        }

        const size_t new_capacity = this->size() + additional;

        if (new_capacity < this->size())
        {
            panic("New capacity overflows in String::reserve");
        }

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
     * @brief Splits the string into two at the given byte index
     * @param at The byte index at which to split
     * @return A newly allocated String containing bytes [at, len)
     * @note Panics If at is not on a UTF-8 code point boundary, or if it is beyond the last code point
     */
    auto split_off(size_t at) -> String
    {
        if (at > this->m_len)
        {
            panic("Split index out of bounds");
        }

        // Verify that 'at' is on a UTF-8 code point boundary
        if (this->m_len > 0 &&
            (!is_valid_utf8(this->m_data, at) || !is_valid_utf8(this->m_data + at, this->m_len - at)))
        {
            panic("Split index not on UTF-8 code point boundary");
        }

        // Create new string with the second part
        String result(this->m_alloc_and_capacity.first(), nullptr, 0, 0);
        result.reserve(this->m_len - at);

        // Copy the bytes [at, len) to the new string
        if (this->m_len > at)
        {
            std::copy(this->m_data + at,
                     this->m_data + this->m_len,
                     result.m_data);
            result.m_len = this->m_len - at;
            // Add null terminator to the new string
            result.m_data[result.m_len] = std::byte{0};
        }

        // Update this string's length and add null terminator
        this->m_len = at;
        if (this->m_data != nullptr)
        {
            this->m_data[this->m_len] = std::byte{0};
        }

        return result;
    }

    /**
     * @brief Shortens this String to the specified length.
     * @details If new_len is greater than or equal to the string's current length, this has no effect.
     *          Note that this method has no effect on the allocated capacity of the string
     * @param new_len The new length of the string
     * @note Panics if new_len does not lie on a char boundary.
     */
    auto truncate(size_t new_len) noexcept -> void
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
        size_t start = this->m_len - 1;
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
        if (this != &other)
        {
            this->clear();
            (*this) += other;
        }

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

        const size_t new_len = this->m_len + static_cast<size_t>(len);
        if (new_len > this->m_alloc_and_capacity.second)
        {
            this->reserve(new_len - this->m_alloc_and_capacity.second);
        }

        std::copy(utf8, utf8 + len, reinterpret_cast<utf8proc_uint8_t*>(this->m_data + this->m_len));
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

        const size_t new_len = this->m_len + str.size();
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

    auto operator+(const Char ch) -> String
    {
        auto string = String();
        string.reserve(this->m_len + sizeof(Char));
        string += *this;
        string += ch;

        return string;
    }

    auto operator+(const char* str) -> String
    {
        return (*this) + str::from(str).expect("Invalid UTF-8 sequence while calling String::operator+=");
    }

    [[nodiscard]] constexpr auto operator<=>(const String& other) const noexcept -> std::strong_ordering
    {
        return this->as_str() <=> other.as_str();
    }

    [[nodiscard]] constexpr auto operator==(const String& other) const noexcept -> bool
    {
        if (this->m_len != other.m_len)
        {
            return false;
        }

        return std::equal(this->m_data, this->m_data + this->m_len, other.m_data);
    }

    [[nodiscard]] constexpr auto operator==(const str& other) const noexcept -> bool
    {
        if (this->size() != other.size())
        {
            return false;
        }

        return std::equal(this->data(), this->data() + this->size(), other.data());
    }

    [[nodiscard]] constexpr auto operator==(const std::string& other) const noexcept -> bool
    {
        if (this->size() != other.size())
        {
            return false;
        }

        return std::equal(this->data(), this->data() + this->size(), other.data());
    }

    [[nodiscard]] constexpr auto operator==(const char* other) const noexcept -> bool
    {
        if (this->size() != std::strlen(other))
        {
            return false;
        }

        return std::equal(this->data(), this->data() + this->size(), reinterpret_cast<const std::byte*>(other));
    }

private:
    /**
     * For internal use
     */
    auto push_str_unchecked(const str& str) noexcept
    {
        const size_t new_len = this->m_len + str.size();
        std::copy(str.data(), str.data() + str.size(), this->m_data + this->m_len);

        this->m_len = new_len;
        // Add null terminator
        this->m_data[this->m_len] = std::byte{0};

        return *this;
    }
};

}

using String = raw::String<std::allocator<std::byte>>;

template<typename Alloc = std::allocator<std::byte>>
[[nodiscard]] constexpr auto operator==(const str& lhs, const raw::String<Alloc>& rhs) noexcept -> bool
{
    return rhs == lhs;
}

template<typename Alloc = std::allocator<std::byte>>
[[nodiscard]] constexpr auto operator==(const std::string& lhs, const raw::String<Alloc>& rhs) noexcept -> bool
{
    return rhs == lhs;
}

template<typename Alloc = std::allocator<std::byte>>
[[nodiscard]] auto operator==(const char* lhs, const raw::String<Alloc>& rhs) noexcept -> bool
{
    return rhs == lhs;
}

[[nodiscard]] auto operator==(const str& lhs, const char* rhs) noexcept -> bool
{
    if (lhs.size() != std::strlen(rhs))
    {
        return false;
    }
    return std::equal(lhs.data(), lhs.data() + lhs.size(), reinterpret_cast<const std::byte*>(rhs));
}

[[nodiscard]] auto operator==(const char* lhs, const str& rhs) noexcept -> bool
{
    return rhs == lhs;
}

[[nodiscard]] constexpr auto operator==(const str& lhs, const std::string& rhs) noexcept -> bool
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    return std::equal((lhs.data()), lhs.data() + lhs.size(), reinterpret_cast<const std::byte*>(rhs.data()));
}

[[nodiscard]] constexpr auto operator==(const std::string& lhs, const str& rhs) noexcept -> bool
{
    return rhs == lhs;
}

[[nodiscard]] constexpr auto operator==(const str& lhs, const std::string_view& rhs) noexcept -> bool
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    return std::equal((lhs.data()), lhs.data() + lhs.size(), reinterpret_cast<const std::byte*>(rhs.data()));
}

[[nodiscard]] constexpr auto operator==(const std::string_view& lhs, const str& rhs) noexcept -> bool
{
    return rhs == lhs;
}

/**
 * @brief Explicit specialization of `parse` for `bool`
 * @details `std::from_chars` has no `bool` overload, so the generic
 *          implementation does not compile for `bool` even though `bool`
 *          satisfies the primary template's `std::integral` constraint. This
 *          specialization accepts exactly `"true"` or `"false"` (matching
 *          Rust's `str::parse::<bool>()`), reusing `operator==`, and reports
 *          `std::errc::invalid_argument` for anything else.
 * @return A Result containing either the parsed bool or a std::from_chars_result
 */
template<>
constexpr auto str::parse<bool>() const noexcept -> Result<bool, std::from_chars_result>
{
    if (*this == "true")
    {
        return true;
    }

    if (*this == "false")
    {
        return false;
    }

    return std::from_chars_result{this->as_raw(), std::errc::invalid_argument};
}

namespace literal
{
    [[nodiscard]] auto operator""_s(const char* str, size_t len) noexcept -> crab_cpp::str
    {
        // Use the length the compiler provides rather than strlen, so literals
        // containing embedded NUL bytes are not silently truncated.
        return str::from_raw_parts(str, len).expect("Invalid UTF-8 sequence while calling operator\"\"_s");
    }

    [[nodiscard]] auto operator""_S(const char* str, size_t len) -> String
    {
        return std::move(String::from_raw_parts(str, len).ok()).expect("Invalid UTF-8 sequence while calling operator\"\"_S");
    }
}

/**
 * @brief A hash function for String that supports transparent hashing
 */
struct StringHasher
{
    using is_transparent = void;

    auto operator()(const char* str) const -> size_t
    {
        return std::hash<std::string_view>{}(str);
    }

    auto operator()(const std::string& str) const -> size_t
    {
        return std::hash<std::string_view>{}(str);
    }

    auto operator()(const std::string_view& str) const -> size_t
    {
        return std::hash<std::string_view>{}(str);
    }

    auto operator()(const str& str) const -> size_t
    {
        return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(str.data()), str.size()));
    }

    auto operator()(const String& str) const -> size_t
    {
        return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(str.data()), str.size()));
    }
};

}

template<>
struct std::formatter<crab_cpp::Char> : std::formatter<std::uint32_t>
{
    static auto format(const crab_cpp::Char& ch, std::format_context& ctx)
    {
        return std::format_to(ctx.out(), "u{:x}", ch.code_point());
    }
};

export auto operator<<(std::ostream& os, const crab_cpp::Char& ch) -> std::ostream&
{
    // Save/restore the flags of the stream actually being written to. The
    // previous implementation read from (and restored) `std::cout`, which
    // leaked `std::hex` onto any other stream (e.g. a std::ostringstream).
    const std::ios_base::fmtflags original_flags = os.flags();
    os << std::hex << "u" << ch.code_point();
    os.flags(original_flags);

    return os;
}

template<>
struct std::formatter<crab_cpp::str> : std::formatter<const char*>
{
    static auto format(const crab_cpp::str& str, std::format_context& ctx)
    {
        return std::format_to(ctx.out(), "{}", std::string_view(str.as_raw(), str.size()));
    }
};

export auto operator<<(std::ostream& os, const crab_cpp::str& str) -> std::ostream&
{
    os << std::string_view(str.as_raw(), str.size());
    return os;
}

template<typename Alloc>
struct std::formatter<crab_cpp::raw::String<Alloc>> : std::formatter<const char*>
{
    static auto format(const crab_cpp::raw::String<Alloc>& str, std::format_context& ctx)
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

template<>
struct std::hash<crab_cpp::Char>
{
    auto operator()(const crab_cpp::Char& ch) const noexcept -> size_t
    {
		return std::hash<std::uint32_t>{}(ch.code_point());
	}
};

template<>
struct std::hash<crab_cpp::str>
{
    auto operator()(const crab_cpp::str& str) const noexcept -> size_t
    {
		return std::hash<std::string_view>{}(std::string_view(str.as_raw(), str.size()));
	}
};

template<typename Alloc>
struct std::hash<crab_cpp::raw::String<Alloc>>
{
    auto operator()(const crab_cpp::raw::String<Alloc>& str) const noexcept -> size_t
    {
		return std::hash<std::string_view>{}(std::string_view(reinterpret_cast<const char*>(str.data()), str.size()));
	}
};

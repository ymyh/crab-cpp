module;

#include "include/utf8proc.h"

export module crab_cpp:string;

#if defined(_MSC_VER) && defined(__clang__)
#define offsetof __builtin_offsetof
#endif

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

    [[nodiscard]] constexpr static auto offset_of_second() noexcept -> size_t
    {
        return 0;
    }
};

template<typename T1, typename T2>
struct compressed_pair<T1, T2, false>
{
    T1 first;
    T2 second;

    constexpr compressed_pair(T1& x, const T2& y) noexcept : first(x), second(y) {}

    [[nodiscard]] constexpr auto get_first() noexcept -> T1& { return first; }
    [[nodiscard]] constexpr auto get_first() const noexcept -> const T1& { return first; }

    [[nodiscard]] constexpr static auto offset_of_second() noexcept -> size_t
    {
        return offsetof(compressed_pair<T1, T2, false>, second);
    }
};

struct plain_str
{
    const std::byte* data;
    size_t len;

    [[nodiscard]] constexpr auto operator<=>(const plain_str& other) const noexcept -> std::strong_ordering = default;
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
[[nodiscard]] constexpr auto is_valid_utf8(const char* str, std::size_t len) noexcept -> bool
{
    if (str == nullptr || len == 0)
    {
        return true;
    }

    utf8proc_int32_t codepoint;
    std::size_t pos = 0;

    while (pos < len)
    {
        utf8proc_ssize_t result = utf8proc_iterate(
            reinterpret_cast<const utf8proc_uint8_t*>(str + pos),
            len - pos,
            &codepoint
        );

        if (result < 0)
        {
            return false;
        }
        pos += result;
    }
    return true;
}

[[nodiscard]] constexpr auto is_valid_utf8(const std::byte* str, std::size_t len) noexcept -> bool
{
    return is_valid_utf8(std::bit_cast<const char*>(str), len);
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
                utf8proc_ssize_t result = utf8proc_iterate(
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
     * @brief Returns a byte slice of this String's contents
     * @return A span containing the string's bytes
     */
    [[nodiscard]] constexpr auto as_bytes() const noexcept -> std::span<const std::byte>
    {
        return std::span<const std::byte>(this->m_data, this->m_len);
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
     * @brief Checks if the string is empty
     */
    [[nodiscard]] constexpr auto empty() const noexcept -> bool
    {
        return this->m_len == 0;
    }

    /**
     * @brief Returns the length of the string in bytes
     */
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
    {
        return this->m_len;
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

        return std::memcmp(this->m_data, pattern.m_data, pattern.m_len) == 0;
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

        return std::memcmp(
            this->m_data + (this->m_len - pattern.m_len),
            pattern.m_data,
            pattern.m_len
        ) == 0;
    }

    /**
     * @brief Converts the String to a std::string
     * @return A std::string containing the String's contents
     */
    [[nodiscard]] constexpr auto to_std_string() const -> std::string
    {
        return std::string(reinterpret_cast<const char*>(this->m_data), this->m_len);
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

// iterators
private:
    struct Lines
    {
        struct LinesIter
        {
            using iterator_category = std::forward_iterator_tag;
            using value_type = str;
            using difference_type = std::ptrdiff_t;
            using pointer = str*;
            using reference = str&;

            const str* s = nullptr;
            plain_str span;

            constexpr explicit LinesIter() noexcept {}

            constexpr explicit LinesIter(const str* s) noexcept : s(s)
            {
                if (s != nullptr) {
                    // Initialize the first line
                    auto bytes = s->as_bytes();
                    std::size_t start = 0;
                    std::size_t pos = 0;

                    // Find the first line ending
                    while (pos < bytes.size()) {
                        if (bytes[pos] == std::byte{'\n'}) {
                            span = plain_str{bytes.data() + start, pos - start};
                            pos++; // Skip \n
                            break;
                        } else if (bytes[pos] == std::byte{'\r'} && pos + 1 < bytes.size() && bytes[pos + 1] == std::byte{'\n'}) {
                            span = plain_str{bytes.data() + start, pos - start};
                            pos += 2; // Skip \r\n
                            break;
                        }
                        pos++;
                    }

                    // If no line ending found, take the whole string
                    if (pos >= bytes.size()) {
                        span = plain_str{bytes.data(), bytes.size()};
                        pos = bytes.size();
                    }

                    // Store current position for next iteration
                    span.len = pos - start;
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
                if (s == nullptr)
                {
                    return *this;
                }

                auto bytes = s->as_bytes();
                std::size_t start = span.data - bytes.data() + span.len;

                // If we're at or past the end, mark as finished
                if (start >= bytes.size())
                {
                    s = nullptr;
                    span = plain_str{nullptr, 0};
                    return *this;
                }

                // Skip the line ending we found in the previous iteration
                if (bytes[start] == std::byte{'\n'})
                {
                    start++;
                }
                else if (bytes[start] == std::byte{'\r'} && start + 1 < bytes.size() && bytes[start + 1] == std::byte{'\n'})
                {
                    start += 2;
                }

                // If we're at the end after skipping line ending, mark as finished
                if (start >= bytes.size())
                {
                    s = nullptr;
                    span = plain_str{nullptr, 0};
                    return *this;
                }

                // Find the next line ending
                std::size_t pos = start;
                while (pos < bytes.size())
                {
                    if (bytes[pos] == std::byte{'\n'})
                    {
                        span = plain_str{bytes.data() + start, pos - start};
                        break;
                    } else if (bytes[pos] == std::byte{'\r'} && pos + 1 < bytes.size() && bytes[pos + 1] == std::byte{'\n'})
                    {
                        span = plain_str{bytes.data() + start, pos - start};
                        break;
                    }
                    pos++;
                }

                // If no line ending found, take the rest of the string
                if (pos >= bytes.size())
                {
                    span = plain_str{bytes.data() + start, bytes.size() - start};
                }

                return *this;
            }

            constexpr auto operator++(int) noexcept -> LinesIter
            {
                LinesIter temp = *this;
                ++(*this);
                return temp;
            }

            [[nodiscard]] constexpr auto operator<=>(const LinesIter& other) const noexcept -> std::strong_ordering = default;
        };

    public:
        const str& s;

    public:
        constexpr explicit Lines(const str& s) : s(s) {}

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

// operators
public:
    [[nodiscard]] constexpr auto operator<=>(const str& other) const noexcept -> std::strong_ordering
    {
        const std::size_t min_len = std::min(this->m_len, other.m_len);
        if (min_len == 0)
        {
            return this->m_len <=> other.m_len;
        }

        const int result = std::memcmp(this->m_data, other.m_data, min_len);
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
        return std::memcmp(this->m_data, other.m_data, this->m_len) == 0;
    }
};

/**
 * @brief A UTF-8–encoded, null-terminated, growable string with custom allocator support
 * @tparam Alloc The allocator type to use for memory management
 */
template<typename Alloc = std::allocator<std::byte>>
struct String
{
    using pointer = typename std::allocator_traits<Alloc>::pointer;

private:
    struct StrProxy
    {
        str value;

        constexpr explicit StrProxy(const str& s) noexcept : value(s) {}
        constexpr auto operator->() const noexcept -> const str* { return &value; }
    };

    compressed_pair<Alloc, pointer> m_alloc_and_data;
    std::size_t m_len;
    std::size_t m_capacity;

    /**
     * @brief Private constructor for internal use
     */
    String(const Alloc& alloc, pointer data, std::size_t len, std::size_t capacity) noexcept
        : m_alloc_and_data(alloc, data), m_len(len), m_capacity(capacity) {}

    /**
     * @brief Allocates memory for the string
     * @param capacity The capacity to allocate in bytes
     */
    auto allocate(std::size_t capacity) -> void
    {
        if (capacity == 0)
        {
            this->m_alloc_and_data.second = nullptr;
            this->m_capacity = 0;
            return;
        }

        // Add 1 to capacity for null terminator
        this->m_alloc_and_data.second = std::allocator_traits<Alloc>::allocate(
            this->m_alloc_and_data.first(),
            capacity + 1
        );
        this->m_capacity = capacity;
        // Add null terminator
        this->m_alloc_and_data.second[0] = std::byte{0};
    }

// constructors
public:
    /**
     * @brief Default constructor
     */
    constexpr explicit String() noexcept : m_alloc_and_data(Alloc(), nullptr), m_len(0), m_capacity(0) {}

    /**
     * @brief Constructs a String from a str
     * @param str The str to construct from
     * @param alloc The allocator to use
     */
    constexpr explicit String(const str& str, const Alloc& alloc = Alloc()) : String(alloc, str.data(), str.size(), str.size()) {}

    /**
     * @brief Creates a string from a UTF-8 string
     * @param str The input string
     * @param len The length of the input string
     * @param alloc The allocator to use
     * @return A Result containing either a String or a FromUtf8Error
     */
    [[nodiscard]] static auto from_raw_parts(const char* str, std::size_t len, const Alloc& alloc = Alloc()) noexcept -> Result<String, FromUtf8Error>
    {
        auto alloc_copy = alloc;

        if (!str || len == 0)
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
    [[nodiscard]] static auto from(const char* str, const Alloc& alloc = Alloc()) noexcept -> Result<String, FromUtf8Error>
    {
        return from_raw_parts(str, std::strlen(str), alloc);
    }

    /**
     * @brief Copy constructor
     * @param other The string to copy from
     */
    String(const String& other)
        : m_alloc_and_data(
            std::allocator_traits<Alloc>::select_on_container_copy_construction(other.m_alloc_and_data.first()),
            nullptr
        )
        , m_len(0), m_capacity(0)
    {
        if (other.m_len > 0)
        {
            this->allocate(other.m_len);
            std::copy(other.m_alloc_and_data.second,
                     other.m_alloc_and_data.second + other.m_len,
                     m_alloc_and_data.second);

            this->m_len = other.m_len;
        }
    }

    /**
     * @brief Move constructor
     * @param other The string to move from
     */
    String(String&& other) noexcept
        : m_alloc_and_data(std::move(other.m_alloc_and_data)), m_len(other.m_len), m_capacity(other.m_capacity)
    {
        other.m_alloc_and_data.second = nullptr;
        other.m_len = 0;
        other.m_capacity = 0;
    }

    /**
     * @brief Destructor
     */
    ~String()
    {
        if (this->m_alloc_and_data.second)
        {
            std::allocator_traits<Alloc>::deallocate(
                this->m_alloc_and_data.first(),
                this->m_alloc_and_data.second,
                this->m_capacity + 1
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
            reinterpret_cast<const std::byte*>(this->m_alloc_and_data.second),
        this->m_len
        );
    }

    /**
     * @brief Returns the current capacity of the buffer in bytes
     * @return The capacity in bytes
     */
    [[nodiscard]] constexpr auto capacity() const noexcept -> std::size_t
    {
        return this->m_capacity;
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
        return this->m_alloc_and_data.second;
    }

    /**
     * @brief Returns a pointer to the string data
     */
    [[nodiscard]] constexpr auto data() noexcept -> std::byte*
    {
        return this->m_alloc_and_data.second;
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
        if (new_capacity <= this->m_capacity)
        {
            return;
        }

        // Allocate new memory (add 1 for null terminator)
        pointer new_data = std::allocator_traits<Alloc>::allocate(
            this->m_alloc_and_data.first(),
            new_capacity + 1
        );

        // Copy existing data
        if (this->m_len > 0)
        {
            std::copy(this->m_alloc_and_data.second,
                    this->m_alloc_and_data.second + this->m_len,
                    new_data);
        }
        // Add null terminator
        new_data[this->m_len] = std::byte{0};

        // Deallocate old memory
        if (this->m_alloc_and_data.second != nullptr)
        {
            std::allocator_traits<Alloc>::deallocate(
                this->m_alloc_and_data.first(),
                this->m_alloc_and_data.second,
                this->m_capacity + 1
            );
        }

        // Update members
        this->m_alloc_and_data.second = new_data;
        this->m_capacity = new_capacity;
    }

    /**
     * @brief Appends the given Char to the end of this String.
     * @param ch The Char to append
     */
    auto push(Char ch) -> void
    {
        // Convert Unicode scalar value to UTF-8
        utf8proc_uint8_t utf8[4];
        utf8proc_ssize_t len = utf8proc_encode_char(
            static_cast<utf8proc_int32_t>(ch.code_point()),
            utf8
        );

        if (len < 0)
        {
            panic("Failed to encode Unicode scalar value to UTF-8");
        }

        const std::size_t new_len = this->m_len + static_cast<std::size_t>(len);
        if (new_len > this->m_capacity)
        {
            this->reserve(new_len - this->m_capacity);
        }

        std::copy(utf8, utf8 + len, this->m_alloc_and_data.second + this->m_len);
        this->m_len = new_len;
        // Add null terminator
        this->m_alloc_and_data.second[this->m_len] = std::byte{0};
    }

    /**
     * @brief Appends a given str onto the end of this String.
     * @param str The str to append
     */
    auto push_str(const str& str) -> void
    {
        if (str.empty())
        {
            return;
        }

        const std::size_t new_len = this->m_len + str.size();
        if (new_len > this->m_capacity)
        {
            this->reserve(new_len - this->m_capacity);
        }

        std::copy(str.data(), str.data() + str.size(), this->m_alloc_and_data.second + this->m_len);
        this->m_len = new_len;
        // Add null terminator
        this->m_alloc_and_data.second[this->m_len] = std::byte{0};
    }

    /**
     * @brief Appends a given C-string onto the end of this String.
     * @param str The C-string to append
     */
    auto push_str(const char* str) -> void
    {
        this->push_str(str::from(str).expect("Failed to convert C-string to str while calling String::push_str"));
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
        if (!is_valid_utf8(reinterpret_cast<const char*>(this->m_alloc_and_data.second), at) ||
            !is_valid_utf8(reinterpret_cast<const char*>(this->m_alloc_and_data.second + at), this->m_len - at))
        {
            panic("Split index not on UTF-8 code point boundary");
        }

        // Create new string with the second part
        String result(this->m_alloc_and_data.first(), nullptr, 0, 0);
        result.reserve(this->m_len - at);

        // Copy the bytes [at, len) to the new string
        std::copy(this->m_alloc_and_data.second + at,
                 this->m_alloc_and_data.second + this->m_len,
                 result.m_alloc_and_data.second);
        result.m_len = this->m_len - at;
        // Add null terminator to the new string
        result.m_alloc_and_data.second[result.m_len] = std::byte{0};

        // Update this string's length and add null terminator
        this->m_len = at;
        this->m_alloc_and_data.second[this->m_len] = std::byte{0};

        return result;
    }

    /**
     * @brief Converts the String to a std::string
     * @return A std::string containing the String's contents
     */
    [[nodiscard]] auto to_std_string() const -> std::string
    {
        return std::string(std::bit_cast<const char*>(this->m_alloc_and_data.second), this->m_len);
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

        if (!is_valid_utf8(this->m_alloc_and_data.second, new_len))
        {
            panic("Invalid UTF-8 sequence");
        }

        this->m_len = new_len;
        this->m_alloc_and_data.second[this->m_len] = '\0';
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
        while (start > 0 && (this->m_alloc_and_data.second[start] & std::byte{0xC0}) == std::byte{0x80})
        {
            start -= 1;
        }

        // Decode the UTF-8 sequence
        utf8proc_int32_t codepoint;
        utf8proc_iterate(
            reinterpret_cast<const utf8proc_uint8_t*>(this->m_alloc_and_data.second + start),
            this->m_len - start,
            &codepoint
        );

        // Update string length
        this->m_len = start;
        // Add null terminator at new end
        this->m_alloc_and_data.second[this->m_len] = std::byte{0};

        return Char(static_cast<std::uint32_t>(codepoint));
    }

//operators
public:
    [[nodiscard]] auto operator->() noexcept -> StrProxy
    {
        return StrProxy(str::from_bytes_unchecked(this->m_alloc_and_data.second, this->m_len));
    }

    [[nodiscard]] auto operator->() const noexcept -> StrProxy
    {
        return StrProxy(str::from_bytes_unchecked(this->m_alloc_and_data.second, this->m_len));
    }

    /**
     * @brief Appends a str to this String using the += operator
     * @param str The str to append
     * @return A reference to this String
     */
    auto operator+=(const str& str) -> String&
    {
        this->push_str(str);
        return *this;
    }

    /**
     * @brief Appends a C-string to this String using the += operator
     * @param str The C-string to append
     * @return A reference to this String
     */
    auto operator+=(const char* str) -> String&
    {
        this->push_str(str);
        return *this;
    }

    [[nodiscard]] constexpr auto operator<=>(const String& other) const noexcept -> std::strong_ordering
    {
        const std::size_t min_len = std::min(this->m_len, other.m_len);
        if (min_len == 0)
        {
            return this->m_len <=> other.m_len;
        }

        const int result = std::memcmp(this->m_alloc_and_data.second, other.m_alloc_and_data.second, min_len);
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
        return std::memcmp(this->m_alloc_and_data.second, other.m_alloc_and_data.second, this->m_len) == 0;
    }
};

template<typename Alloc>
[[nodiscard]] constexpr auto operator==(const String<Alloc>& lhs, const str& rhs) noexcept -> bool
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

template<typename Alloc>
[[nodiscard]] constexpr auto operator==(const str& lhs, const String<Alloc>& rhs) noexcept -> bool
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

}

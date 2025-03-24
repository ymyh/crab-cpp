module;

#include "include/utf8proc.h"

export module crab_cpp:string;

import :option;
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
};

template<typename T1, typename T2>
struct compressed_pair<T1, T2, false>
{
    T1 first;
    T2 second;

    constexpr compressed_pair(T1& x, const T2& y) noexcept : first(x), second(y) {}

    [[nodiscard]] constexpr auto get_first() noexcept -> T1& { return first; }
    [[nodiscard]] constexpr auto get_first() const noexcept -> const T1& { return first; }
};

export namespace crab_cpp
{

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
     * @throw std::invalid_argument if the value is not a valid Unicode scalar value
     */
    constexpr explicit Char(std::uint32_t value)
    {
        if (!is_valid_scalar_value(value))
        {
            throw std::invalid_argument("Invalid Unicode scalar value");
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

/**
 * @brief A UTF-8 string implementation with custom allocator support
 * @tparam Alloc The allocator type to use for memory management
 */
template<typename Alloc>
struct String
{
private:
    using pointer = typename std::allocator_traits<Alloc>::pointer;

    compressed_pair<Alloc, pointer> m_alloc_and_data;
    std::size_t m_len;
    std::size_t m_capacity;

    /**
     * @brief Validates if the given string is valid UTF-8
     * @param str The string to validate
     * @param len The length of the string in bytes
     * @return true if the string is valid UTF-8, false otherwise
     */
    [[nodiscard]] static constexpr auto is_valid_utf8(const char* str, std::size_t len) noexcept -> bool
    {
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

    /**
     * @brief Allocates memory for the string
     * @param capacity The capacity to allocate in bytes
     */
    void allocate(std::size_t capacity)
    {
        if (capacity == 0)
        {
            this->m_alloc_and_data.second = nullptr;
            this->m_capacity = 0;
            return;
        }

        this->m_alloc_and_data.second = std::allocator_traits<Alloc>::allocate(
            this->m_alloc_and_data.first(),
            capacity
        );
        this->m_capacity = capacity;
    }

public:
    /**
     * @brief Default constructor
     */
    constexpr explicit String() noexcept : m_alloc_and_data(Alloc(), nullptr), m_len(0), m_capacity(0) {}

    /**
     * @brief Constructs a string from a UTF-8 string
     * @param str The input string
     * @param len The length of the input string
     * @param alloc The allocator to use
     * @throw std::invalid_argument if the input is not valid UTF-8
     */
    explicit String(const char* str, std::size_t len, const Alloc& alloc = Alloc())
        : m_alloc_and_data(alloc, nullptr), m_len(0), m_capacity(0)
    {
        if (!str || len == 0)
        {
            return;
        }

        if (!is_valid_utf8(str, len))
        {
            throw std::invalid_argument("Invalid UTF-8 string");
        }

        this->allocate(len);
        std::copy(str, str + len, m_alloc_and_data.second);
        m_len = len;
    }

    /**
     * @brief Constructs a string from a null-terminated UTF-8 string
     * @param str The input string
     * @param alloc The allocator to use
     */
    explicit String(const char* str, const Alloc& alloc = Alloc()) : String(str, std::strlen(str), alloc) {}

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
                this->m_capacity
            );
        }
    }

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
     * @brief Returns the current length of the string in bytes
     * @return The length in bytes
     */
    [[nodiscard]] constexpr auto len() const noexcept -> std::size_t
    {
        return this->m_len;
    }

    /**
     * @brief Clears the string content but keeps the allocated memory
     */
    constexpr auto clear() noexcept -> void
    {
        this->m_len = 0;
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

        // Allocate new memory
        pointer new_data = std::allocator_traits<Alloc>::allocate(
            this->m_alloc_and_data.first(),
            new_capacity
        );

        // Copy existing data
        if (this->m_len > 0)
        {
            std::copy(this->m_alloc_and_data.second,
                     this->m_alloc_and_data.second + this->m_len,
                     new_data);
        }

        // Deallocate old memory
        if (this->m_alloc_and_data.second != nullptr)
        {
            std::allocator_traits<Alloc>::deallocate(
                this->m_alloc_and_data.first(),
                this->m_alloc_and_data.second,
                this->m_capacity
            );
        }

        // Update members
        this->m_alloc_and_data.second = new_data;
        this->m_capacity = new_capacity;
    }
};

}

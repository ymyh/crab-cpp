module;

#include "include/utf8proc.h"

export module crab_cpp:string;

import :option;

import std;

template<typename T1, typename T2, bool = std::is_empty_v<T1> && !std::is_final_v<T1>>
struct compressed_pair : private T1
{
    T2 second;

    compressed_pair() : T1(), second() {}
    compressed_pair(const T1& x, const T2& y) : T1(x), second(y) {}

    T1& first() { return *this; }
    const T1& first() const { return *this; }
};

template<typename T1, typename T2>
struct compressed_pair<T1, T2, false>
{
    T1 first;
    T2 second;

    compressed_pair(T1& x, const T2& y) : first(x), second(y) {}

    T1& get_first() { return first; }
    const T1& get_first() const { return first; }
};

export namespace crab_cpp
{

template<typename Alloc>
struct String
{
private:
    // Pair of allocator and data pointer
    compressed_pair<Alloc, typename std::allocator_traits<Alloc>::pointer> m_alloc_and_data;
    // Current length in bytes
    std::size_t m_len;
    // Buffer capacity in bytes
    std::size_t m_capacity;

    // Helper function to validate UTF-8 string
    static bool is_valid_utf8(const char* str, std::size_t len)
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

    // Helper function to allocate memory
    void allocate(std::size_t capacity)
    {
        if (capacity == 0)
        {
            m_alloc_and_data.second = nullptr;
            m_capacity = 0;
            return;
        }

        m_alloc_and_data.second = std::allocator_traits<Alloc>::allocate(
            m_alloc_and_data.first(),
            capacity
        );
        m_capacity = capacity;
    }

public:
    String() : m_alloc_and_data(Alloc(), nullptr), m_len(0), m_capacity(0)
    {

    }

    // Constructor from UTF-8 string
    String(const char* str, std::size_t len, const Alloc& alloc = Alloc())
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

        allocate(len);
        std::copy(str, str + len, m_alloc_and_data.second);
        m_len = len;
    }

    // Constructor from null-terminated UTF-8 string
    String(const char* str, const Alloc& alloc = Alloc()) : String(str, std::strlen(str), alloc) {}

    String(const String& other)
        : m_alloc_and_data(
            std::allocator_traits<Alloc>::select_on_container_copy_construction(other.m_alloc_and_data.first()),
            nullptr
        )
        , m_len(0), m_capacity(0)
    {
        if (other.m_len > 0)
        {
            allocate(other.m_len);
            std::copy(other.m_alloc_and_data.second,
                     other.m_alloc_and_data.second + other.m_len,
                     m_alloc_and_data.second);

            m_len = other.m_len;
        }
    }

    String(String&& other) noexcept
        : m_alloc_and_data(std::move(other.m_alloc_and_data)), m_len(other.m_len), m_capacity(other.m_capacity)
    {
        other.m_alloc_and_data.second = nullptr;
        other.m_len = 0;
        other.m_capacity = 0;
    }

    ~String()
    {
        if (m_alloc_and_data.second)
        {
            std::allocator_traits<Alloc>::deallocate(
                m_alloc_and_data.first(),
                m_alloc_and_data.second,
                m_capacity
            );
        }
    }
};

}

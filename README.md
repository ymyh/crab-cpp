## Crab_cpp: Rust-like Struct and Function in C++23

### Overview
Crab_cpp is a C++23 library that simulates Rust's Option<T>, Result<T, E>, panic!, unimplemented!, and a match macro to modern C++. Designed for developers who appreciate Rust's explicit error handling and pattern matching but work in C++ ecosystems. This library supports only C++23 and requires std module.

### Features & Usage

#### panic & unimplemented
Prints a message and calls std::terminate().
- If CRAB_CPP_ENABLE_BACKTRACE is defined, it will print a backtrace using std::stacktrace::current()

```c++
import crab_cpp;

int main()
{
    panic("Panic because answer isn't {}", 42);
    unimplemented("Unimplemented!")
}
```

#### Option &lt;T&gt;
Option<T> is inherited from `std::variant<None, T>`, where None is alias of `std::monostate`.
Have two different specialization holds lvalue reference and pointer that has same size as size_t.
Also includes several monadic functions inspired by Rust.

```c++
import crab_cpp;

#include <assert.h>

// For the pattern matching macro
#include "crab_cpp/include/macros.h"

int main()
{
    // Option<int> is a subclass of std::variant<std::monostate, int>
    Option<int> opt = 42;
    assert(opt.is_some());

    // unwrap() returns int&
    int num = opt.unwrap();

    // run the lambda if present
    opt.inspect([](int x)
    {
        // Do something...
    });

    // map to Option<double>
    auto opt2 = opt.map([](int a) -> Option<double>
    {
        return 3.14;
    });

    // take() moves int out of the Option, leaving None behind
    num = opt.take();

    // None is alias of std::monostate
    assert(opt.is_none());

    // reference or pointer are specialized which has same size as size_t
    Option<int&> opt3 = num;

    match (opt3)
    {
        [](int& x)
        {
            // Do something...
        },

        [](None)
        {
            // Do something else...
        }
    };
}
```

#### Result &lt;T, E&gt;
Result<T, E> is inherited from `std::variant<T, E>`, also includes several monadic functions inspired by Rust.

```c++
import crab_cpp;

#include <assert.h>

// For the pattern matching macro
#include "crab_cpp/include/macros.h"

enum class ErrType
{
    Blocked, Timeout
}

Result<int, ErrType> foo(bool b)
{
    if (b)
    {
        return 0;
    }

    return ErrType::Blocked;
}

int main()
{
    auto res = foo(true);

    assert(res.is_ok());

    // unwrap() returns int&
    int num = res.unwrap();

    // ok() moves out the value to an Option, do not use this Result anymore
    Option<int> opt = res.ok();

    match (foo(false))
    {
        [](int x)
        {
            // Do something...
        },

        [](ErrType e)
        {
            // Do something else...
        }
    };
}
```

#### Char & str & String (Optional feature)
Define macro `CRAB_CPP_ENABLE_STRING` to enable this sub-module.
Those three types are basically a weaken version of `char`, `&str`, `String` in Rust, but with null terminator for better interoperability with C/C++ APIs.
Using utf8proc to handle utf8 encoding, supports Unicode 16.
Still lacking many methods, but can be used for simple string operations.
implemented std::formatter and operator<< for print, but it will stop printing while encountering first null terminator.
Noted `String` is actually a alias of `crab_cpp::raw::String<std::allocator<std::byte>>`, use the raw one if you want to use a custom allocator.

```c++
import std;
import crab_cpp;

#include <assert.h>

int main()
{
    auto s = str::from("Hello, world!").unwrap();
    auto ss = String::from("Hello, world!").unwrap();

    assert(s == ss);

    // operator-> returns str, which simulates Deref<Target=str> in Rust
    // split returns a iterator that yields a internal type that can be constructed to str
    auto vec = ss->split(",") | std::ranges::to<std::vector<str>>();
    assert(vec[0] == "Hello");
    assert(vec[1].trim_ascii() == "world!");

    // `join_with` inside namespace `strings` are wrappers of std::views::join_with
    ss = s.split(",") | strings::join_with("\n") | std::ranges::to<String>();
    assert(ss == "Hello\n World!");
}
```

### Why Crab_cpp ?
Compared to std::optional, std::expected, Crab_cpp offers
- Rust-like APIs
- Simple pattern matching support

Compared to std::string_view, std::string, Crab_cpp offers
- UTF-8 encoded
- Rust-like APIs
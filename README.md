## Table of Contents
- [Table of Contents](#table-of-contents)
- [Crab\_cpp: Rust-like Struct and Function in C++23](#crab_cpp-rust-like-struct-and-function-in-c23)
  - [Overview](#overview)
  - [How to Compile](#how-to-compile)
  - [Compiler Requirement](#compiler-requirement)
  - [Features \& Usage](#features--usage)
    - [panic \& unimplemented](#panic--unimplemented)
    - [Option \<T\>](#option-t)
    - [Result \<T, E\>](#result-t-e)
    - [Char \& str \& String (Optional feature)](#char--str--string-optional-feature)
  - [Why Crab\_cpp ?](#why-crab_cpp-)
  - [Todos](#todos)
  - [Thanks](#thanks)

## Crab_cpp: Rust-like Struct and Function in C++23

### Overview
Crab_cpp is a C++23 library that simulates Rust's Option<T>, Result<T, E>, panic!, unimplemented!, a match macro and optional Char, str, String to modern C++. Designed for developers who appreciate Rust's explicit error handling and pattern matching but work in C++ ecosystems. This library supports only C++23 and depends on std module.

### How to Compile
Currently use [xmake](https://github.com/xmake-io/xmake) to compile this library. Welcome to contribute if you need more build systems.

### Compiler Requirement
gcc >= 15
msvc >= 1943
clang >= 18 /clang-cl >= 19.1.1

### Features & Usage

#### panic & unimplemented
Prints a message and calls std::terminate().
- If the macro `CRAB_CPP_ENABLE_BACKTRACE` is defined, it will print std::stacktrace::current()

```c++
import crab_cpp;

int main()
{
    panic("Panic because answer isn't {}", 42);
    unimplemented("Unimplemented!")
}
```

#### Option &lt;T&gt;
Option<T> is inherited from `std::variant<None, T>`, add few more Rust-like methods, where `None` is alias for `std::monostate`.
It features two specialized variants, one holding lvalue references, another containing pointers. Both maintaining sizeof(size_t) memory footprint for optimal compatibility.

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
Result<T, E> is inherited from `std::variant<T, E>`, add few more Rust-like methods.

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

    // ok() try to moves out the value to an Option, do not use this Result anymore
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
To enable this sub-module, define the `CRAB_CPP_ENABLE_STRING` macro.

These three types represent simplified counterparts of Rust's `char`, `&str`, and `String`. `String` is ends with null-terminated to ensure better interoperability with C/C++ APIs. Using the `from` method to construct `str` or `String` instead of constructors, except you need a empty one.

We've implemented `std::formatter` support and `operator<<` for printing, though output will stop at the first null terminator. Note that String is an alias for `crab_cpp::raw::String<std::allocator<std::byte>>`, use the raw version for custom allocators. UTF-8 handling is powered by utf8proc, supporting up to Unicode 16.

While core functionality is implemented, many methods remain unimplemented. Contributions are welcome!

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
Compared to existing corresponding classes, Crab_cpp offers
- Rust-like APIs
- Simple pattern matching support
- UTF-8 based string

### Todos
- Support C++20
- Implement more methods

### Thanks
[utf8proc](https://github.com/JuliaStrings/utf8proc): Provide utf8 process.

[Google Test](https://github.com/google/googletest): For testing.
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

### Why Crab_cpp ?
Compared to std::optional and std::expected, Crab_cpp offers
- Rust-like APIs
- Simple pattern matching support

## Crab_cpp: Rust-like Struct and Function in C++23

### Overview
Crab_cpp is a C++23 library that ports Rust's Option<T>, Result<T, E>, panic!, unimplemented!, and a match macro to modern C++. Designed for developers who appreciate Rust's explicit error handling and pattern matching but work in C++ ecosystems. This library supports only C++23 and requires std module.

### Features & Usage

#### Option &lt;T&gt;
```c++
import crab_cpp;

#include <cassert>

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
}
```

#### Result &lt;T, E&gt;
```c++
import crab_cpp;

#include <cassert>

enum class enum ErrType
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

    // unwrap() returns int&, unlike Option, Result doesn't have take()
    int num = res.unwrap();
}
```
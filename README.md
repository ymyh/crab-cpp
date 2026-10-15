## Table of Contents
- [Table of Contents](#table-of-contents)
- [Crab\_cpp: Rust-like Struct and Function in C++23](#crab_cpp-rust-like-struct-and-function-in-c23)
  - [Overview](#overview)
  - [Compilation](#compilation)
    - [Options](#options)
    - [Requirements](#requirements)
  - [Features \& Usage](#features--usage)
    - [panic \& unimplemented](#panic--unimplemented)
    - [Value category](#value-category)
    - [Option \<T\>](#option-t)
    - [Result \<T, E\>](#result-t-e)
    - [match](#match)
    - [Mutex \<T\>](#mutex-t)
    - [Char \& str \& String (Optional feature)](#char--str--string-optional-feature)
  - [Why Crab\_cpp ?](#why-crab_cpp-)
  - [Thanks](#thanks)

## Crab_cpp: Rust-like Struct and Function in C++23

### Overview
Crab_cpp is a C++23 library that brings Rust-like `Option<T>`, `Result<T, E>`, `panic` / `unimplemented`, a `match` macro, `Mutex<T>`, and optional `Char` / `str` / `String` to modern C++. It is for code that wants explicit error handling and pattern matching while staying in C++. The library is C++23 only and is used through `import std` and `import crab_cpp`.

### Compilation
Currently, we use cmake or [xmake](https://github.com/xmake-io/xmake) to compile this library. Welcome to contribute if you need more build systems.

Use `FetchContent` to introduce `crab_cpp` to your cmake project.

Here is a boilerplate code that can introduce `crab_cpp` to your xmake project:

```lua
package("crab_cpp")

    set_kind("library", {moduleonly = true})
    set_urls("https://github.com/ymyh/crab-cpp/releases/download/v$(version)/crab_cpp-$(version).tar.gz")

    add_versions("0.1.0", "215cba62efccfba0a1db6b30781a57dfe157fa003fd1675dee023b014fe457bc")
    add_configs("enableString", {description = "Enable string module", default = false, type = "boolean"})
    add_configs("enableBacktrace", {description = "Enable backtrace", default = false, type = "boolean"})
    add_configs("enableTests", {description = "Run tests after installation", default = false, type = "boolean"})

    on_install(function (package)

        local configs = {}

        if package:config("enableString") then
            package:add("defines", "CRAB_CPP_ENABLE_STRING")
            table.insert(configs, "--enable-string=y")
        end

        if package:config("enableBacktrace") then
            package:add("defines", "CRAB_CPP_ENABLE_BACKTRACE")
            table.insert(configs, "--enable-backtrace=y")
        end

        if package:config("enableTests") then
            table.insert(configs, "--enable-test=y")
        end

        package:add("includedirs", "include")
        os.cp("include/crab_cpp/*.h", package:installdir("include", "crab_cpp"))

        import("package.tools.xmake").install(package, configs)
    end)

    on_test(function (package)

        if package:config("enableTests") then
            os.exec(path.join(package:installdir(), "bin", "crab_cpp_test"))
        end
    end)

package_end()

add_requires("crab_cpp", {configs = { enableString = true }})
-- if you have set enableString = true, you will need this and add_packages("utf8proc") in your targets
add_requires("utf8proc")
-- same reason for gtest if you set enableTests = true
add_requires("gtest", {configs = {main = true}})

-- your targets...
```

#### Options
--enable-string: defines macro `CRAB_CPP_ENABLE_STRING`
--enable-backtrace: defines macro `CRAB_CPP_ENABLE_BACKTRACE`

#### Requirements
gcc >= 15

msvc >= 1943

clang >= 19 / clang-cl >= 19.1.1

### Features & Usage

#### panic & unimplemented
`panic` prints `Panic encountered` to `std::cerr` and calls `std::terminate`. `unimplemented` prints `Unimplemented yet.` and then panics. A `std::string_view` or a `std::format_string` may be passed as the message. When `CRAB_CPP_ENABLE_BACKTRACE` is defined, `std::stacktrace::current()` is printed before termination.

```c++
import crab_cpp;

int main()
{
    panic("Panic because answer isn't {}", 42);
    unimplemented("Unimplemented!");
}
```

#### Value category
Methods that take `this` use the value category of the object to mean borrow or consume.

- An lvalue, including a const lvalue, borrows. A callback sees `T&` or `const T&`, and the object is left unchanged. A returned value is copy-constructed.
- A non-const rvalue consumes. A callback sees `T&&`. A reference alternative still passes an lvalue bound to the referent. `Option` then becomes `None`. `Result` stays Ok or Err: a moved-from value is left in place, and a reference alternative is not cleared.
- A const rvalue is copied and is not cleared.
- `inspect` always borrows and does not clear the object. An lvalue returns `*this` by reference. An rvalue returns by value, so `make().inspect(f).unwrap()` still consumes.

`unwrap` returns by value. An lvalue copies. A non-const rvalue moves the value out. `expect` is the same call with your own panic message. `unwrap_unchecked` skips the check; calling it on a missing value is undefined. A reference alternative returns that reference rather than a copy of the referent.

#### Option &lt;T&gt;
`Option<T>` holds either a `T` or `None`. `None` is an alias of `std::monostate`. It is not a subclass of `std::variant`. A value is stored in `std::variant<None, T>`. A pointer or an lvalue reference is stored as one pointer, so `sizeof(Option<T*>)` and `sizeof(Option<T&>)` equal `sizeof(std::size_t)`. A null pointer is `None`: `Option` cannot represent `Some(nullptr)`. A pointer passed to a callback is copied, so the callback does not refer to the pointer stored inside the option. A reference option binds an lvalue only. A temporary is rejected, so the stored address cannot refer to a temporary that has already died. `T` must be move-constructible when it is neither a pointer nor an lvalue reference. Rvalue references are not supported.

```c++
import crab_cpp;

#include <assert.h>
#include "crab_cpp/macros.h"

int main()
{
    Option<int> opt = 42;
    assert(opt.is_some());

    // Lvalue unwrap copies. opt is still Some.
    int num = opt.unwrap();

    opt.inspect([](int x) { });

    // map's callback returns U. The result is Option<U>.
    // and_then is the method whose callback returns an Option.
    Option<int> doubled = opt.map([](int a) { return a * 2; });
    assert(doubled.unwrap() == 84);

    // Non-const rvalue unwrap moves the value out and leaves None.
    num = std::move(opt).unwrap();
    assert(opt.is_none());

    Option<int&> ref = num;
    ref.unwrap() = 7;
    assert(num == 7);

    match (ref)
    {
        [](int& x) { },
        [](None) { }
    };
}
```

One method stands for each family. The others differ only in which side they touch, or in how the fallback is produced.

- `is_some`. `is_none` is the complement.
- `contains` compares the contained value. A pointer option compares addresses. A reference option compares the referents.
- `is_some_and`. `is_none_or` is the other predicate. Neither consumes the option.
- `unwrap`, and likewise `expect` and `unwrap_unchecked`.
- `unwrap_or`, and likewise `unwrap_or_else` and `unwrap_or_default`. These do not panic. `unwrap_or_default` needs a default-constructible `T` and is not provided for references. A reference fallback must be an lvalue.
- `map`. The callback returns `U` and the method returns `Option<U>`. `map_or`, `map_or_else`, and `map_or_default` return `U` directly, using a fallback when the option is `None`.
- `and_then`. The callback returns an `Option`.
- `and_`. `Some` returns the other option. `None` returns `None`.
- `or_`. `Some` keeps this option. `None` takes the other. `or_else` builds that other from a callback.
- `xor_`. The result is `Some` only when exactly one side is `Some`.
- `filter`. The predicate borrows. A non-const rvalue becomes `None` whether or not the predicate holds.
- `inspect`.
- `zip` pairs two `Some` values. `unzip` splits a `Some` of a two-element tuple. `flatten` collapses `Option<Option<T>>`.
- `visit` calls the visitor with `None` or with the value, and does not clear the option.
- `replace` swaps in a new value and returns the old one as an `Option`.
- `insert`, and likewise `get_or_insert`, `get_or_insert_with`, and `get_or_insert_default`. Value options only, and only on an lvalue. The returned `T&` refers to the value stored in the option. Pointer and reference options do not own a `T`, so these methods are not provided. `get_or_insert_default` also requires `T` to be default-constructible.
- `as_ref`, and `as_mut`. Value options only, lvalue only. They borrow the contained value as `Option<const T&>` or `Option<T&>`.
- `ok_or`, and `ok_or_else`. `Some` becomes Ok. `None` becomes Err.
- `==` and `<=>`. `None` is less than `Some`. `std::hash<Option<T>>` mixes a Some/None tag with the value hash, so `None` and `Some(0)` do not collide. References hash the referent. Pointers hash the address.

An operand that is copied or moved is taken before either side is reset. Writing `std::move(opt).and_(opt)` therefore still observes the value.

#### Result &lt;T, E&gt;
`Result<T, E>` holds either an Ok `T` or an Err `E`. It is not a subclass of `std::variant`. Values are stored in a variant. If either alternative is an lvalue reference, that side is stored as a pointer, so `Result<T&, E>`, `Result<T, E&>`, and `Result<T&, E&>` are valid. Rvalue references are not. A reference alternative binds an lvalue only. There is no empty state: after a consuming call the result is still Ok or Err.

The converting constructor selects the single alternative that can be built from the argument. When `T` and `E` are the same type that constructor is ambiguous; use `std::in_place_index<0>` for Ok and `std::in_place_index<1>` for Err. Default construction is `Ok(T{})` when `T` is default-constructible and not a reference.

```c++
import crab_cpp;

#include <assert.h>
#include "crab_cpp/macros.h"

enum class ErrType
{
    Blocked, Timeout
};

Result<int, ErrType> foo(bool ok)
{
    if (ok)
    {
        return 0;
    }

    return ErrType::Blocked;
}

int main()
{
    auto res = foo(true);
    assert(res.is_ok());

    // Lvalue unwrap copies. res is still Ok.
    int num = res.unwrap();

    // Lvalue ok() copies into an Option. res is still Ok.
    Option<int> opt = res.ok();
    assert(res.is_ok());

    match (foo(false))
    {
        [](int x) { },
        [](ErrType e) { }
    };
}
```

- `is_ok`. `is_err` is the complement.
- `contains`. `contains_err` checks the error. A reference alternative compares the referent.
- `is_ok_and`. `is_err_and` checks the error. Neither call consumes the result.
- `unwrap`, and likewise `expect` and `unwrap_unchecked`. The error side is `unwrap_err` and `expect_err`.
- `unwrap_or`, and likewise `unwrap_or_else` and `unwrap_or_default`. `unwrap_or_default` is not provided when `T` is a reference.
- `map`. The callback returns the new Ok value and the error type stays `E`. `map_err` maps the error instead. `map_or` returns a plain `U`, using the fallback on Err.
- `and_then`. The callback receives the Ok value and returns a `Result`.
- `and_`. Ok returns the other result, so the Ok type may change. Err keeps `E`.
- `or_`. Ok keeps `T` and the error type may change. Err returns the other result. `or_else` builds that result from the error.
- `inspect`. `inspect_err` borrows the error. Neither call changes which alternative is held.
- `flatten` collapses `Result<Result<U, E>, E>`.
- `ok`. `err` is the error side. An lvalue copies into an `Option`. A non-const rvalue moves. The `Result` stays on its alternative.
- `as_ref`, and `as_mut`. Lvalue only. They borrow whichever alternative is active.
- `visit` calls the visitor with the Ok value or the Err value, and does not empty the result.
- `==` and `<=>`. Ok is less than Err. `std::hash<Result<T, E>>` mixes an Ok/Err tag with the active value. References hash the referent.

#### match
The `match` macro lives in a header, because a macro cannot cross a module boundary.

```c++
#include "crab_cpp/macros.h"
```

`match (expr) { lambdas }` builds an overload set from the lambdas and visits the argument. One lambda handles each alternative, as in the `Option` and `Result` samples above. The arms must return the same type when the result is used.

#### Mutex &lt;T&gt;
`Mutex<T>` owns a `T` guarded by `std::mutex`, in the shape of Rust's `std::sync::Mutex`. It does not implement poisoning: a panic on another thread does not poison the lock. `lock` blocks and returns a `MutexGuard<T>`. `try_lock` returns `Option<MutexGuard<T>>`. The guard releases the lock when it is destroyed. `operator*` and `operator->` reach the protected value. The guard must not outlive the mutex. `into_inner` on an rvalue consumes the mutex and returns `T`. `get_mut` returns `T&` without locking; the caller must already have exclusive access.

```c++
import crab_cpp;

#include <assert.h>

Mutex<int> mutex{ 1 };
{
    auto guard = mutex.lock();
    *guard += 1;
}
assert(*mutex.lock() == 2);

if (auto maybe = mutex.try_lock(); maybe.is_some())
{
    // MutexGuard is move-only, so the Option has to be consumed.
    auto guard = std::move(maybe).unwrap();
    *guard = 3;
}
```

#### Char & str & String (Optional feature)
To enable this sub-module, define the `CRAB_CPP_ENABLE_STRING` macro.

These three types represent simplified counterparts of Rust's `char`, `&str`, and `String`. `String` is ends with null-terminated to ensure better interoperability with C/C++ APIs. Using the `from` static member function to construct `str` or `String` instead of constructors, unless you need a empty one.

We've implemented `std::formatter` support and `operator<<` for printing, although output will stop at the first null terminator. Note that String is an alias for `crab_cpp::raw::String<std::allocator<std::byte>>`, use the raw version for custom allocators. UTF-8 handling is powered by utf8proc, supporting up to Unicode 18.

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
    assert(ss == "Hello\n world!");

    using namespace literal;

    // The whole Rust split family is available, forwards and in reverse:
    // split/rsplit, splitn/rsplitn, split_terminator/rsplit_terminator,
    // split_inclusive/rsplit_inclusive and (Unicode) split_whitespace.
    auto pieces = "a,b,".split_inclusive(',') | std::ranges::to<std::vector<str>>();
    assert(pieces.size() == 2); // ["a,", "b,"]

    // match_indices/rmatch_indices yield `(byte index, match)` pairs.
    for (auto [index, matched] : "a中b中".match_indices(Char(0x4E2D)))
    {
        assert(matched == "中");
    }

    // Unicode-aware trimming, case conversion and escaping.
    assert("\u00A0hi\u3000".trim() == "hi");  // NBSP and U+3000 are whitespace
    assert("ß".to_uppercase() == "ẞ");         // utf8proc simple 1:1 mapping
    assert("a\tb".escape_debug() == "a\\tb");
}
```

### Why Crab_cpp ?
Compared to existing corresponding classes, Crab_cpp offers
- Rust-like APIs
- Simple pattern matching support
- UTF-8 based string

### Thanks
[utf8proc](https://github.com/JuliaStrings/utf8proc): Provide utf8 process.

[Google Test](https://github.com/google/googletest): For testing.
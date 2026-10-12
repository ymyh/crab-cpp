# Code Structure

This document explains what each file and module in the repository is for.
Paths are relative to the repository root.

## Top-level layout

| Path | Purpose |
| --- | --- |
| `src/` | C++20 module implementation units (`.cppm`). |
| `include/crab_cpp/` | The only classic headers shipped to users (currently just `macros.h`). |
| `tests/` | GoogleTest test sources (`.cpp`), built as `crab_cpp_test`. |
| `CMakeLists.txt` | CMake build (primary, enables experimental `import std`). |
| `xmake.lua` | xmake build (alternative). |
| `README.md` | User-facing usage documentation. |
| `CONTRIBUTING.md` | Contribution guidelines, including commit message format. |
| `AGENTS.md` | Entry point for AI agents; points to `.agents/`. |
| `.agents/` | Internal design/style documentation for agents and maintainers. |

## Modules in `src/`

The library is a set of C++20 modules joined by an umbrella module. Sub-modules
use the `crab_cpp:<name>` naming convention and are imported as `:name`.

### `src/mod.cppm` — umbrella module `crab_cpp`

- Declares `export module crab_cpp;`.
- Re-exports every sub-module with `export import :...;`.
- The `:string` sub-module is only imported when `CRAB_CPP_ENABLE_STRING` is
  defined, so the UTF-8 string feature stays optional.
- Users write `import crab_cpp;` and get everything.

### `src/panic.cppm` — module `crab_cpp:panic`

- Implements `crab_cpp::panic(...)` and `crab_cpp::unimplemented(...)`.
- Each overload prints to `std::cerr` and calls `std::terminate()`; they are
  `[[noreturn]]`.
- When `CRAB_CPP_ENABLE_BACKTRACE` is defined, prints
  `std::stacktrace::current()` before terminating.
- Depended on by `option`, `result`, and `string` for their panic paths.

### `src/option.cppm` — module `crab_cpp:option`

- Defines `None` (an alias of `std::monostate`) and `Option<T>` plus its
  specializations.
- Primary `Option<T>` derives from `std::variant<None, T>` and adds Rust-like
  methods: `is_some`/`is_none`, `unwrap`/`unwrap_unchecked`, `expect`,
  `take`, `take_if`, `take_or_default`, `take_or_else`, `replace`, `inspect`,
  `map`, `map_or_else`, `and_then`.
- Specializations for **pointer** (`Option<T*>`) and **lvalue reference**
  (`Option<T&>`) types store a single pointer so the footprint stays
  `sizeof(size_t)`. They expose a subset of the same methods.
- Provides a deduction guide `Option(T) -> Option<T>`.
- Imports `:panic`.

### `src/result.cppm` — module `crab_cpp:result`

- Defines `Result<T, E>`, derived from `std::variant<T, E>`, with Rust-like
  methods: `is_ok`/`is_err`, `ok`/`err`, `unwrap`/`unwrap_err`, `expect`/
  `expect_err`, `inspect`/`inspect_err`, `map`/`map_err`, `and_then`, `replace`.
- Imports `:panic` and `:option` (it converts to `Option`).

### `src/match.cppm` — module `crab_cpp:match`

- Implements the machinery behind the `match` macro in
  `crab_cpp::internal`: an `overload` helper (a variadic `operator()` set built
  from lambdas) and a `matcher` that applies a visitor to one or more variants
  via `std::visit`/`std::apply`.
- The user-facing macro itself lives in `include/crab_cpp/macros.h`, which
  expands `match(...)` into `matcher{...} ->* overload`.

### `src/string.cppm` — module `crab_cpp:string` (optional)

Enabled only when `CRAB_CPP_ENABLE_STRING` is defined; requires `utf8proc`.
This is the largest module and contains several logical sections:

- **Global module fragment** that includes the C header `utf8proc.h`.
- **Internal helpers** (not exported): `compressed_pair` (empty-base storage
  optimization for allocators) and `plain_str` (a raw byte span with a forward
  iterator).
- `FromUtf8Error` — error type carrying the byte offset of an invalid sequence.
- `is_valid_utf8(...)` and `Char` — Unicode scalar value wrapper with
  classification helpers (digit, alphabetic, whitespace, etc.).
- `str` — a borrowed UTF-8 slice with `from`/`from_raw_parts` constructors
  returning `Result<str, FromUtf8Error>`, iteration, search, splitting, and
  trimming helpers.
- `raw::String<Alloc>` — an owned, UTF-8, null-terminated, growable string with
  custom allocator support. `crab_cpp::String` is an alias for
  `raw::String<std::allocator<std::byte>>`.
- `strings` — range adaptor helpers such as `join_with`.
- `literal` — user-defined literals `_s` (→ `str`) and `_S` (→ `String`).
- `StringHasher` — transparent hasher for use as a map/set key.
- `std::formatter`, `std::hash`, and `operator<<` specializations for `Char`,
  `str`, and `raw::String`.

## Headers in `include/crab_cpp/`

- `macros.h` — defines the `match(...)` macro. This is a real header (not a
  module) because macros cannot cross module boundaries. Users include it as
  `#include "crab_cpp/macros.h"`.

## Tests in `tests/`

| File | Covers |
| --- | --- |
| `tests/option.cpp` | `Option<T>` and its pointer/reference specializations. |
| `tests/result.cpp` | `Result<T, E>`. |
| `tests/str.cpp` | The `str` view type. |
| `tests/string.cpp` | The owned `String` type. |

Tests include `<gtest/gtest.h>`, `import crab_cpp;`, and use
`using namespace crab_cpp;`. Panic paths are asserted with `EXPECT_DEATH`.

## Build targets

- CMake target `crab_cpp` (interface/module library) and, when
  `CRAB_CPP_ENABLE_TEST=ON`, `crab_cpp_test`.
- Options: `CRAB_CPP_ENABLE_STRING`, `CRAB_CPP_ENABLE_BACKTRACE`,
  `CRAB_CPP_ENABLE_TEST`.
- xmake mirrors the same options and defines the same
  `CRAB_CPP_ENABLE_*` macros.

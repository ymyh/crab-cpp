# 0.3.0

_Released 2026-10-12._

## Added

### `Mutex<T>` / `MutexGuard<T>` (new sub-module `crab_cpp:mutex`)

A Rust-like owning mutex modelled after `std::sync::Mutex`, re-exported from the
top-level `crab_cpp` module.

- `lock() -> MutexGuard<T>` and `try_lock() -> Option<MutexGuard<T>>`.
- `MutexGuard<T>` is a move-only RAII handle that dereferences (`*`, `->`, `get()`)
  to the protected value and releases the lock on destruction, including during
  stack unwinding.
- `Mutex<T>::get_mut()` for lock-free access and `into_inner()` to consume the mutex.
- No poisoning; `Mutex` itself is neither copyable nor movable.

### `Option<T>`

- `and_(optb)` and `or_(optb)` combinators, including the specializations for
  pointer (`Option<T*>`) and reference (`Option<T&>`) options.
- `unzip()` for options holding a two-element tuple-like value, returning a tuple
  of options `(Option<A>, Option<B>)`.
- `map_or_else` now returns the mapped/default value directly.

### `str` (String sub-module)

Large expansion of the Rust-aligned string API:

- **Splitting:** `split`, `rsplit`, `splitn`, `rsplitn`, `split_terminator`,
  `rsplit_terminator`, `split_inclusive`, `rsplit_inclusive`, `split_whitespace`,
  `split_ascii_whitespace`, `split_once`, `rsplit_once`, `split_at`,
  `split_at_checked`.
- **Matching:** `matches`, `rmatches`, `match_indices`, `rmatch_indices`.
- **Searching / prefix & suffix:** `find`, `rfind`, `contains`, `starts_with`,
  `ends_with`, `strip_prefix`, `strip_suffix`, with `str` / `const char*` /
  `String` / `Char` / `char` overloads.
- **Trimming:** Unicode-aware `trim`, `trim_start`, `trim_end`, `trim_matches`,
  `trim_start_matches`, `trim_end_matches`, plus `trim_ascii`,
  `trim_ascii_start`, `trim_ascii_end`.
- **Case & escaping:** `to_lowercase`, `to_uppercase`, `escape_default`,
  `escape_debug`, `escape_unicode`.
- **Slicing & indexing:** `is_char_boundary`, `get`, `char_indices`,
  `previous_boundary`, `decode_at`.
- **Conversion:** `to_std_string`, `to_string`.

### `Char`

Unicode-aware helpers: `is_whitespace`, `len_utf8`, `encode_utf8`,
`to_ascii_lowercase`, `to_ascii_uppercase`, and `from_u32_unchecked`.

### Lazy iterators

New iterators backing the above: `Chars`, `CharIndices`, `Graphemes`, `Lines`,
`Split`, `SplitWhitespace`, `SplitASCIIWhiteSpace`, `Matches`, `MatchIndices`
(double-ended where Rust's counterparts are).

## Changed

- All `Option<T>` query/accessor methods are now `[[nodiscard]]`.
- `Option<T>::map_or_else(d, f)` argument order and return type aligned with Rust
  (returns `U`; previously returned `Option<U>`). **Breaking change.**
- `str` iteration internals reworked; the old per-type iterators were replaced by
  the new iterator family.
- `String` sub-module now tracks utf8proc 2.12 (Unicode 18).

## Build

- CMake: module sources are now declared as a `CXX_MODULES` file set; `import std`
  is enabled via `CMAKE_CXX_MODULE_STD`; the String sub-module is optional;
  MinGW/libstdc++ link workarounds added (`stdc++exp`,
  `--allow-multiple-definition`).
- Dependencies bumped: utf8proc 2.12 and GoogleTest 1.18, both pinned with
  checksums.
- xmake: dependency toolchain inheritance and the same MinGW workarounds.

## Tests

Added extensive unit tests across `Option`, `Result`, `Mutex`, and the
`str` / `String` API surface.

# 0.4.0

_Released 2026-10-15._

## Added

### `Option<T>`

- `insert`, `get_or_insert`, `get_or_insert_with`, and `get_or_insert_default`
  on value options. Lvalue only; the returned `T&` refers to the stored value.
- `unwrap_or`, `unwrap_or_else`, `unwrap_or_default`.
- `map_or` and `map_or_default`.
- `or_else`, `xor_`, `filter`, `flatten`, and `zip`.
- `contains`, `is_some_and`, and `is_none_or`.
- `as_ref()` → `Option<const T&>` and `as_mut()` → `Option<T&>`. Lvalue only.
- `ok_or` and `ok_or_else`.
- `visit`.
- `operator==`, `operator<=>`, and `std::hash<Option<T>>`. `None` is less than
  `Some`. The hash mixes a Some/None tag with the value, so `None` and `Some(0)`
  do not collide.

### `Result<T, E>`

- `and_`, `or_`, and `or_else`.
- `unwrap_or`, `unwrap_or_else`, `unwrap_or_default`, and `unwrap_unchecked`.
- `map_or`.
- `contains`, `contains_err`, `is_ok_and`, and `is_err_and`.
- `as_ref()` → `Result<const T&, const E&>` and `as_mut()` → `Result<T&, E&>`.
  Lvalue only.
- `flatten` and `visit`.
- `operator==`, `operator<=>`, and `std::hash<Result<T, E>>`. Ok is less than Err.
- A converting constructor that selects the single alternative buildable from the
  argument. When `T` and `E` are the same type, use `std::in_place_index`.
- Reference alternatives stored as pointers: `Result<T&, E>`, `Result<T, E&>`,
  and `Result<T&, E&>`. A reference alternative binds an lvalue only.

### `match`

`match` visits `Option`, `Result`, and `std::variant`, including more than one
scrutinee. Each arm is one alternative. The arms must return the same type when
the result is used.

## Changed

Value category is the ownership rule for methods that take `this`. An lvalue
borrows and leaves the object unchanged. A non-const rvalue consumes: `Option`
becomes `None`; `Result` stays Ok or Err with a moved-from value. A const rvalue
is copied and is not cleared. `inspect` always borrows.

- `unwrap`, `expect`, and `unwrap_unchecked` return by value. An lvalue copies.
  A non-const rvalue moves the value out. A reference alternative returns that
  reference. **Breaking change:** these used to return a reference into the
  object. Borrow with `as_ref()` / `as_mut()` and then `unwrap()`.
- Removed `Option`'s `take`, `take_if`, `take_or_default`, `take_or_else`, and
  `expect_take`. **Breaking change.** `std::move(opt).unwrap()` replaces `take()`;
  `std::move(opt).expect(msg)` replaces `expect_take(msg)`.
- Removed `Result::replace`. **Breaking change.**
- `Result::ok` and `Result::err` follow the value category. An lvalue copies.
  They no longer always move the alternative out.
- `and_` and `or_` take the operand by forwarding reference and copy or move
  before either side is reset.
- `inspect` on an lvalue returns `*this` by reference. On an rvalue it returns
  by value, so `make().inspect(f).unwrap()` still consumes.

## Build

- MinGW test binaries also link `pthread`.

## Tests

- `tests/match.cpp` covers `match` on `Option`, `Result`, and `std::variant`.
- Option and Result matrix tests (`tests/option_matrix.inl`,
  `tests/result_matrix.inl`, `tests/probe_types.h`) exercise copy, move, pointer,
  and reference types across lvalue, const lvalue, rvalue, and const rvalue.

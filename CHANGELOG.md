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

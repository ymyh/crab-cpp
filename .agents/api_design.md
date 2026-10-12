# Public API Design

This document records the design decisions behind the public API of
`crab_cpp`. It is written before implementation so that the surface, naming and
edge-case semantics are agreed on first.

Currently it covers:

- [`str`](src/string.cppm) — extending the borrowed UTF-8 slice so it can stand
  in for Rust's `&str` in everyday code.

Related documents: @.agents/codeStyle.md, @.agents/codeStructure.md,
@.agents/phases.md.

---

## 1. Goal

`str` already implements the hard, UTF-8-aware parts (validation, graphemes,
`find`/`replace`, formatting). What is missing is mostly the *bread-and-butter*
surface: boundary-safe indexing, the full split family, `char`-pattern
overloads, Unicode trimming, byte/char-index iteration and escaping.

The goal of this phase is:

- Reach day-to-day parity with Rust's `&str` for the ~90% of methods people
  actually call.
- Keep the Rust names and semantics, including the panicking vs.
  `Option`-returning split (`slice`/`split_at` panic, `get`/`split_at_checked`
  return `Option`).
- Fix the inconsistencies that already exist (missing `const char*` overloads,
  the broken `trim_ascii_end`, `trim_ascii*` only handling `U+0020`).

Non-goal: a full `Pattern` trait, `str` indexing, or non-`constexpr`
normalization.

---

## 2. Current surface

`str` (see `src/string.cppm`) currently exposes:

| Group | Methods |
| --- | --- |
| Constructors | `str()`, `str(const plain_str&)`, `from_raw_parts`, `from_bytes_unchecked`, `from(const char*)` |
| Views | `as_bytes`, `as_raw`, `data`, `begin`/`end` |
| Queries | `empty`, `size`, `is_ascii`, `eq_ignore_ascii_case` |
| Search | `contains(str)`, `find(str)`, `find(const char*)`, `rfind(str)`, `rfind(const char*)`, `starts_with(str)`, `ends_with(str)` |
| Indexing | `slice(from, to)`, `slice(from)` |
| Strip | `strip_prefix`/`strip_suffix` (`str` + `const char*`) |
| Trim | `trim_ascii`, `trim_ascii_start`, `trim_ascii_end`, `trim_start_matches(str)`, `trim_end_matches(str)` |
| Transform | `replace`, `replace_n`, `repeat`, `to_ascii_lowercase`, `to_ascii_uppercase`, `to_std_string` |
| Parse | `parse<T>()` |
| Iterators | `chars`, `lines`, `matches`, `split`, `split_ascii_whitespace`, `graphemes` |
| Operators | `==`, `<=>`, `format`/`hash`/`<<` support |

### Known defects to fix along the way

1. **`trim_ascii_end` is UB for an all-whitespace string.** `size_t i` counts
   down with `for (; i >= 0; i -= 1)`; when every byte is a space, `i` wraps to
   `SIZE_MAX` and `m_data[SIZE_MAX]` is read.
2. **`trim_ascii*` only strips `U+0020`.** Per Rust, ASCII whitespace is
   `U+0009 U+000A U+000C U+000D U+0020`.
3. **Overload sets are inconsistent.** `contains`/`starts_with`/`ends_with` and
   `trim_*_matches` have no `const char*` overload, while `find`/`rfind`/
   `split`/`strip_*` have no `raw::String` overload (except `split`/`matches`).
   Today `s.contains("x")` does not compile, because there is no implicit
   `const char* -> str` conversion.
4. **`slice(from, to)` underflows when `to < from`.** The guard checks
   `from`/`to` against the length but never `to >= from`, so `slice(5, 2)`
   computes `to - from` as a huge `size_t` and then validates a wildly
   out-of-bounds range. The doc comment already promises a panic; the guard must
   honour it.
5. **`chars()` / `graphemes()` drop their last element in a range-for.** Their
   iterators model "end" as `pos == size`, but the last element is read when
   `pos` already equals `size`, so the iterator compares equal to `end` before
   it is yielded. Fixed by keying the end sentinel off a null `s` pointer, as
   `lines`/`matches`/`split` already do. (`it->…` on the old "end" masked this,
   which is why the original tests passed.)
6. **`split(pattern)` dropped a trailing empty piece.** `"a,b,".split(',')`
   yielded `["a", "b"]` in a range-for instead of Rust's `["a", "b", ""]`.
7. **`lines()` corrupted a single line that ends with a line ending.** In the
   first-line scan `pos` was advanced past the `\n`/`\r\n` before testing
   `pos >= size`, so the "no ending found" fallback fired and `"a\n"` was
   reported as the single line `"a\n"` (likewise `"a\r\n"`). An empty string
   also reported one empty line instead of none.

---

## 3. Gap analysis against Rust `&str`

Priority: **P1** = core, expected by almost every caller; **P2** = common but
can wait; **P3** = rare / unstable.

| Rust `&str` | Status | Proposed `crab_cpp` | Priority |
| --- | --- | --- | --- |
| `as_bytes` | done | `as_bytes` | — |
| `as_ptr` / `as_ptr_range` | partial | `data()` (`as_ptr_range` P3) | — |
| `len` / `is_empty` | done (`size`/`empty`) | optional aliases `len`/`is_empty` | P3 |
| `is_char_boundary` | **missing** | `is_char_boundary` | **P1** |
| `get(range)` | **missing** | `get(from, to)` / `get(from)` | **P1** |
| `split_at` / `split_at_checked` | **missing** | `split_at` / `split_at_checked` | **P1** |
| `floor_char_boundary` / `ceil_char_boundary` | **missing** | same | P3 |
| `chars` | done | `chars` | — |
| `char_indices` | **missing** | `char_indices` | **P1** |
| `bytes` | **missing** | `bytes` (thin view over `as_bytes`) | P2 |
| `encode_utf16` | **missing** | `encode_utf16` | P3 |
| `find` / `rfind` (str) | done | — | — |
| `find` / `rfind` (char) | **missing** | `find(Char)`, `find(char)`, … | **P1** |
| `contains` / `starts_with` / `ends_with` (char) | **missing** | overloads | **P1** |
| `contains`/`starts_with`/`ends_with` (str literal) | **missing** | `const char*` overloads | **P1** |
| `split_once` / `rsplit_once` | **missing** | same | **P1** |
| `split` (char) | **missing** | `split(Char)` / `split(char)` | **P1** |
| `splitn` / `rsplitn` | **missing** | same | P2 |
| `rsplit` | **missing** | `rsplit` | P2 |
| `split_terminator` / `rsplit_terminator` | **missing** | same | P2 |
| `split_inclusive` / `rsplit_inclusive` | **missing** | same | P2 |
| `split_whitespace` | **missing** | `split_whitespace` | P2 |
| `split_ascii_whitespace` | done | — | — |
| `lines` | done | `lines` | — |
| `matches` / `rmatches` | done (`matches`/`rmatches` yield the matched `str` slices) | `match_indices`/`rmatch_indices` add `(index, str)` | — |
| `strip_prefix` / `strip_suffix` (char) | **missing** | overloads | P2 |
| `trim` / `trim_start` / `trim_end` (Unicode) | **missing** | same | **P1** |
| `trim_matches` (+ char/predicate) | **missing** | same | P2 |
| `to_ascii_lowercase` / `to_ascii_uppercase` | done | — | — |
| `to_lowercase` / `to_uppercase` (Unicode) | **missing** | same | P2 |
| `escape_default` / `escape_debug` / `escape_unicode` | **missing** | same | P2 |
| `replace` / `replacen` | done (`replace`/`replace_n`) | keep `replace_n` | — |
| `repeat` | done | — | — |
| `parse` | done | — | — |
| `is_ascii` / `eq_ignore_ascii_case` | done | — | — |
| `to_owned` / `to_string` | done | `to_string` (C++ idiom; Rust's `to_owned` name rejected) | — |
| `from_utf8_lossy` | **missing** | `raw::String<Alloc>::from_utf8_lossy` | P3 |

---

## 4. Design decisions

### 4.1 Pattern handling

Rust uses the `Pattern` trait so `split`, `find`, `trim_matches`, … accept
`char`, `&str`, `&[char]`, and closures through one signature. A faithful C++
port is possible but expensive (a concept + type-erased searcher), and it would
rewrite every existing call site.

**Decision:** keep explicit overloads, and add the missing pattern kinds:
`str`, `const char*`, `raw::String<Alloc>`, `Char`, and plain `char`.

To keep this maintainable, the *implementation* normalizes every byte-pattern
to a `str` and every character-pattern to its UTF-8 encoding, and all overloads
forward to one private implementation per operation. Public signatures stay
explicit; the duplication lives in one-line forwards.

Notes:

- **`Char` vs `char`.** `Char` is the correct Unicode scalar type, but `'a'` in
  C++ has type `char`. We therefore provide `char` convenience overloads in
  addition to `Char`, interpreting the `char` as its numeric value `0..=255`
  and encoding it as UTF-8 (exact for ASCII, Latin-1 for `>= 0x80`). This is
  documented; callers needing a real non-ASCII scalar use `Char`.
- **`const char*` overloads** delegate via `str::from(...).expect(...)`, exactly
  like the existing `find(const char*)`, so invalid UTF-8 panics.
- A UTF-8 encoded `Char` search is safe: UTF-8 is self-synchronizing, so a
  substring match can never land inside another sequence.

### 4.2 Indexing: panicking vs. `Option`

Rust deliberately splits this: `s[..]`, `slice`, `split_at` panic; `get`,
`split_at_checked` return `Option`. We mirror that:

```cpp
[[nodiscard]] constexpr auto get(size_t from, size_t to) const noexcept -> Option<str>;
[[nodiscard]] constexpr auto get(size_t from) const noexcept -> Option<str>;
[[nodiscard]] constexpr auto split_at(size_t mid) const -> std::pair<str, str>;      // panics
[[nodiscard]] constexpr auto split_at_checked(size_t mid) const noexcept -> Option<std::pair<str, str>>;
```

Both validate that the index is a char boundary (`is_char_boundary`). `get`
returns `None` on non-boundary/out-of-range instead of panicking — this makes
`get` the preferred primitive inside library code.

### 4.3 Return types

| Operation | Return |
| --- | --- |
| fallible index | `Option<str>` |
| fallible split into two borrowed halves | `Option<std::pair<str, str>>` |
| infallible split into two borrowed halves | `std::pair<str, str>` |
| character iteration | `std::pair<size_t, Char>` (index, scalar) |
| match iteration | `std::pair<size_t, plain_str>` (index, matched slice) |
| allocating transforms (`to_string`, `escape_*`, Unicode case, `repeat`, …) | `raw::String<Alloc>`, `Alloc = std::allocator<std::byte>` defaulted |

`plain_str` stays the iterator value type (as the existing `Split`/`Lines`
iterators already do); it converts implicitly to `str`, so
`for (auto part : s.split(","))` and `std::ranges::to<std::vector<str>>()` keep
working. Structured bindings desugar `std::pair` cleanly:

```cpp
if (auto idx = s.find('@'); idx.is_some()) { /* ... */ }
if (auto parts = s.split_once('='); parts.is_some())
{
    auto [key, value] = parts.unwrap();
}
```

Option: introduce a `Match { size_t start; size_t end; str as_str(); }` type so
`match_indices` mirrors Rust's `Match`. Deferred — the pair is enough for P1/P2.

### 4.4 Naming

- Keep Rust names wherever they do not clash with C++ (`is_char_boundary`,
  `char_indices`, `split_once`, `split_terminator`, `trim_start_matches`).
- Existing C++-flavoured deviations stay for compatibility:
  `size()` (not `len()`), `empty()` (not `is_empty()`),
  `slice()` (Rust has no direct equivalent) and `replace_n()` (Rust:
  `replacen`). Optional `len()`/`is_empty()` aliases are P3.
- Exit/entry naming: `rsplit`/`rsplit_once` use the `r` prefix exactly as Rust.
- New allocating methods follow the existing `template<typename Alloc>` shape of
  `to_ascii_lowercase` so custom allocators keep working.

### 4.5 Unicode whitespace

`trim`/`split_whitespace` need `char::is_whitespace` (Unicode `White_Space`).
utf8proc does not expose the property directly, so we add a `Char::is_whitespace`
helper defined as: category `Zs`/`Zl`/`Zp`, or the code points
`U+0009..=U+000D` and `U+0085`. This matches Unicode `White_Space` exactly
(it intentionally excludes `U+001C..=U+001F`).

`trim_ascii*` is instead fixed to use `Char::is_ascii_whitespace` (the existing
`Char` method already implements the correct set).

### 4.6 Empty-pattern semantics

Rust gives the empty pattern very regular behaviour, and the current code
diverges from it:

| Call | Rust | Current `crab_cpp` |
| --- | --- | --- |
| `"abc".find("")` | `Some(0)` | `None` |
| `"abc".rfind("")` | `Some(3)` (i.e. `len`) | `None` |
| `"abc".matches("")` | matches at every char boundary (incl. 0 and len) | empty range |
| `"abc".split("")` | `["", "a", "b", "c", ""]` | `["abc"]` |

**Decision (implemented in Phase 1):**

- `split` now keeps a trailing empty piece (defect 6). This matches Rust and
  did not require changing the existing tests.
- `split_once("")` / `rsplit_once("")` (new) follow Rust and return
  `Some(("", self))` / `Some((self, ""))`.

**Phase 2 update:** the empty pattern of `find`/`rfind`/`matches`/`split` is now
aligned with Rust as well. `find("")` returns `Some(0)`, `rfind("")` returns
`Some(len)`, `matches("")` matches at every code point boundary (including `0`
and `len`), and `split("")` yields a piece per boundary. This changed
already-shipped behaviour, so the corresponding tests were updated.

### 4.7 Iterator ownership (important)

`plain_str` only *borrows*. For character patterns the encoded bytes are up to
4 bytes and would otherwise point at a dead local. Therefore every new
iterator that stores a character pattern must own its encoding:

```cpp
struct Split
{
    std::byte pattern_buf[4];   // owned storage for a Char pattern
    plain_str pattern;          // points into pattern_buf when needed
    ...
};
```

The same applies to `SplitN`/`RSplit`/`SplitInclusive`/`Matches` and the
`trim_*_matches` helpers. Byte patterns keep pointing at the caller's buffer
(borrow semantics, matching the argument's lifetime), exactly as today.

### 4.8 `constexpr` and `noexcept`

- `noexcept` on everything that cannot throw — true for all non-allocating
  methods.
- `constexpr` where the code path allows it. Methods that call `utf8proc`
  (`chars`, `char_indices`, `trim`/`split_whitespace`, Unicode case) keep the
  `constexpr` keyword like the existing `Chars`/`Graphemes` iterators do; they
  simply are not usable in a constant expression.
- Allocating methods cannot be `constexpr` (allocation).

---

## 5. Proposed API

Signatures below are the intended public form. `…` denotes the existing
`str` / `const char*` / `raw::String<Alloc>` overload family, repeated for
`Char` and `char` where noted.

### A. Boundary and indexing — P1

```cpp
// true for index in [0, size], including size (end) and any UTF-8 boundary
[[nodiscard]] constexpr auto is_char_boundary(size_t index) const noexcept -> bool;

// get: None unless from <= to, to <= size and both are char boundaries
[[nodiscard]] constexpr auto get(size_t from, size_t to) const noexcept -> Option<str>;
[[nodiscard]] constexpr auto get(size_t from) const noexcept -> Option<str>;

// panics when mid is out of bounds / not a boundary
[[nodiscard]] constexpr auto split_at(size_t mid) const -> std::pair<str, str>;
[[nodiscard]] constexpr auto split_at_checked(size_t mid) const noexcept -> Option<std::pair<str, str>>;
```

### B. Iteration — P1/P2

```cpp
struct Bytes;        // value_type std::byte
struct CharIndices;  // value_type std::pair<size_t, Char>

[[nodiscard]] constexpr auto bytes() const noexcept -> Bytes;              // P2
[[nodiscard]] constexpr auto char_indices() const noexcept -> CharIndices; // P1
```

### C. Split family — P1/P2

```cpp
// P1: unify + add char
[[nodiscard]] constexpr auto split(…) const noexcept -> Split;
[[nodiscard]] constexpr auto split_once(…) const noexcept -> Option<std::pair<str, str>>;
[[nodiscard]] constexpr auto rsplit_once(…) const noexcept -> Option<std::pair<str, str>>;

// P2
[[nodiscard]] constexpr auto splitn(size_t n, …) const noexcept -> SplitN;          // Rust arg order
[[nodiscard]] constexpr auto rsplitn(size_t n, …) const noexcept -> RSplitN;
[[nodiscard]] constexpr auto rsplit(…) const noexcept -> RSplit;
[[nodiscard]] constexpr auto split_terminator(…) const noexcept -> SplitTerminator;
[[nodiscard]] constexpr auto rsplit_terminator(…) const noexcept -> RSplitTerminator;
[[nodiscard]] constexpr auto split_inclusive(…) const noexcept -> SplitInclusive;
[[nodiscard]] constexpr auto rsplit_inclusive(…) const noexcept -> RSplitInclusive;
[[nodiscard]] constexpr auto split_whitespace() const noexcept -> SplitWhitespace;  // Unicode
```

Semantics follow Rust exactly, including the easy-to-miss ones:

- `splitn(n, p)` yields at most `n` parts; the last part contains the remainder
  **with** its separators.
- `split_terminator` behaves like `split` but drops a trailing empty part when
  the string ends with the separator.
- `split_inclusive` keeps the separator on the part it terminates;
  `rsplit*` variants yield parts in reverse order.
- `split_whitespace` splits on runs of Unicode whitespace and yields no empty
  parts (unlike `split(' ')`).

### D. Search and match — P1/P2

```cpp
// P1: char/const char* overloads
[[nodiscard]] constexpr auto find(Char ch) const noexcept -> Option<size_t>;
[[nodiscard]] constexpr auto rfind(Char ch) const noexcept -> Option<size_t>;
[[nodiscard]] constexpr auto contains(const char* pattern) const noexcept -> bool;
[[nodiscard]] constexpr auto contains(Char ch) const noexcept -> bool;
[[nodiscard]] constexpr auto starts_with(const char* pattern) const noexcept -> bool;
[[nodiscard]] constexpr auto starts_with(Char ch) const noexcept -> bool;
[[nodiscard]] constexpr auto ends_with(const char* pattern) const noexcept -> bool;
[[nodiscard]] constexpr auto ends_with(Char ch) const noexcept -> bool;

// P2
[[nodiscard]] constexpr auto match_indices(…) const noexcept -> MatchIndices;    // (index, matched slice)
[[nodiscard]] constexpr auto rmatch_indices(…) const noexcept -> RMatchIndices;
[[nodiscard]] constexpr auto strip_prefix(Char ch) const noexcept -> Option<str>;
[[nodiscard]] constexpr auto strip_suffix(Char ch) const noexcept -> Option<str>;
```

### E. Trim family — P1/P2

```cpp
// P1: Unicode-aware, same definition of whitespace as Rust
[[nodiscard]] auto trim() const noexcept -> str;
[[nodiscard]] auto trim_start() const noexcept -> str;
[[nodiscard]] auto trim_end() const noexcept -> str;

// P1: bug fix — strip the full ASCII whitespace set and stop the wrap-around
[[nodiscard]] auto trim_ascii() const noexcept -> str;
[[nodiscard]] auto trim_ascii_start() const noexcept -> str;
[[nodiscard]] auto trim_ascii_end() const noexcept -> str;

// P2: char + predicate forms (in addition to the existing str form)
[[nodiscard]] constexpr auto trim_start_matches(Char ch) const noexcept -> str;
[[nodiscard]] constexpr auto trim_end_matches(Char ch) const noexcept -> str;
[[nodiscard]] constexpr auto trim_matches(…) const noexcept -> str;

template<typename Pred>
    requires std::predicate<Pred, Char>
[[nodiscard]] constexpr auto trim_matches(Pred pred) const noexcept -> str;
// …same for trim_start_matches / trim_end_matches
```

`trim_matches(pred)` is the only place a closure pattern is accepted; it avoids
a general `Pattern` trait while covering the common "strip custom chars" case.

### F. Case and escaping — P2

```cpp
template<typename Alloc = std::allocator<std::byte>>
[[nodiscard]] auto to_lowercase() const -> raw::String<Alloc>;   // Unicode simple mapping

template<typename Alloc = std::allocator<std::byte>>
[[nodiscard]] auto to_uppercase() const -> raw::String<Alloc>;

template<typename Alloc = std::allocator<std::byte>>
[[nodiscard]] constexpr auto escape_default() const -> raw::String<Alloc>;

template<typename Alloc = std::allocator<std::byte>>
[[nodiscard]] constexpr auto escape_debug() const -> raw::String<Alloc>;

template<typename Alloc = std::allocator<std::byte>>
[[nodiscard]] constexpr auto escape_unicode() const -> raw::String<Alloc>;
```

**Case-mapping fidelity (documented limitation):** utf8proc only exposes
*simple*, 1:1 case mappings (`utf8proc_tolower`/`utf8proc_toupper`). Rust's
`to_uppercase` performs *full* mappings, so e.g. `"ß"` becomes `"SS"` in Rust
but stays `"ß"` here. `to_lowercase` is essentially identical to Rust (the only
common full-mapping case, `ẞ -> ß`, is also the simple mapping). We document the
`to_uppercase` difference rather than hand-rolling a full mapping table.
`to_ascii_*` (existing) is unaffected.

`escape_default` follows Rust's `char::escape_default`: `\t`, `\r`, `\n`,
`\\`, `\'`, `\"`; printable ASCII is kept; ASCII controls become `\xNN`;
everything else becomes `\u{…}`. `escape_debug` additionally keeps printable
non-ASCII (grapheme-extend handling is P3). `escape_unicode` escapes every
non-ASCII scalar as `\u{…}`.

### G. Conversion — P1

```cpp
template<typename Alloc = std::allocator<std::byte>>
[[nodiscard]] constexpr auto to_string() const -> raw::String<Alloc>;

// P3: lossy decode, on the owned type rather than on str
//   raw::String<Alloc>::from_utf8_lossy(std::span<const std::byte> bytes) -> raw::String<Alloc>
//   (replaces every invalid sequence with U+FFFD)
```

### H. Supporting `Char` additions

These are internal dependencies of the methods above but are useful on their
own; `Char` already has all `is_ascii_*` predicates.

```cpp
[[nodiscard]] constexpr auto is_whitespace() const noexcept -> bool;         // Unicode White_Space (§4.5)
[[nodiscard]] constexpr auto len_utf8() const noexcept -> size_t;
[[nodiscard]] constexpr auto encode_utf8(std::span<std::byte, 4> out) const noexcept -> size_t;
[[nodiscard]] constexpr auto to_ascii_lowercase() const noexcept -> Char;
[[nodiscard]] constexpr auto to_ascii_uppercase() const noexcept -> Char;

// replaces the existing std::bit_cast<Char>(codepoint) hack used in iterators
[[nodiscard]] static constexpr auto from_u32_unchecked(std::uint32_t) noexcept -> Char;
```

---

## 6. Non-goals

- A general `Pattern` trait / type-erased searcher.
- `str` indexing (`s[0]`) and `Index` machinery.
- Reversible (`DoubleEndedIterator`) versions of the existing iterators; the
  `rsplit*` family provides reverse traversal explicitly instead. Making
  `chars`/`lines`/`split` reversible is a possible follow-up.
- `Range<usize>`-style arguments; `get(from, to)` is the C++ spelling.
- Unicode full case mapping tables and normalization.

---

## 7. Testing plan

Add to `tests/str.cpp`, one `TEST(StringTest, …)` per group, following the
existing style (`literal::operator""_s`, `EXPECT_EQ`, `EXPECT_DEATH`):

- boundaries: `get`/`split_at` on ASCII, multi-byte (`"你好"`), combining
  marks; `get` returns `None` mid-codepoint; `split_at` panics mid-codepoint.
- `char_indices`: byte offsets around multi-byte scalars; empty string.
- split family: trailing/leading/consecutive separators, `n == 0`, `n` larger
  than the number of parts, empty pattern, reverse variants.
- `split_whitespace` vs `split(' ')` on mixed tabs/newlines.
- `match_indices`/`rmatch_indices` on overlapping patterns.
- trim: Unicode spaces (NBSP, `U+3000`), the all-whitespace `trim_ascii_end`
  regression, non-boundary safety.
- `to_string`, `escape_*`, `to_lowercase`/`to_uppercase` incl. a documented
  `ß`/`ẞ` case.
- char-pattern overloads for every operation touched, including the `char` vs
  `const char*` overload resolution.

---

## 8. Rollout

| Phase | Contents | Rationale |
| --- | --- | --- |
| 1 | §5A, §5B (`char_indices`), §5D char/`const char*` overloads, `split`/`split_once`/`rsplit_once`, `to_string`, `trim_ascii*` bug fix, §5H helpers | Unblocks the most common absent operations; fixes existing defects. |
| 2 | §5C remaining split family, `match_indices`, Unicode `trim*` + `trim_matches`, `split_whitespace`, `escape_*`, `to_lowercase`/`to_uppercase` | Completes the "call it like Rust" surface. |
| 3 | `bytes`, `encode_utf16`, `floor`/`ceil_char_boundary`, `from_utf8_lossy`, `len`/`is_empty` aliases, `Match` struct, reversible iterators | Parity polish. |

Each phase should land with its tests and a README section update.

Phase 1 is **implemented** (`src/string.cppm`, `tests/str.cpp`): §5A, §5B
(`char_indices`), §5D `char`/`const char*`/`raw::String` overloads,
`split`/`split_once`/`rsplit_once`, `to_string`, the `trim_ascii*` fixes, §5H
helpers, plus the §2 defects 5, 6 and 7 (iterator end sentinel, `split`
trailing-empty, `lines` first-line/empty) and defensive `char`-pattern handling
that keeps the encoded bytes owned by the iterator.

Phase 2 is **implemented** (`src/string.cppm`, `tests/str.cpp`): the remaining
§5C split family (`rsplit`, `splitn`/`rsplitn`, `split_terminator`/
`rsplit_terminator`, `split_inclusive`/`rsplit_inclusive`, `split_whitespace`),
`match_indices`/`rmatch_indices`, the Unicode `trim`/`trim_start`/`trim_end` and
`trim_matches` family (including the `Char`/`char`/predicate forms of
`trim_*_matches`), `escape_default`/`escape_debug`/`escape_unicode`, and
`to_lowercase`/`to_uppercase`. The empty-pattern alignment from §4.6/§9 landed
with it, and the existing tests were updated accordingly.

Implementation notes:

- Every split variant is backed by one internal `split_state` (a direct port of
  Rust's `SplitInternal`) plus a `pattern_searcher` that also models the empty
  pattern as a match at every code point boundary. A single `Split` range exposes
  all eight variants through constructor flags.
- Rust has no `rsplit_inclusive`; the doc asks for it, so it is implemented as
  `split_inclusive` iterated in reverse.
- `escape_default` escapes ASCII controls (including DEL) as `\u{...}`, matching
  `char::escape_default`; the `\xNN` form mentioned in §5F was incorrect.
- `to_uppercase` uses utf8proc's simple 1:1 mapping, so `"ß".to_uppercase()` is
  `U+1E9E` (`ẞ`) rather than Rust's full-mapping `"SS"`.

The `str` test suite was expanded to cover empty subjects, empty patterns,
non-matches, multi-byte boundaries, repeated/`n`-limited operations and
`*_once` "only splits once" behaviour, which is how defects 5–7 were found.

---

## 9. Decisions and open questions

Resolved (Phase 1):

1. **Plain `char` overloads** — **yes**, accept `s.split(',')`; `char` is treated
   as its numeric value in `[0, 255]` and encoded as UTF-8 (exact for ASCII).
2. **`replace_n` vs `replacen`** — keep the current `replace_n` name.
3. **`to_uppercase` fidelity** — accept utf8proc's simple 1:1 mapping and
   document the limitation (defers until the Unicode case methods land, P2).
4. **`match_indices` value** — `std::pair<size_t, str>` (P2).
5. **`matches` compat** — leave `matches` yielding `size_t`; do not align it to
   Rust's `(index, slice)`. New `match_indices` (P2) will carry the richer shape.

Resolved (Phase 2):

- **Empty-pattern alignment** — **yes**, `find`/`rfind`/`matches`/`split` now
  follow Rust (`Some(0)`, `Some(len)`, boundary matches, boundary split).
- **`to_uppercase`/`to_lowercase`** — use utf8proc's simple mapping; the only
  notable difference from Rust's full mapping is `"ß" -> "ẞ"` instead of `"SS"`.

Still open:

- Whether to introduce a dedicated `Match` type instead of a pair (P3).

---

## 10. String correctness fixes (test-driven)

Reviewing `raw::String` while expanding `tests/string.cpp` surfaced five
defects. All are fixed:

1. **`from_raw_parts` allocated `len` but not `len + 1`.** It never wrote the
   terminating `\0`, yet the destructor (and every other path) assumes
   `capacity + 1` bytes are allocated. Fixed to allocate `len + 1` and write
   the terminator. This affected `String::from`, `String::from_raw_parts` and
   the `_S` literal.
2. **`clear()` did not re-terminate.** It only reset `m_len`, leaving the old
   first byte in place, so `c_str()` was not an empty C string. Fixed to write
   `m_data[0] = '\0'` when a buffer exists.
3. **The `String(View&&)` constructor wrote the terminator one byte too far**
   (`m_data[i + 1]` instead of `m_data[i]`), so the view/`std::ranges::to<String>`
   path produced a non-terminated string.
4. **`split_off(at)` panicked for `at == len`.** Rust allows it (returning an
   empty string and leaving `self` intact). Fixed to allow `at <= len` and to
   handle an empty remainder.
5. **Self-assignment lost the contents.** `operator=` called `clear()` before
   appending `other`, so `s = s` emptied the string. Fixed with a `this != &other`
   guard.

`tests/string.cpp` gained edge cases for `split_off`/`truncate`/`pop`/`clear`/
`reserve`/`push`/`+=`/construction (empty, boundary, multi-byte, out-of-range
panic) plus a `FillingAllocator` that makes a missing terminator or allocation
size mismatch fail deterministically instead of reading uninitialized memory.

---

## 11. `str::c_str()` removal (review-driven)

`str` is a non-owning view, so it cannot honour the NUL-termination contract that
the name `c_str()` implies. For a sub-slice (`slice`, `strip_prefix`, `trim*`,
`split().next()`, …) the pointer lands in the middle of the backing buffer, so
`strlen(s.c_str())` returns the length of the parent's tail rather than the
slice's.

Resolved by removing `str::c_str()` entirely:

- It was byte-for-byte identical to `str::as_raw()` and had no callers.
- `raw::String::c_str()` is the only member that can guarantee termination (it
  allocates `capacity + 1` and re-writes the terminator on every mutation), so it
  stays.
- `str::as_raw()` remains as the honest, non-terminating accessor, with a
  documented `@note`; callers needing a C string use `to_std_string()` (or
  `raw::String`).
- This mirrors `std::string_view`, which deliberately provides no `c_str()`.

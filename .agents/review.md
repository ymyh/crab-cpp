# Review: `tests/str.cpp` coverage of `crab_cpp::str`

- Date: 2026-10-12
- Scope: `tests/str.cpp` (63 test cases) vs. the public API of `str` / `Char` /
  `literal` / `strings` in `src/string.cppm`
- Verdict: **solid on the happy paths and most split/search edge cases, but with
  clear blind spots**: the panic surface of `Char`/literals, null / default-constructed
  degenerate inputs, and the whole interop surface (`std::hash`, `std::format`,
  `operator<<`, `StringHasher`) are essentially untested.

## What is already well covered

- Construction (`str()`, `from_raw_parts`, `from`, invalid UTF-8 → `FromUtf8Error`),
  `_s` literal, comparisons (`==`, `<=>`, mixed `const char*` / `std::string` /
  `raw::String`).
- `contains` / `starts_with` / `ends_with` / `find` / `rfind` / `strip_prefix` /
  `strip_suffix`: all 5 pattern overloads plus empty pattern, empty subject,
  longer-than-subject, multi-byte patterns, invalid-UTF-8 `const char*` panic.
- Full split family (`split`, `rsplit`, `splitn`, `rsplitn`, `split_terminator`,
  `rsplit_terminator`, `split_inclusive`, `rsplit_inclusive`, `split_once`,
  `rsplit_once`): empty pattern, trailing empties, adjacent separators,
  non-overlapping `aaa`/`aa` semantics, multi-byte separators, most overloads.
  The empty-pattern expectations match Rust's actual behaviour (verified against
  `core::str` sources, incl. `split_inclusive("")` → `["", "a", "b", "c"]`).
- Slicing: `slice` / `get` / `split_at` / `split_at_checked`, boundary and
  off-boundary indices, `is_char_boundary`, panic paths via `EXPECT_DEATH`.
- Trimming: ASCII vs Unicode whitespace, `trim_matches` with all pattern kinds
  **and the predicate overload**, regression tests for past bugs.
- Iteration: `chars`, `char_indices`, `graphemes` (combining marks), `lines`
  (`\n`, `\r\n`, mixed, empty lines, lone `\r` mid-string), `split_whitespace`
  vs `split_ascii_whitespace` (NBSP divergence), raw byte iteration.
- `parse` (int/float, overflow, partial, empty, `INT_MAX` boundary), case
  conversion (ASCII + Unicode incl. the `ß`/`ẞ` divergence, documented),
  `escape_default` / `escape_debug` / `escape_unicode`, `replace` / `replace_n`
  (non-overlap, no re-scan), `repeat` incl. overflow panic.

## Missing coverage

### P0 — contract / correctness gaps

| # | Gap | Why it matters |
| --- | --- | --- |
| 1 | `Char` panics on invalid scalar values: `Char(0x110000)`, `Char(0xD800)`, `Char(0xDFFF)` | `Char(std::uint32_t)` is the only validating constructor and is `explicit`ly documented to panic. Zero `EXPECT_DEATH` tests for it anywhere. |
| 2 | `_s` literal with invalid UTF-8 | `operator""_s` calls `.expect(...)`, which calls `panic()`; `panic()` unconditionally calls `std::terminate()` (`panic.cppm` L9–32). The `noexcept` specifier is **irrelevant** to the outcome — removing it would not change anything. Verified to terminate; the single most-used API has an untested panic path. |
| 3 | ~~`std::hash<str>` / `std::formatter<str>` reachability~~ — **refuted, see Verification** | The specializations are genuinely declared at module scope in `string.cppm` **without `export`** (L5170–5244), but importing TUs still see them. `std::format("{}", s)` and `std::unordered_set<str>` compile **and run correctly** on both clang/libstdc++ and GCC 16.2.0. What remains is only the untested-ness, not a compile failure. |
| 4 | `StringHasher` | Exported specifically for heterogeneous unordered lookup, but has zero tests (all 5 `operator()` overloads, `is_transparent`). |
| 5 | `operator<<` for `str` / `Char` | Not tested at all. For `Char` this hides a real quirk (see Issues below). |
| 6 | Null / degenerate construction: `str::from(nullptr)`, `from_raw_parts(nullptr, 0)`, `from_raw_parts(nullptr, 3)` → `Err(0)`, and default-constructed `str()` (null `m_data`) across `as_bytes` / `to_std_string` / `to_string` / `parse` / `trim_ascii` / … | These branches exist in the implementation (`from_raw_parts` has an explicit `data == nullptr && len > 0` check; `trim_ascii_*` check `m_data == nullptr`) and are exactly the kind of code that regresses silently. Note `std::string(nullptr, 0)` is formally UB. |

Suggested snippets:

```cpp
TEST(StringTest, CharInvalidScalarValuePanics)
{
    EXPECT_DEATH((void)Char(0x110000), "Invalid Unicode scalar value");
    EXPECT_DEATH((void)Char(0xD800), "Invalid Unicode scalar value");
    EXPECT_DEATH((void)Char(0xDFFF), "Invalid Unicode scalar value");
}

TEST(StringTest, StrLiteralInvalidUtf8Panics)
{
    // operator""_s expects valid UTF-8; embedded C0 80 is invalid.
    // The death comes from panic() -> std::terminate(), not from noexcept.
    EXPECT_DEATH((void)"\xC0\x80"_s, "Invalid UTF-8 sequence");
}

TEST(StringTest, StrNullPointerHandling)
{
    EXPECT_TRUE(str::from(nullptr).unwrap().empty());
    EXPECT_TRUE(str::from_raw_parts(nullptr, 0).unwrap().empty());
    EXPECT_TRUE(str::from_raw_parts(nullptr, 3).is_err());
    EXPECT_EQ(str::from_raw_parts(nullptr, 3).unwrap_err().pos, 0u);
}
```

### P1 — API surface holes

| # | Gap | Notes |
| --- | --- | --- |
| 7 | `Char` classification family: `is_ascii_alphabetic`, `is_ascii_alphanumeric`, `is_ascii_lowercase`, `is_ascii_uppercase`, `is_ascii_control`, `is_ascii_digit`, `is_ascii_hexdigit` | 7 public predicates, zero tests. Only `is_whitespace`, `len_utf8`, `encode_utf8`, `to_ascii_*` are covered. |
| 8 | `matches(const raw::String&)` and `rmatch_indices` overloads (`const char*`, `raw::String`, `Char`, `char`) | `rmatch_indices` is only tested with a `str` pattern (L1756). The **`Char`/`char` overloads of `matches` were added** (closing the asymmetry noted in earlier revisions), but they are still untested. |
| 9 | U+000B VERTICAL TAB in `is_whitespace` / `split_whitespace` / `trim` | U+000B is Unicode `White_Space` (Rust: `'\u{b}'.is_whitespace() == true`) but `Char::is_whitespace` checks ASCII-ws (which excludes VT, matching Rust's `is_ascii_whitespace`) + U+0085 + Zs/Zl/Zp — VT is Cc, so it is **not** recognized. A test would document (or expose) this divergence. Same gap applies to `"\v"` in `trim()`. |
| 10 | `lines()` with a trailing lone `\r` (`"a\r"` → `["a\r"]` per Rust docs) | The mid-string lone `\r` case is tested; the final-position case (`pos + 1 < bytes.size()` guard in `LinesIter`) is not. |
| 11 | `rsplit_inclusive("")` and `rsplitn(n, "")` | The trickiest code path in `split_state` (`next_back_inclusive` with `allow_trailing_empty = false`, double-empty emission) is only exercised by `rsplit_terminator`/`split_inclusive` with non-empty patterns. |
| 12 | `FromUtf8Error::pos` at a non-zero offset | Only position 0 is tested (`{'\xC0', '\x80'}` at start). `"ab\xC0\x80"` should report `pos == 2`. |
| 13 | `join_with` overloads: `char`, `str`, `raw::String`; empty range; single element | Currently one line of coverage (L515–521, `const char*` only). |
| 14 | Grapheme content, not just counts | Emoji ZWJ sequences (👨‍👩‍👧‍👦), regional indicators (flags), `\r\n` as one cluster; assert the actual bytes of each cluster, not only the count. |
| 15 | `escape_default` for `\r` and `\n` | Only `\t`, quotes, backslash and non-printables are covered. |

### P2 — nice to have

- `c_str()` — **resolved** by removing `str::c_str()` (it was identical to
  `as_raw()` and could not honour NUL-termination for sub-slices); see
  `.agents/api_design.md` §11.
- `to_std_string()` with non-ASCII content — **done** (`StrToStdString`).
- `from_bytes_unchecked` round-trip as a direct (not helper) assertion — **done**
  (`StrFromBytesUncheckedRoundTrip`).
- `to_owned<CustomAlloc>()` / `replace<CustomAlloc>()` (allocator template
  parameters are public API) — **done** (`StrAllocatorTransforms`). In the same
  pass `to_owned` was renamed to `to_string` (Rust's name reads wrong in C++).
- `parse<bool>` / `parse<unsigned>` (negative → `std::errc` behaviour) — **done**
  (`StrParseBool` / `StrParseUnsigned`). `parse<bool>` also required a new
  explicit specialization, because `std::from_chars` has no `bool` overload.
- Use the actual panic message in more `EXPECT_DEATH` matchers — **done**; no
  `".*"` matcher remains in `tests/str.cpp`.
- Several assertions only check `.size()` of the collected pieces without
  content (e.g. `StrSplitEdgeCases` L1141, L1189, L1212–1213) — **done**; every
  content-relevant case now asserts each piece.
- Suite name is `StringTest` for everything in `str.cpp`; `StrTest` would match
  the file's purpose (see `.agents/codeStructure.md`) — **intentionally not
  changed**.

## Implementation issues discovered during this review (test-worthy)

1. ~~**`Char`'s `operator<<` saves/restores flags on the wrong stream**~~ —
   **fixed.** `string.cppm` used to read `std::cout.flags()` but write `std::hex`
   to `os` and restore on `std::cout`. It now saves/restores `os.flags()`, and
   `StringTest.OperatorStreamInsertion` pins the regression.
2. ~~**`std::formatter` / `std::hash` specializations are not exported**~~ —
   **refuted** (see Verification below). The declarations at `string.cppm`
   L5170–5244 really do lack `export`, but non-exported explicit specializations
   are still visible to importing TUs; `std::format` and `std::unordered_set`
   compile and run on both clang and GCC. Adding `export` explicitly is optional
   hygiene, not a bug fix.
3. ~~**`str::slice(from, to)` doc comment says `to` is inclusive**~~ —
   **fixed.** The doc now says `to` is exclusive and the panic notes match the
   implementation.
4. ~~**`operator""_s` / `operator""_S` ignore the `size_t len` parameter**~~ —
   **fixed.** Both now go through `str::from_raw_parts(str, len)` /
   `String::from_raw_parts(str, len)`, so embedded NULs are preserved;
   `StringTest.StrLiteralEmbeddedNul` pins it.
5. ~~**`Char::is_whitespace` misses U+000B**~~ — **fixed.** U+000B is now
   recognised as Unicode whitespace (matching Rust's `char::is_whitespace`);
   `is_ascii_whitespace` still excludes it, and `trim_ascii` therefore leaves it
   alone. `StringTest.VerticalTabIsWhitespace` pins both.
6. ~~**`matches()` yields byte indices, not matched slices**~~ — **fixed.**
   `matches()` now yields the matched `str` slices (Rust semantics), its
   `Char`/`char` overloads were added, and `rmatches()` (all five overloads) was
   added for symmetry. `match_indices`/`rmatch_indices` keep the
   `(index, slice)` shape, so `matches(p)` is now `match_indices(p)` without the
   index. Covered by `StrMatches`, `StrMatchesEdgeCases`, `MatchesOverloads` and
   `RMatches`.

## Verification of the P0 claims

The P0 items were re-checked with real compiles and runs (clang/libstdc++ and
GCC 16.2.0, using the prebuilt modules) rather than by reading alone:

- **#1 — confirmed.** `Char(0x110000)`, `Char(0xD800)` and `Char(0xDFFF)` all
  terminate with `Panic encountered: Invalid Unicode scalar value`.
- **#2 — confirmed (behaviour), corrected (mechanism).** `"\xC0\x80"_s`
  terminates via `panic() -> std::terminate()`, independent of the `noexcept`.
- **#3 — refuted.** `std::format("{}", s)` compiles and prints `hello`;
  `std::unordered_set<str>` compiles; both clang and GCC agree. The custom
  formatter is genuinely used (a probe showed `std::format("{}",
  std::array<std::byte, 1>{})` does *not* compile, so there is no
  range-formatter fallback). A control module confirmed clang still hides
  ordinary non-exported entities, so the visibility of explicit specializations
  is real and not an artefact of `-fmodule-file`. Only the untested-ness
  remains.
- **#4 / #5 — confirmed coverage gaps** (both usages compile).
- **#6 — confirmed.** `from(nullptr)` is empty, `from_raw_parts(nullptr, 0)` is
  `Ok`, `from_raw_parts(nullptr, 3)` is `Err(pos == 0)`, and default
  `str().as_bytes()` is empty.

All real P0 items now have tests in `tests/str.cpp`:
`CharInvalidScalarValuePanics`, `StrLiteralInvalidUtf8Panics`,
`StringHasherTransparent`, `StringHasherHeterogeneousLookup`,
`OperatorStreamInsertion` (also pins the `Char` stream-flag fix) and
`StrNullPointerHandling`. Verified green: **140/140 tests pass under both GCC
16.2.0 and clang (mingw/libstdc++)**.

Writing those tests also exposed a related omission, now fixed: there was no
`operator==` between `str` and `std::string_view` in either direction, so
`StringHasher`'s `string_view` overload could not drive heterogeneous lookup.
Both directions were added to `string.cppm`, and `StrStringViewComparison`
covers them.

## Suggested next steps

1. Add the P0 tests first; they protect the panic contract and are cheap.
2. `std::format` + `std::unordered_set<str>` usage was compiled and runs fine
   (issue #2 refuted) — still add it as a smoke test so the interop surface
   (P0 #4/#5, `StringHasher`, `operator<<`) is covered going forward.
3. Add the U+000B and trailing-`\r` cases; both are one-liners that pin down
   Unicode conformance.
4. Rename the suite to `StrTest` opportunistically (not worth a standalone PR).

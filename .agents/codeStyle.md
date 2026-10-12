# Code Style

This document describes the code style used across `crab_cpp`. The existing
code in `src/`, `tests/`, and `include/` is the source of truth; when in doubt,
match the file you are editing. All rules below are mandatory for new code.

## Language and layout basics

- Target **C++23** only. Use modules; do not add classic headers except the
  global module fragment (`module;` + `#include`) and `include/crab_cpp/*.h`
  for macros.
- Indent with **4 spaces**. Never use tab characters for indentation.
- End files with a single trailing newline. Avoid trailing whitespace.
- Keep one blank line between logical blocks (e.g. before a `return` that
  follows a multi-line `if` body).
- Wrap code at a reasonable width; follow the existing files' line breaks
  rather than hard-wrapping at an arbitrary column.

## Braces (Microsoft style / Allman)

- Opening braces go on their **own line**, for namespaces, types, functions,
  control statements, and lambdas with statement bodies.

```cpp
if (self.is_some())
{
    return f(std::get<1>(self));
}

return None{};
```

- **Every control statement must have braces**, even when the body is a single
  statement. This is forbidden:

```cpp
// Bad
if (x > 0) return;

// Bad
if (x > 0)
    return;

// Good
if (x > 0)
{
    return;
}
```

- `else` goes on its own line after the closing brace:

```cpp
if (cond)
{
    // ...
}
else
{
    // ...
}
```

- Short single-line functions and one-line lambdas are acceptable when the body
  is a single expression, matching the existing style:

```cpp
auto and_then(this Self&& self, F&& f) -> Option<typename U::type>
{
    // ...
}

auto mapped = some.map([](int x) { return x * 2; });
```

## Whitespace and operators

- Put a space after control keywords: `if (`, `while (`, `for (`, `switch (`.
- Put a space around binary operators and after commas.
- Do **not** put a space between a function name and its argument list:
  `foo(x)`, not `foo (x)`.
- Prefer the trailing-comma, multi-line style already used for argument lists
  and templates.

## Pointers and references

- The `&` and `*` must be **attached to the type**, not the variable name.
  This applies to parameters, return types, locals, and casts.

```cpp
// Bad
void f(int &x);
void f(int *p);
int &ref = ...;

// Good
void f(int& x);
void f(int* p);
int& ref = ...;
```

- `const`/`volatile` qualifiers attach to the type as usual:
  `const std::byte* data`, `const T1& x`.

## Naming

- Types (struct/class/enum/alias): `PascalCase` — `Option`, `Result`,
  `FromUtf8Error`, `plan_str` (lowercase exception exists in current code for
  internal helpers).
- Functions and variables: `snake_case` — `is_some`, `unwrap`, `expect_take`.
- Private data members: `m_` prefix — `m_data`, `m_len`, `m_value`.
- Template parameters: `PascalCase` for types (`T`, `Alloc`, `Self`, `F`, `U`).
- Namespaces: `snake_case` — `crab_cpp`, `raw`, `strings`, `literal`, `internal`.
- Macros: `SCREAMING_SNAKE_CASE` with the `CRAB_CPP_` prefix where applicable.

## Functions and declarations

- Use **trailing return types**: `auto name(args) -> Ret`.

```cpp
[[nodiscard]] auto is_none() const noexcept -> bool
{
    return this->index() == 0;
}
```

- Apply attributes consistently with the existing code:
  - `[[nodiscard]]` on queries and factory functions.
  - `noexcept` when the function cannot throw.
  - `constexpr` where the operation is compile-time capable.
  - `explicit` on single-argument constructors.
- Mark overridden operators and comparisons with `= default` where possible.
- Inside templates, use `this->` to access dependent members.
- Prefer `using` type aliases over `typedef`.

## Modules and includes

- Each module starts with its module declaration, then imports:

```cpp
export module crab_cpp:option;

import std;
import :panic;
```

- The string module needs a global module fragment for the C header:

```cpp
module;

#include "utf8proc.h"

export module crab_cpp:string;

import :panic;
import :option;
import :result;
import std;
```

- The umbrella module re-exports sub-modules and gates optional ones behind
  feature macros:

```cpp
export module crab_cpp;

export import :match;
export import :panic;
export import :option;
export import :result;

#ifdef CRAB_CPP_ENABLE_STRING
export import :string;
#endif
```

- Put public API in `export namespace crab_cpp { ... }`. Use nested namespaces
  (e.g. `raw`, `strings`, `literal`, `internal`) as the existing code does.

## Templates and concepts

- Declare template parameters with `template<typename ...>`.
- Use `requires` clauses and requires-expressions for constraints, matching the
  existing style. Put the `requires` clause on its own indented line below the
  template header:

```cpp
template<typename F, typename Self>
    requires std::invocable<F, T&>
auto inspect(this Self&& self, F&& f) -> Self
{
    // ...
}
```

## Comments

- Use Doxygen-style `@brief` comments for public types and functions, including
  `@tparam`, `@param`, `@return`, and `@note`/`@warning` where relevant.

```cpp
/**
 * @brief Returns the contained Some value, leaving None in its place
 * @param msg The panic message to display if the value is None
 * @return The contained value
 * @note Panics if the value is None
 */
```

- Use `//` for short inline or section comments (`// operators`,
  `// constructors`), as found in `string.cppm`.
- Preprocessor directives inside function bodies are indented like the existing
  `#ifdef CRAB_CPP_ENABLE_BACKTRACE` blocks in `panic.cppm`.

## Tests

- Tests live in `tests/*.cpp`, use GoogleTest, and `import crab_cpp;`.
- Name test cases `TEST(SuiteName, CaseName)` with `PascalCase`, e.g.
  `TEST(OptionTest, IsSomeNone)`.
- Use `EXPECT_*` assertions and `EXPECT_DEATH` for panic paths.

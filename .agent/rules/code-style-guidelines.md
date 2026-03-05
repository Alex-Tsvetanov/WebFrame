# C++23 Code Style Guidelines

These guidelines are enforced via `.clang-format` and `.clang-tidy`. `WarningsAsErrors: "*"` is enabled, meaning all linter warnings are treated as errors and will break the build. Ensure you adhere to these rules when writing code.

## 1. Formatting (`.clang-format`)

- **Indentation:** 4 spaces. No tabs.
- **Line Length:** 120 characters maximum.
- **Braces:** Braces are wrapped on their own line after classes, functions, control statements, enums, namespaces, and structs (`BreakBeforeBraces: Custom`).
- **Pointers and References:** Left-aligned (e.g., `int* ptr`, `const std::string& str`).
- **Access Modifiers:** Aligned with the class body (`AccessModifierOffset: -4`), meaning no extra indentation for `public:`, `protected:`, `private:`.
- **Namespaces:** Indent contents inside namespaces.
- **Includes:** Automatically grouped but not strictly sorted alphabetically. 

## 2. Naming Conventions (`.clang-tidy`)

- **Types (Classes, Structs, Enums):** `CamelCase` (e.g., `MyClass`, `GameState`).
- **Enum Constants:** `CamelCase` (e.g., `ActiveState`, `Pending`).
- **Functions & Methods:** `lower_case` (e.g., `get_user_data()`, `process_input()`).
- **Variables (Local, Global, Parameters):** `lower_case` (e.g., `player_name`, `index`).
- **Namespaces:** `lower_case` (e.g., `network`, `core`).
- **Data Members (Private/Protected):** `lower_case` with a trailing underscore `_` (e.g., `buffer_`, `connection_id_`).

## 3. Linter Rules & Exceptions

This project uses modern, aggressive checks (`bugprone-*`, `cert-*`, `cppcoreguidelines-*`, `google-*`, `modernize-*`, `performance-*`, `readability-*`) with some notable exceptions for flexibility:

- **Magic Numbers:** Allowed (`-cppcoreguidelines-avoid-magic-numbers`, `-readability-magic-numbers`).
- **Arrays & Pointers:** C-style arrays and pointer arithmetic/decay are explicitly allowed.
- **Auto Keyword:** Use `auto` for typenames evaluating to 5+ characters (`modernize-use-auto.MinTypeNameLength: 5`).
- **Return Types:** Trailing return types (`auto foo() -> int`) are *not* rigidly enforced.
- **Implicit Conversions:** Implicit boolean conversions are allowed.
- **Performance Warnings:** `WarnOnAllAutoCopies` is active for range-based for loops. Always use references (`const auto&` or `auto&`) in for-range loops unless explicitly copying.

## 4. C++23 Best Practices

As a modern C++23 codebase, embrace the following patterns and avoid legacy C++ idioms where better alternatives exist:

### Error Handling & State
- **Prefer `std::expected` and `std::optional`:** Avoid out-parameters and excessive exception throwing. Use monadic operations (`.and_then()`, `.transform()`, `.or_else()`) for clean and functional flow control.

### Memory & Views
- **No Raw Pointers for Ownership:** Rely strictly on smart pointers (`std::unique_ptr`, `std::shared_ptr`) when ownership is transferred.
- **Use `std::span` & `std::string_view`:** When passing contiguous non-owning memory or strings to functions, prefer `std::span<T>` and `std::string_view` over `const std::vector<T>&`, `const std::string&`, or pointer-size pairs. 

### Const Correctness
- **`constexpr` / `consteval` Everything:** Evaluate at compile-time as much as possible. If a function or variable logic can be evaluated at compile-time, it should be marked `constexpr` or `consteval`.

### Templates & Generic Programming
- **Use Concepts over SFINAE:** Constrain templates using C++20/C++23 `requires` clauses and Concepts instead of `std::enable_if`.
- **Deducing this (C++23):** Use explicit object parameters (`this auto&& self`) to de-duplicate `const` and non-`const` method overloads.

### Loops & Algorithms
- **Prefer `std::ranges` / `std::views`:** Use composable views over raw loops and mutating algorithms. Examples: `std::views::filter`, `std::views::transform`.

### Strings & Formatting
- **Formatting:** Use C++20 `std::format` and C++23 `std::print` for building strings and printing, replacing traditional `<iostream>` and `<cstdio>` wrappers.

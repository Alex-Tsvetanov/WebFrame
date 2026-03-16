---
title: Type-Safe Parameter Extraction
tags:
  - coroute
  - architecture
  - type-safety
  - routing
---

# Type-Safe Parameter Extraction

> [!abstract]
> Coroute uses C++20 concepts and variadic templates to provide compile-time validation of URL parameters. This eliminates boilerplate manual conversion and prevents entire classes of runtime errors.

## Motivation

In traditional frameworks, URL parameters are often extracted as strings and manually parsed:

```cpp
// Traditional approach (error-prone)
app.get("/user/{id}", [](Request& req) -> Task<Response> {
    auto id_str = req.param("id");
    int id = std::stoi(id_str); // Manual, might throw
    // ...
});
```

Coroute automates this:

```cpp
// Coroute approach (type-safe)
app.get<int>("/user/{id}", [](int id, Request& req) -> Task<Response> {
    // id is already an int, validated before the handler is called
    co_return Response::ok("User ID: " + std::to_string(id));
});
```

## Internal Machinery

### 1. `route<Args...>` Registration
The `App` class uses variadic templates to specify expected types. It uses `requires std::invocable` to ensure the handler signature matches the specified types plus a trailing `Request&`.

### 2. `make_handler<Args...>` Wrapper
When a route is registered, Coroute creates a wrapper lambda that:
1.  Extracts the raw strings from the router's match result.
2.  Converts them using the `FromString<T>` trait system.
3.  Checks if all conversions succeeded.
4.  Invokes the user's handler with the typed arguments.

### 3. `invoke_with_params`
Uses C++ index sequences to unpack the tuple of extracted parameters into the handler function:

```cpp
template<typename... Args, typename F, size_t... Is>
static Task<Response> invoke_with_params(...) {
    auto params = std::make_tuple(extract_param<Args>(req, Is)...);
    if (!all_valid(params)) {
        co_return Response::bad_request("Invalid route parameters");
    }
    co_return co_await func(*std::get<Is>(params)..., req);
}
```

## `FromString<T>` Trait System

The conversion logic is extensible via template specializations.

| Type | Implementation | Notes |
| :--- | :--- | :--- |
| `int`, `long`, etc. | `std::from_chars` | Locale-independent, high performance. |
| `float`, `double` | `std::from_chars` | Fast floating point conversion. |
| `std::string` | Identity | Direct move from the match result. |
| **Custom** | User-defined | Define `FromString<MyType>::parse` to support custom types. |

### Example: Custom UUID Support
```cpp
template<>
struct FromString<UUID> {
    static expected<UUID, Error> parse(std::string_view s) {
        auto uuid = UUID::from_string(s);
        return uuid ? *uuid : unexpected(Error::parse("Invalid UUID"));
    }
};

// Now usable in routing:
app.get<UUID>("/items/{id}", [](UUID id, Request& req) { ... });
```

## Performance Benefits
- **`std::from_chars`**: Up to 5x faster than `std::stoi` or `sscanf` due to lack of locale overhead and zero allocations.
- **Zero Allocations**: Most parameter extractions work directly on `std::string_view` into the original request buffer.

## Compile-Time Guarantees

The C++ compiler verifies:
1.  The number of template arguments matches the handler's parameters.
2.  The types are compatible with the `FromString` trait.
3.  The handler returns a `Task<Response>`.

If a mismatch occurs, the code fails to compile, preventing bugs from reaching production.

## Related Notes
- [[Architecture/DFA Routing and Middleware]]
- [[Coroutine Model and Task <T>]]

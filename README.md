# Coroute v2 - Modern C++23 Web Framework

![C++](https://img.shields.io/badge/c++23-%2300599C.svg?&logo=c%2B%2B&logoColor=white) ![CMake](https://img.shields.io/badge/CMake-%23008FBA.svg?&logo=cmake&logoColor=white)

[![CI Build](https://github.com/cpp-for-everything/WebFrame/actions/workflows/ci.yml/badge.svg)](https://github.com/cpp-for-everything/WebFrame/actions/workflows/ci.yml) [![Documentation](https://github.com/cpp-for-everything/WebFrame/actions/workflows/documentation.yml/badge.svg)](https://github.com/cpp-for-everything/WebFrame/actions/workflows/documentation.yml) [![CodeQL](https://github.com/cpp-for-everything/WebFrame/actions/workflows/codeql.yml/badge.svg)](https://github.com/cpp-for-everything/WebFrame/actions/workflows/codeql.yml)

[![Issues](https://img.shields.io/github/issues/cpp-for-everything/WebFrame?)](https://github.com/cpp-for-everything/WebFrame/issues) [![Stars](https://img.shields.io/github/stars/cpp-for-everything/WebFrame?)](https://github.com/cpp-for-everything/WebFrame) [![License](https://img.shields.io/github/license/cpp-for-everything/WebFrame?)](https://github.com/cpp-for-everything/WebFrame/blob/master/LICENSE.md)

**High-performance web framework built on C++20 coroutines with HTTP/2, TLS, and WebSocket support**
## ✨ Features

- 🚀 **C++20 Coroutines** - Natural async/await syntax with `co_await` and `Task<T>`
- ⚡ **Platform-Optimized I/O** - io_uring (Linux), kqueue (macOS), IOCP (Windows)
- 🔒 **TLS/SSL Support** - Secure HTTPS via OpenSSL with ALPN
- 📡 **HTTP/2** - Stream multiplexing and HPACK header compression. Server push is not implemented: a PUSH_PROMISE is answered with GOAWAY
- 🔌 **WebSocket** - Bidirectional real-time communication
- 📝 **Template Engine** - Inja (Jinja2-style) for dynamic HTML rendering
- 🗜️ **Compression** - Built-in gzip, deflate, and optional Brotli
- 🎯 **Type-Safe Routing** - Automatic parameter extraction with compile-time type checking
- 🔧 **Middleware System** - Composable request/response pipeline
- 📊 **Metrics & Logging** - Built-in performance tracking and structured logging

## 📋 Requirements

| Component | Version |
|-----------|----------|
| **Compiler** | GCC 13+, Clang 17+, or MSVC 2022+ |
| **C++ Standard** | C++23 (coroutines are C++20) |
| **CMake** | 3.25 or higher for presets, 3.20 without |
| **OpenSSL** | 1.1.1+ or 3.0+, required only when `COROUTE_ENABLE_TLS=ON` |
| **liburing** | Linux, any build with the default io_uring backend; not needed with `-DCOROUTE_IO_BACKEND=epoll` |
| **pkg-config** | Linux, same condition (`pkgconf` on Arch) |
| **Ninja** | any `cmake --preset`; the plain `cmake -B build` above uses the default generator |
| **Platform** | Linux (kernel 5.1+ for io_uring), macOS 10.13+, Windows 10+ |

Arch: `sudo pacman -S --needed base-devel cmake ninja openssl liburing` (pkgconf is in base-devel).
## 📚 Documentation & Resources

- 📖 **[API Documentation](https://cpp-for-everything.github.io/WebFrame/docs/)** - Full Doxygen documentation
- 🔍 **[CodeQL Analysis](https://cpp-for-everything.github.io/WebFrame/codeql_report/)** - Security and code quality reports
- 📊 **[Performance Benchmarks](https://cpp-for-everything.github.io/WebFrame/benchmark/)** - Latest benchmark results
- 💡 **[Examples](./examples/)** - Complete working examples
## 🚀 Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/cpp-for-everything/WebFrame.git
cd WebFrame

# Build with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run examples
./build/examples/Samples/hello_world/hello_world
```

### Hello World Example

```cpp
#include <coroute/coroute.hpp>

using namespace coroute;

int main() {
    App app;
    
    // Simple GET route
    app.get("/", [](Request&) -> Task<Response> {
        co_return Response::ok("<h1>Hello, World!</h1>");
    });
    
    // Route with path parameter (automatic type extraction)
    app.get<std::string>("/user/{name}", [](std::string name, Request&) -> Task<Response> {
        co_return Response::ok("Hello, " + name + "!");
    });
    
    // Run server on port 8080
    app.threads(std::thread::hardware_concurrency());
    app.run(8080);
    
    return 0;
}
```

## 📖 Usage Guide

### Basic Routing

```cpp
App app;

// GET request
app.get("/", [](Request&) -> Task<Response> {
    co_return Response::html("<h1>Home</h1>");
});

// POST request
app.post("/submit", [](Request& req) -> Task<Response> {
    std::string body = std::string(req.body());
    co_return Response::ok("Received: " + body);
});

// Multiple HTTP methods
app.put("/update/{id}", [](Request& req) -> Task<Response> {
    size_t id = req.param<size_t>(0).value_or(0);
    co_return Response::json(R"({"updated": )" + std::to_string(id) + "}");
});
```

### Path Parameters with Type Safety

```cpp
// Single parameter
app.get<size_t>("/user/{id}", [](size_t id, Request&) -> Task<Response> {
    co_return Response::ok("User ID: " + std::to_string(id));
});

// Multiple parameters
app.get<size_t, size_t>("/user/{uid}/post/{pid}", 
    [](size_t uid, size_t pid, Request&) -> Task<Response> {
    nlohmann::json j = {{"user_id", uid}, {"post_id", pid}};
    co_return Response::json(j.dump());
});

// String parameters
app.get<std::string, std::string>("/blog/{category}/{slug}",
    [](std::string category, std::string slug, Request&) -> Task<Response> {
    co_return Response::ok("Category: " + category + ", Slug: " + slug);
});
```

### Query Parameters

```cpp
app.get("/search", [](Request& req) -> Task<Response> {
    // Required parameter
    auto query = req.query<std::string>("q");
    if (!query) {
        co_return Response::bad_request("Missing 'q' parameter");
    }
    
    // Optional parameter with default
    auto page = req.query_opt<int>("page").value_or(1);
    auto limit = req.query_opt<int>("limit").value_or(10);
    
    nlohmann::json result = {
        {"query", *query},
        {"page", page},
        {"limit", limit}
    };
    
    co_return Response::json(result.dump());
});
```

### Template Rendering (Inja)

```cpp
App app;

// Configure template directory
app.set_templates("./templates");

// Render template with data
app.get("/profile", [&app](Request&) -> Task<Response> {
    nlohmann::json data = {
        {"name", "Alice"},
        {"email", "alice@example.com"},
        {"logged_in", true}
    };
    
    co_return app.render_html("profile.html", data);
});

// Add custom template function
app.add_template_callback("upper", 1, [](inja::Arguments& args) {
    std::string s = args.at(0)->get<std::string>();
    for (char& c : s) c = std::toupper(c);
    return s;
});
```

## 🔧 Advanced Features

### Middleware

```cpp
// Compression middleware
app.use(compression({.min_size = 256, .level = 6}));

// Custom logging middleware
app.use([](Request& req, Next next) -> Task<Response> {
    auto start = std::chrono::steady_clock::now();
    std::cout << "[REQUEST] " << method_to_string(req.method()) 
              << " " << req.path() << std::endl;
    
    auto resp = co_await next(req);
    
    auto duration = std::chrono::steady_clock::now() - start;
    std::cout << "[RESPONSE] " << resp.status() << " (" 
              << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
              << "ms)" << std::endl;
    
    co_return resp;
});

// Authentication middleware
app.use([](Request& req, Next next) -> Task<Response> {
    auto auth = req.header("Authorization");
    if (!auth || !validate_token(*auth)) {
        co_return Response(401, {{"WWW-Authenticate", "Bearer"}}, "Unauthorized");
    }
    co_return co_await next(req);
});
```

### TLS/HTTPS

```cpp
AppTlsConfig tls_config{
    .cert_file = "server.crt",
    .key_file = "server.key",
    .alpn_protocols = {"h2", "http/1.1"}  // Enable HTTP/2 negotiation
};

app.enable_tls(tls_config);
app.run(443);
```

### WebSocket

```cpp
app.ws("/chat", [](WebSocketConnection& ws) -> Task<void> {
    std::cout << "New WebSocket connection" << std::endl;
    
    while (auto msg = co_await ws.receive()) {
        std::cout << "Received: " << *msg << std::endl;
        co_await ws.send("Echo: " + *msg);
    }
    
    std::cout << "WebSocket closed" << std::endl;
});
```

### HTTP/2

```cpp
// HTTP/2 is automatically enabled when TLS with ALPN is configured
app.enable_tls(tls_config);
app.enable_http2(true);  // Enabled by default with TLS
app.run(443);
```

### Response Builders

```cpp
app.get("/custom", [](Request&) -> Task<Response> {
    co_return ResponseBuilder()
        .status(200)
        .header("X-Custom-Header", "value")
        .header("X-Request-ID", generate_uuid())
        .content_type("application/json")
        .body(R"({"status":"ok"})"
        .build();
});
```

### Graceful Shutdown

```cpp
ShutdownOptions opts{
    .drain_timeout = std::chrono::seconds(30),
    .force_close_after_timeout = true
};

app.shutdown(opts);
```

## 🏗️ Building from Source

### CMake Presets (preferred)

`CMakePresets.json` selects the platform I/O backend and the build gates by name, so
sanitizer and lint configurations are reproducible locally rather than living only in
CI shell lines.

```bash
cmake --list-presets
cmake --preset linux-release && cmake --build --preset linux-release
```

| Preset | What it builds |
|--------|----------------|
| `linux-debug` / `linux-release` | io_uring backend |
| `macos-debug` / `macos-release` | kqueue backend |
| `windows-debug` / `windows-release` | IOCP backend |
| `asan` | AddressSanitizer + UBSan, examples included |
| `tsan` | ThreadSanitizer |
| `msan` | MemorySanitizer, core only (see note below) |
| `tidy` | clang-tidy over the library sources |
| `coverage` | llvm-cov instrumentation |
| `bench` | Release, tests off. Benchmarks are never run against Debug |

Configure, build and test a gate in one step:

```bash
cmake --workflow --preset asan
```

MemorySanitizer needs every dependency instrumented. The `msan` preset therefore turns
TLS, HTTP/2 and simdjson off; with them on, uninstrumented library code produces false
positives that bury the real findings.

### Manual configuration

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCOROUTE_BUILD_EXAMPLES=ON \
  -DCOROUTE_BUILD_TESTS=ON \
  -DCOROUTE_ENABLE_TLS=ON \
  -DCOROUTE_ENABLE_HTTP2=ON \
  -DCOROUTE_ENABLE_SIMDJSON=ON \
  -DCOROUTE_ENABLE_BROTLI=OFF

cmake --build build --parallel
```

| Option | Default | Description |
|--------|---------|-------------|
| `COROUTE_BUILD_EXAMPLES` | ON | Build example applications |
| `COROUTE_BUILD_TESTS` | ON | Build unit tests |
| `COROUTE_ENABLE_TLS` | ON | TLS/SSL support. OpenSSL is required only when this is ON |
| `COROUTE_ENABLE_HTTP2` | ON | Enable HTTP/2 support |
| `COROUTE_ENABLE_SIMDJSON` | ON | Fast JSON parsing |
| `COROUTE_ENABLE_BROTLI` | OFF | Brotli compression |
| `COROUTE_SANITIZER` | (none) | `address`, `undefined`, `address+undefined`, `thread`, `memory` |
| `COROUTE_ENABLE_COVERAGE` | OFF | Coverage instrumentation |
| `COROUTE_ENABLE_CLANG_TIDY` | OFF | Run clang-tidy over the library target |
| `COROUTE_USE_STD_EXPECTED` | OFF | Use `std::expected` instead of the bundled implementation |

### Running Tests

```bash
ctest --test-dir build --output-on-failure --parallel
```

## 📊 Performance

Coroute achieves high performance through:
- **Zero-copy I/O** where possible
- **Platform-native async I/O** (io_uring, kqueue, IOCP)
- **Efficient coroutine scheduling**
- **Object pooling** for reduced allocations
- **HTTP/2 multiplexing** for concurrent requests

See [benchmark results](https://cpp-for-everything.github.io/WebFrame/benchmark/) for detailed performance metrics.

## 🤝 Contributing

Contributions are welcome! Please see our contributing guidelines and code of conduct.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📜 License

This project is licensed under the terms specified in [LICENSE.md](LICENSE.md).

## 🔗 Links

- **Documentation**: https://cpp-for-everything.github.io/WebFrame/docs/
- **Examples**: [./examples/](./examples/)
- **Issue Tracker**: https://github.com/cpp-for-everything/WebFrame/issues
## 👤 Author

**Alex Tsvetanov**

[![LinkedIn](https://img.shields.io/badge/linkedin-%230077B5.svg?logo=linkedin&logoColor=white)](https://www.linkedin.com/in/alex-tsvetanov/)

---

**Made with ❤️ using C++20 Coroutines**

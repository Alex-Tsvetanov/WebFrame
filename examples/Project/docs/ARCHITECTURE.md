# Production Architecture Guide

This document outlines best practices for building production-level web applications with Coroute.

## Directory Structure

```text
Project/
├── src/
│   ├── main.cpp              # Entry point - minimal, just starts server
│   ├── app/
│   │   ├── config.hpp/cpp    # Configuration management
│   │   └── server.hpp/cpp    # Server setup & route registration
│   ├── middleware/           # Request/response interceptors
│   ├── handlers/             # HTTP route handlers
│   │   ├── pages.hpp/cpp     # Server-rendered pages
│   │   ├── api/              # REST API endpoints
│   │   └── websocket/        # WebSocket handlers
│   ├── models/               # Data structures & serialization
│   └── services/             # Business logic (HTTP-agnostic)
├── tests/
│   ├── unit/                 # Fast, isolated tests
│   ├── integration/          # Full HTTP round-trip tests
│   └── fixtures/             # Shared test utilities
├── templates/                # Jinja2-style HTML templates
├── static/                   # CSS, JS, images
└── docs/                     # Documentation
```

## Design Principles

### 1. Separation of Concerns

**Handlers** - HTTP layer only

- Parse request parameters
- Call services
- Format responses
- Should NOT contain business logic

```cpp
// Good: Handler delegates to service
Task<Response> create_task(Request& req, TaskService& service) {
    auto body = co_await req.json();
    auto task = service.create(body);  // Business logic in service
    co_return Response::json(task.to_json().dump(), 201);
}

// Bad: Business logic in handler
Task<Response> create_task(Request& req) {
    auto body = co_await req.json();
    // Validation, database calls, etc. mixed in handler
    if (body["title"].empty()) { ... }
    db.insert("tasks", ...);
    // ...
}
```

**Services** - Business logic

- Validation
- Data transformation
- Orchestration of operations
- Should NOT know about HTTP (Request/Response)

```cpp
// Good: Service is HTTP-agnostic
class TaskService {
public:
    Task create(const CreateTaskRequest& req);  // Plain C++ types
    std::vector<Task> list(const TaskFilter& filter);
};

// Bad: Service depends on HTTP types
class TaskService {
public:
    Response create(Request& req);  // Coupled to HTTP
};
```

**Models** - Data structures

- Plain data classes
- JSON serialization/deserialization
- Validation rules

```cpp
struct Task {
    int64_t id;
    std::string title;
    std::string description;
    TaskStatus status;
    int64_t user_id;
    int64_t created_at;
    
    nlohmann::json to_json() const;
    static Task from_json(const nlohmann::json& j);
    static std::optional<std::string> validate(const nlohmann::json& j);
};
```

### 2. Dependency Injection

Pass dependencies explicitly rather than using globals:

```cpp
// Good: Dependencies injected
class Server {
    App app_;
    TaskService task_service_;
    UserService user_service_;
    
public:
    Server(Config config, TaskService ts, UserService us)
        : task_service_(std::move(ts))
        , user_service_(std::move(us)) {}
};

// Bad: Global state
TaskService g_task_service;  // Global

void setup_routes(App& app) {
    app.get("/tasks", [](Request&) {
        return g_task_service.list();  // Hidden dependency
    });
}
```

### 3. Route Registration Pattern

Group related routes in dedicated functions:

```cpp
// handlers/api/tasks.hpp
namespace project::handlers::api::tasks {
    void register_routes(App& app, TaskService& service);
}

// handlers/api/tasks.cpp
namespace project::handlers::api::tasks {

void register_routes(App& app, TaskService& service) {
    app.get("/api/tasks", [&service](Request& req) -> Task<Response> {
        auto tasks = service.list();
        co_return Response::json(to_json_array(tasks).dump());
    });
    
    app.post("/api/tasks", [&service](Request& req) -> Task<Response> {
        auto body = nlohmann::json::parse(req.body());
        auto task = service.create(CreateTaskRequest::from_json(body));
        co_return Response::json(task.to_json().dump(), 201);
    });
    
    // ... more routes
}

}  // namespace
```

### 4. Middleware Composition

Create reusable middleware functions:

```cpp
// middleware/auth.hpp
namespace project::middleware {

// Factory function returns configured middleware
Middleware require_auth(const std::string& secret) {
    return [secret](Request& req, Next next) -> Task<Response> {
        auto session = req.session();
        if (!session.has("user_id")) {
            co_return Response::unauthorized("Authentication required");
        }
        co_return co_await next(req);
    };
}

Middleware require_role(const std::string& role) {
    return [role](Request& req, Next next) -> Task<Response> {
        auto user_role = req.context<std::string>("role");
        if (user_role != role) {
            co_return Response::forbidden("Insufficient permissions");
        }
        co_return co_await next(req);
    };
}

}  // namespace
```

### 5. Error Handling

Use structured error responses:

```cpp
// models/api_error.hpp
struct ApiError {
    int code;
    std::string message;
    std::optional<nlohmann::json> details;
    
    Response to_response() const {
        nlohmann::json body;
        body["error"]["code"] = code;
        body["error"]["message"] = message;
        if (details) body["error"]["details"] = *details;
        return Response::json(body.dump(), code);
    }
};

// Usage in handlers
Task<Response> get_task(Request& req, TaskService& service) {
    auto id = req.param<int64_t>(0);
    if (!id) {
        co_return ApiError{400, "Invalid task ID"}.to_response();
    }
    
    auto task = service.find(*id);
    if (!task) {
        co_return ApiError{404, "Task not found"}.to_response();
    }
    
    co_return Response::json(task->to_json().dump());
}
```

### 6. Configuration Management

Externalize configuration:

```cpp
// app/config.hpp
struct Config {
    uint16_t port = 8080;
    std::string host = "0.0.0.0";
    std::string session_secret;
    std::filesystem::path static_dir;
    std::filesystem::path template_dir;
    bool enable_tls = false;
    std::filesystem::path cert_file;
    std::filesystem::path key_file;
    
    // Load from environment variables
    static Config from_env();
    
    // Load from JSON file
    static Config from_file(const std::filesystem::path& path);
};
```

### 7. WebSocket Patterns

Use a hub pattern for broadcasting:

```cpp
// handlers/websocket/task_hub.hpp
class TaskHub {
    std::unordered_map<int, WebSocketConnection*> connections_;
    std::mutex mutex_;
    
public:
    void add_connection(int id, WebSocketConnection* conn);
    void remove_connection(int id);
    
    // Broadcast to all connected clients
    Task<void> broadcast(const nlohmann::json& message);
    
    // Send to specific user
    Task<void> send_to_user(int user_id, const nlohmann::json& message);
};

// Integration with services
class TaskService {
    TaskHub& hub_;
    
public:
    Task create(const CreateTaskRequest& req) {
        auto task = /* create task */;
        
        // Notify all clients
        co_await hub_.broadcast({
            {"type", "task_created"},
            {"data", task.to_json()}
        });
        
        co_return task;
    }
};
```

## Testing Guidelines

### Unit Tests

- Test services and models in isolation
- Mock external dependencies
- Fast execution (< 1 second per test)

```cpp
TEST_CASE("TaskService creates task with valid data") {
    MockRepository repo;
    TaskService service(repo);
    
    auto task = service.create({.title = "Test"});
    
    CHECK(task.title == "Test");
    CHECK(task.status == TaskStatus::Pending);
}
```

### Integration Tests

- Test full HTTP request/response cycle
- Use real server instance on random port
- Test authentication flows

```cpp
TEST_CASE("POST /api/tasks requires authentication") {
    TestServer server;
    
    auto response = server.post("/api/tasks", {{"title", "Test"}});
    
    CHECK(response.status == 401);
}
```

## Performance Considerations

1. **Use HTTP/2** - Multiplexed connections reduce latency
2. **Enable compression** - `app.use(compression())`
3. **Template caching** - `app.set_template_caching(true)`
4. **Connection pooling** - Reuse database connections
5. **Static file caching** - Set appropriate Cache-Control headers

## Security Checklist

- [ ] Use HTTPS in production (TLS)
- [ ] Set secure session cookies (HttpOnly, Secure, SameSite)
- [ ] Validate all input data
- [ ] Use parameterized queries (prevent SQL injection)
- [ ] Implement rate limiting
- [ ] Set security headers (CSP, X-Frame-Options, etc.)
- [ ] Hash passwords with bcrypt/argon2
- [ ] Use CSRF tokens for forms

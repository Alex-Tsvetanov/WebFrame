#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <memory>

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"

namespace coroute {

// Forward declare middleware types
using Next = std::function<Task<Response>(Request&)>;
using Middleware = std::function<Task<Response>(Request&, Next)>;

// ============================================================================
// Log Levels
// ============================================================================

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5,
    Off = 6
};

std::string_view log_level_name(LogLevel level) noexcept;
LogLevel parse_log_level(std::string_view name) noexcept;

// ============================================================================
// Log Entry
// ============================================================================

struct LogEntry {
    LogLevel level;
    std::chrono::system_clock::time_point timestamp;
    std::string message;
    std::string logger_name;
    
    // Optional structured fields
    std::vector<std::pair<std::string, std::string>> fields;
    
    // Add field
    LogEntry& field(std::string key, std::string value) {
        fields.emplace_back(std::move(key), std::move(value));
        return *this;
    }
    
    template<typename T>
    LogEntry& field(std::string key, T value) {
        std::ostringstream oss;
        oss << value;
        return field(std::move(key), oss.str());
    }
};

// ============================================================================
// Log Sink Interface
// ============================================================================

class LogSink {
public:
    virtual ~LogSink() = default;
    virtual void write(const LogEntry& entry) = 0;
    virtual void flush() {}
};

// ============================================================================
// Console Sink
// ============================================================================

class ConsoleSink : public LogSink {
    bool colored_ = true;
    std::mutex mutex_;
    
public:
    explicit ConsoleSink(bool colored = true) : colored_(colored) {}
    void write(const LogEntry& entry) override;
};

// ============================================================================
// JSON Sink (structured logging)
// ============================================================================

class JsonSink : public LogSink {
    std::ostream& out_;
    std::mutex mutex_;
    
public:
    explicit JsonSink(std::ostream& out = std::cout) : out_(out) {}
    void write(const LogEntry& entry) override;
};

// ============================================================================
// Logger
// ============================================================================

class Logger {
    std::string name_;
    LogLevel level_ = LogLevel::Info;
    std::vector<std::shared_ptr<LogSink>> sinks_;
    mutable std::mutex mutex_;
    
public:
    Logger() = default;
    explicit Logger(std::string name) : name_(std::move(name)) {}
    
    // Configuration
    Logger& set_level(LogLevel level) { level_ = level; return *this; }
    Logger& add_sink(std::shared_ptr<LogSink> sink);
    
    // Logging methods
    void log(LogLevel level, std::string message) const;
    
    void trace(std::string message) const { log(LogLevel::Trace, std::move(message)); }
    void debug(std::string message) const { log(LogLevel::Debug, std::move(message)); }
    void info(std::string message) const { log(LogLevel::Info, std::move(message)); }
    void warn(std::string message) const { log(LogLevel::Warn, std::move(message)); }
    void error(std::string message) const { log(LogLevel::Error, std::move(message)); }
    void fatal(std::string message) const { log(LogLevel::Fatal, std::move(message)); }
    
    // Structured logging
    LogEntry entry(LogLevel level, std::string message) const;
    void log(const LogEntry& entry) const;
    
    // Check if level is enabled
    bool is_enabled(LogLevel level) const { return level >= level_; }
    
    // Getters
    const std::string& name() const { return name_; }
    LogLevel level() const { return level_; }
};

// ============================================================================
// Global Logger
// ============================================================================

Logger& default_logger();
void set_default_logger(Logger logger);

// Convenience functions using default logger
inline void log_trace(std::string msg) { default_logger().trace(std::move(msg)); }
inline void log_debug(std::string msg) { default_logger().debug(std::move(msg)); }
inline void log_info(std::string msg) { default_logger().info(std::move(msg)); }
inline void log_warn(std::string msg) { default_logger().warn(std::move(msg)); }
inline void log_error(std::string msg) { default_logger().error(std::move(msg)); }
inline void log_fatal(std::string msg) { default_logger().fatal(std::move(msg)); }

// ============================================================================
// Request Logging Middleware
// ============================================================================

struct RequestLogOptions {
    LogLevel level = LogLevel::Info;
    bool log_headers = false;
    bool log_body = false;
    size_t max_body_log_size = 1024;
    std::string format = "combined";  // "combined", "common", "short", "dev"
};

// Create request logging middleware
Middleware request_logger(RequestLogOptions options = {});
Middleware request_logger(Logger& logger, RequestLogOptions options = {});

} // namespace coroute

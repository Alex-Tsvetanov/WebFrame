#include "coroute/core/logging.hpp"

#include <ctime>

namespace coroute {

// ============================================================================
// Log Level Utilities
// ============================================================================

std::string_view log_level_name(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Warn:  return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Fatal: return "FATAL";
        case LogLevel::Off:   return "OFF";
        default: return "UNKNOWN";
    }
}

LogLevel parse_log_level(std::string_view name) noexcept {
    if (name == "trace" || name == "TRACE") return LogLevel::Trace;
    if (name == "debug" || name == "DEBUG") return LogLevel::Debug;
    if (name == "info" || name == "INFO") return LogLevel::Info;
    if (name == "warn" || name == "WARN" || name == "warning" || name == "WARNING") return LogLevel::Warn;
    if (name == "error" || name == "ERROR") return LogLevel::Error;
    if (name == "fatal" || name == "FATAL") return LogLevel::Fatal;
    if (name == "off" || name == "OFF") return LogLevel::Off;
    return LogLevel::Info;
}

// ============================================================================
// Console Sink
// ============================================================================

namespace {

const char* level_color(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "\033[90m";      // Gray
        case LogLevel::Debug: return "\033[36m";      // Cyan
        case LogLevel::Info:  return "\033[32m";      // Green
        case LogLevel::Warn:  return "\033[33m";      // Yellow
        case LogLevel::Error: return "\033[31m";      // Red
        case LogLevel::Fatal: return "\033[35m";      // Magenta
        default: return "\033[0m";
    }
}

std::string format_timestamp(std::chrono::system_clock::time_point tp) {
    auto time_t_val = std::chrono::system_clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t_val), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

} // anonymous namespace

void ConsoleSink::write(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ostringstream oss;
    
    // Timestamp
    oss << format_timestamp(entry.timestamp) << " ";
    
    // Level (with color if enabled)
    if (colored_) {
        oss << level_color(entry.level);
    }
    oss << "[" << log_level_name(entry.level) << "]";
    if (colored_) {
        oss << "\033[0m";
    }
    
    // Logger name
    if (!entry.logger_name.empty()) {
        oss << " [" << entry.logger_name << "]";
    }
    
    // Message
    oss << " " << entry.message;
    
    // Fields
    for (const auto& [key, value] : entry.fields) {
        oss << " " << key << "=" << value;
    }
    
    oss << "\n";
    
    std::cerr << oss.str();
}

// ============================================================================
// JSON Sink
// ============================================================================

void JsonSink::write(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    out_ << "{";
    out_ << "\"timestamp\":\"" << format_timestamp(entry.timestamp) << "\"";
    out_ << ",\"level\":\"" << log_level_name(entry.level) << "\"";
    
    if (!entry.logger_name.empty()) {
        out_ << ",\"logger\":\"" << entry.logger_name << "\"";
    }
    
    // Escape message for JSON
    out_ << ",\"message\":\"";
    for (char c : entry.message) {
        switch (c) {
            case '"': out_ << "\\\""; break;
            case '\\': out_ << "\\\\"; break;
            case '\n': out_ << "\\n"; break;
            case '\r': out_ << "\\r"; break;
            case '\t': out_ << "\\t"; break;
            default: out_ << c;
        }
    }
    out_ << "\"";
    
    // Fields
    for (const auto& [key, value] : entry.fields) {
        out_ << ",\"" << key << "\":\"" << value << "\"";
    }
    
    out_ << "}\n";
}

// ============================================================================
// Logger Implementation
// ============================================================================

Logger& Logger::add_sink(std::shared_ptr<LogSink> sink) {
    std::lock_guard<std::mutex> lock(mutex_);
    sinks_.push_back(std::move(sink));
    return *this;
}

void Logger::log(LogLevel level, std::string message) const {
    if (level < level_) return;
    
    LogEntry entry;
    entry.level = level;
    entry.timestamp = std::chrono::system_clock::now();
    entry.message = std::move(message);
    entry.logger_name = name_;
    
    log(entry);
}

LogEntry Logger::entry(LogLevel level, std::string message) const {
    LogEntry e;
    e.level = level;
    e.timestamp = std::chrono::system_clock::now();
    e.message = std::move(message);
    e.logger_name = name_;
    return e;
}

void Logger::log(const LogEntry& entry) const {
    if (entry.level < level_) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& sink : sinks_) {
        sink->write(entry);
    }
}

// ============================================================================
// Global Logger
// ============================================================================

namespace {

Logger& get_default_logger() {
    static Logger logger("coroute");
    static bool initialized = false;
    if (!initialized) {
        logger.add_sink(std::make_shared<ConsoleSink>());
        initialized = true;
    }
    return logger;
}

} // anonymous namespace

Logger& default_logger() {
    return get_default_logger();
}

void set_default_logger(Logger logger) {
    // Can't move Logger due to mutex, so we just set level and sinks
    auto& def = get_default_logger();
    def.set_level(logger.level());
    // Note: sinks would need a different approach to transfer
}

// ============================================================================
// Request Logging Middleware
// ============================================================================

Middleware request_logger(RequestLogOptions options) {
    return request_logger(default_logger(), std::move(options));
}

Middleware request_logger(Logger& logger, RequestLogOptions options) {
    return [&logger, options](Request& req, Next next) -> Task<Response> {
        auto start = std::chrono::steady_clock::now();
        
        // Call next handler
        Response resp = co_await next(req);
        
        auto end = std::chrono::steady_clock::now();
        auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        // Build log entry
        auto entry = logger.entry(options.level, "");
        
        // Format based on style
        std::ostringstream msg;
        
        if (options.format == "dev") {
            msg << method_to_string(req.method()) << " " << req.path() 
                << " " << resp.status() << " " << duration_ms << "ms";
        } else if (options.format == "short") {
            msg << req.path() << " " << resp.status() << " " << duration_ms << "ms";
        } else {
            // combined/common format
            msg << method_to_string(req.method()) << " " << req.path();
            if (!req.query_string().empty()) {
                msg << "?" << req.query_string();
            }
            msg << " " << req.http_version() << " " << resp.status();
        }
        
        entry.message = msg.str();
        entry.field("method", std::string(method_to_string(req.method())));
        entry.field("path", std::string(req.path()));
        entry.field("status", resp.status());
        entry.field("duration_ms", duration_ms);
        
        if (options.log_headers) {
            for (const auto& [key, value] : req.headers()) {
                entry.field("req_" + key, value);
            }
        }
        
        logger.log(entry);
        
        co_return resp;
    };
}

} // namespace coroute

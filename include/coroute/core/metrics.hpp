#pragma once

#include <string>
#include <string_view>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <chrono>
#include <functional>
#include <memory>
#include <sstream>
#include <cmath>

#include "coroute/core/request.hpp"
#include "coroute/core/response.hpp"
#include "coroute/coro/task.hpp"

namespace coroute {

// Forward declare middleware types
using Next = std::function<Task<Response>(Request&)>;
using Middleware = std::function<Task<Response>(Request&, Next)>;

// ============================================================================
// Metric Types
// ============================================================================

// Counter - monotonically increasing value
class Counter {
    std::atomic<uint64_t> value_{0};
    std::string name_;
    std::string help_;
    std::vector<std::pair<std::string, std::string>> labels_;
    
public:
    Counter(std::string name, std::string help = "")
        : name_(std::move(name)), help_(std::move(help)) {}
    
    void inc(uint64_t delta = 1) { value_.fetch_add(delta, std::memory_order_relaxed); }
    uint64_t value() const { return value_.load(std::memory_order_relaxed); }
    void reset() { value_.store(0, std::memory_order_relaxed); }
    
    const std::string& name() const { return name_; }
    const std::string& help() const { return help_; }
    
    Counter& label(std::string key, std::string value) {
        labels_.emplace_back(std::move(key), std::move(value));
        return *this;
    }
    const auto& labels() const { return labels_; }
};

// Gauge - value that can go up or down
class Gauge {
    std::atomic<double> value_{0.0};
    std::string name_;
    std::string help_;
    
public:
    Gauge(std::string name, std::string help = "")
        : name_(std::move(name)), help_(std::move(help)) {}
    
    void set(double value) { value_.store(value, std::memory_order_relaxed); }
    void inc(double delta = 1.0) { 
        double old = value_.load(std::memory_order_relaxed);
        while (!value_.compare_exchange_weak(old, old + delta, std::memory_order_relaxed));
    }
    void dec(double delta = 1.0) { inc(-delta); }
    double value() const { return value_.load(std::memory_order_relaxed); }
    
    const std::string& name() const { return name_; }
    const std::string& help() const { return help_; }
};

// Histogram - distribution of values
class Histogram {
    std::string name_;
    std::string help_;
    std::vector<double> buckets_;
    std::unique_ptr<std::atomic<uint64_t>[]> bucket_counts_;
    size_t bucket_count_size_{0};
    std::atomic<uint64_t> count_{0};
    std::atomic<double> sum_{0.0};
    mutable std::mutex mutex_;
    
public:
    Histogram(std::string name, std::vector<double> buckets, std::string help = "");
    
    void observe(double value);
    
    uint64_t count() const { return count_.load(std::memory_order_relaxed); }
    double sum() const { return sum_.load(std::memory_order_relaxed); }
    const std::vector<double>& buckets() const { return buckets_; }
    uint64_t bucket_count(size_t idx) const;
    
    const std::string& name() const { return name_; }
    const std::string& help() const { return help_; }
    
    // Default buckets for latency (in seconds)
    static std::vector<double> default_latency_buckets();
};

// ============================================================================
// Metrics Registry
// ============================================================================

class MetricsRegistry {
    std::unordered_map<std::string, std::shared_ptr<Counter>> counters_;
    std::unordered_map<std::string, std::shared_ptr<Gauge>> gauges_;
    std::unordered_map<std::string, std::shared_ptr<Histogram>> histograms_;
    mutable std::mutex mutex_;
    
public:
    // Get or create metrics
    std::shared_ptr<Counter> counter(const std::string& name, const std::string& help = "");
    std::shared_ptr<Gauge> gauge(const std::string& name, const std::string& help = "");
    std::shared_ptr<Histogram> histogram(const std::string& name, 
                                          const std::vector<double>& buckets,
                                          const std::string& help = "");
    
    // Export in Prometheus format
    std::string prometheus_export() const;
    
    // Clear all metrics
    void clear();
};

// ============================================================================
// Global Registry
// ============================================================================

MetricsRegistry& default_metrics();

// ============================================================================
// HTTP Metrics
// ============================================================================

struct HttpMetrics {
    std::shared_ptr<Counter> requests_total;
    std::shared_ptr<Counter> requests_by_status;
    std::shared_ptr<Histogram> request_duration;
    std::shared_ptr<Gauge> requests_in_flight;
    
    HttpMetrics();
    explicit HttpMetrics(MetricsRegistry& registry);
};

// ============================================================================
// Metrics Middleware
// ============================================================================

struct MetricsOptions {
    std::string path = "/metrics";  // Path to expose metrics
    bool expose_endpoint = true;    // Auto-add /metrics endpoint
};

// Create metrics middleware
Middleware metrics_middleware(MetricsOptions options = {});
Middleware metrics_middleware(MetricsRegistry& registry, MetricsOptions options = {});

// ============================================================================
// Convenience
// ============================================================================

// Timer helper for measuring duration
class ScopedTimer {
    Histogram& histogram_;
    std::chrono::steady_clock::time_point start_;
    
public:
    explicit ScopedTimer(Histogram& h) 
        : histogram_(h), start_(std::chrono::steady_clock::now()) {}
    
    ~ScopedTimer() {
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double>(end - start_).count();
        histogram_.observe(duration);
    }
};

} // namespace coroute

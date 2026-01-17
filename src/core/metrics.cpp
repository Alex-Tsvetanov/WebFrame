#include "coroute/core/metrics.hpp"

#include <algorithm>
#include <iomanip>

namespace coroute {

// ============================================================================
// Histogram Implementation
// ============================================================================

Histogram::Histogram(std::string name, std::vector<double> buckets, std::string help)
    : name_(std::move(name))
    , help_(std::move(help))
    , buckets_(std::move(buckets))
{
    // Sort buckets
    std::sort(buckets_.begin(), buckets_.end());
    
    // Add +Inf bucket
    buckets_.push_back(std::numeric_limits<double>::infinity());
    
    // Initialize bucket counts
    bucket_count_size_ = buckets_.size();
    bucket_counts_ = std::make_unique<std::atomic<uint64_t>[]>(bucket_count_size_);
    for (size_t i = 0; i < bucket_count_size_; ++i) {
        bucket_counts_[i].store(0, std::memory_order_relaxed);
    }
}

void Histogram::observe(double value) {
    // Increment count and sum
    count_.fetch_add(1, std::memory_order_relaxed);
    
    double old_sum = sum_.load(std::memory_order_relaxed);
    while (!sum_.compare_exchange_weak(old_sum, old_sum + value, std::memory_order_relaxed));
    
    // Find bucket and increment
    for (size_t i = 0; i < buckets_.size(); ++i) {
        if (value <= buckets_[i]) {
            bucket_counts_[i].fetch_add(1, std::memory_order_relaxed);
            break;
        }
    }
}

uint64_t Histogram::bucket_count(size_t idx) const {
    if (idx >= bucket_count_size_) return 0;
    return bucket_counts_[idx].load(std::memory_order_relaxed);
}

std::vector<double> Histogram::default_latency_buckets() {
    return {0.001, 0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0};
}

// ============================================================================
// MetricsRegistry Implementation
// ============================================================================

std::shared_ptr<Counter> MetricsRegistry::counter(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = counters_.find(name);
    if (it != counters_.end()) {
        return it->second;
    }
    auto counter = std::make_shared<Counter>(name, help);
    counters_[name] = counter;
    return counter;
}

std::shared_ptr<Gauge> MetricsRegistry::gauge(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = gauges_.find(name);
    if (it != gauges_.end()) {
        return it->second;
    }
    auto gauge = std::make_shared<Gauge>(name, help);
    gauges_[name] = gauge;
    return gauge;
}

std::shared_ptr<Histogram> MetricsRegistry::histogram(const std::string& name,
                                                       const std::vector<double>& buckets,
                                                       const std::string& help) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = histograms_.find(name);
    if (it != histograms_.end()) {
        return it->second;
    }
    auto histogram = std::make_shared<Histogram>(name, buckets, help);
    histograms_[name] = histogram;
    return histogram;
}

std::string MetricsRegistry::prometheus_export() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    
    // Export counters
    for (const auto& [name, counter] : counters_) {
        if (!counter->help().empty()) {
            out << "# HELP " << name << " " << counter->help() << "\n";
        }
        out << "# TYPE " << name << " counter\n";
        out << name;
        if (!counter->labels().empty()) {
            out << "{";
            bool first = true;
            for (const auto& [k, v] : counter->labels()) {
                if (!first) out << ",";
                out << k << "=\"" << v << "\"";
                first = false;
            }
            out << "}";
        }
        out << " " << counter->value() << "\n";
    }
    
    // Export gauges
    for (const auto& [name, gauge] : gauges_) {
        if (!gauge->help().empty()) {
            out << "# HELP " << name << " " << gauge->help() << "\n";
        }
        out << "# TYPE " << name << " gauge\n";
        out << name << " " << std::fixed << std::setprecision(6) << gauge->value() << "\n";
    }
    
    // Export histograms
    for (const auto& [name, hist] : histograms_) {
        if (!hist->help().empty()) {
            out << "# HELP " << name << " " << hist->help() << "\n";
        }
        out << "# TYPE " << name << " histogram\n";
        
        uint64_t cumulative = 0;
        const auto& buckets = hist->buckets();
        for (size_t i = 0; i < buckets.size(); ++i) {
            cumulative += hist->bucket_count(i);
            out << name << "_bucket{le=\"";
            if (std::isinf(buckets[i])) {
                out << "+Inf";
            } else {
                out << std::fixed << std::setprecision(3) << buckets[i];
            }
            out << "\"} " << cumulative << "\n";
        }
        out << name << "_sum " << std::fixed << std::setprecision(6) << hist->sum() << "\n";
        out << name << "_count " << hist->count() << "\n";
    }
    
    return out.str();
}

void MetricsRegistry::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.clear();
    gauges_.clear();
    histograms_.clear();
}

// ============================================================================
// Global Registry
// ============================================================================

MetricsRegistry& default_metrics() {
    static MetricsRegistry registry;
    return registry;
}

// ============================================================================
// HTTP Metrics
// ============================================================================

HttpMetrics::HttpMetrics() : HttpMetrics(default_metrics()) {}

HttpMetrics::HttpMetrics(MetricsRegistry& registry) {
    requests_total = registry.counter("http_requests_total", "Total HTTP requests");
    requests_by_status = registry.counter("http_requests_by_status", "HTTP requests by status code");
    request_duration = registry.histogram("http_request_duration_seconds",
                                           Histogram::default_latency_buckets(),
                                           "HTTP request duration in seconds");
    requests_in_flight = registry.gauge("http_requests_in_flight", "Current in-flight requests");
}

// ============================================================================
// Metrics Middleware
// ============================================================================

Middleware metrics_middleware(MetricsOptions options) {
    return metrics_middleware(default_metrics(), std::move(options));
}

Middleware metrics_middleware(MetricsRegistry& registry, MetricsOptions options) {
    auto http_metrics = std::make_shared<HttpMetrics>(registry);
    auto registry_ptr = &registry;
    
    return [http_metrics, registry_ptr, options](Request& req, Next next) -> Task<Response> {
        // Check if this is a metrics endpoint request
        if (options.expose_endpoint && req.path() == options.path) {
            std::string body = registry_ptr->prometheus_export();
            Response resp = Response::ok(std::move(body), "text/plain; version=0.0.4");
            co_return resp;
        }
        
        // Track request
        http_metrics->requests_total->inc();
        http_metrics->requests_in_flight->inc();
        
        auto start = std::chrono::steady_clock::now();
        
        // Call next handler
        Response resp = co_await next(req);
        
        // Record duration
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double>(end - start).count();
        http_metrics->request_duration->observe(duration);
        
        // Track by status (simplified - would need labeled counter for proper impl)
        http_metrics->requests_in_flight->dec();
        
        co_return resp;
    };
}

} // namespace coroute

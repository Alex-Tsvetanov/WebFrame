//
//  Metrics.cpp
//  valdi-pc
//
//  Created by Simon Corsin on 4/20/20.
//

#include "valdi/runtime/Metrics/Metrics.hpp"

namespace Valdi {

constexpr auto kSlowAsyncJsCallThreshold =
    snap::utils::time::Duration<std::chrono::steady_clock>(std::chrono::milliseconds(50));
constexpr auto kSlowSyncJsCallThreshold =
    snap::utils::time::Duration<std::chrono::steady_clock>(std::chrono::milliseconds(25));

static ScopedMetrics makeScopedMetrics(const Ref<Metrics>& metrics,
                                       ScopedMetricsCompletion&& completion,
                                       snap::utils::time::Duration<std::chrono::steady_clock> threshold = {}) {
    if (metrics == nullptr) {
        return ScopedMetrics();
    }

    return ScopedMetrics(std::move(completion), threshold);
}

ScopedMetrics Metrics::scopedOnScrollLatency(const Ref<Metrics>& metrics,
                                             const StringBox& module,
                                             const StringBox& backend) {
    return makeScopedMetrics(
        metrics, [metrics, module, backend](const snap::utils::time::Duration<std::chrono::steady_clock>& elapsed) {
            metrics->emitOnScrollLatency(module, backend, elapsed);
        });
}

ScopedMetrics Metrics::scopedOnCreateLatency(const Ref<Metrics>& metrics, const StringBox& module) {
    return makeScopedMetrics(metrics,
                             [metrics, module](const snap::utils::time::Duration<std::chrono::steady_clock>& elapsed) {
                                 metrics->emitOnCreateLatency(module, elapsed);
                             });
}

ScopedMetrics Metrics::scopedOnDestroyLatency(const Ref<Metrics>& metrics, const StringBox& module) {
    return makeScopedMetrics(metrics,
                             [metrics, module](const snap::utils::time::Duration<std::chrono::steady_clock>& elapsed) {
                                 metrics->emitOnDestroyLatency(module, elapsed);
                             });
}

ScopedMetrics Metrics::scopedOnViewModelUpdatedLatency(const Ref<Metrics>& metrics, const StringBox& module) {
    return makeScopedMetrics(metrics,
                             [metrics, module](const snap::utils::time::Duration<std::chrono::steady_clock>& elapsed) {
                                 metrics->emitOnViewModelUpdatedLatency(module, elapsed);
                             });
}

ScopedMetrics Metrics::scopedCalculateLayoutLatency(const Ref<Metrics>& metrics,
                                                    const StringBox& module,
                                                    const StringBox& backend) {
    return makeScopedMetrics(
        metrics, [metrics, module, backend](const snap::utils::time::Duration<std::chrono::steady_clock>& elapsed) {
            metrics->emitCalculateLayoutLatency(module, backend, elapsed);
        });
}

ScopedMetrics Metrics::scopedCalculateLazyLayoutLatency(const Ref<Metrics>& metrics,
                                                        const StringBox& module,
                                                        const StringBox& backend) {
    return makeScopedMetrics(
        metrics, [metrics, module, backend](const snap::utils::time::Duration<std::chrono::steady_clock>& elapsed) {
            metrics->emitCalculateLazyLayoutLatency(module, backend, elapsed);
        });
}

ScopedMetrics Metrics::scopedDestroyContextLatency(const Ref<Metrics>& metrics, const StringBox& module) {
    return makeScopedMetrics(metrics,
                             [metrics, module](const snap::utils::time::Duration<std::chrono::steady_clock>& elapsed) {
                                 metrics->emitDestroyContextLatency(module, elapsed);
                             });
}

ScopedMetrics Metrics::thresholdedScopedSlowSyncJsCall(const Ref<Metrics>& metrics, const StringBox& module) {
    return makeScopedMetrics(
        metrics,
        [metrics, module](const snap::utils::time::Duration<std::chrono::steady_clock>& elapsed) {
            metrics->emitSlowSyncJsCallThreshold(module, elapsed);
        },
        kSlowSyncJsCallThreshold);
}

ScopedMetrics Metrics::thresholdedScopedSlowAsyncJsCall(const Ref<Metrics>& metrics, const StringBox& module) {
    return makeScopedMetrics(
        metrics,
        [metrics, module](const snap::utils::time::Duration<std::chrono::steady_clock>& elapsed) {
            metrics->emitSlowAsyncJsCall(module, elapsed);
        },
        kSlowAsyncJsCallThreshold);
}

MetricsStopWatch::MetricsStopWatch() {
    _sw.start();
}

MetricsDuration MetricsStopWatch::elapsed() const {
    return _sw.elapsed();
}

ScopedMetrics::ScopedMetrics(ScopedMetricsCompletion&& completionFunction,
                             snap::utils::time::Duration<std::chrono::steady_clock> threshold)
    : _completionFunction(std::move(completionFunction)), _threshold(threshold) {}

ScopedMetrics::ScopedMetrics() = default;

ScopedMetrics::~ScopedMetrics() {
    if (!_completionFunction) {
        return;
    }

    auto elapsed = _sw.elapsed();
    if (elapsed > _threshold) {
        _completionFunction(elapsed);
    }
}

MetricsDuration ScopedMetrics::elapsed() const {
    return _sw.elapsed();
}

} // namespace Valdi

//
//  Trace.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 6/25/19.
//

#pragma once

#include "utils/debugging/Trace.hpp"
#include "utils/time/StopWatch.hpp"
#include "valdi_core/cpp/Threading/ThreadBase.hpp"
#include "valdi_core/cpp/Utils/Defer.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace Valdi {

class StringBox;

std::string getTraceName(std::string_view prefix, std::string_view suffix);
std::string getTraceName(std::string_view prefix, const StringBox& suffix);

using TraceDuration = snap::utils::time::Duration<std::chrono::steady_clock>;
using TraceTimePoint = std::chrono::steady_clock::time_point;

struct RecordedTrace {
    inline RecordedTrace(std::string&& trace,
                         const TraceTimePoint& start,
                         const TraceTimePoint& end,
                         const ThreadId& threadId,
                         size_t recordingSequence)
        : trace(std::move(trace)), start(start), end(end), threadId(threadId), recordingSequence(recordingSequence) {}

    std::string trace;
    TraceTimePoint start;
    TraceTimePoint end;
    ThreadId threadId;
    size_t recordingSequence;

    TraceDuration duration() const;
};

class Tracer {
public:
    Tracer();
    ~Tracer();

    inline bool isRecording() const {
        return _recording;
    }

    size_t startRecording();
    std::vector<RecordedTrace> stopRecording(size_t recordingIdentifier);

    void append(std::string&& trace, const TraceTimePoint& start, const TraceTimePoint& end);

    static Tracer& shared();

private:
    std::mutex _mutex;
    std::atomic_bool _recording = false;
    size_t _recordingSequence = 0;
    std::vector<RecordedTrace> _pendingTraces;
    std::vector<size_t> _recorders;
};

class ScopedTrace {
public:
    explicit ScopedTrace(const StringBox& trace);
    explicit ScopedTrace(std::string&& trace);
    ~ScopedTrace();

protected:
    std::string _trace;
    std::optional<TraceTimePoint> _startTime;
    snap::utils::debugging::ScopedTrace _snapTrace;
    snap::profiling::OsTraceEmitter _osEmitter;

private:
    void begin();
    void end();
};

} // namespace Valdi

#if !SC_TRACING_COMPILED_IN()

#define VALDI_TRACE(name)
#define VALDI_TRACE_META(name, meta)

#else

#define VALDI_TRACE(name) Valdi::ScopedTrace ___scopedTrace(name);
#define VALDI_TRACE_META(name, meta)                                                                                   \
    Valdi::ScopedTrace ___scopedTrace(Valdi::kTracingEnabled ? Valdi::getTraceName(name, meta) : "");

#endif

namespace Valdi {
constexpr auto kTracingEnabled = snap::kTracingEnabled;
}

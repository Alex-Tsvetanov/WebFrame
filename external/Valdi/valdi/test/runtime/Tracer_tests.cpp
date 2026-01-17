#include "valdi_core/cpp/Utils/Trace.hpp"
#include <gtest/gtest.h>

using namespace Valdi;

namespace ValdiTest {

TraceTimePoint appendMs(const TraceTimePoint& start, int ms) {
    return start + std::chrono::milliseconds(ms);
}

TEST(Tracer, canRecordOperations) {
    Tracer tracer;
    auto start = std::chrono::steady_clock::now();

    auto id = tracer.startRecording();

    tracer.append("hello", start, appendMs(start, 50));
    tracer.append("world", start, appendMs(start, 100));

    auto result = tracer.stopRecording(id);

    ASSERT_EQ(static_cast<size_t>(2), result.size());
    ASSERT_EQ("hello", result[0].trace);
    ASSERT_EQ("world", result[1].trace);
    ASSERT_EQ(50.0, result[0].duration().milliseconds());
    ASSERT_EQ(100.0, result[1].duration().milliseconds());
}

TEST(Tracer, returnsEmptyOnInvalidRecordId) {
    Tracer tracer;
    auto start = std::chrono::steady_clock::now();

    auto id = tracer.startRecording();

    tracer.append("hello", start, appendMs(start, 50));
    tracer.append("world", start, appendMs(start, 100));

    auto result = tracer.stopRecording(id + 1);
    ASSERT_TRUE(result.empty());
}

TEST(Tracer, canRecordOperationsConcurrentlyWithSequentialOrdering) {
    Tracer tracer;
    auto start = std::chrono::steady_clock::now();

    auto id = tracer.startRecording();

    tracer.append("hello", start, appendMs(start, 50));

    auto id2 = tracer.startRecording();

    tracer.append("world", start, appendMs(start, 100));

    auto result = tracer.stopRecording(id);
    auto result2 = tracer.stopRecording(id2);

    // Result1 should have both traces
    ASSERT_EQ(static_cast<size_t>(2), result.size());
    ASSERT_EQ("hello", result[0].trace);
    ASSERT_EQ("world", result[1].trace);
    ASSERT_EQ(50.0, result[0].duration().milliseconds());
    ASSERT_EQ(100.0, result[1].duration().milliseconds());

    // Result2 should have only 1
    ASSERT_EQ(static_cast<size_t>(1), result2.size());
    ASSERT_EQ("world", result2[0].trace);
    ASSERT_EQ(100.0, result2[0].duration().milliseconds());
}

TEST(Tracer, canRecordOperationsConcurrentlyWithRandomOrdering) {
    Tracer tracer;
    auto start = std::chrono::steady_clock::now();

    auto id = tracer.startRecording();

    tracer.append("hello", start, appendMs(start, 50));

    auto id2 = tracer.startRecording();

    tracer.append("world", start, appendMs(start, 100));

    auto result2 = tracer.stopRecording(id2);
    auto result = tracer.stopRecording(id);

    // Result1 should have both traces
    ASSERT_EQ(static_cast<size_t>(2), result.size());
    ASSERT_EQ("hello", result[0].trace);
    ASSERT_EQ("world", result[1].trace);
    ASSERT_EQ(50.0, result[0].duration().milliseconds());
    ASSERT_EQ(100.0, result[1].duration().milliseconds());

    // Result2 should have only 1
    ASSERT_EQ(static_cast<size_t>(1), result2.size());
    ASSERT_EQ("world", result2[0].trace);
    ASSERT_EQ(100.0, result2[0].duration().milliseconds());
}

TEST(Tracer, onlyReturnsRecordingWhenLastRecorderEnd) {
    Tracer tracer;

    ASSERT_FALSE(tracer.isRecording());

    auto id1 = tracer.startRecording();

    ASSERT_TRUE(tracer.isRecording());

    auto id2 = tracer.startRecording();

    ASSERT_TRUE(tracer.isRecording());

    tracer.stopRecording(id2);

    ASSERT_TRUE(tracer.isRecording());

    tracer.stopRecording(id1);

    ASSERT_FALSE(tracer.isRecording());
}

} // namespace ValdiTest

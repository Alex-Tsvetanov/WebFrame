#include "valdi/runtime/JavaScript/JavaScriptHeapDumpBuilder.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"
#include "valdi_core/cpp/Utils/StringCache.hpp"
#include <benchmark/benchmark.h>

using namespace Valdi;

struct BenchmarkHelper {
    std::vector<StringBox> names;
    std::vector<StringBox> propertyNames;

    BenchmarkHelper() {
        for (size_t i = 0; i < 20; i++) {
            names.emplace_back(STRING_FORMAT("Object{}", i));
        }
        for (size_t i = 0; i < 1000; i++) {
            propertyNames.emplace_back(STRING_FORMAT("prop{}", i));
        }
    }

    void populateHeapDumpBuilder(JavaScriptHeapDumpBuilder& builder) {
        constexpr size_t nodesCount = 100000;
        constexpr size_t edgesPerNodeCount = 5;

        for (size_t i = 0; i < nodesCount; i++) {
            auto nodeType =
                static_cast<JavaScriptHeapDumpNodeType>(i % static_cast<size_t>(JavaScriptHeapDumpNodeType::COUNT));
            builder.beginNode(nodeType, names[i % names.size()], static_cast<uint64_t>(i), 42);
            for (size_t j = i + 1; j < nodesCount && j - i < edgesPerNodeCount; j++) {
                builder.appendEdge(JavaScriptHeapDumpEdgeType::PROPERTY,
                                   JavaScriptHeapEdgeIdentifier::named(propertyNames[j % propertyNames.size()]),
                                   static_cast<uint64_t>(j));
            }
        }
    }
};

static void BuildHeapDump(benchmark::State& state) {
    BenchmarkHelper helper;

    for (auto _ : state) {
        JavaScriptHeapDumpBuilder builder;
        helper.populateHeapDumpBuilder(builder);

        benchmark::DoNotOptimize(builder.build());
    }
}
BENCHMARK(BuildHeapDump);

static void ParseHeapDump(benchmark::State& state) {
    BenchmarkHelper helper;
    JavaScriptHeapDumpBuilder builder;
    helper.populateHeapDumpBuilder(builder);
    auto heapDump = builder.build();

    for (auto _ : state) {
        SimpleExceptionTracker exceptionTracker;
        JavaScriptHeapDumpParser parser(heapDump.data(), heapDump.size(), exceptionTracker);

        for (const auto& node : parser.getNodes()) {
            for (const auto& edge : node.getEdges()) {
                benchmark::DoNotOptimize(edge.getIdentifier());
            }
        }
    }
}
BENCHMARK(ParseHeapDump);

static void MergeHeapDump(benchmark::State& state) {
    BenchmarkHelper helper;
    JavaScriptHeapDumpBuilder builder;
    helper.populateHeapDumpBuilder(builder);
    auto heapDump = builder.build();

    for (auto _ : state) {
        JavaScriptHeapDumpBuilder builder;
        SimpleExceptionTracker exceptionTracker;
        builder.appendDump(heapDump.data(), heapDump.size(), exceptionTracker);
        builder.appendDump(heapDump.data(), heapDump.size(), exceptionTracker);
        builder.appendDump(heapDump.data(), heapDump.size(), exceptionTracker);
        benchmark::DoNotOptimize(builder.build());
    }
}
BENCHMARK(MergeHeapDump);

BENCHMARK_MAIN();

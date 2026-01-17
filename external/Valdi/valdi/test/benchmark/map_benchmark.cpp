#include "benchmark_utils.hpp"
#include <benchmark/benchmark.h>

#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include <unordered_map>

using namespace Valdi;

static void CreateUnorderedMap(benchmark::State& state) {
    StringCache stringCache;
    auto cachedStrings = makeRandomInternedStrings(stringCache, state.range(0), 10);

    for (auto _ : state) {
        std::unordered_map<StringBox, bool> map;

        for (const auto& str : cachedStrings) {
            map[str] = true;
        }
    }
}
BENCHMARK(CreateUnorderedMap)->DenseRange(8, 128, 8);

static void CreateFlatMap(benchmark::State& state) {
    StringCache stringCache;
    auto cachedStrings = makeRandomInternedStrings(stringCache, state.range(0), 10);

    for (auto _ : state) {
        Valdi::FlatMap<StringBox, bool> map;

        for (const auto& str : cachedStrings) {
            map[str] = true;
        }
    }
}
BENCHMARK(CreateFlatMap)->DenseRange(8, 128, 8);

static void CreateUnorderedMapWithReserve(benchmark::State& state) {
    StringCache stringCache;
    auto cachedStrings = makeRandomInternedStrings(stringCache, state.range(0), 10);

    for (auto _ : state) {
        std::unordered_map<StringBox, bool> map;
        map.reserve(cachedStrings.size());

        for (const auto& str : cachedStrings) {
            map[str] = true;
        }
    }
}
BENCHMARK(CreateUnorderedMapWithReserve)->DenseRange(8, 128, 8);

static void CreateFlatMapWithReserve(benchmark::State& state) {
    StringCache stringCache;
    auto cachedStrings = makeRandomInternedStrings(stringCache, state.range(0), 10);

    for (auto _ : state) {
        Valdi::FlatMap<StringBox, bool> map;
        map.reserve(cachedStrings.size());

        for (const auto& str : cachedStrings) {
            map[str] = true;
        }
    }
}
BENCHMARK(CreateFlatMapWithReserve)->DenseRange(8, 128, 8);

static void QueryUnorderedMap(benchmark::State& state) {
    StringCache stringCache;
    auto cachedStrings = makeRandomInternedStrings(stringCache, state.range(0), 10);

    std::unordered_map<StringBox, bool> map;

    for (const auto& str : cachedStrings) {
        map[str] = true;
    }

    for (auto _ : state) {
        for (const auto& str : cachedStrings) {
            const auto& it = map.find(str);

            benchmark::DoNotOptimize(it != map.end());
        }
    }
}
BENCHMARK(QueryUnorderedMap)->DenseRange(8, 128, 8);

static void QueryFlatMap(benchmark::State& state) {
    StringCache stringCache;
    auto cachedStrings = makeRandomInternedStrings(stringCache, state.range(0), 10);

    Valdi::FlatMap<StringBox, bool> map;

    for (const auto& str : cachedStrings) {
        map[str] = true;
    }

    for (auto _ : state) {
        for (const auto& str : cachedStrings) {
            const auto& it = map.find(str);

            benchmark::DoNotOptimize(it != map.end());
        }
    }
}
BENCHMARK(QueryFlatMap)->DenseRange(8, 128, 8);

static void IterateUnorderedMap(benchmark::State& state) {
    StringCache stringCache;
    auto cachedStrings = makeRandomInternedStrings(stringCache, state.range(0), 10);

    std::unordered_map<StringBox, bool> map;

    for (const auto& str : cachedStrings) {
        map[str] = true;
    }

    for (auto _ : state) {
        for ([[maybe_unused]] const auto& it : map) {
            benchmark::DoNotOptimize(it);
        }
    }
}
BENCHMARK(IterateUnorderedMap)->DenseRange(8, 128, 8);

static void IterateFlatMap(benchmark::State& state) {
    StringCache stringCache;
    auto cachedStrings = makeRandomInternedStrings(stringCache, state.range(0), 10);

    Valdi::FlatMap<StringBox, bool> map;

    for (const auto& str : cachedStrings) {
        map[str] = true;
    }

    for (auto _ : state) {
        for ([[maybe_unused]] const auto& it : map) {
            benchmark::DoNotOptimize(it);
        }
    }
}
BENCHMARK(IterateFlatMap)->DenseRange(8, 128, 8);

BENCHMARK_MAIN();

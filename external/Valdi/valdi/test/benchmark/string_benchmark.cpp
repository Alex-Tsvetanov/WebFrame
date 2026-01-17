#include "benchmark/utils/benchmark_utils.hpp"
#include <benchmark/benchmark.h>

using namespace Valdi;

static void StringCacheMakeStringCold(benchmark::State& state) {
    auto strings = makeRandomStrings(state.range(0));

    for (auto _ : state) {
        state.PauseTiming();
        auto& stringCache = StringCache::getGlobal();
        state.ResumeTiming();

        for (const auto& str : strings) {
            benchmark::DoNotOptimize(stringCache.makeString(str));
        }
    }
}
BENCHMARK(StringCacheMakeStringCold)->Range(8, 512);

static void StringCacheMakeStringWarm(benchmark::State& state) {
    auto strings = makeRandomStrings(state.range(0));
    auto& stringCache = StringCache::getGlobal();

    auto cachedStrings = internStrings(stringCache, strings);

    for (auto _ : state) {
        for (const auto& str : strings) {
            benchmark::DoNotOptimize(stringCache.makeString(str));
        }
    }
}
BENCHMARK(StringCacheMakeStringWarm)->Range(8, 512);

// How long does it take to compare regular std::strings
// Used as base
static void RegularStringComparison(benchmark::State& state) {
    auto strings = makeRandomStrings(state.range(0));

    for (auto _ : state) {
        for (const auto& str : strings) {
            for (const auto& other : strings) {
                benchmark::DoNotOptimize(str == other);
            }
        }
    }
}
BENCHMARK(RegularStringComparison)->Range(8, 512);

// How long does it take to compare StringBox objects
static void InternedStringComparison(benchmark::State& state) {
    auto strings = makeRandomStrings(state.range(0));
    auto& stringCache = StringCache::getGlobal();

    auto cachedStrings = internStrings(stringCache, strings);

    for (auto _ : state) {
        for (const auto& str : cachedStrings) {
            for (const auto& other : cachedStrings) {
                benchmark::DoNotOptimize(str == other);
            }
        }
    }
}
BENCHMARK(InternedStringComparison)->Range(8, 512);

static void CreateMapWithRegularStrings(benchmark::State& state) {
    auto strings = makeRandomStrings(state.range(0));
    for (auto _ : state) {
        FlatMap<std::string, bool> map;
        map.reserve(strings.size());

        for (const auto& str : strings) {
            map[str] = true;
        }
    }
}
BENCHMARK(CreateMapWithRegularStrings)->Range(8, 512);

static void CreateMapWithInternedStrings(benchmark::State& state) {
    auto strings = makeRandomStrings(state.range(0));
    auto& stringCache = StringCache::getGlobal();

    auto cachedStrings = internStrings(stringCache, strings);

    for (auto _ : state) {
        FlatMap<StringBox, bool> map;
        map.reserve(cachedStrings.size());

        for (const auto& str : cachedStrings) {
            map[str] = true;
        }
    }
}
BENCHMARK(CreateMapWithInternedStrings)->Range(8, 512);

static void QueryMapWithRegularStrings(benchmark::State& state) {
    auto strings = makeRandomStrings(state.range(0));
    FlatMap<std::string, bool> map;

    for (const auto& str : strings) {
        map[str] = true;
    }

    for (auto _ : state) {
        for (const auto& str : strings) {
            const auto& it = map.find(str);

            benchmark::DoNotOptimize(it != map.end());
        }
    }
}
BENCHMARK(QueryMapWithRegularStrings)->Range(8, 512);

static void QueryMapWithInternedStrings(benchmark::State& state) {
    auto strings = makeRandomStrings(state.range(0));
    auto& stringCache = StringCache::getGlobal();

    auto cachedStrings = internStrings(stringCache, strings);
    FlatMap<StringBox, bool> map;

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
BENCHMARK(QueryMapWithInternedStrings)->Range(8, 512);

BENCHMARK_MAIN();

#include "benchmark_utils.hpp"
#include "valdi_test_utils.hpp"
#include <benchmark/benchmark.h>

using namespace ValdiTest;

static void CreateRenderAndDestroy(benchmark::State& state) {
    RuntimeWrapper wrapper(false);
    wrapper.logger.setMinLogType(Valdi::LogTypeWarn);
    wrapper.flushQueues();

    auto context = wrapper.runtime->createContext(STRING_LITERAL("test"), STRING_LITERAL("CSSBenchmark"), Value());
    context->onCreate();

    for (auto _ : state) {
        auto tree = wrapper.runtime->createViewNodeTree(context);
        wrapper.runtime->render(tree, nullptr);
        wrapper.runtime->destroyViewNodeTree(*tree);
    }
}
BENCHMARK(CreateRenderAndDestroy);

static void Destroy(benchmark::State& state) {
    RuntimeWrapper wrapper(false);
    wrapper.logger.setMinLogType(Valdi::LogTypeWarn);
    wrapper.flushQueues();

    auto context = wrapper.runtime->createContext(STRING_LITERAL("test"), STRING_LITERAL("CSSBenchmark"), Value());
    context->onCreate();

    for (auto _ : state) {
        state.PauseTiming();
        auto tree = wrapper.runtime->createViewNodeTree(context);
        wrapper.runtime->render(tree, nullptr);
        state.ResumeTiming();
        wrapper.runtime->destroyViewNodeTree(*tree);
    }
}
BENCHMARK(Destroy);

static void Render(benchmark::State& state) {
    RuntimeWrapper wrapper(false);
    wrapper.logger.setMinLogType(Valdi::LogTypeWarn);
    wrapper.flushQueues();

    auto context = wrapper.runtime->createContext(STRING_LITERAL("test"), STRING_LITERAL("CSSBenchmark"), Value());
    context->onCreate();

    for (auto _ : state) {
        state.PauseTiming();
        auto tree = wrapper.runtime->createViewNodeTree(context);
        state.ResumeTiming();
        wrapper.runtime->render(tree, nullptr);
        state.PauseTiming();
        wrapper.runtime->destroyViewNodeTree(*tree);
        state.ResumeTiming();
    }
}
BENCHMARK(Render);

static void UpdateCSS(benchmark::State& state) {
    RuntimeWrapper wrapper(false);
    wrapper.logger.setMinLogType(Valdi::LogTypeWarn);
    wrapper.flushQueues();

    auto context = wrapper.runtime->createContext(STRING_LITERAL("test"), STRING_LITERAL("CSSBenchmark"), Value());
    context->onCreate();
    auto tree = wrapper.runtime->createViewNodeTree(context);
    wrapper.runtime->render(tree, nullptr);

    for (auto _ : state) {
        state.PauseTiming();
        tree->getRootViewNode()->resetCssSubtree();
        state.ResumeTiming();

        tree->updateCSS(nullptr);
    }
}
BENCHMARK(UpdateCSS);

BENCHMARK_MAIN();

#include "valdi/JSModule.hpp"
#include "valdi/jsbridge/JavaScriptBridge.hpp"
#include "valdi_core/cpp/Constants.hpp"
#include "valdi_core/cpp/Utils/DiskUtils.hpp"
#include "valdi_core/cpp/Utils/ExceptionTracker.hpp"
#include "valdi_test_utils.hpp"
#include <benchmark/benchmark.h>

using namespace ValdiTest;
using namespace Valdi;
using namespace snap::valdi;

struct RuntimeWrapper {
    Ref<ValdiStandaloneRuntime> runtime;
    Runtime* runtime = nullptr;
    Value benchmarkModule;

    RuntimeWrapper(Valdi::IJavaScriptBridge* jsBridge) {
        ConsoleLogger::getLogger().setMinLogType(LogTypeWarn);
        Valdi::traceLoadModules = false;
        Valdi::traceInitialization = false;

        auto resourceLoader = makeShared<Valdi::StandaloneResourceLoader>();

        auto mainQueue = makeShared<MainQueue>();
        auto diskCache = makeShared<InMemoryDiskCache>();

        runtime = Valdi::ValdiStandaloneRuntime::create(
            false, false, false, true, true, jsBridge, mainQueue, diskCache, nullptr, resourceLoader, nullptr);
        runtime = &runtime->getRuntime();
        runtime->getResourceManager().setLazyModulePreloadingEnabled(false);

        mainQueue->runOnce();

        auto benchmarkModule = runtime->getJavaScriptRuntime()->getModule(
            nullptr, STRING_LITERAL("valdi_protobuf_benchmark/test/ProtobufBench"));

        SimpleExceptionTracker exceptionTracker;
        Marshaller marshaller(exceptionTracker);
        auto objectIndex = benchmarkModule->pushToMarshaller(reinterpret_cast<int64_t>(&marshaller));
        this->benchmarkModule = marshaller.get(objectIndex);
    }

    ~RuntimeWrapper() = default;

    BytesView getProtoData(std::string_view filename) {
        auto path = resolveTestPath("testdata/pb").append(filename);
        return DiskUtils::load(path).moveValue();
    }
};

static void doBench(JavaScriptEngineType jsEngineType, std::string_view functionName, benchmark::State& state) {
    RuntimeWrapper wrapper(JavaScriptBridge::get(jsEngineType));
    auto bytes = wrapper.getProtoData("card_51.data");

    auto moduleFunction = wrapper.benchmarkModule.getMapValue(functionName).getFunctionRef();

    wrapper.runtime->getJavaScriptRuntime()->dispatchOnJsThreadSync(nullptr, [&](const auto& jsEntry) {
        auto doBench = (*moduleFunction)(ValueFunctionFlagsPropagatesError,
                                         {Value(makeShared<ValueTypedArray>(Uint8Array, bytes))})
                           .value()
                           .getFunctionRef();

        for (auto _ : state) {
            (*doBench)(ValueFunctionFlagsPropagatesError, {});
        }
    });
}

static void DecodeQuickJS(benchmark::State& state) {
    doBench(JavaScriptEngineType::QuickJS, "makeDecoder", state);
}
BENCHMARK(DecodeQuickJS);

static void DecodeJSCore(benchmark::State& state) {
    doBench(JavaScriptEngineType::JSCore, "makeDecoder", state);
}
BENCHMARK(DecodeJSCore);

static void EncodeQuickJS(benchmark::State& state) {
    doBench(JavaScriptEngineType::QuickJS, "makeEncoder", state);
}
BENCHMARK(EncodeQuickJS);

static void EncodeJSCore(benchmark::State& state) {
    doBench(JavaScriptEngineType::JSCore, "makeEncoder", state);
}
BENCHMARK(EncodeJSCore);

static void DecodeAndVisitQuickJS(benchmark::State& state) {
    doBench(JavaScriptEngineType::QuickJS, "makeDecoderVisitor", state);
}
BENCHMARK(DecodeAndVisitQuickJS);

static void DecodeAndVisitJSCore(benchmark::State& state) {
    doBench(JavaScriptEngineType::JSCore, "makeDecoderVisitor", state);
}
BENCHMARK(DecodeAndVisitJSCore);

static void DecodeAndVisitMapQuickJS(benchmark::State& state) {
    doBench(JavaScriptEngineType::QuickJS, "makeMapDecoderVisitor", state);
}
BENCHMARK(DecodeAndVisitMapQuickJS);

static void DecodeAndVisitMapJSCore(benchmark::State& state) {
    doBench(JavaScriptEngineType::JSCore, "makeMapDecoderVisitor", state);
}
BENCHMARK(DecodeAndVisitMapJSCore);

BENCHMARK_MAIN();

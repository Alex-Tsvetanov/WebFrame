#include "valdi/runtime/JavaScript/JavaScriptWorker.hpp"
#include "valdi/runtime/JavaScript/JavaScriptUtils.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"

namespace Valdi {

VALDI_CLASS_IMPL(JavaScriptWorker);

JavaScriptWorker::JavaScriptWorker(Ref<JavaScriptRuntime> runtime, const StringBox& url)
    : _runtime(std::move(runtime)), _url(url) {
    VALDI_INFO(_runtime->getLogger(), "Created JS Worker with URL: {}", url);
}

JavaScriptWorker::~JavaScriptWorker() {
    VALDI_INFO(_runtime->getLogger(), "Destroying JS Worker with URL: {}", _url);
    _runtime->fullTeardown();
}

// Retrieve the onmessage function set by the script
static Ref<ValueFunction> getGlobalOnMessage(JavaScriptEntryParameters& entry) {
    auto globalObj = entry.jsContext.getGlobalObject(entry.exceptionTracker);
    auto onMessageKey = STRING_LITERAL("onmessage");
    auto onmessage =
        entry.jsContext.getObjectProperty(globalObj.get(), onMessageKey.toStringView(), entry.exceptionTracker);
    if (entry.jsContext.isValueFunction(onmessage.get())) {
        return jsValueToFunction(
            entry.jsContext, onmessage.get(), ReferenceInfoBuilder().withObject(onMessageKey), entry.exceptionTracker);
    } else {
        return nullptr;
    }
}

// Call the onmessage function with data
static void dispatchMessage(const Ref<ValueFunction>& func, const Value& data) {
    if (func == nullptr) {
        return;
    }

    auto e = makeShared<ValueMap>();
    (*e)[STRING_LITERAL("data")] = data;
    (*func)({Value(e)});
}

void JavaScriptWorker::postInit() {
    _runtime->dispatchOnJsThread(
        nullptr, JavaScriptTaskScheduleTypeDefault, 0, [self = strongSmallRef(this)](JavaScriptEntryParameters& entry) {
            self->doPostInit();
        });
}

void JavaScriptWorker::setHostOnMessage(const Ref<ValueFunction>& func) {
    _runtime->dispatchOnJsThread(
        nullptr,
        JavaScriptTaskScheduleTypeDefault,
        0,
        [self = strongSmallRef(this), func](JavaScriptEntryParameters& entry) { self->doSetHostOnMessage(func); });
}

void JavaScriptWorker::postMessage(const Value& value) {
    _runtime->dispatchOnJsThread(
        nullptr,
        JavaScriptTaskScheduleTypeDefault,
        0,
        [self = strongSmallRef(this), value](JavaScriptEntryParameters& entry) { self->doPostMessage(entry, value); });
}

void JavaScriptWorker::close() {
    _runtime->dispatchOnJsThread(nullptr,
                                 JavaScriptTaskScheduleTypeDefault,
                                 0,
                                 [self = strongSmallRef(this)](JavaScriptEntryParameters& entry) { self->doClose(); });
}

void JavaScriptWorker::doPostInit() {
    auto weakSelf = weakRef(this);
    // Set up globals in the worker runtime
    // - onmessage
    // - postMessage
    // - close
    // - location, https://developer.mozilla.org/en-US/docs/Web/API/WorkerLocation, only href and search are populated
    _runtime->setValueToGlobalObject(STRING_LITERAL("onmessage"), Value::undefined());
    auto postMessageFunc = [weakSelf](const ValueFunctionCallContext& callContext) -> Value {
        auto self = weakSelf.lock();
        if (self && !self->_closed) {
            dispatchMessage(self->_hostOnMessage, callContext.getParameter(0));
        }
        return Value::undefined();
    };
    _runtime->setValueToGlobalObject(STRING_LITERAL("postMessage"),
                                     Value(makeShared<ValueFunctionWithCallable>(postMessageFunc)));
    auto closeFunc = [weakSelf](const ValueFunctionCallContext& callContext) -> Value {
        auto self = weakSelf.lock();
        if (self) {
            self->close();
        }
        return Value::undefined();
    };

    _runtime->setValueToGlobalObject(STRING_LITERAL("close"), Value(makeShared<ValueFunctionWithCallable>(closeFunc)));

    auto queryStartIndex = _url.indexOf('?');
    auto scriptUrl = queryStartIndex.has_value() ? _url.substring(0, queryStartIndex.value()) : _url;
    auto location = makeShared<ValueMap>();
    (*location)[STRING_LITERAL("href")] = Value(_url);
    (*location)[STRING_LITERAL("search")] =
        Value(queryStartIndex.has_value() ? _url.substring(queryStartIndex.value()) : STRING_LITERAL(""));
    _runtime->setValueToGlobalObject(STRING_LITERAL("location"), Value(location));

    // Evaluate the worker script
    auto result = _runtime->evalModuleSync(scriptUrl, false);
}

void JavaScriptWorker::doSetHostOnMessage(const Ref<ValueFunction>& func) {
    _hostOnMessage = func;
}

void JavaScriptWorker::doPostMessage(JavaScriptEntryParameters& entry, const Value& value) const {
    if (!_closed) {
        auto onMessageFunc = getGlobalOnMessage(entry);
        dispatchMessage(onMessageFunc, value);
    }
}

void JavaScriptWorker::doClose() {
    _closed = true;
}

} // namespace Valdi

//
//  JavaScriptComponentContextHandler.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/17/19.
//

#pragma once

#include "valdi/runtime/Attributes/AttributeIds.hpp"
#include "valdi/runtime/Context/ContextHandler.hpp"
#include "valdi/runtime/Context/RawViewNodeId.hpp"
#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi/runtime/JavaScript/JSPropertyNameIndex.hpp"
#include "valdi/runtime/JavaScript/JSValueRefHolder.hpp"
#include "valdi/runtime/JavaScript/JavaScriptTaskScheduler.hpp"
#include "valdi_core/cpp/Resources/ResourceId.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class JavaScriptComponentContextHandler;
class ILogger;
class Metrics;

class JavaScriptComponentContextHandlerListener {
public:
    virtual ~JavaScriptComponentContextHandlerListener() = default;

    virtual void requestUpdateJsContextHandler(JavaScriptEntryParameters& jsEntry,
                                               JavaScriptComponentContextHandler& handler) = 0;

    virtual const Ref<Metrics>& getMetrics() const = 0;
};

class JavaScriptComponentContextHandler : public ContextHandler {
public:
    JavaScriptComponentContextHandler(JavaScriptTaskScheduler& jsTaskScheduler,
                                      JavaScriptComponentContextHandlerListener* listener,
                                      ILogger& logger);
    ~JavaScriptComponentContextHandler() override;

    void clear();
    void removeJsContextHandler();
    void setJsContextHandler(const Result<Shared<JSValueRefHolder>>& jsContextHandler);
    bool needsJsContextHandler() const;

    void onCreate(const Ref<Context>& context,
                  const Shared<ValueConvertible>& viewModel,
                  const Shared<ValueConvertible>& componentContext,
                  ContextUpdateId updateId,
                  bool shouldUpdateSync) override;

    void onViewModelUpdate(const Ref<Context>& context,
                           const Shared<ValueConvertible>& viewModel,
                           ContextUpdateId updateId,
                           bool shouldUpdateSync) override;

    void onDestroy(const Ref<Context>& context, bool shouldUpdateSync) override;

    void onRuntimeIssue(const Ref<Context>& context,
                        bool isError,
                        const StringBox& message,
                        bool shouldUpdateSync) override;

    void onAttributeChange(const Ref<Context>& context,
                           RawViewNodeId viewNodeId,
                           const StringBox& attributeName,
                           const Value& attributeValue,
                           bool shouldUpdateSync) override;

    void callComponentFunction(const Ref<Context>& context,
                               const StringBox& functionName,
                               const Ref<ValueArray>& parameters);

    // Not thread safe, should be called in the JS thread
    Result<Ref<Context>> getContextForId(ContextId contextId) const;

private:
    Weak<JavaScriptTaskScheduler> _jsTaskScheduler;
    JavaScriptComponentContextHandlerListener* _listener;
    [[maybe_unused]] ILogger& _logger;

    Result<Shared<JSValueRefHolder>> _jsContextHandler;
    FlatMap<ContextId, Ref<Context>> _contextsById;
    std::unique_ptr<JSValueRefHolder> _stashedHotReloadData;
    JSPropertyNameIndex<8> _propertyNames;

    void doOnCreate(JavaScriptEntryParameters& jsEntry,
                    const Shared<ValueConvertible>& viewModel,
                    const Shared<ValueConvertible>& componentContext);
    void doOnDestroy(JavaScriptEntryParameters& jsEntry);
    void doOnViewModelUpdate(JavaScriptEntryParameters& jsEntry, const Shared<ValueConvertible>& viewModel);
    void doOnRuntimeIssue(JavaScriptEntryParameters& jsEntry, bool isError, const StringBox& message);
    void doOnAttributeChange(JavaScriptEntryParameters& jsEntry,
                             RawViewNodeId viewNodeId,
                             const StringBox& attributeName,
                             const Value& attributeValue);

    void doCallComponentFunction(JavaScriptEntryParameters& jsEntry,
                                 const StringBox& functionName,
                                 const Ref<ValueArray>& parameters);

    void deassociateContext(ContextId contextId);

    void onFailedToNotifyRuntimeIssue(JavaScriptEntryParameters& jsEntry);

    JSValueRef callJsContextHandlerFunction(size_t propertyNameIndex,
                                            JavaScriptEntryParameters& jsEntry,
                                            JSFunctionCallContext& callContext);
};

} // namespace Valdi

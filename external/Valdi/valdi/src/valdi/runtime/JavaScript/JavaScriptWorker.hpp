#pragma once

#include "valdi/runtime/JavaScript/JavaScriptRuntime.hpp"

namespace Valdi {

class JavaScriptWorker : public ValdiObject {
public:
    VALDI_CLASS_HEADER(JavaScriptWorker);

    JavaScriptWorker(Ref<JavaScriptRuntime> runtime, const StringBox& url);
    ~JavaScriptWorker() override;

    void postInit();
    void setHostOnMessage(const Ref<ValueFunction>& func);
    void postMessage(const Value& value);
    void close();

private:
    Ref<JavaScriptRuntime> _runtime;
    const StringBox _url;
    // onmessage callback in the owner's js context
    Ref<ValueFunction> _hostOnMessage;
    bool _closed = false;

    // Called from JS runtime thread
    void doPostInit();
    void doSetHostOnMessage(const Ref<ValueFunction>& func);
    void doPostMessage(JavaScriptEntryParameters& entry, const Value& value) const;
    void doClose();
};

} // namespace Valdi

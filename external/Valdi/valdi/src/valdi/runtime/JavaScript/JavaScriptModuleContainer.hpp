//
//  JavaScriptModuleContainer.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 8/9/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi/runtime/Interfaces/IJavaScriptContext.hpp"
#include "valdi/runtime/JavaScript/JSValueRefHolder.hpp"
#include "valdi/runtime/JavaScript/JavaScriptTaskScheduler.hpp"
#include "valdi/runtime/Resources/Bundle.hpp"
#include <memory>

namespace Valdi {

class Marshaller;

class JavaScriptModuleContainer : public JSValueRefHolder {
public:
    JavaScriptModuleContainer(Ref<Bundle> bundle,
                              IJavaScriptContext& context,
                              const JSValue& value,
                              JSExceptionTracker& exceptionTracker);
    ~JavaScriptModuleContainer() override;

    void didUnload();

    const Ref<Bundle>& getBundle() const;

    int32_t pushToMarshaller(IJavaScriptContext& jsContext, Marshaller& marshaller);

    void addUnloadObserver(const Valdi::Value& observer);

private:
    Ref<Bundle> _bundle;
    std::vector<Value> _unloadObservers;
    bool _isUnloaded = false;
};

} // namespace Valdi

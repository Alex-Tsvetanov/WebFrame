//
//  ProtobufModuleFactory.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 11/21/19.
//

#pragma once

#include "valdi/runtime/JavaScript/Modules/JavaScriptModuleFactory.hpp"
#include "valdi_core/ModuleFactory.hpp"
#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class ResourceManager;
class IJavaScriptContext;
class DispatchQueue;

class ProtobufModuleFactory : public JavaScriptModuleFactory {
public:
    ProtobufModuleFactory(ResourceManager& resourcesManager, const Ref<DispatchQueue>& workerQueue, ILogger& logger);
    ~ProtobufModuleFactory() override;

    static void preloadProtoModule(const Ref<ResourceManager>& resourcesManager,
                                   const StringBox& modulePath,
                                   ILogger& logger);

    StringBox getModulePath() const final;
    JSValueRef loadModule(IJavaScriptContext& context,
                          const ReferenceInfoBuilder& referenceInfoBuilder,
                          JSExceptionTracker& exceptionTracker) override;

private:
    [[maybe_unused]] ResourceManager& _resourcesManager;
    Ref<DispatchQueue> _workerQueue;
    [[maybe_unused]] ILogger& _logger;
};

} // namespace Valdi

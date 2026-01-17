#pragma once

#include "valdi_core/ModuleFactory.hpp"

namespace Valdi {

class StandaloneGlobalScrollPerfLoggerBridgeModuleFactory
    : public snap::valdi_core::ModuleFactory,
      public std::enable_shared_from_this<StandaloneGlobalScrollPerfLoggerBridgeModuleFactory> {
public:
    StandaloneGlobalScrollPerfLoggerBridgeModuleFactory();

    StringBox getModulePath() override;
    Value loadModule() override;
};

} // namespace Valdi

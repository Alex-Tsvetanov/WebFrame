//
//  TCPSocketModuleFactory.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/20.
//

#pragma once

#include "valdi_core/ModuleFactory.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class TCPSocketModuleFactory : public Valdi::SharedPtrRefCountable, public snap::valdi_core::ModuleFactory {
public:
    TCPSocketModuleFactory();
    ~TCPSocketModuleFactory() override;

    StringBox getModulePath() override;
    Value loadModule() override;
};

} // namespace Valdi

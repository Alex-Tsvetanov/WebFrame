//
//  HTTPRequestManagerModuleFactory.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/20.
//

#pragma once

#include "valdi_core/HTTPRequestManager.hpp"
#include "valdi_core/ModuleFactory.hpp"

namespace Valdi {

class HTTPRequestManagerModuleFactory : public Valdi::SharedPtrRefCountable, public snap::valdi_core::ModuleFactory {
public:
    HTTPRequestManagerModuleFactory();
    ~HTTPRequestManagerModuleFactory() override;

    StringBox getModulePath() override;
    Value loadModule() override;
};

} // namespace Valdi

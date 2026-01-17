#pragma once

#include "valdi_core/ModuleFactory.hpp"
#include "valdi_core/cpp/Utils/Shared.hpp"

namespace Valdi {

class FileSystemFactory : public Valdi::SharedPtrRefCountable, public snap::valdi_core::ModuleFactory {
public:
    FileSystemFactory();
    ~FileSystemFactory() override;

    StringBox getModulePath() override;
    Value loadModule() override;
};

} // namespace Valdi

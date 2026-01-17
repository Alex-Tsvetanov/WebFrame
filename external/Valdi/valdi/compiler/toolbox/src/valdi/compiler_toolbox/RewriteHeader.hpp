#pragma once

#include "valdi_core/cpp/Utils/Result.hpp"
#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace Valdi {

Result<Void> rewriteHeader(const StringBox& inputFile,
                           const StringBox& outputFile,
                           const StringBox& moduleName,
                           const std::vector<StringBox>& preservedModuleNames,
                           bool flattenImportPaths);

}
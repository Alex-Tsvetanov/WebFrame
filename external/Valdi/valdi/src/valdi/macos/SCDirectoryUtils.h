//
//  SCCacheDirectoryUtils.h
//  valdi-desktop-apple
//
//  Created by Ramzy Jaber on 4/6/22.
//

#include "valdi_core/cpp/Utils/StringBox.hpp"

namespace ValdiMacOS {

Valdi::StringBox resolveDocumentsDirectory(bool relativeToTemporaryDirectory);
Valdi::StringBox resolveCachesDirectory(bool relativeToTemporaryDirectory);
Valdi::StringBox resolveShaderCacheDirectory(bool relativeToTemporaryDirectory);

} // namespace ValdiMacOS

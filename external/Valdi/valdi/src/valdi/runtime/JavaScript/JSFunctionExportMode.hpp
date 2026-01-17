//
//  JSFunctionExportMode.hpp
//  valdi
//
//  Created by Simon Corsin on 9/16/22.
//

#pragma once

#include <cstdint>

namespace Valdi {

using JSFunctionExportMode = uint8_t;
constexpr JSFunctionExportMode kJSFunctionExportModeEmpty = 0;
constexpr JSFunctionExportMode kJSFunctionExportModeSyncWithMainThread = 1 << 0;
constexpr JSFunctionExportMode kJSFunctionExportModeInterruptible = 1 << 1;
constexpr JSFunctionExportMode kJSFunctionExportModeSingleCall = 1 << 2;

} // namespace Valdi

//
//  Constants.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/28/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include <cstdint>

namespace Valdi {

extern int64_t valdiVersion;
extern bool traceRenderingPerformance;
extern bool traceReloaderPerformance;
extern bool traceLoadModules;
extern bool traceInitialization;
extern bool forceRetainJsObjects;

#if defined(__GNUC__) || defined(__clang__)
#define VALDI_LIKELY(x) __builtin_expect(!!(x), true)
#define VALDI_UNLIKELY(x) __builtin_expect(!!(x), false)
#else
#define VALDI_LIKELY(x) (x)
#define VALDI_UNLIKELY(x) (x)
#endif

} // namespace Valdi

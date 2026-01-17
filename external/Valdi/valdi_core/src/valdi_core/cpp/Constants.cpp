//
//  Constants.cpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/28/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi_core/cpp/Constants.hpp"

#if defined(ADDRESS_SANITIZER) || defined(THREAD_SANITIZER) || defined(MEMORY_SANITIZER)
// JavaScriptCore's GC cannot detect JS objects usage in stack with some asan options
#define VALDI_FORCE_RETAIN_JS_OBJECTS
#elif __APPLE__

#include <TargetConditionals.h>
#if TARGET_OS_SIMULATOR && TARGET_CPU_ARM64
// iOS arm64 simulator builds seem to break the JavaScriptCore's GC objects detection in stack
#define VALDI_FORCE_RETAIN_JS_OBJECTS
#endif

#endif

namespace Valdi {

int64_t valdiVersion = 1;
bool traceRenderingPerformance = false;
bool traceReloaderPerformance = false;
bool traceLoadModules = true;
bool traceInitialization = true;
#if defined(VALDI_FORCE_RETAIN_JS_OBJECTS)
bool forceRetainJsObjects = true;
#else
bool forceRetainJsObjects = false;
#endif

} // namespace Valdi

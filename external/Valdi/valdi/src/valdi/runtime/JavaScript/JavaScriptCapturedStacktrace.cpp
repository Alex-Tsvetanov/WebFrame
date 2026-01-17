//
//  JavaScriptCapturedStacktrace.cpp
//  valdi
//
//  Created by Simon Corsin on 07/30/2024.
//  Copyright © 2024 Snap Inc. All rights reserved.
//

#include "valdi/runtime/JavaScript/JavaScriptCapturedStacktrace.hpp"
#include "valdi/runtime/Context/Context.hpp"

namespace Valdi {

JavaScriptCapturedStacktrace::JavaScriptCapturedStacktrace(Status status,
                                                           const StringBox& stackTrace,
                                                           const Ref<Context>& context)
    : _status(status), _stackTrace(stackTrace), _context(context) {}

JavaScriptCapturedStacktrace::~JavaScriptCapturedStacktrace() = default;

const Ref<Context>& JavaScriptCapturedStacktrace::getContext() const {
    return _context;
}

} // namespace Valdi

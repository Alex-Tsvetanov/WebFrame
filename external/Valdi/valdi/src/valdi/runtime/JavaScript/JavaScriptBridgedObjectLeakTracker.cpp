//
//  JavaScriptBridgedObjectLeakTracker.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 7/17/19.
//

#include "valdi/runtime/JavaScript/JavaScriptBridgedObjectLeakTracker.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"

namespace Valdi {
JavaScriptBridgedObjectLeakTracker::JavaScriptBridgedObjectLeakTracker(ILogger& logger, StringBox name)
    : _logger(logger), _name(std::move(name)) {
    VALDI_INFO(_logger, "Created bridge JS object {}", _name);
}

JavaScriptBridgedObjectLeakTracker::~JavaScriptBridgedObjectLeakTracker() {
    VALDI_INFO(_logger, "Destroyed bridge JS object {}", _name);
}

VALDI_CLASS_IMPL(JavaScriptBridgedObjectLeakTracker)

} // namespace Valdi

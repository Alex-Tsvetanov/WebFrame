//
//  BridgeLogger.cpp
//  valdi-ios
//
//  Created by Simon Corsin on 2/19/19.
//

#include "valdi/runtime/Utils/BridgeLogger.hpp"

namespace Valdi {

BridgeLogger::BridgeLogger(const Ref<ILogger>& innerLogger) : _innerLogger(innerLogger) {}

BridgeLogger::~BridgeLogger() = default;

bool BridgeLogger::isEnabledForType(LogType type) {
    return _innerLogger->isEnabledForType(type);
}

void BridgeLogger::log(LogType type, std::string message) {
    auto listener = _listener.lock();
    if (listener != nullptr) {
        listener->willLog(type, message);
    }
    _innerLogger->log(type, std::move(message));
}

void BridgeLogger::setListener(Weak<BridgeLoggerListener> listener) {
    _listener = std::move(listener);
}

const Weak<BridgeLoggerListener>& BridgeLogger::getListener() const {
    return _listener;
}

} // namespace Valdi

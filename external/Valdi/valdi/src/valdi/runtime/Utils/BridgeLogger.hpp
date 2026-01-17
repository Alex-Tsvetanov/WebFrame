//
//  BridgeLogger.hpp
//  valdi-ios
//
//  Created by Simon Corsin on 2/19/19.
//

#pragma once

#include "valdi_core/cpp/Interfaces/ILogger.hpp"

namespace Valdi {

class BridgeLogger;
class BridgeLoggerListener {
public:
    BridgeLoggerListener() = default;
    virtual ~BridgeLoggerListener() = default;

    virtual void willLog(LogType type, const std::string& message) = 0;
};

class BridgeLogger : public ILogger {
public:
    explicit BridgeLogger(const Ref<ILogger>& innerLogger);
    ~BridgeLogger() override;

    bool isEnabledForType(LogType type) override;

    void log(LogType type, std::string message) override;

    void setListener(Weak<BridgeLoggerListener> listener);
    const Weak<BridgeLoggerListener>& getListener() const;

private:
    Ref<ILogger> _innerLogger;
    Weak<BridgeLoggerListener> _listener;
};

} // namespace Valdi

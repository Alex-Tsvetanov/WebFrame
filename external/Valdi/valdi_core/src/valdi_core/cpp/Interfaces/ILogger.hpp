//
//  ILogger.hpp
//  ValdiRuntime
//
//  Created by Simon Corsin on 5/28/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Utils/Shared.hpp"
#include <string>

namespace Valdi {

enum LogType { LogTypeDebug = 0, LogTypeInfo, LogTypeWarn, LogTypeError, LogTypeCount };

class ILogger : public SimpleRefCountable {
public:
    ILogger() = default;

    virtual bool isEnabledForType(LogType /*type*/) {
        return true;
    }

    virtual void log(LogType type, std::string message) = 0;
};

} // namespace Valdi

//
//  Logger.hpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#pragma once

#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Utils/FlatMap.hpp"
#include "valdi_core/jni/GlobalRefJavaObject.hpp"

#include <array>

namespace ValdiAndroid {

class Logger : GlobalRefJavaObjectBase, public Valdi::ILogger {
public:
    Logger(JavaEnv env, jobject logger);
    ~Logger() override;

    void log(Valdi::LogType type, std::string message) override;

    bool isEnabledForType(Valdi::LogType type) override;

private:
    JavaMethod<VoidType, int32_t, std::string> _method;
    JavaMethod<bool, int32_t> _enabledForTypeMethod;
    std::array<bool, Valdi::LogTypeCount> _enabledForType;
};

} // namespace ValdiAndroid

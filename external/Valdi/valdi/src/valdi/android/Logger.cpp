//
//  Logger.cpp
//  ValdiAndroidNative
//
//  Created by Simon Corsin on 6/6/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#include "valdi/android/Logger.hpp"

namespace ValdiAndroid {

Logger::Logger(JavaEnv env, jobject logger) : GlobalRefJavaObjectBase(env, logger, "Logger") {
    auto jObject = toObject();
    auto cls = jObject.getClass();
    cls.getMethod("log", _method);
    cls.getMethod("isEnabledForType", _enabledForTypeMethod);

    for (int32_t type = Valdi::LogTypeDebug; type < Valdi::LogTypeCount; type++) {
        auto result = _enabledForTypeMethod.call(jObject, type);
        _enabledForType[static_cast<Valdi::LogType>(type)] = result;
    }
}

Logger::~Logger() = default;

void Logger::log(Valdi::LogType type, std::string message) {
    auto jType = static_cast<int32_t>(type);

    _method.call(toObject(), jType, message);
}

bool Logger::isEnabledForType(Valdi::LogType type) {
    if (type >= _enabledForType.size()) {
        return false;
    }

    return _enabledForType[type];
}

} // namespace ValdiAndroid

//
//  SCValdiLogger.m
//  ValdiIOS
//
//  Created by Simon Corsin on 5/30/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi/ios/CPPBindings/SCValdiLoggerBridge.h"

#import "valdi_core/SCValdiLogger.h"
#import "valdi_core/cpp/Utils/ConsoleLogger.hpp"
#import "valdi_core/SCValdiObjCConversionUtils.h"

#import <iostream>

namespace ValdiIOS
{

Logger::Logger()
{
}

SCValdiLoggerLevel toObjCLogLevel(Valdi::LogType logType) {
   switch (logType) {
    case Valdi::LogTypeDebug:
        return SCValdiLoggerLevelDebug;
    case Valdi::LogTypeInfo:
        return SCValdiLoggerLevelInfo;
    case Valdi::LogTypeWarn:
        return SCValdiLoggerLevelWarn;
    case Valdi::LogTypeError:
        return SCValdiLoggerLevelError;
    case Valdi::LogTypeCount:
        return SCValdiLoggerLevelError; // No proper mapping, should not happen in practice
    }
}

bool Logger::isEnabledForType(Valdi::LogType logType) {
    id<SCValdiLogger> externalLogger = SCValdiGetSharedLogger();
    if (externalLogger == nullptr) {
        return true;
    }

    SCValdiLoggerLevel objcLevel = toObjCLogLevel(logType);

    return [externalLogger isLogEnabledForLevel:objcLevel];
}

void Logger::log(Valdi::LogType logType, std::string message)
{
    id<SCValdiLogger> externalLogger = SCValdiGetSharedLogger();
    if (externalLogger != nullptr) {
        NSString *objcMessage = NSStringFromSTDStringView(message);
        SCValdiLoggerLevel objcLevel = toObjCLogLevel(logType);

        [externalLogger outputLog:objcMessage forLevel:objcLevel];
    } else {
        Valdi::ConsoleLogger::getLogger().log(logType, std::move(message));
    }
}

}; // namespace ValdiIOS

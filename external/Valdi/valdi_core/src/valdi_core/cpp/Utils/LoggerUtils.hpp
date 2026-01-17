//
//  LoggerUtils.h
//  valdi
//
//  Created by Simon Corsin on 11/7/18.
//

#pragma once

#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Utils/Format.hpp"

#define VALDI_LOG_ARGS(...) , ##__VA_ARGS__

#if SC_LOGGING_COMPILED_IN()

#define VALDI_LOG(logger, type, _format, ...)                                                                          \
    if ((logger).isEnabledForType(type)) {                                                                             \
        (logger).log(type, fmt::format(_format VALDI_LOG_ARGS(__VA_ARGS__)));                                          \
    }

#else
#define VALDI_LOG(logger, type, _format, ...) void();
#endif

#define VALDI_DEBUG(logger, _format, ...) VALDI_LOG(logger, Valdi::LogTypeDebug, _format VALDI_LOG_ARGS(__VA_ARGS__))
#define VALDI_INFO(logger, _format, ...) VALDI_LOG(logger, Valdi::LogTypeInfo, _format VALDI_LOG_ARGS(__VA_ARGS__))
#define VALDI_WARN(logger, _format, ...) VALDI_LOG(logger, Valdi::LogTypeWarn, _format VALDI_LOG_ARGS(__VA_ARGS__))
#define VALDI_ERROR(logger, _format, ...) VALDI_LOG(logger, Valdi::LogTypeError, _format VALDI_LOG_ARGS(__VA_ARGS__))

//
//  ConsoleLogger.hpp
//  valdi-pc
//
//  Created by Simon Corsin on 10/14/19.
//

#pragma once

#include "utils/platform/TargetPlatform.hpp"
#include "valdi_core/cpp/Interfaces/ILogger.hpp"
#include "valdi_core/cpp/Utils/LoggerUtils.hpp"
#include "valdi_core/cpp/Utils/Mutex.hpp"

#include <iostream>
#include <mutex>

#if defined(SC_IOS)
#include <os/log.h>
#elif !defined(__ANDROID__)
#include "utils/time/StopWatch.hpp"
#endif

namespace Valdi {

class ConsoleLogger : public ILogger {
public:
#if !defined(__ANDROID__)
    explicit ConsoleLogger(std::ostream& stream);
#endif
    bool isEnabledForType(LogType type) override;

    void log(LogType type, std::string message) override;

    void setMinLogType(LogType minLogType);

    void writeDirect(std::string_view message);

    static ConsoleLogger& getLogger();
    static ConsoleLogger& getErrorLogger();

private:
#if defined(SC_IOS)
    os_log_t _valdiLogger;
#elif !defined(__ANDROID__)
    std::ostream& _stream;
    snap::utils::time::StopWatch _stopwatch;
#endif

    Mutex _mutex;
    LogType _minLogType = Valdi::LogTypeInfo;
};

} // namespace Valdi

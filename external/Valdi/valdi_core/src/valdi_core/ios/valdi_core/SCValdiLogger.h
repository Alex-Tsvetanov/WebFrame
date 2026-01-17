
#import "valdi_core/SCValdiSharedLogger.h"
#import <Foundation/Foundation.h>

#define SCLogValdiWrap(logLevel, log, ...)                                                                             \
    do {                                                                                                               \
        id<SCValdiLogger> __logger = SCValdiGetSharedLogger();                                                         \
        if ([__logger isLogEnabledForLevel:logLevel]) {                                                                \
            [__logger outputLog:[NSString stringWithFormat:log, ##__VA_ARGS__] forLevel:logLevel];                     \
        }                                                                                                              \
    } while (0);

#define SCLogValdiVerbose(log, ...) SCLogValdiWrap(SCValdiLoggerLevelVerbose, log, ##__VA_ARGS__)
#define SCLogValdiDebug(log, ...) SCLogValdiWrap(SCValdiLoggerLevelDebug, log, ##__VA_ARGS__)
#define SCLogValdiInfo(log, ...) SCLogValdiWrap(SCValdiLoggerLevelInfo, log, ##__VA_ARGS__)
#define SCLogValdiWarning(log, ...) SCLogValdiWrap(SCValdiLoggerLevelWarn, log, ##__VA_ARGS__)
#define SCLogValdiError(log, ...) SCLogValdiWrap(SCValdiLoggerLevelError, log, ##__VA_ARGS__)

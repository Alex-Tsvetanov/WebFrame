//
//  SCValdiError.m
//  valdi-ios
//
//  Created by Simon Corsin on 9/23/19.
//

#import "valdi_core/SCValdiError.h"
#import "valdi_core/SCValdiLogger.h"

static NSString *const kStackTraceExceptionUserInfoKey = @"valdiStackTrace";

@implementation SCValdiError

- (instancetype)initWithReason:(NSString *)reason
{
    return [self initWithReason:reason stackTrace:nil];
}

- (instancetype)initWithReason:(NSString *)reason stackTrace:(NSString * _Nullable)stackTrace
{
    self = [super initWithName:@"SCValdiError" reason:reason userInfo:stackTrace != nil ? @{kStackTraceExceptionUserInfoKey: stackTrace} : nil];

    if (self) {
    }

    return self;
}

@end

void SCValdiErrorThrow(NSString* reason) 
{
    SCValdiErrorThrowWithStacktrace(reason, nil);
}

void SCValdiErrorThrowWithStacktrace(NSString* reason, NSString * _Nullable stackTrace)
{
    SCLogValdiError(@"%@", reason);
    if (stackTrace) {
        SCLogValdiError(@"Stacktrace:\n%@", stackTrace);
    }
    @throw [[SCValdiError alloc] initWithReason:reason stackTrace:stackTrace];
}

void SCValdiErrorRethrow(SCValdiError *initialError, NSString *messagePrefixFormat, ...)
{
    va_list v;
    va_start(v, messagePrefixFormat);

    NSString *messagePrefix = [[NSString alloc] initWithFormat:messagePrefixFormat arguments:v];

    va_end(v);

    SCValdiErrorThrow([messagePrefix stringByAppendingFormat:@": %@", initialError.reason]);
}

void SCValdiErrorThrowUnimplementedMethod()
{
    SCValdiErrorThrow(@"Unimplemented method");;
}

NSString *SCValdiErrorGetStackTrace(NSException *exception)
{
    return exception.userInfo[kStackTraceExceptionUserInfoKey];
}

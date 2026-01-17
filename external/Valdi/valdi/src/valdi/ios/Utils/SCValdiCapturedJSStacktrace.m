// Copyright © 2023 Snap, Inc. All rights reserved.

#import "SCValdiCapturedJSStacktrace.h"

@implementation SCValdiCapturedJSStacktrace

- (instancetype)initWithStackTrace:(NSString *)stackTrace threadStatus:(SCValdiJSThreadStatus)threadStatus
{
    self = [super init];

    if (self) {
        _stackTrace = stackTrace;
        _threadStatus = threadStatus;
    }

    return self;
}


@end

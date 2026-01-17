// Copyright © 2023 Snap, Inc. All rights reserved.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, SCValdiJSThreadStatus) {
    SCValdiJSThreadStatusNotRunning,
    SCValdiJSThreadStatusRunning,
    SCValdiJSThreadStatusTimedOut,
};

@interface SCValdiCapturedJSStacktrace : NSObject

@property (readonly, nonatomic) NSString* stackTrace;
@property (readonly, nonatomic) SCValdiJSThreadStatus threadStatus;

- (instancetype)initWithStackTrace:(NSString*)stackTrace threadStatus:(SCValdiJSThreadStatus)threadStatus;

@end

NS_ASSUME_NONNULL_END

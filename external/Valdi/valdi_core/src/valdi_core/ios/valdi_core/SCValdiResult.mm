//
//  SCValdiResult.m
//  valdi-ios
//
//  Created by Simon Corsin on 9/5/19.
//

#import "valdi_core/SCValdiResult+CPP.h"
#import "valdi_core/cpp/Utils/PlatformResult.hpp"
#import "valdi_core/SCValdiObjCConversionUtils.h"

@interface SCValdiResult()

@property (readonly, nonatomic) Valdi::PlatformResult platformResult;

@end

@implementation SCValdiResult

- (instancetype)initWithPlatformResult:(Valdi::PlatformResult)platformResult
{
    self = [super init];

    if (self) {
        _platformResult = std::move(platformResult);
    }
    return self;
}

- (BOOL)success
{
    return _platformResult.success();
}

- (NSString *)errorMessage
{
    return ValdiIOS::NSStringFromSTDStringView(_platformResult.error().toString());
}

- (id)successValue
{
    return ValdiIOS::NSObjectFromValue(_platformResult.value());
}

@end

SCValdiResult *SCValdiResultSuccess() {
    static SCValdiResult *successResult;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        successResult = [[SCValdiResult alloc] initWithPlatformResult:Valdi::Value::undefined()];
    });
    return successResult;
}

SCValdiResult *SCValdiResultFailure(NSString *errorMessage) {
    return [[SCValdiResult alloc] initWithPlatformResult:Valdi::Error(ValdiIOS::InternedStringFromNSString(errorMessage))];
}

SCValdiResult *SCValdiResultSuccessWithData(id data) {
    return [[SCValdiResult alloc] initWithPlatformResult:ValdiIOS::ValueFromNSObject(data)];
}

SCValdiResult* SCValdiResultFromPlatformResult(const Valdi::PlatformResult& platformResult) {
    if (platformResult.success()) {
        return SCValdiResultSuccess();
    } else {
        return [[SCValdiResult alloc] initWithPlatformResult:platformResult];
    }
}

Valdi::PlatformResult SCValdiResultToPlatformResult(__unsafe_unretained SCValdiResult* result) {
    SC_ASSERT(result != nil);
    return result.platformResult;
}

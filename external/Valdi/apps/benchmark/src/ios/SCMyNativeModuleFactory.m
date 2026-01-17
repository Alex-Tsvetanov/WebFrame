#import "valdi_core/SCValdiModuleFactoryRegistry.h"
#import <BenchmarkTypes/BenchmarkTypes.h>
#import <Foundation/Foundation.h>
#include <time.h>

@interface NativeHelperImpl: NSObject<BenchmarkNativeHelperModule>

@end

@implementation NativeHelperImpl {
    NSMutableArray<BenchmarkFunctionTiming*>* _nativeTiming;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _nativeTiming = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)recordTiming {
    struct timespec ts;
    clock_gettime(CLOCK_UPTIME_RAW, &ts);
    uint64_t nanosSinceEpoch = (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    BenchmarkFunctionTiming* nativeTiming = [[BenchmarkFunctionTiming alloc] init];
    nativeTiming.enterTime = nanosSinceEpoch / 1000000.0;
    nativeTiming.leaveTime = nativeTiming.enterTime;
    [_nativeTiming addObject:nativeTiming];
}

- (NSArray<NSString*>*)measureStringsWithData:(NSArray<NSString*>*)data
{
    [self recordTiming];
    return data;
}

- (NSArray<NSDictionary<NSString*, id>*>* _Nonnull)measureStringMapsWithData:(NSArray<NSDictionary<NSString*, id>*>* _Nonnull)data
{
    [self recordTiming];
    return data;
}

- (void)clearNativeFunctionTiming
{
    [_nativeTiming removeAllObjects];
}

- (NSArray<BenchmarkFunctionTiming*>*)getNativeFunctionTiming
{
    return _nativeTiming;
}

@end

@interface NativeHelperFactoryImpl : BenchmarkNativeHelperModuleFactory

@end

@implementation NativeHelperFactoryImpl

VALDI_REGISTER_MODULE()

- (id<BenchmarkNativeHelperModule>)onLoadModule
{
    return [NativeHelperImpl new];
}

@end

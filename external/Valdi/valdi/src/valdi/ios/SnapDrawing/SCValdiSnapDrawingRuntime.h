//
//  SCValdiSnapDrawingRuntime.h
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/22.
//

#ifdef __cplusplus

#import "valdi_core/SCSnapDrawingRuntime.h"
#import <Foundation/Foundation.h>

namespace Valdi {
class IDiskCache;
class IViewManager;
class ILogger;
class DispatchQueue;
class AssetLoaderManager;
}; // namespace Valdi

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiSnapDrawingRuntime : NSObject <SCSnapDrawingRuntime>

- (instancetype)initWithDiskCache:(Valdi::IDiskCache*)diskCache
                  hostViewManager:(Valdi::IViewManager*)hostViewManager
                           logger:(Valdi::ILogger*)logger
                      workerQueue:(Valdi::DispatchQueue*)workerQueue
               assetLoaderManager:(Valdi::AssetLoaderManager*)assetLoaderManager
              maxCacheSizeInBytes:(uint64_t)maxCacheSizeInBytes;

- (void)onApplicationEnteringBackground;
- (void)onApplicationEnteringForeground;
- (void)onApplicationIsInLowMemory;

@end

NS_ASSUME_NONNULL_END

#endif
//
//  SCValdiAssetWithVideo.m
//  valdi_core
//
//  Created by Carson Holgate on 3/20/23.
//

#import "valdi_core/SCNValdiCoreAsset.h"
#import "valdi_core/SCValdiVideoPlayer.h"
#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

NS_ASSUME_NONNULL_BEGIN

/**
 Return a SCNValdiCoreAsset instance that holds the given SCValdiImage.
 This can be used in place to pass already loaded images into TypeScript
 and later set then on Image elements.
 */
SCNValdiCoreAsset* SCValdiAssetFromVideoPlayer(id<SCValdiVideoPlayer> player);

NS_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

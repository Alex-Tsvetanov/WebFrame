//
//  SCValdiAssetWithImage.h
//  valdi_core-ios
//
//  Created by Simon Corsin on 12/13/22.
//

#import "valdi_core/SCNValdiCoreAsset.h"
#import "valdi_core/SCValdiImage.h"
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
SCNValdiCoreAsset* SCValdiAssetFromImage(SCValdiImage* image);

NS_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

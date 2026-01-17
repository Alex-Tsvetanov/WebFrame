//
//  SCValdiVideoView.h
//  valdi_core
//
//  Created by Carson Holgate on 3/21/23.
//

#import "valdi_core/SCNValdiCoreAsset.h"
#import "valdi_core/SCValdiView.h"
#import "valdi_core/SCValdiViewAssetHandlingProtocol.h"
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiVideoView : SCValdiView <SCValdiViewAssetHandlingProtocol>

- (void)setAsset:(SCNValdiCoreAsset* _Nullable)asset tintColor:(UIColor* _Nullable)tintColor flipOnRtl:(BOOL)flipOnRtl;

@end

NS_ASSUME_NONNULL_END

//
//  SCValdiImageView.h
//  Valdi
//
//  Created by Simon Corsin on 2/20/19.
//

#import "valdi_core/SCNValdiCoreAsset.h"
#import "valdi_core/SCValdiView.h"
#import "valdi_core/SCValdiViewAssetHandlingProtocol.h"
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@interface SCValdiImageView : SCValdiView <SCValdiViewAssetHandlingProtocol>

- (void)setAsset:(SCNValdiCoreAsset* _Nullable)asset tintColor:(UIColor* _Nullable)tintColor flipOnRtl:(BOOL)flipOnRtl;

@end

NS_ASSUME_NONNULL_END

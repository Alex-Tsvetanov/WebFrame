//
//  SCValdiAnimator.h
//  Valdi
//
//  Created by Simon Corsin on 7/16/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi_core/SCNValdiCoreAnimator.h"
#import "valdi_core/SCValdiAnimatorBase.h"

#import <UIKit/UIKit.h>

@interface SCValdiAnimator : NSObject <SCValdiAnimatorProtocol, SCNValdiCoreAnimator>

@property (readonly, nonatomic) BOOL crossfade;
@property (readonly, nonatomic) BOOL wasCancelled;

- (void)setDisableRemoveOnComplete:(BOOL)disableRemoveOnComplete;

@end

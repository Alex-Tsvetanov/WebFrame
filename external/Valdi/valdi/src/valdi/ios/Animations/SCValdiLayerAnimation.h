//
//  SCValdiLayerAnimation.h
//  Valdi
//
//  Created by Simon Corsin on 7/18/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface SCValdiLayerAnimation : NSObject

@property (readonly, nonatomic, nonnull) CALayer* layer;
@property (readonly, nonatomic, nonnull) CAAnimation* animation;
@property (readonly, nonatomic, nonnull) NSString* key;

- (nonnull instancetype)initWithLayer:(nonnull CALayer*)layer
                            animation:(nonnull CAAnimation*)animation
                                  key:(nonnull NSString*)key
              disableRemoveOnComplete:(BOOL)disableRemoveOnComplete;

- (void)performAnimationWithCompletion:(nonnull void (^)(BOOL finished))completion;

- (void)cancelAnimationIfRunning;

@end

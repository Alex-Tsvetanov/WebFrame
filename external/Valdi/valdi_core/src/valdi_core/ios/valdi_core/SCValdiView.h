//
//  SCValdiView.h
//  Valdi
//
//  Created by Simon Corsin on 12/11/17.
//  Copyright © 2017 Snap Inc. All rights reserved.
//
#import "valdi_core/SCMacros.h"

#import "valdi_core/UIView+ValdiBase.h"
#import "valdi_core/UIView+ValdiObjects.h"

#import <UIKit/UIKit.h>

/**
 Wraps UIKit layout methods to use Yoga instead.
 This allows the view to behave like a regular
 view with AutoLayout even if it's not using
 auto layout constraints.
 */
@interface SCValdiView : UIView

@end

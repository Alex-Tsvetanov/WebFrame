//
//  SCSnapDrawingUIView.h
//  valdi-ios
//
//  Created by Simon Corsin on 8/30/22.
//

#import "valdi_core/SCMacros.h"
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@protocol SCSnapDrawingRuntime;

@interface SCSnapDrawingUIView : UIView

- (instancetype)initWithRuntime:(id<SCSnapDrawingRuntime>)runtime;

VALDI_NO_VIEW_INIT

@end

NS_ASSUME_NONNULL_END

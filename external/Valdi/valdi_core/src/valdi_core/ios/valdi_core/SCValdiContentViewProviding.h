//
//  SCValdiContentViewProviding.h
//  Valdi
//
//  Created by Brandon Francis on 3/10/19.
//

#import <UIKit/UIKit.h>

@protocol SCValdiContentViewProviding <NSObject>

/**
 A content view for adding subviews to. Useful for when a view does not allow subviews to be directly added to
 itself, such as UIVisualEffectView.
 */
- (UIView*)contentViewForInsertingValdiChildren;

@end

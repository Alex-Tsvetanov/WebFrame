//
//  GradientUtils.h
//  Valdi
//
//  Created by Jacob Huang on 11/3/23.
//

#import "valdi_core/UIColor+Valdi.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface GradientUtils : NSObject

typedef NS_ENUM(NSInteger, SCValdiLinearGradientOrientation) {
    /** draw the gradient from the top to the bottom */
    SCValdiLinearGradientOrientationTopBottom,
    /** draw the gradient from the top-right to the bottom-left */
    SCValdiLinearGradientOrientationTopRightBottomLeft,
    /** draw the gradient from the right to the left */
    SCValdiLinearGradientOrientationRightLeft,
    /** draw the gradient from the bottom-right to the top-left */
    SCValdiLinearGradientOrientationBottomRightTopLeft,
    /** draw the gradient from the bottom to the top */
    SCValdiLinearGradientOrientationBottomTop,
    /** draw the gradient from the bottom-left to the top-right */
    SCValdiLinearGradientOrientationBottomLeftTopRight,
    /** draw the gradient from the left to the right */
    SCValdiLinearGradientOrientationLeftRight,
    /** draw the gradient from the top-left to the bottom-right */
    SCValdiLinearGradientOrientationTopLeftBottomRight,
};

CAGradientLayer* setUpGradientLayerForRawAttributes(NSArray* rawAttributes, CAGradientLayer* existingGradientLayer);

@end

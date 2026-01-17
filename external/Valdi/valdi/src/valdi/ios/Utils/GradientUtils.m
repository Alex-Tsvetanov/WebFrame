//
//  GradientUtils.m
//  Valdi
//
//  Created by Jacob Huang on 11/3/23.
//

#import "valdi/ios/Utils/GradientUtils.h"
#import "valdi_core/SCMacros.h"

@implementation GradientUtils

static void getStartEndPointsForOrientation(SCValdiLinearGradientOrientation orientation, CGPoint *points) {
    switch (orientation) {
        case SCValdiLinearGradientOrientationTopBottom:
            points[0] = CGPointMake(0.5, 0.0);
            points[1] = CGPointMake(0.5, 1.0);
            break;
        case SCValdiLinearGradientOrientationTopRightBottomLeft:
            points[0] = CGPointMake(1.0, 0.0);
            points[1] = CGPointMake(0, 1.0);
            break;
        case SCValdiLinearGradientOrientationRightLeft:
            points[0] = CGPointMake(1.0, 0.5);
            points[1] = CGPointMake(0.0, 0.5);
            break;
        case SCValdiLinearGradientOrientationBottomRightTopLeft:
            points[0] = CGPointMake(1.0, 1.0);
            points[1] = CGPointMake(0.0, 0.0);
            break;
        case SCValdiLinearGradientOrientationBottomTop:
            points[0] = CGPointMake(0.5, 1.0);
            points[1] = CGPointMake(0.5, 0.0);
            break;
        case SCValdiLinearGradientOrientationBottomLeftTopRight:
            points[0] = CGPointMake(0.0, 1.0);
            points[1] = CGPointMake(1.0, 0.0);
            break;
        case SCValdiLinearGradientOrientationLeftRight:
            points[0] = CGPointMake(0, 0.5);
            points[1] = CGPointMake(1.0, 0.5);
            break;
        case SCValdiLinearGradientOrientationTopLeftBottomRight:
            points[0] = CGPointMake(0.0, 0.0);
            points[1] = CGPointMake(1.0, 1.0);
            break;
        default:
            points[0] = CGPointMake(0.5, 0.0);
            points[1] = CGPointMake(0.5, 1.0);
            break;
    }
}

CAGradientLayer *setUpGradientLayerForRawAttributes(NSArray *rawAttributes, CAGradientLayer *existingGradientLayer) {
    // Extracted raw attributes
    NSArray *colors = rawAttributes.firstObject;
    NSArray *locations = rawAttributes.count > 1 ? rawAttributes[1] : nil;

    if (colors == nil || colors.count < 2) {
        return existingGradientLayer;
    }

    NSMutableArray *colorStops = [[NSMutableArray alloc] initWithCapacity:colors.count];
    for (NSNumber *colorValue in colors) {
        [colorStops addObject:(id)UIColorFromValdiAttributeValue((int64_t)[colorValue integerValue]).CGColor];
    }

    SCValdiLinearGradientOrientation orientation = SCValdiLinearGradientOrientationTopBottom;
    if (rawAttributes.count > 2) {
        orientation = (SCValdiLinearGradientOrientation)[ObjectAs(rawAttributes[2], NSNumber) integerValue];
    }

    CGPoint points[2];
    BOOL radial = NO;
    if (rawAttributes.count > 3) {
        radial = [ObjectAs(rawAttributes[3], NSNumber) boolValue];
    }

    if (radial) {
        points[0] = CGPointMake(0.5, 0.5);
        points[1] = CGPointMake(1.0, 1.0);
    } else {
        getStartEndPointsForOrientation(orientation, points);
    }

    // Set up gradient layer
    CAGradientLayer *gradientLayer = existingGradientLayer != nil ? existingGradientLayer : [[CAGradientLayer alloc] init];

    gradientLayer.colors = [colorStops copy];

    if (locations.count == colorStops.count) {
        gradientLayer.locations = locations;
    }

    gradientLayer.type = radial ? kCAGradientLayerRadial : kCAGradientLayerAxial;
    gradientLayer.startPoint = points[0];
    gradientLayer.endPoint = points[1];

    return gradientLayer;
}

@end

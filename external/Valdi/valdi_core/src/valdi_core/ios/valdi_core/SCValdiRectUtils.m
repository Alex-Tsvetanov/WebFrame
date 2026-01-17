//
//  SCValdiRectUtils.m
//  Valdi
//
//  Created by Simon Corsin on 2/21/19.
//

#import "valdi_core/SCValdiRectUtils.h"

CGFloat UIScreenGetDeviceScale()
{
    static dispatch_once_t onceToken;
    static CGFloat scale;
    dispatch_once(&onceToken, ^{
        scale = UIScreen.mainScreen.scale;
    });
    return scale;
}

CGFloat CGFloatNormalize(float value)
{
    return CGFloatNormalizeD((CGFloat)value);
}

CGFloat CGFloatNormalizeD(CGFloat value)
{
    CGFloat scale = UIScreenGetDeviceScale();
    return round(value * scale) / scale;
}

CGFloat CGFloatNormalizeCeil(CGFloat value)
{
    CGFloat scale = UIScreenGetDeviceScale();
    return ceil(value * scale) / scale;
}

CGFloat CGFloatNormalizeFloor(CGFloat value)
{
    CGFloat scale = UIScreenGetDeviceScale();
    return floor(value * scale) / scale;
}

SCValdiViewLayout SCValdiMakeViewLayout(float x, float y, float width, float height)
{
    CGFloat xCG = CGFloatNormalize(x);
    CGFloat yCG = CGFloatNormalize(y);
    CGFloat widthCG = CGFloatNormalize(width);
    CGFloat heightCG = CGFloatNormalize(height);

    return (SCValdiViewLayout){.center = CGPointMake(xCG + widthCG / 2, yCG + heightCG / 2),
                                  .size = CGSizeMake(widthCG, heightCG)};
}

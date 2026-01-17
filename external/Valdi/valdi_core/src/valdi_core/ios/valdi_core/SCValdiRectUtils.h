//
//  SCValdiRectUtils.h
//  Valdi
//
//  Created by Simon Corsin on 2/21/19.
//

#import <UIKit/UIKit.h>

typedef struct SCValdiViewLayout {
    CGPoint center;
    CGSize size;
} SCValdiViewLayout;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CGFloat UIScreenGetDeviceScale();

/**
 * Rounds to the nearest pixel on the current device's screen
 */
CGFloat CGFloatNormalize(float value);

/**
 * Rounds to the nearest pixel on the current device's screen
 */
CGFloat CGFloatNormalizeD(CGFloat value);

/**
 * Rounds to the nearest highest pixel on the current device's screen
 */
CGFloat CGFloatNormalizeCeil(CGFloat value);

/**
 * Rounds to the nearest lowest pixel on the current device's screen
 */
CGFloat CGFloatNormalizeFloor(CGFloat value);

SCValdiViewLayout SCValdiMakeViewLayout(float x, float y, float width, float height);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

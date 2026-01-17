//
//  SCValdiGeometricPath.h
//  valdi_core-ios
//
//  Created by Simon Corsin on 3/4/22.
//

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#ifdef __cplusplus
extern "C" {
#endif

NS_ASSUME_NONNULL_BEGIN

CGPathRef CGPathFromGeometricPathData(NSData* data, CGSize size);

NS_ASSUME_NONNULL_END

#ifdef __cplusplus
}
#endif

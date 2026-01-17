//
//  UIColor+Valdi.h
//  Valdi
//
//  Created by Simon Corsin on 12/12/17.
//  Copyright © 2017 Snap Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIColor (Valdi)

/**
 Convert the color into a 64bits RGBA, which is the format
 used by Valdi C++.
 */
@property (readonly, nonatomic) int64_t valdiAttributeValue;

@end

#ifdef __cplusplus
extern "C" {
#endif
UIColor* UIColorFromValdiAttributeValue(int64_t value);

#ifdef __cplusplus
}
#endif

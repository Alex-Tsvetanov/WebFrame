//
//  UIImage+Valdi.h
//  Valdi
//
//  Created by Simon Corsin on 12/12/17.
//  Copyright © 2017 Snap Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "valdi_core/SCValdiContextProtocol.h"

@interface UIImage (Valdi)

+ (UIImage*)imageFromValdiAttributeValue:(id)attributeValue context:(id<SCValdiContextProtocol>)context;
+ (UIImage*)imageNamed:(NSString*)imageName inValdiBundle:(NSString*)bundleName;

@end

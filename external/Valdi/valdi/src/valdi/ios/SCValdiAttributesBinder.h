//
//  SCValdiAttributesBinder.h
//  iOS
//
//  Created by Simon Corsin on 4/18/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi_core/SCValdiAttributesBinderBase.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

typedef struct SCValdiAttributesBinderNative SCValdiAttributesBinderNative;

@interface SCValdiAttributesBinder : NSObject <SCValdiAttributesBinderProtocol>

- (instancetype)initWithNativeAttributesBindingContext:(SCValdiAttributesBinderNative*)nativeAttributesBindingContext
                                           fontManager:(id<SCValdiFontManagerProtocol>)fontManager;

@end

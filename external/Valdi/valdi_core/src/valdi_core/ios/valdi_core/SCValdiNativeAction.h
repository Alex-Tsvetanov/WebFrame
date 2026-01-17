//
//  SCValdiNativeAction.h
//  Valdi
//
//  Created by Simon Corsin on 4/27/18.
//

#import "valdi_core/SCMacros.h"
#import "valdi_core/SCValdiActionHandlerHolder.h"
#import "valdi_core/SCValdiFunctionCompat.h"

#import "valdi_core/SCValdiAction.h"

#import <Foundation/Foundation.h>

@interface SCValdiNativeAction : SCValdiFunctionCompat <SCValdiAction>

VALDI_NO_INIT

- (instancetype)initWithSelectorName:(NSString*)selectorName
                 actionHandlerHolder:(SCValdiActionHandlerHolder*)actionHandlerHolder;

@end

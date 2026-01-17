//
//  SCValdiContext+CPP.h
//  ValdiIOS
//
//  Created by Simon Corsin on 5/24/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#ifdef __cplusplus

#import "valdi/ios/SCValdiContext.h"
#import "valdi_core/SCValdiActions.h"

#import "valdi/runtime/Context/Context.hpp"
#import "valdi/runtime/Runtime.hpp"
#import <utils/ObjCppPtrWrapper.hpp>

@class SCValdiRuntime;

@interface SCValdiContext ()

@property (readonly, nonatomic) Valdi::SharedContext context;

- (instancetype)initWithContext:(Valdi::SharedContext)context enableReferenceTracking:(BOOL)enableReferenceTracking;

- (void)onDestroyed;

@end

#endif

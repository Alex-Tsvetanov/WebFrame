//
//  SCValdiViewNode+CPP.h
//  ValdiIOS
//
//  Created by Simon Corsin on 5/25/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#ifdef __cplusplus

#import "valdi/ios/SCValdiViewNode.h"

#import "valdi/runtime/Context/ViewNode.hpp"

@interface SCValdiViewNode (CPP)

@property (readonly, nonatomic) Valdi::ViewNode* cppViewNode;

- (instancetype)initWithViewNode:(Valdi::ViewNode*)cppViewNode;

@end

#endif
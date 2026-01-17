//
//  UIView+ValdiObjects.m
//  ValdiIOS
//
//  Created by Simon Corsin on 5/25/18.
//  Copyright © 2018 Snap Inc. All rights reserved.
//

#import "valdi_core/UIView+ValdiObjects.h"

#import <objc/runtime.h>

@implementation UIView (ValdiObjects)

- (id<SCValdiContextProtocol>)valdiContext
{
    return objc_getAssociatedObject(self, @selector(valdiContext));
}

- (void)setValdiContext:(id<SCValdiContextProtocol>)valdiContext
{
    objc_setAssociatedObject(self, @selector(valdiContext), valdiContext, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (id<SCValdiViewNodeProtocol>)valdiViewNode
{
    return objc_getAssociatedObject(self, @selector(valdiViewNode));
}

- (void)setValdiViewNode:(id<SCValdiViewNodeProtocol>)valdiViewNode
{
    objc_setAssociatedObject(self, @selector(valdiViewNode), valdiViewNode, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

@end

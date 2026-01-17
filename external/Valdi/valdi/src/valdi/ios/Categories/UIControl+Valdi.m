//
//  UIControl+Valdi.m
//  Valdi
//
//  Created by Simon Corsin on 12/12/17.
//  Copyright © 2017 Snap Inc. All rights reserved.
//

#import "valdi/ios/Categories/UIControl+Valdi.h"

#import "valdi_core/SCValdiLogger.h"
#import <objc/runtime.h>

@interface SCValdiControlEventsHandler: NSObject

@property (readonly, nonatomic) UIControlEvents events;
@property (readonly, nonatomic) id<SCValdiFunction> function;

- (void)perform;

@end

@implementation SCValdiControlEventsHandler

- (instancetype)initWithControlEvents:(UIControlEvents)events function:(id<SCValdiFunction>)function
{
    self = [super init];
    if (self) {
        _events = events;
        _function = function;
    }
    return self;
}

- (void)perform
{
    SCValdiMarshallerScoped(marshaller, {
        [_function performWithMarshaller:marshaller];
    });;
}

@end

@implementation UIControl (Valdi)

- (NSMutableArray<SCValdiControlEventsHandler *> *)valdiEventsHandler
{
    NSMutableArray *eventsHandler = objc_getAssociatedObject(self, @selector(valdiEventsHandler));
    if (!eventsHandler) {
        eventsHandler = [NSMutableArray array];
        objc_setAssociatedObject(self, @selector(valdiEventsHandler), eventsHandler, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }

    return eventsHandler;
}

- (void)valdi_addTargetWithControlEvents:(UIControlEvents)controlEvents
                             attributeValue:(id<SCValdiFunction>)attributeValue
{
    [self valdi_removeFunctionWithControlEvent:controlEvents];
    SCValdiControlEventsHandler *eventsHandler = [[SCValdiControlEventsHandler alloc] initWithControlEvents:controlEvents function:attributeValue];
    [[self valdiEventsHandler] addObject:eventsHandler];
    [self addTarget:eventsHandler action:@selector(perform) forControlEvents:controlEvents];
}

- (void)valdi_removeFunctionWithControlEvent:(UIControlEvents)controlEvents
{
    NSMutableArray<SCValdiControlEventsHandler *>* handlers = [self valdiEventsHandler];

    for (NSUInteger i = 0; i < handlers.count;) {
        SCValdiControlEventsHandler *handler = handlers[i];

        if (handler.events == controlEvents) {
            [self removeTarget:handler action:@selector(perform) forControlEvents:handler.events];
            [handlers removeObjectAtIndex:i];
        } else {
            i++;
        }
    }
}

+ (void)bindAttributes:(id<SCValdiAttributesBinderProtocol>)attributesBinder
{
    [attributesBinder bindAttribute:@"onTap"
        withFunctionBlock:^(UIControl *view, id<SCValdiFunction> attributeValue) {
            [view valdi_addTargetWithControlEvents:UIControlEventTouchUpInside attributeValue:attributeValue];
        }
        resetBlock:^(__kindof UIView *view) {
            [view valdi_removeFunctionWithControlEvent:UIControlEventTouchUpInside];
        }];
    [attributesBinder bindAttribute:@"onChange"
        withFunctionBlock:^(UIControl *view, id<SCValdiFunction> attributeValue) {
            [view valdi_addTargetWithControlEvents:UIControlEventValueChanged attributeValue:attributeValue];
        }
        resetBlock:^(__kindof UIView *view) {
            [view valdi_removeFunctionWithControlEvent:UIControlEventValueChanged];
        }];
}

@end

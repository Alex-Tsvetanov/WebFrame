//
//  SCValdiNativeAction.m
//  Valdi
//
//  Created by Simon Corsin on 4/27/18.
//

#import "valdi_core/SCValdiNativeAction.h"

#import "valdi_core/SCValdiActionUtils.h"
#import "valdi_core/SCValdiLogger.h"

@implementation SCValdiNativeAction {
    NSString *_messageSelectorName;
    SCValdiActionHandlerHolder *_actionHandlerHolder;
}

- (instancetype)initWithSelectorName:(NSString *)selectorName
                 actionHandlerHolder:(SCValdiActionHandlerHolder *)actionHandlerHolder
{
    self = [super init];

    if (self) {
        _messageSelectorName = selectorName;
        _actionHandlerHolder = actionHandlerHolder;
    }

    return self;
}

- (void)performWithSender:(id)sender
{
    [self performWithParameters:SCValdiActionParametersWithSender(sender)];
}

- (id)performWithParameters:(NSArray<id> *)parameters
{
    if ([NSThread isMainThread]) {
        [self _doPerformWithParameters:parameters];
    } else {
        dispatch_async(dispatch_get_main_queue(), ^{
            [self _doPerformWithParameters:parameters];
        });
    }
    return nil;
}

- (void)_doPerformWithParameters:(NSArray<id> *)parameters
{
    id actionHandler = _actionHandlerHolder.actionHandler;
    if (!actionHandler) {
        SCLogValdiError(@"No action handler set to call action '%@'", _messageSelectorName);
        return;
    }

    SEL selWithoutParams;
    SEL selWithParams;
    if ([_messageSelectorName hasSuffix:@":"]) {
        selWithParams = NSSelectorFromString(_messageSelectorName);
        selWithoutParams =
            NSSelectorFromString([_messageSelectorName substringToIndex:_messageSelectorName.length - 1]);
    } else {
        selWithParams = NSSelectorFromString([_messageSelectorName stringByAppendingString:@":"]);
        selWithoutParams = NSSelectorFromString(_messageSelectorName);
    }

    if ([actionHandler respondsToSelector:selWithParams]) {
        IMP method = [actionHandler methodForSelector:selWithParams];
        ((void (*)(id, SEL, NSArray<id> *))method)(actionHandler, selWithParams, parameters);
    } else if ([actionHandler respondsToSelector:selWithoutParams]) {
        IMP method = [actionHandler methodForSelector:selWithoutParams];
        ((void (*)(id, SEL))method)(actionHandler, selWithParams);
    } else {
        SCLogValdiError(@"Object class '%@' does not implement method '%@'", [actionHandler class],
                           _messageSelectorName);
        return;
    }
}

@end

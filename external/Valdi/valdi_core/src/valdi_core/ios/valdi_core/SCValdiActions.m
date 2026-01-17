//
//  SCValdiActions.m
//  Valdi
//
//  Created by Simon Corsin on 4/27/18.
//

#import "valdi_core/SCValdiActions.h"
#import "valdi_core/SCValdiNativeAction.h"

@implementation SCValdiActions {
    NSMutableDictionary<NSString *, id<SCValdiAction>> *_actionByName;
}

- (instancetype)initWithActionByName:(NSDictionary<NSString *, id<SCValdiAction>> *)actionByName
                 actionHandlerHolder:(SCValdiActionHandlerHolder *)actionHandlerHolder
{
    self = [super init];

    if (self) {
        _actionByName = [actionByName mutableCopy];
        _actionHandlerHolder = actionHandlerHolder;
    }

    return self;
}

- (NSDictionary *)actionByName
{
    return _actionByName.copy;
}

- (id<SCValdiAction>)actionForName:(NSString *)name
{
    id<SCValdiAction> action = _actionByName[name];
    if (!action) {
        action = [[SCValdiNativeAction alloc] initWithSelectorName:name actionHandlerHolder:_actionHandlerHolder];
        _actionByName[name] = action;
    }

    return action;
}

@end

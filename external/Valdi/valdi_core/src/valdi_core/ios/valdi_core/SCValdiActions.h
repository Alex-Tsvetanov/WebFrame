//
//  SCValdiActions.h
//  Valdi
//
//  Created by Simon Corsin on 4/27/18.
//

#import "valdi_core/SCMacros.h"

#import "valdi_core/SCValdiAction.h"

#import <Foundation/Foundation.h>

@class SCValdiActionHandlerHolder;

/**
 Contains all actions given their name.
 */
@interface SCValdiActions : NSObject

@property (readonly, nonatomic) SCValdiActionHandlerHolder* actionHandlerHolder;
@property (readonly, nonatomic) NSDictionary<NSString*, id<SCValdiAction>>* actionByName;

VALDI_NO_INIT

- (instancetype)initWithActionByName:(NSDictionary<NSString*, id<SCValdiAction>>*)actionByName
                 actionHandlerHolder:(SCValdiActionHandlerHolder*)actionHandlerHolder;

- (id<SCValdiAction>)actionForName:(NSString*)name;

@end
